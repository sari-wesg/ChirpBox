import cbmng
import numpy as np
import time

def generate_start():
    node_num_list = [16, 19, 11, 1, 17, 18, 20, 14, 15, 5, 4, 8, 2, 9, 6, 3, 13, 12, 7, 10, 0]
    # node_num_list = [16, 19, 11, 5, 4, 9, 13, 12,]
    print(len(node_num_list))
    for i in range(len(node_num_list)):
        bitmap = 1 << node_num_list[i]
        bitmap_str = str(hex(bitmap)[2:])
        task_start = "cbmng.py " + "-start " + '0' + " " + "9ff4 " + '7' + " " + "com11 " + bitmap_str + " " + '80' + " " + "14 "
        print(task_start.split())
        cbmng.main(task_start.split())

    exit(0)

generate_start()
task_coldata_run = "cbmng.py " + "-coldata " + "232 " + "7 " + "com11 " + "80 " + "14 "
print(task_coldata_run)
cbmng.main(task_coldata_run.split())