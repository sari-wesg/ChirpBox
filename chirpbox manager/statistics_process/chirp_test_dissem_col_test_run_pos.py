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
# list
# 4 2 1 6 5 3
# test sequence
# 4 2 1 6 3 5
def generate_command_dissem(com_serial):
    count = 0
    payload_len = 232
    dissem_back_sf = 7
    dissem_back_slot = 100
    used_sf = 7
    generation_size = 16
    used_tp = 0
    slot_number = 140
    bitmap = '15'
    task_bitmap = '1fffff'

    # slot_number_list_dis = [120, 125, 130, 135, 145, 140]
    # slot_number_list_col = [60, 64, 68, 75, 83, 80]
    slot_number_list_dis = [120]
    slot_number_list_col = [60]
    for test_count in range(1):
        for i in range(len(slot_number_list_dis)):
            # slot_number = slot_number_list_dis[i]
            # task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "ffdc " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
            # print(task_dissem_run)
            # cbmng.main(task_dissem_run.split())

            slot_number = slot_number_list_col[i]
            task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "0 " + "1fffff"
            print(task_coldata_run_1)
            cbmng.main(task_coldata_run_1.split())

            task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff"
            print(task_coldata_run_2)
            cbmng.main(task_coldata_run_2.split())
            count += 1
            print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

