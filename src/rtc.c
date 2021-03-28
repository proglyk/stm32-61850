#include "rtc.h"
#include "stm32f4xx_hal_rtc.h"
#include "lwip/debug.h"
#include "lwipopts.h"

// The RTC initialization timeout
#define RTC_INIT_TIMEOUT      ((uint32_t)0x00002000)
// The RTC synchronization timeout
#define RTC_SYNC_TIMEOUT      ((uint32_t)0x00008000)

#define RTC_ShiftAdd1S_Reset      ((uint32_t)0x00000000)
#define RTC_ShiftAdd1S_Set        ((uint32_t)0x80000000)

#define SHPF_TIMEOUT             ((uint32_t) 0x00001000)

#define RTC_ASYNCH_PREDIV  31		//0x7F   /* LSE as RTC clock */
#define RTC_SYNCH_PREDIV   1023	//0x00FF /* LSE as RTC clock */


#define __HAL_RTC_WRITEPROTECTION_DISABLE_USER(__INSTANCE__)             \
                        do{                                       \
                            (__INSTANCE__)->WPR = 0xCAU;  \
                            (__INSTANCE__)->WPR = 0x53U;  \
                          } while(0U)

#define __HAL_RTC_WRITEPROTECTION_ENABLE_USER(__INSTANCE__)              \
                        do{                                       \
                            (__INSTANCE__)->WPR = 0xFFU;  \
                          } while(0U)                            
 

extern RTC_HandleTypeDef RtcHandle;
RTC_TimeTypeDef_user RTC_Time; // Current RTC time
RTC_DateTypeDef_user RTC_Date; // Current RTC date

uint32_t SecFraction = 0;
float micro = 0.0f;
static HAL_StatusTypeDef status;
static uint32_t time_view;
static struct tm *local;

static 
	ErrorStatus RTC_SynchroShiftConfig(uint32_t RTC_ShiftAdd1S, uint32_t RTC_ShiftSubFS);

int
	RTC_SetDateTime(time_t *, uint32_t);

HAL_StatusTypeDef 
	HAL_RTCEx_SetSynchroShift_user(RTC_TypeDef *,uint32_t ShiftAdd1S, uint32_t ShiftSubFS);
 
HAL_StatusTypeDef
	HAL_RTC_WaitForSynchro_user(RTC_TypeDef* hrtc);
  


ErrorStatus RTC_WaitForSynchro(void) {
	uint32_t wait = RTC_SYNC_TIMEOUT;

	// Clear the RSF flag
	RTC->ISR &= ~RTC_ISR_RSF;

	// Wait the registers to be synchronized
	while (!(RTC->ISR & RTC_ISR_RSF) && --wait);

	return (RTC->ISR & RTC_ISR_RSF) ? SUCCESS : ERROR;
}

ErrorStatus 
	RTC_EnterInitMode_user(void) {
	
	uint32_t wait = RTC_INIT_TIMEOUT;

	if (!(RTC->ISR & RTC_ISR_INITF)) {
	    // Set the initialization mode
	    RTC->ISR = RTC_ISR_INIT;
		wait = RTC_INIT_TIMEOUT;

		// Wait till RTC is in INIT state or timeout
		while (!(RTC->ISR & RTC_ISR_INITF) && --wait);
	}

	return (RTC->ISR & RTC_ISR_INITF) ? SUCCESS : ERROR;
}


void 
	RCC_RTCCLKCmd(FunctionalState NewState) {
  *(__IO uint32_t *) BDCR_RTCEN_BB = (uint32_t)NewState;
}


