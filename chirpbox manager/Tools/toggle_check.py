import sys
import os

import time
from random import random
from threading import Timer

# insert at 1, 0 is the script path (or '' in REPL)
sys.path.insert(1, os.path.abspath(os.getcwd() + '/../chirpbox manager/'))
sys.path.insert(1, os.path.abspath(os.getcwd() + '/Tools/pystlink_module/'))
import transfer_to_initiator.myserial.serial_send

from pystlink_module import pystlink

toggle_check_serial = True

""" alarm function changes the global variable """
def timeout():
    global toggle_check_serial
    print("Alarm!")
    toggle_check_serial = False

""" check if the firmware under test can be toggled back """
def check_toggle(FUT):
    # initialize pystlink as pystlink_cmd
    pystlink_cmd = pystlink.PyStlink()
    # reserve sys.argv
    sys_argv_initial = sys.argv

    # 1. flash erase, flash daemon in bank1 and flash firmware under test in bank2
    sys.argv = sys_argv_initial
    sys.argv += ["flash:erase", "flash:verify:0x08000000:"+"Toggle_check.bin", "flash:verify:0x08080000:"+FUT]
    pystlink_cmd.start()

    # 2. The daemon will switch to bank2 and come back in 60 seconds (depending on the alarm settings). Check daemon with serial and flash verify
    time.sleep(60)
    check_daemon("com9", 10)

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
    # check_daemon('com9', 20)
    check_toggle("FUT.bin")


if __name__ == "__main__":
    # execute only if run as a script
    main(sys.argv)
