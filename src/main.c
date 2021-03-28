#include "stm32f4xx_hal.h"
#include "system_stm32f4xx.h"
#include "debug.h"
#include "hal/stm32e407.h"
#include "FreeRTOSConfig.h"
#include <lwipopts.h>
#include "rtc.h"

void
  LED_Thread2(void *argument);
void
	vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName );
void
	ApplicationMallocFailedHook( void );

/* Heap for middlewares ----------------------------------------------------------------------*/
uint8_t ucHeap[ configTOTAL_HEAP_SIZE ] __attribute__ ((aligned (4)));
unsigned char lwip_heap_user[MEM_SIZE + 0x14] __attribute__ ((aligned (4)));

static TaskHandle_t pvStackCrashHandle;
signed char * pcStackCrashTaskName;

static size_t heap_size = 0;


int
main(void) {

	TaskHandle_t pthread;

	SystemInit();
	HAL_Init();

	debug__init((uint32_t*)USART3, USART3_IRQn);

	BSP_PB_Init(BUTTON_WAKEUP, BUTTON_MODE_EXTI);

	RTC_Config();

	xTaskCreate(LED_Thread2, "pt2", 2*configMINIMAL_STACK_SIZE, NULL,
	    makeFreeRtosPriority(osPriorityIdle), &(pthread));

	vTaskStartScheduler();

	return 0;
}


void
LED_Thread2(void *argument) {

	for(;;)	{
		heap_size = xPortGetFreeHeapSize();

//		if (netif_is_up(p__netif)){
		//sntp__print_cur_time();
		//LWIP_DEBUGF(SNTP_DEBUG, ("RTC Time: %s\r", sTime));
		//printf("main: ");
//		RTC_GetDateTime(&RTC_Time, &RTC_Date);
	//	LWIP_DEBUGF(SNTP_DEBUG, ("RTC usec: %.4f\r\n", var));
//		}
		//GPIOD->ODR ^= 1<<11;
//    GPIOE->ODR ^= 1<<8;
    //osDelay(delay);
		printf("main: elapsed heap memory 0x%04x bytes\r\n", heap_size);
		vTaskDelay(2500);
	}
}


void
	vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName ) {

	//printf("vApplicationStackOverflowHook: Fail\r\n");
	pvStackCrashHandle = xTask;
	pcStackCrashTaskName = pcTaskName;

	while (1){
		__NOP();
	}
}


void
	vApplicationMallocFailedHook( void ) {

	//printf("vApplicationMallocFailedHook: Fail\r\n");

	while (1){
		__NOP();
	}
}

