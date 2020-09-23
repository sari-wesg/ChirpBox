import os,sys,inspect
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir)

import cbmng
import numpy as np
import time

def generate_command_dissem(com_serial):
    count = 0
    test_sf = 7
    sf_list = [7,12,8,9,10,11]
    for i in range(len(sf_list)):
        test_sf = sf_list[i]
        task_topo_run = "cbmng.py " + "-connect " +str(test_sf) + " " + "470000 " + "0 " + "7 " + "com11 " + "80 " + "6 " + "14 "
        print(task_topo_run.split())
        cbmng.main(task_topo_run.split())

        # col topo
        task_coldata_run_2 = "cbmng.py " + "-coldata " + "232 " + "7 " + com_serial + "80 " + "14 " + "1fffff "+'0807F800 '+'0807F8D0'
        print(task_coldata_run_2)
        cbmng.main(task_coldata_run_2.split())

        count += 1

        print("count", count)
    exit(0)


com_serial = "com11 "
generate_command_dissem(com_serial)

