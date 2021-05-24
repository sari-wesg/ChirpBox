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
    dissem_back_slot = 80
    used_sf = 7
    generation_size = 16
    used_tp = 14
    # for slot_number in np.arange(145, 171, 5):
    slot_number = 80
    # bitmap_list = ['fffff', '7FFF', '3FF', '1F']
    # # except node 10, (except node 13, 10, 15, 6, 9), (except node 13, 10, 15, 6, 9,  3, 1, 14, 5,19), (except node 13, 10, 15, 6, 9,  3, 1, 14, 5, 19,   17, 16, 8, 12, 4)
    # 0,
    # 2,7,8,9,12
    # 3,13,18,15,20
    # 1,11,17,14,4
    # task_bitmap_list = ['1fffff', '16FB9F', '14B38D', '1385']
    for i in range(10):
        # bitmap = bitmap_list[i]
        # task_bitmap = task_bitmap_list[i]
        bitmap = '1fffff'
        task_bitmap = '1fffff'
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "66bc " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
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

