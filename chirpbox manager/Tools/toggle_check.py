import sys
import os

import time
from random import random
from threading import Timer

# insert at 1, 0 is the script path (or '' in REPL)
sys.path.insert(1, os.path.abspath(os.getcwd() + '/../chirpbox manager/'))
import transfer_to_initiator.myserial.serial_send

toggle_check_serial = True

""" alarm function changes the global variable """
def timeout():
    global toggle_check_serial
    print("Alarm!")
    toggle_check_serial = False

""" check if the firmware under test can be toggled back """
def check_toggle():
    # 1. erase flash

    # 2. flash firmware under test

    # 3. flash test daemon, and it will switch to bank2

    # 4. reset the device, and it should come back to daemon

    # 5. check daemon with serial and flash verify

    return True
""" check daemon """
def check_daemon(com_port, alarm_time):
    global toggle_check_serial
    toggle_check_serial = True
    toggle_check_flag = False

    # wait serial for timer in seconds
    t = Timer(alarm_time, timeout)

    # config and open the serial port
    ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)

    # begin clock
    t.start()
    while (toggle_check_serial):
        try:
            line = ser.readline().decode('ascii').strip() # skip the empty data
            if line:
                print(line)
                if (line == "System running from STM32L476 *Bank 1*"):
                    toggle_check_flag = True
                    t.cancel() # clear timer
                    break
        except:
            pass
    print(toggle_check_flag)


def main(argv):
    print(argv)
    check_daemon('com9', 20)


if __name__ == "__main__":
    # execute only if run as a script
    main(sys.argv)
