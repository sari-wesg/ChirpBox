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
    used_sf = 12
    generation_size = 4
    used_tp = 0
    # for slot_number in np.arange(145, 171, 5):
    slot_number = 20
    task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "ac2f " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + "1FFFFF " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " "
    print(generation_size, slot_number, used_sf, count)
    print(task_dissem_run)
    cbmng.main(task_dissem_run.split())

    task_coldata_run = "cbmng.py " + "-coldata " + "120 " + "7 " + com_serial + "120 " + "14 "
    print(task_coldata_run)
    # time.sleep(300)
    cbmng.main(task_coldata_run.split())
    count += 1

    print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

