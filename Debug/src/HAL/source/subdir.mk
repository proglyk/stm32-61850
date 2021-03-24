################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/HAL/source/stm32f4xx_hal.c \
../src/HAL/source/stm32f4xx_hal_can.c \
../src/HAL/source/stm32f4xx_hal_cortex.c \
../src/HAL/source/stm32f4xx_hal_dma.c \
../src/HAL/source/stm32f4xx_hal_eth.c \
../src/HAL/source/stm32f4xx_hal_flash.c \
../src/HAL/source/stm32f4xx_hal_flash_ex.c \
../src/HAL/source/stm32f4xx_hal_flash_ramfunc.c \
../src/HAL/source/stm32f4xx_hal_gpio.c \
../src/HAL/source/stm32f4xx_hal_pwr.c \
../src/HAL/source/stm32f4xx_hal_pwr_ex.c \
../src/HAL/source/stm32f4xx_hal_rcc.c \
../src/HAL/source/stm32f4xx_hal_rcc_ex.c \
../src/HAL/source/stm32f4xx_hal_rtc.c \
../src/HAL/source/stm32f4xx_hal_rtc_ex.c \
../src/HAL/source/stm32f4xx_hal_tim.c \
../src/HAL/source/stm32f4xx_hal_tim_ex.c \
../src/HAL/source/stm32f4xx_hal_uart.c 

OBJS += \
./src/HAL/source/stm32f4xx_hal.o \
./src/HAL/source/stm32f4xx_hal_can.o \
./src/HAL/source/stm32f4xx_hal_cortex.o \
./src/HAL/source/stm32f4xx_hal_dma.o \
./src/HAL/source/stm32f4xx_hal_eth.o \
./src/HAL/source/stm32f4xx_hal_flash.o \
./src/HAL/source/stm32f4xx_hal_flash_ex.o \
./src/HAL/source/stm32f4xx_hal_flash_ramfunc.o \
./src/HAL/source/stm32f4xx_hal_gpio.o \
./src/HAL/source/stm32f4xx_hal_pwr.o \
./src/HAL/source/stm32f4xx_hal_pwr_ex.o \
./src/HAL/source/stm32f4xx_hal_rcc.o \
./src/HAL/source/stm32f4xx_hal_rcc_ex.o \
./src/HAL/source/stm32f4xx_hal_rtc.o \
./src/HAL/source/stm32f4xx_hal_rtc_ex.o \
./src/HAL/source/stm32f4xx_hal_tim.o \
./src/HAL/source/stm32f4xx_hal_tim_ex.o \
./src/HAL/source/stm32f4xx_hal_uart.o 

C_DEPS += \
./src/HAL/source/stm32f4xx_hal.d \
./src/HAL/source/stm32f4xx_hal_can.d \
./src/HAL/source/stm32f4xx_hal_cortex.d \
./src/HAL/source/stm32f4xx_hal_dma.d \
./src/HAL/source/stm32f4xx_hal_eth.d \
./src/HAL/source/stm32f4xx_hal_flash.d \
./src/HAL/source/stm32f4xx_hal_flash_ex.d \
./src/HAL/source/stm32f4xx_hal_flash_ramfunc.d \
./src/HAL/source/stm32f4xx_hal_gpio.d \
./src/HAL/source/stm32f4xx_hal_pwr.d \
./src/HAL/source/stm32f4xx_hal_pwr_ex.d \
./src/HAL/source/stm32f4xx_hal_rcc.d \
./src/HAL/source/stm32f4xx_hal_rcc_ex.d \
./src/HAL/source/stm32f4xx_hal_rtc.d \
./src/HAL/source/stm32f4xx_hal_rtc_ex.d \
./src/HAL/source/stm32f4xx_hal_tim.d \
./src/HAL/source/stm32f4xx_hal_tim_ex.d \
./src/HAL/source/stm32f4xx_hal_uart.d 


# Each subdirectory must supply rules for building sources it contributes
src/HAL/source/%.o: ../src/HAL/source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=c99 -DSTM32 -DSTM32F4 -DSTM32F407VGTx -DSTM32F407G_DISC1 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DARM_MATH_CM4 -DLWIP_DEBUG -DORDER_LITTLE_ENDIAN -D__STM32__ -I"D:/proglyk/git/stm32-61850/Driver/HAL/include" -I"D:/proglyk/git/stm32-61850/cmsis" -I"D:/proglyk/git/stm32-61850/inc" -I"D:/proglyk/git/stm32-61850/src/Middlewares/FreeRTOS/CMSIS" -I"D:/proglyk/git/stm32-61850/src/Middlewares/FreeRTOS/core/include" -I"D:/proglyk/git/stm32-61850/src/Middlewares/FreeRTOS/port/include" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


