import sys
import argparse
import logging
import os.path
from os import path
import shutil
import datetime
import glob
import csv
from pathlib import Path
if sys.version_info[0] < 3:
    from StringIO import StringIO
else:
    from io import StringIO

import pandas as pd

# add path of chirpbox tools
sys.path.append(os.path.join(os.path.dirname(__file__), '..\\..\\..\\..\\Tools\\chirpbox_tool'))
import lib.txt_to_csv

logger = logging.getLogger(__name__)
logger.propagate = False
logger.setLevel(logging.DEBUG) # <<< Added Line
logging.getLogger('matplotlib.font_manager').disabled = True

# Log config:
# Create handlers
c_handler = logging.StreamHandler()
c_handler.setLevel(logging.DEBUG)

# Create formatters and add it to handlers
c_format = logging.Formatter('[%(filename)s:%(lineno)d] %(asctime)s - %(name)s - %(levelname)s - %(message)s')
c_handler.setFormatter(c_format)

# Add handlers to the logger
logger.addHandler(c_handler)

# Examples:
# logger.warning()
# logger.error()
# logger.info()
# logger.debug()

class LOG_CONST(object):
    __slots__ = ()
    # size of TRACE buffer (number of elements)
    API_TRACE_BUFFER_ELEMENTS   = 32

    # buffer length of arguments
    API_TRACE_LENGTH_ARGUMENT   = 32

    # TRACE buffer entry size, implicitly determines number/size of possible var_args
    FLASH_PAGE                  = 0x800
    API_TRACE_BUFFER_ENTRY_SIZE = int(FLASH_PAGE / API_TRACE_BUFFER_ELEMENTS)
    API_TRACE_LENGTH_VAR        = int((API_TRACE_BUFFER_ENTRY_SIZE - API_TRACE_LENGTH_ARGUMENT) / 4)

def log_txt_interpret(txt_dir, file_start_name):
    # txt to csv:
    lib.txt_to_csv.chirpbox_txt().chirpbox_txt_to_csv(txt_dir, file_start_name)

    # csv to log:
    Path(os.path.join(txt_dir, 'log\\')).mkdir(parents=True, exist_ok=True)
    csv_files = glob.glob(txt_dir + "\\" + file_start_name + "*" + ".csv")
    log_csv_interpret(csv_files, txt_dir, file_start_name)

def log_csv_interpret(csv_files, txt_dir, file_start_name):
    for csv_file in csv_files:
        # read csv file
        with open(csv_file, "rt") as infile:
            read = csv.reader(infile)
            list_read = list(read)
            log_message_allnode = []
            # log of the node in this file
            for i in range(len(list_read)):
                node_id = list_read[i][0]
                node_id = lib.txt_to_csv.chirpbox_txt().twos_complement(node_id, 8)
                log_message = list_read[i][1:]
                log_message = log_message_interpret(log_message)
                log_message.insert(0, node_id)
                log_message_allnode.extend([log_message])
            logger.debug(log_message_allnode)

            # save list to csv
            with open(os.path.join(txt_dir, 'log\\', "log_" + os.path.basename(csv_file)), "w", newline="") as f:
                writer = csv.writer(f)
                writer.writerows(log_message_allnode)

    return True

def log_message_interpret(log_message):
    # logger.debug(log_message)
    log_message_node = []
    for i in range(LOG_CONST.API_TRACE_BUFFER_ELEMENTS):
        # split logs
        one_log = log_message[i*LOG_CONST.API_TRACE_BUFFER_ENTRY_SIZE:(i+1)*LOG_CONST.API_TRACE_BUFFER_ENTRY_SIZE]
        # split argument and variables
        one_log_argument = one_log[0:LOG_CONST.API_TRACE_LENGTH_ARGUMENT]
        one_log_variable = one_log[LOG_CONST.API_TRACE_LENGTH_ARGUMENT:LOG_CONST.API_TRACE_BUFFER_ENTRY_SIZE]
        # change hex to int
        one_log_argument = [int(x, 16) for x in one_log_argument]
        # change byte to char
        one_log_argument = ''.join(chr(i) for i in one_log_argument)
        # remove chars from "\n"
        one_log_argument = one_log_argument.split('\n', 1)[0]
        # change 8 bit byte to 32 bit variables
        variable_list = []
        for k in range(LOG_CONST.API_TRACE_LENGTH_VAR):
            variable_tmp = one_log_variable[k*4:(k+1)*4]
            variable = (int(variable_tmp[3], 16) << 24) + (int(variable_tmp[2], 16) << 16) + (int(variable_tmp[1], 16) << 8) + int(variable_tmp[0], 16)
            variable_list.append(variable)
        log = one_log_argument % tuple(variable_list[:one_log_argument.count('%')])
        if((log == len(log) * log[0]) and ((log[0] == '\x00') or (log[0] == '\xFF'))):
            pass
        else:
            log_message_node.append(log)
    return log_message_node

txt_dir = "C:\\Users\\tecop\\Desktop\\results"
log_txt_interpret(txt_dir, "LoRaWAN_")