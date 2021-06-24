import json
import glob
import os
from datetime import datetime, timezone
import pandas as pd
import statistics
import sys
import logging
import matplotlib.pyplot as plt
import numpy as np

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

red_colors = ['#fff5f0', '#fee0d2', '#fcbba1', '#fc9272', '#fb6a4a', '#ef3b2c', '#cb181d', '#a50f15', '#67000d', '#4e000d']
green_colors = ['#ffffe5', '#f7fcb9', '#d9f0a3', '#addd8e', '#78c679', '#41ab5d', '#238443', '#006837', '#004529', '#003729']
blue_colors = ['#ebebff', '#d8d8ff', '#c4c4ff', '#b1b1ff', '#9d9dff', '#7676ff', '#4e4eff', '#3b3bff', '#2727ff', '#1414ff']
three_colors = ['#3b3bff', '#238443', '#cb181d']

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

def gateway_log_count(start_time_utc, end_time_utc, gateway_log, experiment_nodes, UID_list, uplink_SF):
    df = pd.read_csv(gateway_log, sep=',')
    df.columns = ["utc", "id", "rssi", "snr", "channel", "datarate", "data"]
    df = df[df.iloc[:,0].between(start_time_utc, end_time_utc)]
    uplink_count_list = []
    if (uplink_SF == "0xFFFFFFFF"):
        for node in experiment_nodes:
            LoRaWAN_device_ID = ('00000000' + (UID_list[node])[2:]).lower()
            df_node = df.loc[(df['id'] == str(LoRaWAN_device_ID))]
            for i in range(12-7+1):
                df_node_rate = df_node.loc[(df_node['datarate'] == i)]
                uplink_count_list.append(len(df_node_rate))
        uplink_count_list = [x.tolist() for x in np.array_split(uplink_count_list, len(experiment_nodes))]
    else:
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

    # find all experiments settings (config files)
    experiment_settings = glob.glob(os.path.join(experiment_settings_dir, user_name + "_*.json"))
    columns = ['experiment_name','PDR','pdr_std']
    experiment_df = pd.DataFrame(columns=columns)
    for experiment_setting in experiment_settings:
        # read config one by one
        with open(experiment_setting) as data_file:
            data = json.load(data_file)
            experiment_name = data['experiment_name']
            duration = data['experiment_duration']
            experiment_nodes = bitmap_to_nodes(data['experiment_run_bitmap'])
            round_robin = data['experiment_run_round'].lower() == 'true'
            uplink_total = int((experiment_name.split('packet')[0])[len("LoRaWAN_"):])
            uplink_SF = data['FUT_CUSTOM_list'].split(', ')[3]
            logger.debug("experiment_name:%s, duration:%s, experiment_nodes:%s, uplink_SF:%s\n", experiment_name, duration, experiment_nodes, uplink_SF)

            # find all logs belong to this experiment
            experiment_log = glob.glob(log_dir + "\\log_" + experiment_name + "*.csv")

            experiment_pdr = []
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

                nodes_real_uplink_total = gateway_log_count(start_time_utc, end_time_utc, gateway_log, experiment_nodes, UID_list, uplink_SF)
                if (uplink_SF == "0xFFFFFFFF"):
                    nodes_real_uplink_total_T = list(map(list, zip(*nodes_real_uplink_total)))
                    logger.debug(nodes_real_uplink_total)
                    plot_pdr_node(nodes_real_uplink_total, uplink_total)

                    for i in range(len(nodes_real_uplink_total_T)):
                        pdr_list = [(x / uplink_total)* 100 for x in nodes_real_uplink_total_T[i]]
                        pdr_mean= statistics.mean(pdr_list)
                        experiment_pdr.append(pdr_mean)
                else:
                    pdr_list = [(x / uplink_total)* 100 for x in nodes_real_uplink_total]
                    pdr_mean= statistics.mean(pdr_list)
                    experiment_pdr.append(pdr_mean)

            if (uplink_SF == "0xFFFFFFFF"):
                experiment_pdr = [x.tolist() for x in np.array_split(experiment_pdr, 12-7+1)]
                logger.debug(experiment_pdr)
                for i in range(len(experiment_pdr)):
                    pdr_mean= statistics.mean(experiment_pdr[i])
                    if (len(experiment_pdr[i]) > 1):
                        pdr_std = statistics.stdev(experiment_pdr[i])
                    else:
                        pdr_std = 0
                    experiment_df.loc[-1] = [int(12-i), pdr_mean, pdr_std]  # adding a row
                    experiment_df.index = experiment_df.index + 1  # shifting index
                    experiment_df = experiment_df.sort_index()  # sorting by index
            else:
                pdr_mean= statistics.mean(experiment_pdr)
                if (len(experiment_pdr) > 1):
                    pdr_std = statistics.stdev(experiment_pdr)
                else:
                    pdr_std = 0
                experiment_df.loc[-1] = [experiment_name, pdr_mean, pdr_std]  # adding a row
                experiment_df.index = experiment_df.index + 1  # shifting index
                experiment_df = experiment_df.sort_index()  # sorting by index
    experiment_df = experiment_df.sort_values(by=['PDR'], ascending=True)
    experiment_df = experiment_df.reset_index(drop=True)
    logger.debug(experiment_df)
    return experiment_df

