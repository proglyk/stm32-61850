#ifndef __STM32F4xx_HAL_MSP_H
#define __STM32F4xx_HAL_MSP_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "FreeRTOSConfig.h"


//#define CORE_CLK 168000000
//#define APB1_TIM_CLK (CORE_CLK / 2)
//#define APB2_TIM_CLK CORE_CLK

/*******************************************************************************
	Interrupt Priorities
*******************************************************************************/
#define IRQPRIO_ETH							(-2 + configLIBRARY_LOWEST_INTERRUPT_PRIORITY)
#define IRQSUBPRIO_ETH					0

#define IRQPRIO_RS485TIM				(-1 + configLIBRARY_LOWEST_INTERRUPT_PRIORITY)
#define IRQSUBPRIO_RS485TIM			(0 + 0xf)

#define IRQPRIO_RS485SCI				(-1 + configLIBRARY_LOWEST_INTERRUPT_PRIORITY)
#define IRQSUBPRIO_RS485SCI			(0 + 0xf)

#define IRQPRIO_DEBUG						(0 + configLIBRARY_LOWEST_INTERRUPT_PRIORITY)
#define IRQSUBPRIO_DEBUG				(0 + 0xf)

#define IRQPRIO_WKUPKEY					(0 + configLIBRARY_LOWEST_INTERRUPT_PRIORITY)
#define IRQSUBPRIO_WKUPKEY			(0 + 0xf)


/*******************************************************************************
	USART2 is used by RS-485 bus
*******************************************************************************/
#define RS485_SAU_INSTANCE					USART2

#define RS485_SAU_TX_GPIO_PIN				GPIO_PIN_5
#define RS485_SAU_TX_GPIO_PORT			GPIOD
#define RS485_SAU_TX_GPIO_AF				GPIO_AF7_USART2

#define RS485_SAU_RX_GPIO_PIN				GPIO_PIN_6
#define RS485_SAU_RX_GPIO_PORT			GPIOD
#define RS485_SAU_RX_GPIO_AF				GPIO_AF7_USART2

#define RS485_SAU_RE_GPIO_PIN				GPIO_PIN_7
#define RS485_SAU_RE_GPIO_PORT			GPIOD

#define RS485_SAU_CLK_ENABLE()			__HAL_RCC_USART2_CLK_ENABLE()
#define RS485_SAU_FORCE_RESET()			__HAL_RCC_USART2_FORCE_RESET()
#define RS485_SAU_RELEASE_RESET()		__HAL_RCC_USART2_RELEASE_RESET()

#define RS485SCI_IRQn 							USART2_IRQn
#define RS485SCI_IRQHandler 				USART2_IRQHandler

#define RS485_SAU_TIM_INSTANCE			TIM14
#define RS485_SAU_TIM_CLK_ENABLE()	__HAL_RCC_TIM14_CLK_ENABLE()
#define RS485TIM_IRQn 							TIM8_TRG_COM_TIM14_IRQn
#define RS485TIM_IRQHandler 				TIM8_TRG_COM_TIM14_IRQHandler
/*
	**********************************************
*/


#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_MSP_H */
