import cbmng
import numpy as np
import time

def generate_command_connect(slot_number, used_sf, used_payload_len):
    # task_version = "cbmng.py " + "-colver " + str(7) + " " + "com11 " + "60 " + "14 "
    # print(task_version.split())
    # cbmng.main(task_version.split())
    # task_coldata_run = "cbmng.py " + "-coldata " + "232 " + "7 " + "com11 " + "80 " + "14 "
    # print(task_coldata_run)
    # cbmng.main(task_coldata_run.split())

    count = 0
    txpower = 0
    sf = 7
    topo_payload_len = 8
    freq = 470000
    # for i in range(1, 50):
    for sf in np.array([7]):
        # for freq in np.array([480000]):
        # for freq in np.array([486300, 487100, 487700]):
        # for freq in np.array([486300, 487100, 487700]):
        # for topo_payload_len in np.array([1]):
        task_topo_run = "cbmng.py " + "-connect " + "7 " + "470000 " + "0 " + "7 " + " " + "com11 " + "80 " + "8 " + "14 "
        print(task_topo_run.split())
        cbmng.main(task_topo_run.split())
        task_coltopo_run = "cbmng.py " + "-coltopo " + "2 " + "7 " + "120 " + "com11 " + "80 " + "14 "
        print(task_coltopo_run.split())
        cbmng.main(task_coltopo_run.split())
        count += 1
    print("count", count)
    exit(0)

# mixer communication slot number
slot_number = 10
# mixer communication sf
used_sf = 7
# The length of payload to collect data
used_payload_len = 120
generate_command_connect(slot_number, used_sf, used_payload_len)