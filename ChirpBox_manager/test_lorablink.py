import cbmng
import numpy as np
import time

def generate_start():
    start_list = ['19', '20', '21', '22', '18']
    print(len(start_list))
    for i in range(len(start_list)):
        start_blink = start_list[i]
        task_dissem_run = "cbmng.py " + "-dissem " + '0 ' + "ade0 " + '232' + " " + '16' + " " + '7' + " " + com_serial + '1fffdf'  + " " + '80' + " " + '7' + " " + '80' + " " + '14' + " " + '1fffff ' + start_blink
        print(task_dissem_run.split())
        cbmng.main(task_dissem_run.split())
        for k in range(1):
            bitmap_str = '1fffdf'
            task_start = "cbmng.py " + "-start " + '0' + " " + "ade0 " + '7' + " " + "com11 " + bitmap_str + " " + '100' + " " + "14 "
            print(task_start.split())
            cbmng.main(task_start.split())
            time.sleep(200)
    exit(0)

# 19-slot 40, 20- slot 30, 21-slot 20, 22-slot 15,
# 4-slot 60, 3-slot 40, 2-slot 20, 1-slot 15
# 4-packet 8, 3-packet 5, 2-packet 2, 1-packet 1
# 4-time 30s, 3-time 20s, 2-time 10s, 1-time 7.5s

#     start_list = ['19', '20', '21', '22', '18']
# 19-slot 40, 20- slot 30, 21-slot 20, 22-slot 15,
# 17-slot 60

# 19-packet 5, 20-packet 4, 21-packet 2, 22-round 1,
# 17-round 8

# 19-packet 75, 20-packet 70, 21-packet 59, 22-packet 77,
# 17-packet 80

# 19-packet 63, 20-packet 57, 21-packet 49, 22-real 66,
# 17-packet 66

# def generate_start():
#     for k in range(10):
#         bitmap_str = '1fffdf'
#         task_start = "cbmng.py " + "-start " + '0' + " " + "ade0 " + '7' + " " + "com11 " + bitmap_str + " " + '100' + " " + "14 "
#         print(task_start.split())
#         cbmng.main(task_start.split())
#         time.sleep(200)
#     exit(0)


com_serial = "com11 "

generate_start()
