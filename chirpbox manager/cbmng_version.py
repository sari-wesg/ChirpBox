import cbmng
import numpy as np
import time

# collect version and coldata time
def test_version():
    count = 0
    sf = 7
    # for i in range(1, 50):
    for i in range(0, 50):
        task_version = "cbmng.py " + "-colver " + str(sf) + " " + "com11 " + "80 "
        print(task_version.split())
        cbmng.main(task_version.split())
        task_coldata_run = "cbmng.py " + "-coldata " + "232 " + "7 " + "com11 " + "80 "
        print(task_coldata_run)
        cbmng.main(task_coldata_run.split())
        count += 1
    print("count", count)
    exit(0)

test_version()