def autolabel(ax, rects):
    """
    Attach a text label above each bar displaying its height
    """
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width()/2., 1.05*height,
                '%.2f' % float(height),
                ha='center', va='bottom')

def plot_pdr_node(nodes_real_uplink_total, uplink_total):
    data = np.array(nodes_real_uplink_total)
    data = data / uplink_total * 100
    x = np.arange(data.shape[0])
    dx = (np.arange(data.shape[1])-data.shape[1]/2.)/(data.shape[1]+2.)
    d = 1./(data.shape[1]+2.)

    fig, ax=plt.subplots()
    for i in range(data.shape[1]):
        ax.bar(x+dx[i],data[:,i], width=d, label="SF {}".format(12-i), color = red_colors[len(red_colors) - 2 - i])
    ax.set_xticklabels([*range(1,21)], rotation='horizontal')
    ax.set_xticks(np.arange(len(x)))

    ax.set_xlabel('Node ID')
    ax.set_ylabel('PDR (%)')
    plt.title('Each node’s PDR of LoRaWAN uplink packets', fontsize=12)
    # plt.legend(framealpha=1)
    legend = plt.legend(loc='upper right', bbox_to_anchor=(1.14, 1.02), edgecolor='k',fontsize = 10, fancybox=False)
    fig.tight_layout()

    plt.show()

def plot_pdr(experiment_df):
    fig, ax = plt.subplots()
    ax.set_ylim(0,100)
    ax.set_xlabel('Spreading factor')
    ax.set_ylabel('PDR (%)')
    # ax.set_xticklabels(experiment_df['experiment_name'].tolist(), rotation='horizontal')
    width = 0.3
    rects1 = ax.bar(experiment_df['experiment_name'].tolist(), experiment_df['PDR'], width=width, color='b', yerr=experiment_df['pdr_std'].tolist(), error_kw=dict(ecolor='k', lw=0.2, markersize='1', capsize=5, capthick=1, elinewidth=2))
    autolabel(ax, rects1)
    plt.title('Average PDR of LoRaWAN uplink packets', fontsize=12)
    fig.tight_layout()
    plt.show()

def main(argv):
    # argv[1] = User_name
    # argv[2] = experiment_settings_dir
    # argv[3] = log_dir
    # argv[4] = gateway_log_file
    experiment_df = uplink_pdr(argv[1], argv[2], argv[3], argv[4])
    plot_pdr(experiment_df)

if __name__ == "__main__":
    main(sys.argv)