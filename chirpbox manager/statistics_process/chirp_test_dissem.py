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
    # for i in range(1, 50):
    # for generation_size in np.arange(1, 11, 3):
    for generation_size in np.array([4, 8]):
        for slot_number in np.array([30, 50, 70]):
            for used_sf in np.array([7, 9, 10]):
                task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "99c1 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + "3FFFFF " + str(slot_number) + " "
                print(generation_size, slot_number, used_sf, count)
                print(task_dissem_run)
                cbmng.main(task_dissem_run.split())

                task_coldata_run = "cbmng.py " + "-coldata " + "120 " + "9 " + com_serial + "100 "
                print(task_coldata_run)
                time.sleep(300)
                cbmng.main(task_coldata_run.split())
                count += 1

    print("count", count)
    exit(0)


com_serial = "com20 "
generate_command_dissem(com_serial)

