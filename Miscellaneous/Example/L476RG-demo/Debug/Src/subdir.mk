################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/TP/Study/wesg/Chirpbox/Miscellaneous/Toggle/Src/ll_flash.c \
D:/TP/Study/wesg/Chirpbox/Miscellaneous/Toggle/Src/toggle.c 

OBJS += \
./Src/ll_flash.o \
./Src/toggle.o 

C_DEPS += \
./Src/ll_flash.d \
./Src/toggle.d 


# Each subdirectory must supply rules for building sources it contributes
Src/ll_flash.o: D:/TP/Study/wesg/Chirpbox/Miscellaneous/Toggle/Src/ll_flash.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"D:/TP/Study/wesg/Chirpbox/Miscellaneous/Toggle/Inc" -I"D:/TP/Study/wesg/Chirpbox/Daemon/Src/tools/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/ll_flash.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/toggle.o: D:/TP/Study/wesg/Chirpbox/Miscellaneous/Toggle/Src/toggle.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"D:/TP/Study/wesg/Chirpbox/Miscellaneous/Toggle/Inc" -I"D:/TP/Study/wesg/Chirpbox/Daemon/Src/tools/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/toggle.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

