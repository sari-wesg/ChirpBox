################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/TP/Study/wesg/Chirpbox/Daemon/Src/tools/Src/trace_flash.c 

OBJS += \
./tools/Src/trace_flash.o 

C_DEPS += \
./tools/Src/trace_flash.d 


# Each subdirectory must supply rules for building sources it contributes
tools/Src/trace_flash.o: D:/TP/Study/wesg/Chirpbox/Daemon/Src/tools/Src/trace_flash.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"D:/TP/Study/wesg/Chirpbox/Miscellaneous/Toggle/Inc" -I"D:/TP/Study/wesg/Chirpbox/Daemon/Src/tools/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"tools/Src/trace_flash.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

