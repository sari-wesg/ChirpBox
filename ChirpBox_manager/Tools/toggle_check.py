import sys
import os

from threading import Timer

sys.path.append(os.path.join(os.path.dirname(__file__),'..\\..\\ChirpBox_manager\\'))
sys.path.append(os.path.join(os.path.dirname(__file__),'pystlink_module'))
import transfer_to_initiator.myserial.serial_send
import pystlink

toggle_check_alarm = True

""" alarm function changes the global variable """
def timeout():
    global toggle_check_alarm
    print("Alarm!")
    toggle_check_alarm = False

""" check if the firmware under test can be toggled back """
def check_toggle(FUT, serial_sn, com_port, alarm_time = 60):
    # initialize pystlink as pystlink_cmd
    pystlink_command = "pystlink.py "+"-s " + serial_sn + " flash:erase " + "flash:verify:0x08000000:" + str(os.path.join(os.path.dirname(__file__), '..\\..\\Miscellaneous\\Example\\Toggle-daemon\\Debug\\Toggle-daemon.bin')) + " flash:verify:0x08080000:" + FUT
    print(pystlink_command)
    # 1. flash erase, flash daemon in bank1 and flash firmware under test in bank2
    pystlink.main(pystlink_command.split())
    # 2. The daemon will switch to bank2 and come back in 60 seconds (depending on the alarm settings). Check daemon with serial
    is_toggle = check_daemon(com_port, alarm_time)

    return is_toggle

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
    return toggle_check_result


def main(argv):
    # TODO: change the FUT name and com port name
    check_toggle("D:\\TP\Study\ChirpBox\Daemon\Debug\jxr_test.bin", "0670FF515455777867100827", "com3")


if __name__ == "__main__":
    # execute only if run as a script
    main(sys.argv)
