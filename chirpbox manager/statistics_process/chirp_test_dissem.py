import os,sys,inspect
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir)

import cbmng
import numpy as np
import time

# "start_address": "0807E000",
# "end_address": "0807E0D0",
# available:
# [4, 35]
def generate_command_dissem(com_serial):
    count = 0
    payload_len = 200
    dissem_back_sf = 7
    # dissem_back_slot = 60
    used_sf = 7
    config_list = [[1, 10, 60], [1, 6, 60], [4, 35, 60]]
    for i in range(0, len(config_list)):
        generation_size = config_list[i][0]
        slot_number = config_list[i][1]
        dissem_back_slot = config_list[i][2]
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "ac2f " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + "1FFFFF " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " "
        print(generation_size, slot_number, used_sf, count)
        print(task_dissem_run)
        cbmng.main(task_dissem_run.split())

        task_coldata_run = "cbmng.py " + "-coldata " + "120 " + "7 " + com_serial + "200 "
        print(task_coldata_run)
        # time.sleep(300)
        cbmng.main(task_coldata_run.split())
        count += 1

    print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

