################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/FreeRTOS-TCP/include/stm32fxx_hal_eth.c 

OBJS += \
./Middlewares/Third_Party/FreeRTOS-TCP/include/stm32fxx_hal_eth.o 

C_DEPS += \
./Middlewares/Third_Party/FreeRTOS-TCP/include/stm32fxx_hal_eth.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/FreeRTOS-TCP/include/%.o Middlewares/Third_Party/FreeRTOS-TCP/include/%.su: ../Middlewares/Third_Party/FreeRTOS-TCP/include/%.c Middlewares/Third_Party/FreeRTOS-TCP/include/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DSTM32_THREAD_SAFE_STRATEGY=4 -c -I../Core/Inc -I../Middlewares/Third_Party/FreeRTOS-TCP/include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Core/ThreadSafe -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-FreeRTOS-2d-TCP-2f-include

clean-Middlewares-2f-Third_Party-2f-FreeRTOS-2d-TCP-2f-include:
	-$(RM) ./Middlewares/Third_Party/FreeRTOS-TCP/include/stm32fxx_hal_eth.d ./Middlewares/Third_Party/FreeRTOS-TCP/include/stm32fxx_hal_eth.o ./Middlewares/Third_Party/FreeRTOS-TCP/include/stm32fxx_hal_eth.su

.PHONY: clean-Middlewares-2f-Third_Party-2f-FreeRTOS-2d-TCP-2f-include

