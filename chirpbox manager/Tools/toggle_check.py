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

toggle_check_alarm = True

""" alarm function changes the global variable """
def timeout():
    global toggle_check_alarm
    print("Alarm!")
    toggle_check_alarm = False

""" check if the firmware under test can be toggled back """
def check_toggle(FUT, com_port, alarm_time):
    # initialize pystlink as pystlink_cmd
    pystlink_cmd = pystlink.PyStlink()
    # reserve sys.argv
    sys_argv_initial = sys.argv

    # 1. flash erase, flash daemon in bank1 and flash firmware under test in bank2
    sys.argv = sys_argv_initial
    sys.argv += ["flash:erase", "flash:verify:0x08000000:"+"Toggle-daemon.bin", "flash:verify:0x08080000:"+FUT]
    pystlink_cmd.start()

    # 2. The daemon will switch to bank2 and come back in 60 seconds (depending on the alarm settings). Check daemon with serial
    check_daemon(com_port, alarm_time)

    return True

""" check daemon """
def check_daemon(com_port, alarm_time):
    global toggle_check_alarm
    toggle_check_alarm = True
    toggle_check_result = False
    bank1_run = False

    # wait serial for timer in seconds
    t = Timer(alarm_time, timeout)

    # config and open the serial port
    ser = transfer_to_initiator.myserial.serial_send.config_port(com_port)

    # begin clock
    t.start()
    while (toggle_check_alarm):
        try:
            line = ser.readline().decode('ascii').strip() # skip the empty data
            if line:
                print(line)
                # device run in bank1 second time
                if ((line == "Daemon for testing switch bank") and (bank1_run == True)):
                    toggle_check_result = True
                    t.cancel() # clear timer
                    break
                # device run in bank1 first time
                if ((line == "Daemon for testing switch bank") and (bank1_run == False)):
                    bank1_run = True
                # input command for band switch, see details in "Chirpbox\Miscellaneous\Example\Toggle-daemon\Core\Src\main.c"
                if ((line == "Input initiator task:")):
                    ser_cmd = '{0:01}'.format(int(1))
                    ser.write(str(ser_cmd).encode()) # send commands
        except:
            pass

    print(toggle_check_result)


def main(argv):
    # TODO: change the FUT name and com port name
    check_toggle("FUT.bin", "com17", 60)


if __name__ == "__main__":
    # execute only if run as a script
    main(sys.argv)
