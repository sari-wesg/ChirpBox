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
#     # sf_list = [7,8,9,10,11]
#     # slot_number_list = [130, 110, 90, 85, 80]
#     # payload_len_list = [232, 232, 184, 88, 48]
#     # slot_number_list_col = [130, 110, 90, 85, 80]
#     # slot_number_list_col_without_lbt = [120, 100, 85, 80, 70]

#     sf_list = [10,11]
#     slot_number_list = [85, 80]
#     payload_len_list = [88, 48]
#     slot_number_list_col = [85, 80]
#     slot_number_list_col_without_lbt = [80, 70]
#     for test_count in range(1):
#         for i in range(len(sf_list)):
#             used_sf = sf_list[i]
#             payload_len = payload_len_list[i]
#             # slot_number = slot_number_list[i]
#             # task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "81a5 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
#             # print(task_dissem_run)
#             # cbmng.main(task_dissem_run.split())

#             task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "81a5 " + "48 " + "1 " + "7 " + com_serial + bitmap  + " " + "40 " + str(dissem_back_sf) + " " + "80 " + "14 " + task_bitmap + " "
#             print(task_dissem_run)
#             cbmng.main(task_dissem_run.split())

#             slot_number = slot_number_list_col[i]
#             task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "0 " + "1fffff "+'0807E000 '+'0807E266'
#             print(task_coldata_run_1.split())
#             cbmng.main(task_coldata_run_1.split())

#             task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C8C0 '+'0807CA40'
#             print(task_coldata_run_2.split())
#             cbmng.main(task_coldata_run_2.split())

#             task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E000 '+'0807E0D0'
#             print(task_coldata_run_2.split())
#             cbmng.main(task_coldata_run_2.split())

#             count += 1

#             print("count", count)
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
    payload_len = 48
    dissem_back_sf = 7
    dissem_back_slot = 40
    used_sf = 7
    generation_size = 1
    used_tp = 0
    slot_number = 140
    bitmap = '15'
    task_bitmap = '1fffff'

    for test_count in range(3):
        for i in range(len(sf_list)):
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "81a5 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
        print(task_dissem_run.split())
        cbmng.main(task_dissem_run.split())

            slot_number = slot_number_list_col[test_count][i]
            task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "0 " + "1fffff "+'0807E000 '+'0807E266'
            print(task_coldata_run_1.split())
            cbmng.main(task_coldata_run_1.split())

            task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C8C0 '+'0807CA40'
            print(task_coldata_run_2.split())
            cbmng.main(task_coldata_run_2.split())

            # task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E000 '+'0807E0D0'
            # print(task_coldata_run_2.split())
            # cbmng.main(task_coldata_run_2.split())

            count += 1

            print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

