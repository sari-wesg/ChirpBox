# Real Time Clock Using DS3231

The RTC implementation of DS3231 is based on HAL drivers from ST, including I2C configuration and DS3231 functions.
<!-- In order to respect the STM32Cube template structure, besides DS3231 library (ds3231.c, ds3231.h), variable handlers are also related to project files including: stm32l4xx_hal_msp.c, stm32l4xx_it.c. -->

# Usage
- Modify time: ``DS3231_ModifyTime``.
- Set alarm: ``DS3231_SetAlarm1_Time`` or ``DS3231_SetAlarm2_Time``.
- Alarm handler function: The INT pin on DS3231 should be connected to a GPIO on STM32, and the corresponding interrupt handler is defined in ``DS3231_EXTI_Handler``, see ``toogle.c``.

# Bank switch at an exact time
We provide an example project (two versions compiled in ARM-CC and GCC) to demonstrate the usage of RTC control. The bank will switch to the other at the desired time.
**Prerequisites**:
1. Project must include ``toogle.c`` and ``ll_flash.c``.
2. Control the interrupt handler function of with ``DS3231_EXTI_Handler`` of the GPIO Pin that connected to ``DS3231_INTCN_Pin``.
3. All nodes should correct time with the GPS module.
2. Get the experiment duration and start time. Set alarm at the desired time.

**Not allowed for users**:
1. Do not modify the ``EXTI4_IRQHandler()`` in STM32L476RG, which we use for alarm handler.
2. Do not modify the GPIO of PA4 in STM32L476RG, which is connected to INT pin on DS3231.
