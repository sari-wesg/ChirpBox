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
    payload_len = 232
    dissem_back_sf = 7
    dissem_back_slot = 100
    used_sf = 7
    generation_size = 16
    used_tp = 0
    slot_number = 140
    bitmap = '15'
    task_bitmap = '1fffff'
    slot_number_list = [140, 120, 80, 50]
    dissem_back_slot_list = [100, 80, 60, 30]
    slot_number_list_col = [70, 50, 30, 20]
    # 2,7,8,9,12
    # 3,13,18,15,20
    # 1,11,17,14,4
    task_bitmap_list = ['1fffff', '16FB9F', '4F6C5', '6C5']
    for test_count in range(3):
        for i in range(len(slot_number_list)):
            slot_number = slot_number_list[i]
            dissem_back_slot = dissem_back_slot_list[i]
            task_bitmap = task_bitmap_list[i]
            task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "ffdc " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
            print(task_dissem_run)
            cbmng.main(task_dissem_run.split())

            slot_number = slot_number_list_col[i]
            task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "0 " + "1fffff"
            print(task_coldata_run_1)
            cbmng.main(task_coldata_run_1.split())

            task_coldata_run_2 = "cbmng.py " + "-coldata " + "120 " + "7 " + com_serial + "120 " + "14 " + "1fffff"
            print(task_coldata_run_2)
            cbmng.main(task_coldata_run_2.split())
            count += 1

            print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

