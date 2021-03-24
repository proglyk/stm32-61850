#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "HAL/stm32f4xx.h"
#include "cmsis_os.h"
#include <stdbool.h>

BOOL debug__set_baudrate(uint32_t baud);
void debug__init(uint32_t* instance, IRQn_Type irq);
void debug__MspInit(UART_HandleTypeDef* huart);
osEvent debug__message_get(uint32_t millisec);


#endif /*_DEBUG_H_*/
