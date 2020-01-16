1) Add "ll_flash.c" and "toggle.c" to your projects.

2) The last page of the active bank is used for user Data e. So the user data offset should be 0x0807F808.The data size should not more than 2048 bytes.

3) TOGGLE_RESET_EXTI_CALLBACK() is mapped 0x08000534(bank1) or 0x08080534(bank2),offset is 0x00000534.

0x00000534  0x7C 0xB5 0x00 0x20
0x00000538  0x11 0x4E 0x00 0x90
0x0000053C  0x01 0x90 0x0F 0x24
0x00000540  0x30 0x46 0xFF 0xF7
0x00000584  0x50 0x4D 0x55 0x4A  ('P','M','U','J')

PS. The project for the stm32476 board have included "stm32l4xx.h","stm32l476xx.h","system_stm32l4xx.h","system_stm32l4xx.c" and "startup_stm32l476xx.s"(./Toggle/STM32L476)