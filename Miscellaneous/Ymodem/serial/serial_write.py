import serial
import os
import sys

# Serial config
ser = serial.Serial(
    port ='COM11',\
    baudrate = 115200,\
    parity = serial.PARITY_NONE,\
    stopbits = serial.STOPBITS_ONE,\
    bytesize = serial.EIGHTBITS,\
    timeout = 0.01)

print("Starting up")
commandToSend = 2

# Wait for the STM32 sending 'C'
while True:
    try:
        line = ser.readline().decode('ascii').strip() # skip the empty data
        if line:
            print (line)
            if (line == "Input initiator task:"):
                ser.write(str(commandToSend).encode()) # send commands
            if (line == "C"):
                break
    except:
        pass
    # ser.flush() #flush the buffer
