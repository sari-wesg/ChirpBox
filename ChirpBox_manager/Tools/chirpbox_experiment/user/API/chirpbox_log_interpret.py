import sys
import argparse
import logging
import os.path
from os import path
import shutil
import datetime
import glob
import csv

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

def log_txt(txt_dir, file_start_name):
    # change txt to csv:
    lib.txt_to_csv.chirpbox_txt().chirpbox_txt_to_csv(txt_dir, file_start_name)

    # csv to log arguments and variables
    csv_files = glob.glob(txt_dir + "\\" + file_start_name + "*" + ".csv")
    log_csv_interpret(csv_files)

def log_message_interpret(csv_file, node_id, log_message):
    # logger.debug(log_message)
    for i in range(LOG_CONST.API_TRACE_BUFFER_ELEMENTS):
        one_log = log_message[i*LOG_CONST.API_TRACE_BUFFER_ENTRY_SIZE:(i+1)*LOG_CONST.API_TRACE_BUFFER_ENTRY_SIZE]
        one_log_argument = one_log[0:LOG_CONST.API_TRACE_LENGTH_ARGUMENT]
        one_log_variable = one_log[LOG_CONST.API_TRACE_LENGTH_ARGUMENT:LOG_CONST.API_TRACE_BUFFER_ENTRY_SIZE]
        if(node_id == 0) and (i == 0):
            one_log_argument = [int(x, 16) for x in one_log_argument]
            one_log_argument = ''.join(chr(i) for i in one_log_argument)
            logger.debug(one_log_argument)
            logger.debug(one_log_variable)

def log_csv_interpret(csv_files):
    for csv_file in csv_files:
        # obtain the node total number
        with open(csv_file, "rt") as infile:
            logger.debug(csv_file)
            read = csv.reader(infile)
            list_read = list(read)
            for i in range(len(list_read)):
                node_id = list_read[i][0]
                node_id = lib.txt_to_csv.chirpbox_txt().twos_complement(node_id, 8)
                log_message = list_read[i][1:]
                log_message_interpret(csv_file, node_id, log_message)
    return True

txt_dir = "C:\\Users\\tecop\\Desktop\\results"
log_txt(txt_dir, "LoRaWAN_")