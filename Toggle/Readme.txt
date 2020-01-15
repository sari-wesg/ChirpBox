1) Add "ll_flash.h" and "toggle.h" to your include files path.
2) Add "ll_flash.c" and "toggle.c" to your projects.
3) Your "main.c" should be added to
   "#include toggle.h"
4) Meanwhile, you should add the following codes after your "main()" in the main.c.

   "void Reset_Handler(void)
   { 
	SystemInit();
	if(TOGGLE_RESET_EXTI_CALLBACK()==FLAG_WRT_OK)
		main();
    }"
5) The last page of the active bank is used for user Data except the address 0x0807F800 and 0x0807F804. So the user data offset should be 0x0807F808.You mustn't erase the last page of the active bank. And the data size should not more than 2040 bytes.

6) TOGGLE_RESET_EXTI_CALLBACK() is mapped 0x08000534(bank1) or 0x08080534(bank2),offset is 0x00000534.

0x00000534  0x7C 0xB5 0x00 0x20
0x00000538  0x11 0x4E 0x00 0x90
0x0000053C  0x01 0x90 0x0F 0x24
0x00000540  0x30 0x46 0xFF 0xF7
0x00000584  0x50 0x4D 0x55 0x4A  ('P','M','U','J')