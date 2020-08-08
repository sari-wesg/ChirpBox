import cbmng
import numpy as np
import time

def test_alltoall(slot_number, used_sf, used_payload_len):
    count = 0
    # for i in range(1, 50):
    for i in range(0, 50):
        task_sniffer = "cbmng.py " + "-assignsnf " + str(used_sf) + " " + "com11 "  + str(slot_number) + " "
        print(task_sniffer.split())
        cbmng.main(task_sniffer.split())

        task_coldata_run = "cbmng.py " + "-coldata " + "120 " + "7 " + com_serial + "80 "
        print(task_coldata_run)
        # time.sleep(300)
        cbmng.main(task_coldata_run.split())
        count += 1
    print("count", count)
    exit(0)

# mixer communication slot number
slot_number = 60
# mixer communication sf
used_sf = 7
# The length of payload to collect data
used_payload_len = 8
test_alltoall(slot_number, used_sf, used_payload_len)