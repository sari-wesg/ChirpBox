################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Core/Startup/startup_stm32l476rgtx.s 

OBJS += \
./Core/Startup/startup_stm32l476rgtx.o 

S_DEPS += \
./Core/Startup/startup_stm32l476rgtx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Startup/startup_stm32l476rgtx.o: ../Core/Startup/startup_stm32l476rgtx.s
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 '-DGPI_ARCH_PLATFORM=GPI_ARCH_BOARD_NUCLEOL476RG' '-DASSERT_WARN_CT=0' -DCONFIG_GPIO_AS_PINRESET -DARM_MATH_CM4 '-DMX_CONFIG_FILE=mixer_config.h' -c -I../Daemon/Src -I../Daemon/Src/chirpbox -I../Daemon/Src/loradisc -I../Daemon/Src/mixer -I../Daemon/Src/tools -x assembler-with-cpp -MMD -MP -MF"Core/Startup/startup_stm32l476rgtx.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

