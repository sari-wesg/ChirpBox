import serial
import sys
import os
sys.path.append(os.path.join(sys.path[0],'..'))
from modem import YMODEM

# Serial config
ser = serial.Serial(
    baudrate = 115200,\
    parity = serial.PARITY_NONE,\
    stopbits = serial.STOPBITS_ONE,\
    bytesize = serial.EIGHTBITS,\
    timeout = 0.01)

def config_port(port):
    global ser
    ser.port = port
    ser.open()

# YMODEM get a char
def getc(size, timeout):
    ser.timeout = timeout
    result = ser.read(1).decode('ascii').strip()
    return (result)

# YMODEM sends data
def putc(data, timeout = 1):
    if type(data) is bytes:
        return ser.write(data)
    else:
        return ser.write(data.encode())

# Send a file through ymodem
def YMODEM_send(filename):
    ymodem = YMODEM(getc, putc)
    ymodem.send(filename)

# Show all serial tracings
def serial_input(string):
    while True:
        try:
            line = ser.readline().decode('ascii').strip()
            if line:
                print (line)
                if (line == string):
                    # ser.close()
                    break
        except:
            pass

