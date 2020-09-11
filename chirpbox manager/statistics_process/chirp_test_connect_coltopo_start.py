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
    payload_len = 48
    dissem_back_sf = 7
    dissem_back_slot = 80
    used_sf = 7
    generation_size = 1
    used_tp = 14
    slot_number = 40
    bitmap = '15'
    task_bitmap = '1fffff'
    for test_count in range(2):
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "ffdc " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
        print(task_dissem_run)
        cbmng.main(task_dissem_run.split())

        task_topo_run = "cbmng.py " + "-connect " + "0 " + "470000 " + "0 " + "7 " + " " + "com11 " + "120 " + "8 " + "0 "
        print(task_topo_run.split())
        cbmng.main(task_topo_run.split())

        task_coltopo_run = "cbmng.py " + "-coltopo " + "2 " + "7 " + "120 " + "com11 " + "80 " + "14 "
        print(task_coltopo_run.split())
        cbmng.main(task_coltopo_run.split())

        task_start_run = "cbmng.py " + "-start " + "0 " + "c839 " + "7 " + "com11 " + "1fffff " + "40 " + "14 "
        print(task_start_run.split())
        cbmng.main(task_start_run.split())

        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff"
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        count += 1

        print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

