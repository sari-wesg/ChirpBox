import json
import glob
import os
from datetime import datetime, timezone
import pandas as pd
import statistics

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

def uplink_pdr(user_name, experiment_settings_dir, log_dir, gateway_log):

    with open(os.path.join(os.path.dirname(__file__), "../../../../../ChirpBox_manager//Tools//param_patch//param_patch_daemon.json")) as data_file:
        data = json.load(data_file)
        UID_list = data['UID_list']
        UID_list = UID_list.split(', ')
    print (UID_list)

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
            print("experiment_name:", experiment_name, "duration:", duration, "experiment_nodes:", experiment_nodes)

            # find all logs belong to this experiment
            experiment_log = glob.glob(log_dir + "\\log_" + experiment_name + "*.csv")

            # no round robin:
            for file in experiment_log:
                # read log one by one
                start_time = os.path.basename(file)[-len("20210605023553252823).csv"):-len("123456).csv")]
                # convert to utc timestamp
                dt = datetime(int(start_time[0:4]), int(start_time[4:6]), int(start_time[6:8]), int(start_time[8:10]), int(start_time[10:12]), int(start_time[12:14]), 0)
                start_time_utc = dt.timestamp()
                end_time_utc = start_time_utc + duration
                nodes_real_uplink_total = gateway_log_count(start_time_utc, end_time_utc, gateway_log, experiment_nodes, UID_list)
                pdr_list = [(x / uplink_total)* 100 for x in nodes_real_uplink_total]
                print(pdr_list)
                if (len(pdr_list) > 1):
                    print(statistics.mean(pdr_list), statistics.stdev(pdr_list))
                else:
                    print(statistics.mean(pdr_list), 0)


User_name = "TP"

experiment_settings_dir = "D:\\TP\\Study\\wesg\\Chirpbox\\Miscellaneous\\Example\\LoRaWAN\\Python_examples\\ChirpBox_processing\\results\\experiment_settings"

log_dir = "D:\\TP\\Study\\wesg\\Chirpbox\\Miscellaneous\\Example\\LoRaWAN\\Python_examples\\ChirpBox_processing\\results\\experiment_logs"

gateway_log = "D:\\TP\\Study\\wesg\\Chirpbox\\Miscellaneous\\Example\\LoRaWAN\\Python_examples\\ChirpBox_processing\\results\\gateway\\lora_2021_06_04_11_34_27.csv"

uplink_pdr(User_name, experiment_settings_dir, log_dir, gateway_log)