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
    used_tp = 14
    slot_number = 140
    bitmap = '15'
    task_bitmap = '1fffff'
    slot_number_list = [70, 65, 60, 55]
    dissem_back_slot_list = [70, 65, 60, 55]
    slot_number_list_col = [70, 65, 60, 55]
    task_bitmap_list = ['17F4DF', '4F6C5', '844D']
    bitmap_list = ['15', '3', '3']
    for test_count in range(3):
        bitmap = bitmap_list[test_count]
        task_bitmap = task_bitmap_list[test_count]

        for i in range(len(slot_number_list)):
            slot_number = slot_number_list[i]
            dissem_back_slot = dissem_back_slot_list[i]

            # 61kb
            task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "1294 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " 50"
            print(task_dissem_run.split())
            cbmng.main(task_dissem_run.split())

            # 2kb
            task_coldata_run_1 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + str(slot_number) + " " + "14 " + task_bitmap + " "+'0807C000 '+'0807C800'
            print(task_coldata_run_1.split())
            cbmng.main(task_coldata_run_1.split())

            # col energy
            task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C800 '+'0807CA40'
            print(task_coldata_run_2.split())
            cbmng.main(task_coldata_run_2.split())
            count = count+1

            print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

