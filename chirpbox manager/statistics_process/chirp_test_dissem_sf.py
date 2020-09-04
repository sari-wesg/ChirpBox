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
    232, 232, 184, 88, 48
    payload_len = 232
    dissem_back_sf = 7
    dissem_back_slot = 80
    used_sf = 7
    generation_size = 16
    used_tp = 0
    # for slot_number in np.arange(145, 171, 5):
    slot_number = 80
    bitmap = '1fffff'
    task_bitmap = '1fffff'
    sf_list = [9, 10, 11]
    for i in range(len(sf_list)):
        used_sf = sf_list[i]
        dissem_back_sf = sf_list[i]
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "f4d4 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
        print(task_dissem_run)
        cbmng.main(task_dissem_run.split())

        task_coldata_run = "cbmng.py " + "-coldata " + "120 " + "7 " + com_serial + "120 " + "14 " + "1fffff"
        print(task_coldata_run)
        # time.sleep(300)
        cbmng.main(task_coldata_run.split())
        count += 1

        print("count", count)
        exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

