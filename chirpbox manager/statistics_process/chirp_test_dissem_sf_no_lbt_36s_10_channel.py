import os,sys,inspect
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir)

import cbmng
import numpy as np
import time

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
    sf_list = [11,10,9,8,7]
    slot_number_list = [80, 90, 90, 90, 100]
    payload_len_list = [48, 88, 184, 232, 232]
    slot_number_list_col = [45, 65, 70, 80, 100]
    # 5760,11520,28160,75264,111104
    # 1240,2480,5456,9184,13664
    dissem_length = [" 5760", " 11520", " 28160", " 75264", " 111104"]
    col_length = ["0807E4D8", "0807E9B0", "0807F550", "080803E0", "08081560"]
    col_length_sf = "08073560"
    for i in range(len(sf_list)):
        used_sf = sf_list[i]
        payload_len = payload_len_list[i]
        slot_number = slot_number_list[i]
        dissem_length_sf = dissem_length[i]
        col_length_sf = col_length[i]
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "187c " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + dissem_length_sf
        print(task_dissem_run)
        cbmng.main(task_dissem_run.split())

        slot_number = slot_number_list_col[i]
        task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "14 " + "1fffff "+'08070000 '+col_length_sf
        print(task_coldata_run_1)
        cbmng.main(task_coldata_run_1.split())

        # col lbt
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E080 '+'0807E0D0'
        print(task_coldata_run_2)
        cbmng.main(task_coldata_run_2.split())

        # col energy
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C8C0 '+'0807CA40'
        print(task_coldata_run_2)
        cbmng.main(task_coldata_run_2.split())

        count += 1

        print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

