import cbmng
import numpy as np
import time

# collect version and coldata time
def test_lorawan():
	count = 0
	for i in range(0, 2):
		task_lorawan = "cbmng.py " + "-start " + "0 " + "9ff4 " + "7 " + "com11 " + "1fffff " + "80 " + "14 "
		print(task_lorawan.split())
		cbmng.main(task_lorawan.split())
		task_coldata_run = "cbmng.py " + "-coldata " + "232 " + "7 " + "com11 " + "80 " + "14 "
		print(task_coldata_run.split())
		cbmng.main(task_coldata_run.split())
		count += 1
		print("count", count)

	for i in range(0, 4):
		task_lorawan = "cbmng.py " + "-start " + "0 " + "9ff4 " + "7 " + "com11 " + "1909B0 " + "80 " + "14 "
		print(task_lorawan.split())
		cbmng.main(task_lorawan.split())
		task_coldata_run = "cbmng.py " + "-coldata " + "232 " + "7 " + "com11 " + "80 " + "14 "
		print(task_coldata_run.split())
		cbmng.main(task_coldata_run.split())
		count += 1
		print("count", count)

	exit(0)

test_lorawan()