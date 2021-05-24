import cbmng
import numpy as np
import time

def generate_start():
    # node_num_list = [16, 19, 11, 1, 17, 18, 20, 14, 15, 5, 4, 8, 2, 9, 6, 3, 13, 12, 7, 10, 0]
    # node_num_list = [16, 19, 11, 5, 4, 9, 13, 12,]
    # start_list = ['1fffff', '1B5D7B', '1F1173']
    start_list = ['1fffff']
    # start_list = ['19DE3B', '1D0031']
    print(len(start_list))
    for i in range(len(start_list)):
        for k in range(3):
            bitmap_str = str(start_list[i])
            task_start = "cbmng.py " + "-start " + '0' + " " + "81a5 " + '7' + " " + "com11 " + bitmap_str + " " + '80' + " " + "14 "
            print(task_start.split())
            cbmng.main(task_start.split())
            time.sleep(300)

            task_colver_run = "cbmng.py " + "-colver " + "7 " + "com11 " + "80 " + "14 "
            print(task_colver_run.split())
            cbmng.main(task_colver_run.split())

            task_coldata_run = "cbmng.py " + "-coldata " + "232 " + "7 " + "com11 " + "120 " + "14 " + "1fffff "+'80FD000 ' + '80FD800'
            print(task_coldata_run.split())
            cbmng.main(task_coldata_run.split())
    exit(0)

generate_start()
