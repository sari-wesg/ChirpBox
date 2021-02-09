################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/gpi/gpi.c 

OBJS += \
./Src/gpi/gpi.o 

C_DEPS += \
./Src/gpi/gpi.d 


# Each subdirectory must supply rules for building sources it contributes
Src/gpi/gpi.o: ../Src/gpi/gpi.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/gpi/gpi.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

