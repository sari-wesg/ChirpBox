import json
import glob
import os
from datetime import datetime, timezone
import pandas as pd
import statistics
import sys
import logging


logger = logging.getLogger(__name__)
logger.propagate = False
logger.setLevel(logging.DEBUG) # <<< Added Line

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

def bitmap_to_nodes(bitmap):
    bitmap = int(bitmap, 16)
    node_list = []
    i = 0
    while (bitmap):
        i += 1
        bitmap = bitmap >> 1
        if(bitmap & 0b1):
            node_list.append(i)

    return node_list

def gateway_log_count(start_time_utc, end_time_utc, gateway_log, experiment_nodes, UID_list):
    df = pd.read_csv(gateway_log, sep=',')
    df = df[df.iloc[:,0].between(start_time_utc, end_time_utc)]
    uplink_count_list = []
    for node in experiment_nodes:
        LoRaWAN_device_ID = ('00000000' + (UID_list[node])[2:]).lower()
        uplink_count = df.iloc[:,1].str.count(str(LoRaWAN_device_ID)).sum()
        uplink_count_list.append(uplink_count)
    return uplink_count_list

def uplink_pdr(user_name, experiment_settings_dir, log_dir, gateway_log, round_robin):

    with open(os.path.join(os.path.dirname(__file__), "../../../../../ChirpBox_manager//Tools//param_patch//param_patch_daemon.json")) as data_file:
        data = json.load(data_file)
        UID_list = data['UID_list']
        UID_list = UID_list.split(', ')

    # find all experiments settings (config files)
    experiment_settings = glob.glob(os.path.join(experiment_settings_dir, user_name + "_*.json"))
    for experiment_setting in experiment_settings:
        # read config one by one
        with open(experiment_setting) as data_file:
            data = json.load(data_file)
            experiment_name = data['experiment_name']
            duration = data['experiment_duration']
            experiment_nodes = bitmap_to_nodes(data['experiment_run_bitmap'])
            uplink_total = int((experiment_name.split('packet')[0])[len("LoRaWAN_"):])
            logger.debug("experiment_name:%s, duration:%s, experiment_nodes:%s\n", experiment_name, duration, experiment_nodes)

            # find all logs belong to this experiment
            experiment_log = glob.glob(log_dir + "\\log_" + experiment_name + "*.csv")

            for file in experiment_log:
                # read log one by one
                start_time = os.path.basename(file)[-len("20210605023553252823).csv"):-len("123456).csv")]
                # convert to utc timestamp
                dt = datetime(int(start_time[0:4]), int(start_time[4:6]), int(start_time[6:8]), int(start_time[8:10]), int(start_time[10:12]), int(start_time[12:14]), 0)
                start_time_utc = dt.timestamp()
                end_time_utc = start_time_utc + duration

                # round robin:
                if round_robin is True:
                    start_time_utc = start_time_utc - (duration + 600) * (len(experiment_nodes) - 1)

                nodes_real_uplink_total = gateway_log_count(start_time_utc, end_time_utc, gateway_log, experiment_nodes, UID_list)
                pdr_list = [(x / uplink_total)* 100 for x in nodes_real_uplink_total]
                logger.debug(pdr_list)
                if (len(pdr_list) > 1):
                    logger.debug(statistics.mean(pdr_list))
                    logger.debug(statistics.stdev(pdr_list))
                else:
                    logger.debug(statistics.mean(pdr_list))
                    logger.debug(0)

def main(argv):
    # argv[1] = User_name
    # argv[2] = experiment_settings_dir
    # argv[3] = log_dir
    # argv[4] = gateway_log_file
    # argv[5] = round_robin
    uplink_pdr(argv[1], argv[2], argv[3], argv[4], argv[5].lower() == 'true')

if __name__ == "__main__":
    main(sys.argv)