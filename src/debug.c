#include "debug.h"
#include "stm32f4xx_hal_msp.h"
#include "stdbool.h"


static UART_HandleTypeDef uart_st = {
	.Instance = NULL,
	.Init = {
		.BaudRate = 115200,
		.WordLength = UART_WORDLENGTH_8B,
		.StopBits = UART_STOPBITS_2,
		.Parity = UART_PARITY_NONE,
		.Mode = UART_MODE_TX_RX,
		.HwFlowCtl = UART_HWCONTROL_NONE,
		.OverSampling = UART_OVERSAMPLING_16
	},
};

UART_HandleTypeDef * pxDebug = &uart_st;


static struct {
//	UART_HandleTypeDef uart;
	IRQn_Type irq;
} debug;

bool periph_initialised = false;
static uint8_t aRxBuffer[1] = {0};
osMessageQId message_q_id;
static osMessageQDef(message_q, 16, uint8_t);


static void
debug__PeripheralInit(UART_HandleTypeDef* huart);


void
debug__init(uint32_t* instance, IRQn_Type irq) {

	message_q_id = osMessageCreate(osMessageQ(message_q), NULL);

	uart_st.Instance = (USART_TypeDef*)instance;
	debug.irq = irq;
	HAL_UART_Init(&uart_st);
	HAL_UART_Receive_IT(&uart_st, (uint8_t *)aRxBuffer, 1);
}


BOOL
debug__set_baudrate(uint32_t baud) {
	uart_st.Init.BaudRate = baud;
	return TRUE;
}


int
fputc(int ch, FILE *f)
{
	if (!periph_initialised)
		return -1;
	HAL_UART_Transmit(&uart_st, (uint8_t *)&ch, 1, 0xFFFF);
	return ch;
}

osEvent
debug__message_get(uint32_t millisec) {
	return osMessageGet(message_q_id, millisec);
}


void
debug__MspInit(UART_HandleTypeDef *huart) {
	
	debug__PeripheralInit(huart);
	periph_initialised = TRUE;
}


static void
debug__PeripheralInit(UART_HandleTypeDef* huart) {
	GPIO_InitTypeDef Gpio;

	__HAL_RCC_USART3_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	Gpio.Pin = (1<<10);
	Gpio.Mode = GPIO_MODE_AF_PP;
	Gpio.Speed = GPIO_SPEED_FAST;
	Gpio.Pull = GPIO_NOPULL;
	Gpio.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOB, &Gpio);

	Gpio.Pin = (1<<11);
	Gpio.Mode = GPIO_MODE_AF_PP;
	Gpio.Speed = GPIO_SPEED_FAST;
	Gpio.Pull = GPIO_NOPULL;
	Gpio.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOB, &Gpio);

	HAL_NVIC_SetPriority(debug.irq, IRQPRIO_DEBUG, IRQSUBPRIO_DEBUG);
	HAL_NVIC_EnableIRQ(debug.irq);
}

