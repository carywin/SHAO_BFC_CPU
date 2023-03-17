################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/coreMQTT/backoff_algorithm.c \
../Middlewares/Third_Party/coreMQTT/core_mqtt.c \
../Middlewares/Third_Party/coreMQTT/core_mqtt_serializer.c \
../Middlewares/Third_Party/coreMQTT/core_mqtt_state.c \
../Middlewares/Third_Party/coreMQTT/sockets_wrapper.c \
../Middlewares/Third_Party/coreMQTT/tcp_sockets_wrapper.c \
../Middlewares/Third_Party/coreMQTT/transport_plaintext.c 

OBJS += \
./Middlewares/Third_Party/coreMQTT/backoff_algorithm.o \
./Middlewares/Third_Party/coreMQTT/core_mqtt.o \
./Middlewares/Third_Party/coreMQTT/core_mqtt_serializer.o \
./Middlewares/Third_Party/coreMQTT/core_mqtt_state.o \
./Middlewares/Third_Party/coreMQTT/sockets_wrapper.o \
./Middlewares/Third_Party/coreMQTT/tcp_sockets_wrapper.o \
./Middlewares/Third_Party/coreMQTT/transport_plaintext.o 

C_DEPS += \
./Middlewares/Third_Party/coreMQTT/backoff_algorithm.d \
./Middlewares/Third_Party/coreMQTT/core_mqtt.d \
./Middlewares/Third_Party/coreMQTT/core_mqtt_serializer.d \
./Middlewares/Third_Party/coreMQTT/core_mqtt_state.d \
./Middlewares/Third_Party/coreMQTT/sockets_wrapper.d \
./Middlewares/Third_Party/coreMQTT/tcp_sockets_wrapper.d \
./Middlewares/Third_Party/coreMQTT/transport_plaintext.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/coreMQTT/%.o Middlewares/Third_Party/coreMQTT/%.su Middlewares/Third_Party/coreMQTT/%.cyclo: ../Middlewares/Third_Party/coreMQTT/%.c Middlewares/Third_Party/coreMQTT/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DSTM32_THREAD_SAFE_STRATEGY=4 -c -I../Core/Inc -I../Middlewares/Third_Party/coreSNTP/include -I../Middlewares/Third_Party/cJSON -I../Middlewares/Third_Party/coreMQTT-Agent/include -I../Middlewares/Third_Party/coreMQTT/include -I../Middlewares/Third_Party/FreeRTOS-TCP/include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Core/ThreadSafe -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-coreMQTT

clean-Middlewares-2f-Third_Party-2f-coreMQTT:
	-$(RM) ./Middlewares/Third_Party/coreMQTT/backoff_algorithm.cyclo ./Middlewares/Third_Party/coreMQTT/backoff_algorithm.d ./Middlewares/Third_Party/coreMQTT/backoff_algorithm.o ./Middlewares/Third_Party/coreMQTT/backoff_algorithm.su ./Middlewares/Third_Party/coreMQTT/core_mqtt.cyclo ./Middlewares/Third_Party/coreMQTT/core_mqtt.d ./Middlewares/Third_Party/coreMQTT/core_mqtt.o ./Middlewares/Third_Party/coreMQTT/core_mqtt.su ./Middlewares/Third_Party/coreMQTT/core_mqtt_serializer.cyclo ./Middlewares/Third_Party/coreMQTT/core_mqtt_serializer.d ./Middlewares/Third_Party/coreMQTT/core_mqtt_serializer.o ./Middlewares/Third_Party/coreMQTT/core_mqtt_serializer.su ./Middlewares/Third_Party/coreMQTT/core_mqtt_state.cyclo ./Middlewares/Third_Party/coreMQTT/core_mqtt_state.d ./Middlewares/Third_Party/coreMQTT/core_mqtt_state.o ./Middlewares/Third_Party/coreMQTT/core_mqtt_state.su ./Middlewares/Third_Party/coreMQTT/sockets_wrapper.cyclo ./Middlewares/Third_Party/coreMQTT/sockets_wrapper.d ./Middlewares/Third_Party/coreMQTT/sockets_wrapper.o ./Middlewares/Third_Party/coreMQTT/sockets_wrapper.su ./Middlewares/Third_Party/coreMQTT/tcp_sockets_wrapper.cyclo ./Middlewares/Third_Party/coreMQTT/tcp_sockets_wrapper.d ./Middlewares/Third_Party/coreMQTT/tcp_sockets_wrapper.o ./Middlewares/Third_Party/coreMQTT/tcp_sockets_wrapper.su ./Middlewares/Third_Party/coreMQTT/transport_plaintext.cyclo ./Middlewares/Third_Party/coreMQTT/transport_plaintext.d ./Middlewares/Third_Party/coreMQTT/transport_plaintext.o ./Middlewares/Third_Party/coreMQTT/transport_plaintext.su

.PHONY: clean-Middlewares-2f-Third_Party-2f-coreMQTT

