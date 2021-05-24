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
    # sf_list = [11, 10, 9, 8, 7]
    # slot_number_list = [75, 75, 75, 80, 80]
    # payload_len_list = [48, 88, 184, 232, 232]
    # 81 round
    sf_list = [11]
    slot_number_list = [75]
    payload_len_list = [48]
    dissem_length_sf = " 50"
    # for test_count in range(1):
    #     for i in range(len(sf_list)):
    #         used_sf = sf_list[i]
    #         slot_number = slot_number_list[i]
    #         payload_len = payload_len_list[i]
    #         task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "ade0 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + dissem_length_sf
    #         print(task_dissem_run.split())
    #         cbmng.main(task_dissem_run.split())

    #         # col 2 kb
    #         task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "14 " + "1fffff "+'0807E000 '+"0807E800"
    #         print(task_coldata_run_1.split())
    #         cbmng.main(task_coldata_run_1.split())

    #         # col lbt and round number
    #         task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E000 '+'0807E0D0'
    #         # task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E080 '+'0807E0D0'
    #         print(task_coldata_run_2.split())
    #         cbmng.main(task_coldata_run_2.split())

    #         # col energy
    #         task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C8C0 '+'0807CA40'
    #         print(task_coldata_run_2.split())
    #         cbmng.main(task_coldata_run_2.split())

    #         count += 1

    #         print("count", count)

    bitmap = '1FFFDF'
    for i in range(len(sf_list)):
        used_sf = sf_list[i]
        slot_number = slot_number_list[i]
        payload_len = payload_len_list[i]
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "ade0 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + dissem_length_sf
        print(task_dissem_run.split())
        cbmng.main(task_dissem_run.split())

        # col 2 kb
        task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "14 " + "1fffff "+'0807E000 '+"0807E800"
        print(task_coldata_run_1.split())
        cbmng.main(task_coldata_run_1.split())

        # col lbt and round number
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E000 '+'0807E0D0'
        # task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E080 '+'0807E0D0'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        # col energy
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C8C0 '+'0807CA40'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        count += 1

        print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

