#include "stm32f4xx_hal.h"
#include "system_stm32f4xx.h"
#include "debug.h"
#include "hal/stm32e407.h"
#include "FreeRTOSConfig.h"

uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
static TaskHandle_t pvStackCrashHandle;
signed char * pcStackCrashTaskName;

void
	vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName );
void
	ApplicationMallocFailedHook( void );

int
main(void) {

	SystemInit();
	HAL_Init();

	debug__init((uint32_t*)USART3, USART3_IRQn);

	BSP_PB_Init(BUTTON_WAKEUP, BUTTON_MODE_EXTI);

	vTaskStartScheduler();

	return 0;
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

