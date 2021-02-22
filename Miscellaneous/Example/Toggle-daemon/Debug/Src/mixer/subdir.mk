################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/mixer/mixer.c \
../Src/mixer/mixer_data.c \
../Src/mixer/mixer_history.c \
../Src/mixer/mixer_processing.c \
../Src/mixer/mixer_rand.c \
../Src/mixer/mixer_request.c 

OBJS += \
./Src/mixer/mixer.o \
./Src/mixer/mixer_data.o \
./Src/mixer/mixer_history.o \
./Src/mixer/mixer_processing.o \
./Src/mixer/mixer_rand.o \
./Src/mixer/mixer_request.o 

C_DEPS += \
./Src/mixer/mixer.d \
./Src/mixer/mixer_data.d \
./Src/mixer/mixer_history.d \
./Src/mixer/mixer_processing.d \
./Src/mixer/mixer_rand.d \
./Src/mixer/mixer_request.d 


# Each subdirectory must supply rules for building sources it contributes
Src/mixer/mixer.o: ../Src/mixer/mixer.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/mixer/mixer.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/mixer/mixer_data.o: ../Src/mixer/mixer_data.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/mixer/mixer_data.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/mixer/mixer_history.o: ../Src/mixer/mixer_history.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/mixer/mixer_history.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/mixer/mixer_processing.o: ../Src/mixer/mixer_processing.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/mixer/mixer_processing.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/mixer/mixer_rand.o: ../Src/mixer/mixer_rand.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/mixer/mixer_rand.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/mixer/mixer_request.o: ../Src/mixer/mixer_request.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L476xx -DDEBUG '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Src -I../Src/chirpbox -I../Src/mixer -I../Src/tools/Inc -I../Src/loradisc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/mixer/mixer_request.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

