import os,sys,inspect
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir)

import cbmng
import numpy as np
import time

# "start_address": "0807E000",
# "end_address": "0807E0D0",
def generate_command_dissem(com_serial):
    count = 0
    payload_len = 200
    dissem_back_sf = 7
    dissem_back_slot = 80
    used_sf = 7
    config_list = [[4, 30], [4, 50], [8, 50], [8, 70], [12, 70], [12, 90]]
    for i in range(0, len(config_list)):
        generation_size = config_list[i][0]
        slot_number = config_list[i][1]
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "d47f " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + "3FFFFF " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " "
        print(generation_size, slot_number, used_sf, count)
        print(task_dissem_run)
        cbmng.main(task_dissem_run.split())

        task_coldata_run = "cbmng.py " + "-coldata " + "120 " + "9 " + com_serial + "100 "
        print(task_coldata_run)
        # time.sleep(300)
        cbmng.main(task_coldata_run.split())
        count += 1

    print("count", count)
    exit(0)


com_serial = "com20 "
generate_command_dissem(com_serial)

