import cbmng
import numpy as np
import time
import sys
import os
# relative to the current working directory
sys.path.append(os.path.join(sys.path[0],'..\\chirpbox manager\\Tools\\chirpbox_tool'))
print(sys.path)
import chirpbox_tool


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

# def generate_start_round():
#     bitmap_str = '1fffff'
#     for i in range(1):
#         task_start = "cbmng.py " + "-start " + '0' + " " + "2f04 " + '7' + " " + "com8 " + bitmap_str + " " + '80' + " " + "14 "
#         print(task_start.split())
#         cbmng.main(task_start.split())
#         time.sleep(1800)
#     exit(0)

def generate_start_round():
    chirpbox_tool_command = "chirpbox_tool.py " + "-sf 7-12 -tp 0 -f 470000 -pl 8 link_quality:measurement"

    bitmap_str = '1fffff'
    task_start = "cbmng.py " + "-colver " + '7' + " " + "com4 " + '80' + " " + "14 "

    for i in range(5):
        print(chirpbox_tool_command.split())
        chirpbox_tool.main(chirpbox_tool_command.split())

        print(task_start.split())
        cbmng.main(task_start.split())

        time.sleep(600)
    exit(0)

generate_start_round()
