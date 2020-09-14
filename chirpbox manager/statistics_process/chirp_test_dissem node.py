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
    # slot_number_list = [130, 130, 80, 140]
    # dissem_back_slot_list = [100, 100, 60, 50]
    # slot_number_list_col = [130, 80, 50, 30]
    # task_bitmap_list = ['1fffff', '17F4DF', '4F6C5', '844D']
    # bitmap_list = ['15', '3', '3']
    slot_number_list = [130, 80]
    dissem_back_slot_list = [100, 60]
    slot_number_list_col = [80, 50]
    task_bitmap_list = ['17F4DF', '4F6C5']
    bitmap_list = ['15', '3']
    for test_count in range(2):
        for i in range(len(slot_number_list)):
            slot_number = slot_number_list[i]
            dissem_back_slot = dissem_back_slot_list[i]
            task_bitmap = task_bitmap_list[i]
            bitmap = bitmap_list[i]
            task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "81a5 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
            print(task_dissem_run)
            cbmng.main(task_dissem_run.split())

            slot_number = slot_number_list_col[i]
            task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "0 " + task_bitmap + " "
            print(task_coldata_run_1)
            cbmng.main(task_coldata_run_1.split())

            task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "80 " + "14 " + "1fffff"
            print(task_coldata_run_2)
            cbmng.main(task_coldata_run_2.split())
            count += 1

            print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

