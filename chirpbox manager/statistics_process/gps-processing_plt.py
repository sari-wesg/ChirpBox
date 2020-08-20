import binascii
import os
os.getcwd()
import numpy as np
import matplotlib.dates as dates
import matplotlib.pyplot as plt
import datetime
import random
import pandas as pd
pd.plotting.register_matplotlib_converters()
import itertools

def plot_with_interval(min_interval_max, time_list, ch_list):
    seconds_interval = np.arange(0, min_interval_max*60+1, 10).tolist()

    # x = [datetime.datetime.strptime('15 Aug 2020', '%d %b %Y') + datetime.timedelta(seconds=i) for i in seconds_interval]
    # y = [i+random.gauss(0,1) for i,_ in enumerate(x)]
    # print(x)
    plt.figure()
    for i in range(21):
        x = time_list[i]
        y = ch_list[i]
        # print(x)
        # print(y)
        # xformatter = dates.DateFormatter('%M:%S')
        # plot
        # ax.xaxis.set_major_formatter(xformatter)
        # ax.set_xlim([pd.to_datetime('2020-08-15 00:00:00'), pd.to_datetime('2020-08-15 00:10:00')])

        plt.plot(x,y, linestyle='none',marker='o', markersize = 1)
    plt.show()

df = pd.read_csv('gps_sf12_8nodes(4_5_9_11_12_13_16_19).csv')

dates = []
time_list = []
for i, date in enumerate(df.iterrows()):
    try:
        time = datetime.datetime(year =2020, month = 8, day = 15, hour=(df.iloc[[i],[4]].values), minute=df.iloc[[i],[5]].values, second=df.iloc[[i],[6]].values)
        dates.append(time)
        # print(i, df.iloc[[i],[6]].values)
    except ValueError as e:
        dates[i] = datetime.datetime(2020,8,15,1)
print(max(dates), min(dates))

dates = []
time_list = []
channel_list = []
node_id = 0
node_gps_data_time_list = [[]*20]*21
node_gps_data_channel_list = [[]*20]*21
max_tx_num = []
for i, date in enumerate(df.iterrows()):
    try:
        time = datetime.datetime(year =2020, month = 8, day = 15, hour=(df.iloc[[i],[4]].values), minute=df.iloc[[i],[5]].values, second=df.iloc[[i],[6]].values)
        time_list.append(time)
        channel_list.append(int(df.iloc[[i],[3]].values))
        if (node_id != df.iloc[[i],[1]].values):
            node_gps_data_time_list[node_id] = time_list
            node_gps_data_channel_list[node_id] = channel_list
            max_tx_num.append(int(df.iloc[[i-1],[2]].values))
            node_id = node_id+1
            time_list = []
            channel_list = []
        # the last node
        elif (i == len(df[["GPS_TIME_HOUR"]]) - 1):
            node_gps_data_time_list[node_id] = time_list
            node_gps_data_channel_list[node_id] = channel_list
            max_tx_num.append(int(df.iloc[[i],[2]].values))
    except ValueError as e:
        dates[i] = datetime.datetime(2020,8,15,1)

# for i in range(21):
#     print(i, node_gps_data_time_list[i])
print(max_tx_num)
plot_with_interval(10, node_gps_data_time_list, node_gps_data_channel_list)