ErrorStatus
	RTC_Config(void) {
		
	uint32_t wait;
	RCC_OscInitTypeDef        RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

	// Enable the PWR peripheral
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;

	// Access to RTC, RTC Backup and RCC CSR registers enabled
	PWR->CR |= PWR_CR_DBP;

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
	
	//while(!(RCC->CSR & RCC_CSR_LSERDY));
  
  RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
	
	
	// Enable RTC clock
	RCC_RTCCLKCmd(ENABLE);

	// Disable the write protection for RTC registers
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

	// Wait for the RTC APB registers synchronization
	if (RTC_WaitForSynchro() != SUCCESS) {
		// Enable the write protection for RTC registers
		RTC->WPR = 0xFF;
		// Access to RTC, RTC Backup and RCC CSR registers disabled
		PWR->CR &= ~PWR_CR_DBP;

		return ERROR;
	}

	// Enter the RTC initialization mode
	if (RTC_EnterInitMode_user() != SUCCESS) {
		// Enable the write protection for RTC registers
		RTC->WPR = 0xFF;
		// Access to RTC, RTC Backup and RCC CSR registers disabled
		PWR->CR &= ~PWR_CR_DBP;

		return ERROR;
	}

	// Clear RTC CR FMT Bit (24-hour format)
	RTC->CR &= ~RTC_CR_FMT;

	// Configure the RTC prescaler
	//RTC->PRER = 0x007f00ff; // Asynch = 128, Synch = 256
	RTC->PRER = (uint32_t)(RTC_SYNCH_PREDIV);
  RTC->PRER |= (uint32_t)(RTC_ASYNCH_PREDIV << 16U);

	// Exit the RTC Initialization mode
	RTC->ISR &= ~RTC_ISR_INIT;

	// Enable the write protection for RTC registers
	RTC->WPR = 0xFF;
	// Access to RTC, RTC Backup and RCC CSR registers disabled
	PWR->CR &= ~PWR_CR_DBP;

	return SUCCESS;
}


void 
	RTC_GetTime(uint32_t * time, uint32_t * usec) {
	
	uint32_t TR,DR;
	struct tm tmm;
	uint32_t subsecs = 0;
	uint32_t secfrac = 0;
	float tmp;
	uint32_t ssr;
	
	long temp;

	// Get date and time (clear reserved bits just for any case)
	TR = RTC->TR & RTC_TR_RESERVED_MASK;
	DR = RTC->DR & RTC_DR_RESERVED_MASK;

	// Convert BCD to human readable format
	tmm.tm_hour   = (((TR >> 20) & 0x03) * 10) + ((TR >> 16) & 0x0f);
	tmm.tm_min = (((TR >> 12) & 0x07) * 10) + ((TR >>  8) & 0x0f);
	tmm.tm_sec = (((TR >>  4) & 0x07) * 10) +  (TR & 0x0f);
	subsecs = RTC_GetSubSecond();
	secfrac = (uint32_t)(RTC->PRER & RTC_PRER_PREDIV_S);
	
	tmm.tm_year    = 100 + (((DR >> 20) & 0x07) * 10) + ((DR >> 16) & 0x0f);
	temp = (((DR >> 12) & 0x01) * 10) + ((DR >>  8) & 0x0f);
	(temp <= 0) ? (tmm.tm_mon) : (tmm.tm_mon = temp-1);
	//tmm.tm_mon   = (((DR >> 12) & 0x01) * 10) + ((DR >>  8) & 0x0f);
	tmm.tm_mday    = (((DR >>  4) & 0x03) * 10) +  (DR & 0x0f);
	//tmm.tm_wday = (DR & RTC_DR_WDU) >> 13;
	tmm.tm_isdst = 0;
	time_view = mktime(&tmm);
	*time = time_view;
	
		//настройка на начальное значение GMT
	/*
	tmm.tm_sec = 0; 	// 0 секунд.
	tmm.tm_min = 0; 	// 0 минут.
  tmm.tm_hour = 0; 	// 0 часов. Если задать только часы, то функция mktime() вернет -1.
	tmm.tm_mday = 1; 	// 01 февраля
	tmm.tm_mon = 0; 	// Январь.
	tmm.tm_year = 100;	// 1970 год. (в RTC разрешаются значения от 0 до 99)
	//tmm.tm_yday = 1;// День в году 33-й (03 февраля).
	//tmm.tm_wday = 3; 	// Вторник.
	tmm.tm_isdst = 0; //
  time_view = mktime(&tmm);*/
	
	
	
	if (subsecs > secfrac){
		ssr = 2*secfrac - subsecs;
		if (tmm.tm_sec>1) tmm.tm_sec -= 1;
	}	else
		ssr = secfrac - subsecs;
	tmp = (float)ssr / ((float)(secfrac+1));
	*usec = (uint32_t)(tmp*1000000.0f);
	
	//printf(ctime(&time_view));
	//LWIP_DEBUGF(SNTP_DEBUG, ());
	
	//LWIP_DEBUGF(SNTP_DEBUG, ("SYS time: %02u/%02u/20%02u %02u:%02u:%02u.%06d GMT+3\r\n", 
	//	date->Date, date->Month+1, date->Year, time->Hour, 
	//		time->Minute, time->Second, usecs));
}




