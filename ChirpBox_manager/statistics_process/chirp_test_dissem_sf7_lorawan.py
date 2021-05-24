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
    dissem_back_slot = 80
    used_sf = 7
    generation_size = 16
    used_tp = 14
    slot_number = 80
    bitmap = '15'
    task_bitmap = '1fffff'

    # whole firmware
    for test_count in range(1):

        # 61kb
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "1294 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " 61"
        print(task_dissem_run.split())
        cbmng.main(task_dissem_run.split())

        # 2kb
        task_coldata_run_1 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "60 " + "14 " + "1fffff "+'0807C000 '+'0807C800'
        print(task_coldata_run_1.split())
        cbmng.main(task_coldata_run_1.split())

        # col lbt
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E080 '+'0807E0D0'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        # start
        task_start_run = "cbmng.py " + "-start " + "0 " + "c839 " + "7 " + "com11 " + "1fffff " + "80 " + "14 "
        print(task_start_run.split())
        cbmng.main(task_start_run.split())

        # col energy
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C800 '+'0807CA40'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        count += 1

        print("count", count)

    # patch
    for test_count in range(1):
        # 4kb
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "1294 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " 0"
        print(task_dissem_run.split())
        cbmng.main(task_dissem_run.split())

        # 2kb
        task_coldata_run_1 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "60 " + "14 " + "1fffff "+'0807C000 '+'0807C800'
        print(task_coldata_run_1.split())
        cbmng.main(task_coldata_run_1.split())

        # col lbt
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E080 '+'0807E0D0'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        # start
        task_start_run = "cbmng.py " + "-start " + "0 " + "c839 " + "7 " + "com11 " + "1fffff " + "80 " + "14 "
        print(task_start_run.split())
        cbmng.main(task_start_run.split())

        # col lbt
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E080 '+'0807E0D0'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        # col energy
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C800 '+'0807CA40'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        count += 1

        print("count", count)

    # start
    for test_count in range(1):
        # 0kb
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "1294 " + str(payload_len) + " " + str(generation_size) + " " + str(used_sf) + " " + com_serial + bitmap  + " " + str(slot_number) + " " + str(dissem_back_sf) + " " + str(dissem_back_slot) + " " + str(used_tp) + " " + task_bitmap + " 00"
        print(task_dissem_run.split())
        cbmng.main(task_dissem_run.split())

        # 2kb
        task_coldata_run_1 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "60 " + "14 " + "1fffff "+'0807C000 '+'0807C800'
        print(task_coldata_run_1.split())
        cbmng.main(task_coldata_run_1.split())

        # col lbt
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E080 '+'0807E0D0'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        # start
        task_start_run = "cbmng.py " + "-start " + "0 " + "c839 " + "7 " + "com11 " + "1fffff " + "80 " + "14 "
        print(task_start_run.split())
        cbmng.main(task_start_run.split())

        # col lbt
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E080 '+'0807E0D0'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        # col energy
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C800 '+'0807CA40'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        count += 1

        print("count", count)

    for test_count in range(1):
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "1294 " + "48 " + "1 " + "7 " + com_serial + bitmap  + " " + "80 " + "7 " + "80 " + "14 " + task_bitmap + " 00"
        print(task_dissem_run.split())
        cbmng.main(task_dissem_run.split())

        task_topo_run = "cbmng.py " + "-connect " + "12 " + "470000 " + "0 " + "7 " + " " + "com11 " + "120 " + "196 " + "0 "
        print(task_topo_run.split())
        cbmng.main(task_topo_run.split())

        # 960 bytes
        task_coldata_run_1 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "80 " + "14 " + "1fffff "+'0807C000 '+'0807C3C0'
        print(task_coldata_run_1.split())
        cbmng.main(task_coldata_run_1.split())

        # energy
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807C980 '+'0807CB00'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

        # col lbt
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "120 " + "14 " + "1fffff "+'0807E080 '+'0807E0D0'
        print(task_coldata_run_2.split())
        cbmng.main(task_coldata_run_2.split())

    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

