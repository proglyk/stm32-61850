#ifndef __RTC_H
#define __RTC_H

#include "HAL/stm32f4xx.h"
#include <time.h>

#define GMT_ZONE	0

typedef struct {
	uint8_t Hour;   		// RTC time hour, the value range is [0..23] or [0..12] depending of hour format
	uint8_t Minute; 		// RTC time minutes, the value range is [0..59]
	uint8_t Second; 		// RTC time minutes, the value range is [0..59]
	uint8_t RTC_H12;     		// RTC AM/PM time
	uint32_t SubSec;				//SubSeconds
	uint32_t SecFraction;		//SecondFraction
} RTC_TimeTypeDef_user;

// RTC date structure
typedef struct {
	uint8_t WeekDay; // RTC date week day (one of RTC_DOW_XXX definitions)
	uint8_t Month;   // RTC date month (in BCD format, one of RTC_MONTH_XXX definitions)
	uint8_t Date;    // RTC date, the value range is [1..31]
	uint8_t Year;    // RTC date year, the value range is [0..99]
} RTC_DateTypeDef_user;


//extern RTC_TimeTypeDef RTC_Time; // Current RTC time
//extern RTC_DateTypeDef RTC_Date; // Current RTC date

ErrorStatus RTC_Config(void);
void RTC_GetDateTime(RTC_TimeTypeDef_user *time, RTC_DateTypeDef_user *date);
//int	RTC_SetDateTime(time_t * t, uint32_t usec);

// Function prototypes
//ErrorStatus RTC_WaitForSynchro(void);
//ErrorStatus RTC_EnterInitMode(void);

/*
void RTC_ITConfig(uint32_t IT, FunctionalState NewState);

ErrorStatus RTC_SetDateTime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);


uint32_t RTC_ToEpoch(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);
void RTC_FromEpoch(uint32_t epoch, RTC_TimeTypeDef *time, RTC_DateTypeDef *date);
void RTC_AdjustTimeZone(RTC_TimeTypeDef *time, RTC_DateTypeDef *date, int8_t offset);
void RTC_CalcDOW(RTC_DateTypeDef *date);*/

#endif // __RTC_H