void 
	RTC_GetDateTime(RTC_TimeTypeDef_user *time, RTC_DateTypeDef_user *date) {
	
	uint32_t TR,DR;
	uint32_t usecs = 0;
	float tmp;
	uint32_t ssr;

	// Get date and time (clear reserved bits just for any case)
	TR = RTC->TR & RTC_TR_RESERVED_MASK;
	DR = RTC->DR & RTC_DR_RESERVED_MASK;

	// Convert BCD to human readable format
	time->Hour   = (((TR >> 20) & 0x03) * 10) + ((TR >> 16) & 0x0f);
	time->Minute = (((TR >> 12) & 0x07) * 10) + ((TR >>  8) & 0x0f);
	time->Second = (((TR >>  4) & 0x07) * 10) +  (TR & 0x0f);
	time->SubSec			= RTC_GetSubSecond();
	time->SecFraction = (uint32_t)(RTC->PRER & RTC_PRER_PREDIV_S);
	
	date->Year    = (((DR >> 20) & 0x07) * 10) + ((DR >> 16) & 0x0f);
	date->Month   = (((DR >> 12) & 0x01) * 10) + ((DR >>  8) & 0x0f);
	date->Date    = (((DR >>  4) & 0x03) * 10) +  (DR & 0x0f);
	date->WeekDay = (DR & RTC_DR_WDU) >> 13;
	
	if (time->SubSec > time->SecFraction){
		ssr = 2*time->SecFraction - time->SubSec;
		if (time->Second>1) time->Second -= 1;
	}	else
		ssr = time->SecFraction - time->SubSec;
		
	tmp = (float)ssr / ((float)(time->SecFraction+1));
	//tmp = time->SubSec;
	usecs = (uint32_t)(tmp*1000000.0f);
	//usecs = (uint32_t)tmp;
	
	LWIP_DEBUGF(SNTP_DEBUG, ("SYS time: %02u/%02u/20%02u %02u:%02u:%02u.%06d GMT+3\r\n", 
		date->Date, date->Month, date->Year, time->Hour, 
			time->Minute, time->Second, usecs));
}


