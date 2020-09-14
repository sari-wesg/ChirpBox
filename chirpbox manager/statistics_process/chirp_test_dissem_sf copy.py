# import os,sys,inspect
# currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
# parentdir = os.path.dirname(currentdir)
# sys.path.insert(0,parentdir)

# import cbmng
# import numpy as np
# import time

# # "start_address": "0807E000",
# # "end_address": "0807E0D0",
# # available:
# # [4, 35]
# def generate_command_dissem(com_serial):
#     count = 0
#     payload_len = 232
#     dissem_back_sf = 7
#     dissem_back_slot = 100
#     used_sf = 7
#     generation_size = 16
#     used_tp = 0
#     slot_number = 140
#     bitmap = '15'
#     task_bitmap = '1fffff'
#     used_sf = 7
#     slot_number = 130
#     payload_len = 232
#     task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "81a5 " + "232 " + "16 " + "7 " + com_serial + bitmap  + " " + "130 " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
#     print(task_dissem_run.split())
#     cbmng.main(task_dissem_run.split())

#     slot_number = 130
#     task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "0 " + "1fffff"
#     print(task_coldata_run_1.split())
#     cbmng.main(task_coldata_run_1.split())

#     count += 1

#     print("count", count)
#     exit(0)


# com_serial = "com11 "
# generate_command_dissem(com_serial)

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
    dissem_back_sf = 7
    dissem_back_slot = 100
    used_sf = 11
    generation_size = 16
    used_tp = 0
    bitmap = '15'
    task_bitmap = '1fffff'
    slot_number = 80
    payload_len = 48
    task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "81a5 " + "88 " + "16 " + "11 " + com_serial + bitmap  + " " + "80 " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
    print(task_dissem_run.split())
    cbmng.main(task_dissem_run.split())

    slot_number = 70
    task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "0 " + "1fffff"
    print(task_coldata_run_1.split())
    cbmng.main(task_coldata_run_1.split())

    count += 1

    print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

