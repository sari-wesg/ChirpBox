import cbmng
import numpy as np
import time

def generate_command_connect():
    count = 0
    serial = "com8 "

    test_sf = 7
    test_freq = 486300
    test_tp = "0 "
    test_payload_len = "8 "
    for count in range(10):
        for test_freq in np.array([486300]):
            for test_sf in np.array([7, 8, 9, 10, 11, 12]):
                task_topo_run = "cbmng.py " + "-connect "+ str(test_sf) + " " + str(test_freq) + " " + test_tp + "7 " + " " + serial + "80 " + test_payload_len + "14 "
                print(task_topo_run.split())
                cbmng.main(task_topo_run.split())
                task_coltopo_run = "cbmng.py " + "-coltopo " + "2 " + "7 " + "120 " + serial + "80 " + "14 "
                print(task_coltopo_run.split())
                cbmng.main(task_coltopo_run.split())
        count += 1
        print("count", count)
    exit(0)

generate_command_connect()