// return: SUCCESS if date and time set, ERROR otherwise
int
	RTC_SetDateTime(time_t * t, uint32_t usec) {
	
	uint32_t TR,DR;
	float micro;
	struct tm * p_tm;
	uint32_t var;
	//uint32_t SecFraction = (uint32_t)(RTC->PRER & RTC_PRER_PREDIV_S);
	
	p_tm = gmtime(t);
	p_tm->tm_mon += 1;
	(p_tm->tm_year >= 100) ? (p_tm->tm_year -= 100) : (p_tm->tm_year = 0);

	// Calculate value for time register
	TR =   (((p_tm->tm_hour   / 10) << 20) + ((p_tm->tm_hour   % 10) << 16) +
			((p_tm->tm_min / 10) << 12) + ((p_tm->tm_min % 10) <<  8) +
			((p_tm->tm_sec / 10) <<  4) +  (p_tm->tm_sec % 10)) & RTC_TR_RESERVED_MASK;
	// Calculate value for date register
	DR =   (((p_tm->tm_year  / 10) << 20) + ((p_tm->tm_year  % 10) << 16) +
			((p_tm->tm_mon / 10) << 12) + ((p_tm->tm_mon % 10) <<  8) +
			((p_tm->tm_mday  / 10) <<  4) +  (p_tm->tm_mday  % 10) +
			 (p_tm->tm_wday << 13)) & RTC_DR_RESERVED_MASK;
	
	//LWIP_DEBUGF(SNTP_DEBUG, ("usec a1: %06d\r\n", usec));
	//LWIP_DEBUGF(SNTP_DEBUG, ("SSR a1: %06d\r\n", RTC_GetSubSecond()));
	

	// Access to RTC, RTC Backup and RCC CSR registers enabled
	PWR->CR |= PWR_CR_DBP;
	// Disable the write protection for RTC registers
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

	// Enter the RTC initialization mode
	if (RTC_EnterInitMode_user() == SUCCESS) {
		// Write date and time to the RTC registers
		RTC->TR = TR;
		RTC->DR = DR;

		// Exit the RTC Initialization mode
		RTC->ISR &= ~RTC_ISR_INIT;

		// Wait for synchronization if BYPSHAD bit is not set in the RTC_CR register
		TR = SUCCESS;
		if (!(RTC->CR & RTC_CR_BYPSHAD)) {
			TR = RTC_WaitForSynchro();
		}
	} else TR = ERROR;

	// Enable the write protection for RTC registers
	RTC->WPR = 0xFF;
	// Access to RTC, RTC Backup and RCC CSR registers disabled
	PWR->CR &= ~PWR_CR_DBP;
	
	//LWIP_DEBUGF(SNTP_DEBUG, ("SSR a2: %06d\r\n", RTC_GetSubSecond()));
	
	SecFraction = (uint32_t)(RTC->PRER & RTC_PRER_PREDIV_S);
	micro = 1.0f - ((float)usec)/1000000.0f;
	micro = micro*((float)(SecFraction+1));

	RTC_SynchroShiftConfig(RTC_ShiftAdd1S_Set, (uint32_t)micro);
	
	//LWIP_DEBUGF(SNTP_DEBUG, ("SSR a3: %06d\r\n", RTC_GetSubSecond()));

	return TR;
}


/**
  * @brief  Configures the Synchronization Shift Control Settings.
  * @note   When REFCKON is set, firmware must not write to Shift control register 
  * @param  RTC_ShiftAdd1S : Select to add or not 1 second to the time Calendar.
  *   This parameter can be one of the following values :
  *     @arg RTC_ShiftAdd1S_Set  : Add one second to the clock calendar. 
  *     @arg RTC_ShiftAdd1S_Reset: No effect.
  * @param  RTC_ShiftSubFS: Select the number of Second Fractions to Substitute.
  *         This parameter can be one any value from 0 to 0x7FFF.
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: RTC Shift registers are configured
  *          - ERROR: RTC Shift registers are not configured
*/
static ErrorStatus
	RTC_SynchroShiftConfig(uint32_t RTC_ShiftAdd1S, uint32_t RTC_ShiftSubFS) {
  
	ErrorStatus status = ERROR;
  uint32_t shpfcount = 0;

  /* Check the parameters */
  assert_param(IS_RTC_SHIFT_ADD1S(RTC_ShiftAdd1S));
  assert_param(IS_RTC_SHIFT_SUBFS(RTC_ShiftSubFS));

	// Access to RTC, RTC Backup and RCC CSR registers enabled
	PWR->CR |= PWR_CR_DBP;	
	
  /* Disable the write protection for RTC registers */
  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;
  
  /* Check if a Shift is pending*/
  if ((RTC->ISR & RTC_ISR_SHPF) != RESET)
  {
    /* Wait until the shift is completed*/
    while (((RTC->ISR & RTC_ISR_SHPF) != RESET) && (shpfcount != SHPF_TIMEOUT))
    {
      shpfcount++;
    }
  }

  /* Check if the Shift pending is completed or if there is no Shift operation at all*/
  if ((RTC->ISR & RTC_ISR_SHPF) == RESET)
  {
    /* check if the reference clock detection is disabled */
    if((RTC->CR & RTC_CR_REFCKON) == RESET)
    {
      /* Configure the Shift settings */
      RTC->SHIFTR = (uint32_t)(uint32_t)(RTC_ShiftSubFS) | (uint32_t)(RTC_ShiftAdd1S);
    
      if(RTC_WaitForSynchro() == ERROR)
      {
        status = ERROR;
      }
      else
      {
        status = SUCCESS;
      }
    }
    else
    {
      status = ERROR;
    }
  }
  else
  {
    status = ERROR;
  }

  /* Enable the write protection for RTC registers */
  RTC->WPR = 0xFF;
	// Access to RTC, RTC Backup and RCC CSR registers disabled
	PWR->CR &= ~PWR_CR_DBP;
  
  return (ErrorStatus)(status);
}

