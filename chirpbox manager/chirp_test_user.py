import cbmng
import numpy as np
import time

def generate_start_node_id():
    node_num_list = np.arange(1, 20, 1).tolist()
    print(len(node_num_list))
    for i in range(len(node_num_list)):
        bitmap = 1 << node_num_list[i]
        bitmap_str = str(hex(bitmap)[2:])
        task_start = "cbmng.py " + "-start " + '0' + " " + "2f04 " + '7' + " " + "com8 " + bitmap_str + " " + '80' + " " + "14 "
        print(task_start.split())
        cbmng.main(task_start.split())
        time.sleep(900)

    exit(0)

def generate_start_round():
    bitmap_str = '1'
    for i in range(100):
        task_start = "cbmng.py " + "-start " + '0' + " " + "2f04 " + '7' + " " + "com8 " + bitmap_str + " " + '80' + " " + "14 "
        print(task_start.split())
        cbmng.main(task_start.split())
        time.sleep(1800)
    exit(0)

generate_start_round()
