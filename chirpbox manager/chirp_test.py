import cbmng
import numpy as np
import time

def generate_command_connect(slot_number, used_sf, used_payload_len):
    count = 0
    txpower = 0
    topo_payload_len = used_payload_len
    sf = 7
    # for sf in range(7, 13):
        # for txpower in np.array([0, 7, 14]):
        # payload_len = 1
        # for payload_len in np.array([1, 100, 200]):
        # task_run = -connect 12 470000 -5 7 com12 20 200
    for i in range(1, 50):
        # time.sleep(5)
        task_topo_run = "cbmng.py " + "-connect " + str(sf) + " " + "470000 " + str(txpower) + " " + str(used_sf) + " " + "com12 " + str(slot_number) + " " + str(topo_payload_len) + " "
        print(task_topo_run.split())
        cbmng.main(task_topo_run.split())
        # time.sleep(5)
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