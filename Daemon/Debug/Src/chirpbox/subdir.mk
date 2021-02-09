################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/chirpbox/chirp_gps.c \
../Src/chirpbox/chirp_irqhandler.c \
../Src/chirpbox/chirp_lbt.c \
../Src/chirpbox/chirp_rtc.c \
../Src/chirpbox/chirp_stats.c \
../Src/chirpbox/chirp_topo.c 

OBJS += \
./Src/chirpbox/chirp_gps.o \
./Src/chirpbox/chirp_irqhandler.o \
./Src/chirpbox/chirp_lbt.o \
./Src/chirpbox/chirp_rtc.o \
./Src/chirpbox/chirp_stats.o \
./Src/chirpbox/chirp_topo.o 

C_DEPS += \
./Src/chirpbox/chirp_gps.d \
./Src/chirpbox/chirp_irqhandler.d \
./Src/chirpbox/chirp_lbt.d \
./Src/chirpbox/chirp_rtc.d \
./Src/chirpbox/chirp_stats.d \
./Src/chirpbox/chirp_topo.d 


# Each subdirectory must supply rules for building sources it contributes
Src/chirpbox/chirp_gps.o: ../Src/chirpbox/chirp_gps.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/chirpbox/chirp_gps.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/chirpbox/chirp_irqhandler.o: ../Src/chirpbox/chirp_irqhandler.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/chirpbox/chirp_irqhandler.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/chirpbox/chirp_lbt.o: ../Src/chirpbox/chirp_lbt.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/chirpbox/chirp_lbt.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/chirpbox/chirp_rtc.o: ../Src/chirpbox/chirp_rtc.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/chirpbox/chirp_rtc.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/chirpbox/chirp_stats.o: ../Src/chirpbox/chirp_stats.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/chirpbox/chirp_stats.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/chirpbox/chirp_topo.o: ../Src/chirpbox/chirp_topo.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/chirpbox/chirp_topo.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

