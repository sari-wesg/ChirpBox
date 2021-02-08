# YMODEM protocol implementation (send)

This work is based on https://github.com/tehmaze/xmodem, which the YMODEM is under development.
Script is implemented with Python 3.7.3.

## Usage
(``serial/example.py`` is running on Windows.)

**1. Config the port of the serial.**

```
config_port('COM1')
```

Other parameters can be changed in ``serial_send.py``

```
ser = serial.Serial(
    baudrate = 115200,\
    parity = serial.PARITY_NONE,\
    stopbits = serial.STOPBITS_ONE,\
    bytesize = serial.EIGHTBITS,\
    timeout = 0.01)
```
**2. Open the file in directory.**
```
filename = 'file/test.bin'
```
**3. Transmit the file.**
```
YMODEM_send(filename)
```


