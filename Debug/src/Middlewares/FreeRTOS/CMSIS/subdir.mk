################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Middlewares/FreeRTOS/CMSIS/cmsis_os.c 

OBJS += \
./src/Middlewares/FreeRTOS/CMSIS/cmsis_os.o 

C_DEPS += \
./src/Middlewares/FreeRTOS/CMSIS/cmsis_os.d 


# Each subdirectory must supply rules for building sources it contributes
src/Middlewares/FreeRTOS/CMSIS/%.o: ../src/Middlewares/FreeRTOS/CMSIS/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=c99 -DSTM32 -DSTM32F4 -DSTM32F407VGTx -DSTM32F407G_DISC1 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DARM_MATH_CM4 -DLWIP_DEBUG -DORDER_LITTLE_ENDIAN -D__STM32__ -I"D:/proglyk/git/stm32-61850/Driver/HAL/include" -I"D:/proglyk/git/stm32-61850/cmsis" -I"D:/proglyk/git/stm32-61850/inc" -I"D:/proglyk/git/stm32-61850/src/Middlewares/FreeRTOS/CMSIS" -I"D:/proglyk/git/stm32-61850/src/Middlewares/FreeRTOS/core/include" -I"D:/proglyk/git/stm32-61850/src/Middlewares/FreeRTOS/port/include" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


