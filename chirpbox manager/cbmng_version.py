import cbmng
import numpy as np
import time

def test_version(slot_number, used_sf, used_payload_len):
    count = 0
    txpower = 0
    sf = 7
    # for i in range(1, 50):
    for i in range(0, 50):
        task_version = "cbmng.py " + "-colver " + str(sf) + " " + "com11 " + "200 "
        print(task_version.split())
        cbmng.main(task_version.split())
        count += 1
    print("count", count)
    exit(0)

# mixer communication slot number
slot_number = 100
# mixer communication sf
used_sf = 9
# The length of payload to collect data
used_payload_len = 120
test_version(slot_number, used_sf, used_payload_len)