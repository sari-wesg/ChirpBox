import os
import sys
from serial_send import *

# config and open the serial port
config_port('COM1')

# define the file
filename = 'file/test.bin'

# open the serial, if receive "C", begin to send file
serial_input("C")

# transmit the file
YMODEM_send(filename)

# open the serial
serial_input('')