/*
uint32_t RTC_GetSubSecond(void) {
  uint32_t tmpreg = 0;
  
  // Get sub seconds values from the correspondent registers
  tmpreg = (uint32_t)(RTC->SSR);
  
  // Read DR register to unfroze calendar registers
  (void) (RTC->DR);
  
  return (tmpreg);
}*/


HAL_StatusTypeDef 
	HAL_RTCEx_SetSynchroShift_user(RTC_TypeDef * hrtc, uint32_t ShiftAdd1S, uint32_t ShiftSubFS) {
  
	uint32_t tickstart = 0U;

  /* Check the parameters */
  assert_param(IS_RTC_SHIFT_ADD1S(ShiftAdd1S));
  assert_param(IS_RTC_SHIFT_SUBFS(ShiftSubFS));

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE_USER(hrtc);

  /* Get tick */
  tickstart = HAL_GetTick();

    /* Wait until the shift is completed*/
    while((hrtc->ISR & RTC_ISR_SHPF) != RESET)
    {
      if((HAL_GetTick() - tickstart ) > RTC_TIMEOUT_VALUE)
      {  
        /* Enable the write protection for RTC registers */
        __HAL_RTC_WRITEPROTECTION_ENABLE_USER(hrtc);  
        return HAL_TIMEOUT;
      }
    }
  
    /* Check if the reference clock detection is disabled */
    if((hrtc->CR & RTC_CR_REFCKON) == RESET)
    {
      /* Configure the Shift settings */
      hrtc->SHIFTR = (uint32_t)(uint32_t)(ShiftSubFS) | (uint32_t)(ShiftAdd1S);
      
      /* If  RTC_CR_BYPSHAD bit = 0, wait for synchro else this check is not needed */
      if((hrtc->CR & RTC_CR_BYPSHAD) == RESET)
      {
        if(HAL_RTC_WaitForSynchro_user(hrtc) != HAL_OK)
        {
          /* Enable the write protection for RTC registers */
          __HAL_RTC_WRITEPROTECTION_ENABLE_USER(hrtc);  
          return HAL_ERROR;
        }
      }
    }
    else
    {
      /* Enable the write protection for RTC registers */
      __HAL_RTC_WRITEPROTECTION_ENABLE_USER(hrtc);
      return HAL_ERROR;
    }
  
  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE_USER(hrtc);
  return HAL_OK;
}


HAL_StatusTypeDef HAL_RTC_WaitForSynchro_user(RTC_TypeDef* hrtc)
{
  uint32_t tickstart = 0U;

  /* Clear RSF flag */
  hrtc->ISR &= (uint32_t)RTC_RSF_MASK;

	/* Get tick */
  tickstart = HAL_GetTick();

  /* Wait the registers to be synchronised */
  while((hrtc->ISR & RTC_ISR_RSF) == (uint32_t)RESET)
  {
    if((HAL_GetTick() - tickstart ) > RTC_TIMEOUT_VALUE)
    {       
      return HAL_TIMEOUT;
    } 
  }

  return HAL_OK;
}
