import cbmng
import numpy as np
import time
import sys
import os
# relative to the current working directory
sys.path.append(os.path.join(os.path.dirname(__file__),'..\\Chirpbox_manager\\Tools\\chirpbox_tool'))
print(sys.path)
import chirpbox_tool

def generate_start_round():
    chirpbox_tool_command = "chirpbox_tool.py " + "-sf 7-12 -tp 0 -f 470000,480000,490000 -pl 8 link_quality:measurement"

    task_start = "cbmng.py " + "-colver " + '7' + " " + "com3 " + '80' + " " + "14 "

    for i in range(10000):
        print(chirpbox_tool_command.split())
        chirpbox_tool.main(chirpbox_tool_command.split())
        print(task_start.split())
        cbmng.main(task_start.split())
    exit(0)

generate_start_round()
