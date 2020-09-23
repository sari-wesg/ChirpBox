# # import os,sys,inspect
# # currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
# # parentdir = os.path.dirname(currentdir)
# # sys.path.insert(0,parentdir)

# # import cbmng
# # import numpy as np
# # import time

# # # "start_address": "0807E000",
# # # "end_address": "0807E0D0",
# # # available:
# # # [4, 35]
# # def generate_command_dissem(com_serial):
# #     count = 0
# #     payload_len = 232
# #     dissem_back_sf = 7
# #     dissem_back_slot = 100
# #     used_sf = 7
# #     generation_size = 16
# #     used_tp = 0
# #     slot_number = 140
# #     bitmap = '15'
# #     task_bitmap = '1fffff'
# #     # sf_list = [7,8,9,10,11]
# #     # slot_number_list = [130, 110, 90, 85, 80]
# #     # payload_len_list = [232, 232, 184, 88, 48]
# #     # slot_number_list_col = [130, 110, 90, 85, 80]
# #     # slot_number_list_col_without_lbt = [120, 100, 85, 80, 70]

# #     sf_list = [10,11]
# #     slot_number_list = [85, 80]
# #     payload_len_list = [88, 48]
# #     slot_number_list_col = [85, 80]
# #     slot_number_list_col_without_lbt = [80, 70]
# #     for test_count in range(1):
# #         for i in range(len(sf_list)):
# #             used_sf = sf_list[i]
# #             payload_len = payload_len_list[i]
# #             # slot_number = slot_number_list[i]
# #             # task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "81a5 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " "
# #             # print(task_dissem_run)
# #             # cbmng.main(task_dissem_run.split())

# #             task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "81a5 " + "48 " + "1 " + "7 " + com_serial + bitmap  + " " + "40 " + str(dissem_back_sf) + " " + "80 " + "14 " + task_bitmap + " "
# #             print(task_dissem_run)
# #             cbmng.main(task_dissem_run.split())

# #             slot_number = slot_number_list_col[i]
# #             task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "0 " + "1fffff "+'0807E000 '+'0807E266'
# #             print(task_coldata_run_1.split())
# #             cbmng.main(task_coldata_run_1.split())

# #             task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C8C0 '+'0807CA40'
# #             print(task_coldata_run_2.split())
# #             cbmng.main(task_coldata_run_2.split())

# #             task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E000 '+'0807E0D0'
# #             print(task_coldata_run_2.split())
# #             cbmng.main(task_coldata_run_2.split())

# #             count += 1

# #             print("count", count)
# #     exit(0)


# # com_serial = "com11 "
# # generate_command_dissem(com_serial)


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
#     # sf_list = [11,10,9,8,7]
#     sf_list = [11]
#     slot_number_list = [[65, 75, 85, 95, 105],[70, 80, 90, 100, 110],[75, 85, 95, 105, 115],[65, 75, 85, 95, 105],[70, 80, 90, 100, 110],[75, 85, 95, 105, 115],[80, 90, 100, 110, 120],[85, 95, 105, 115, 125],[90, 100, 110, 120, 130],[80, 90, 100, 110, 120],[85, 95, 105, 115, 125],[90, 100, 110, 120, 130]]
#     payload_len_list = [48, 88, 184, 232, 232]
#     slot_number_list_col = [[65, 75, 85, 95, 105],[70, 80, 90, 100, 110],[75, 85, 95, 105, 115],[65, 75, 85, 95, 105],[70, 80, 90, 100, 110],[75, 85, 95, 105, 115],[80, 90, 100, 110, 120],[85, 95, 105, 115, 125],[90, 100, 110, 120, 130],[80, 90, 100, 110, 120],[85, 95, 105, 115, 125],[90, 100, 110, 120, 130]]


#     for test_count in range(len(slot_number_list)):
#         for i in range(len(sf_list)):
#             used_sf = sf_list[i]
#             payload_len = payload_len_list[i]
#             slot_number = slot_number_list[test_count][i]
#             task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "81a5 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " 3"
#             print(task_dissem_run.split())
#             cbmng.main(task_dissem_run.split())

#             slot_number = slot_number_list_col[test_count][i]
#             task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "0 " + "1fffff "+'0807E000 '+'0807E266'
#             print(task_coldata_run_1.split())
#             cbmng.main(task_coldata_run_1.split())

#             task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C8C0 '+'0807CA40'
#             print(task_coldata_run_2.split())
#             cbmng.main(task_coldata_run_2.split())

#             # task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E000 '+'0807E0D0'
#             # print(task_coldata_run_2.split())
#             # cbmng.main(task_coldata_run_2.split())

#             count += 1

#             print("count", count)
#     exit(0)


# com_serial = "com11 "
# generate_command_dissem(com_serial)


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
    payload_len = 232
    dissem_back_sf = 7
    dissem_back_slot = 100
    used_sf = 7
    generation_size = 16
    used_tp = 14
    slot_number = 140
    bitmap = '15'
    task_bitmap = '1fffff'
    # sf_list = [11,10,9,8,7]
    # slot_number_list = [80, 90, 90, 90, 100]
    # payload_len_list = [48, 88, 184, 232, 232]
    # slot_number_list_col = [45, 65, 70, 80, 100]
    sf_list = [7]
    slot_number_list = [100]
    payload_len_list = [232]
    slot_number_list_col = [100]
    # 12800,26880,64768,132608,175616
    # 1680,3360,7392,10976,16352
    # dissem_length = [" 12800", " 26880", " 64768", " 132608", " 175616"]
    # col_length = ["0807E690", "0807ED20", "0807FCE0", "08080AE0", "08081FE0"]
    dissem_length = [" 175616"]
    col_length = ["08081FE0"]
    dissem_length_sf = " 3"
    col_length_sf = "0807E266"
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
        task_coldata_run_1 = "cbmng.py " + "-coldata " + str(payload_len) + " " + str(used_sf) + " " + com_serial + str(slot_number) + " " + "14 " + "1fffff "+'0807E000 '+col_length_sf
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

