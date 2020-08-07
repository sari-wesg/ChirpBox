import cbmng
import numpy as np
import time

def generate_command_connect(slot_number, used_sf, used_payload_len):
    count = 0
    txpower = 0
    sf = 7
    # for i in range(1, 50):
    for sf in range(12, 13):
        for txpower in np.array([0, 14]):
        # payload_len = 1
            for topo_payload_len in np.array([120]):
                task_topo_run = "cbmng.py " + "-connect " + str(sf) + " " + "470000 " + str(txpower) + " " + str(used_sf) + " " + "com12 " + str(slot_number) + " " + str(topo_payload_len) + " "
                print(task_topo_run.split())
                cbmng.main(task_topo_run.split())
                task_coltopo_run = "cbmng.py " + "-coltopo " + "2 " + str(used_sf) + " " + str(used_payload_len) + " " + "com12 " + str(slot_number) + " "
                print(task_coltopo_run.split())
                cbmng.main(task_coltopo_run.split())
                count += 1
    print("count", count)
    exit(0)

# mixer communication slot number
slot_number = 100
# mixer communication sf
used_sf = 9
# The length of payload to collect data
used_payload_len = 120
generate_command_connect(slot_number, used_sf, used_payload_len)