################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/coreSNTP/core_sntp_client.c \
../Middlewares/Third_Party/coreSNTP/core_sntp_serializer.c 

OBJS += \
./Middlewares/Third_Party/coreSNTP/core_sntp_client.o \
./Middlewares/Third_Party/coreSNTP/core_sntp_serializer.o 

C_DEPS += \
./Middlewares/Third_Party/coreSNTP/core_sntp_client.d \
./Middlewares/Third_Party/coreSNTP/core_sntp_serializer.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/coreSNTP/%.o Middlewares/Third_Party/coreSNTP/%.su Middlewares/Third_Party/coreSNTP/%.cyclo: ../Middlewares/Third_Party/coreSNTP/%.c Middlewares/Third_Party/coreSNTP/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DSTM32_THREAD_SAFE_STRATEGY=4 -c -I../Core/Inc -I../Middlewares/Third_Party/coreSNTP/include -I../Middlewares/Third_Party/cJSON -I../Middlewares/Third_Party/coreMQTT-Agent/include -I../Middlewares/Third_Party/coreMQTT/include -I../Middlewares/Third_Party/FreeRTOS-TCP/include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Core/ThreadSafe -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-coreSNTP

clean-Middlewares-2f-Third_Party-2f-coreSNTP:
	-$(RM) ./Middlewares/Third_Party/coreSNTP/core_sntp_client.cyclo ./Middlewares/Third_Party/coreSNTP/core_sntp_client.d ./Middlewares/Third_Party/coreSNTP/core_sntp_client.o ./Middlewares/Third_Party/coreSNTP/core_sntp_client.su ./Middlewares/Third_Party/coreSNTP/core_sntp_serializer.cyclo ./Middlewares/Third_Party/coreSNTP/core_sntp_serializer.d ./Middlewares/Third_Party/coreSNTP/core_sntp_serializer.o ./Middlewares/Third_Party/coreSNTP/core_sntp_serializer.su

.PHONY: clean-Middlewares-2f-Third_Party-2f-coreSNTP

