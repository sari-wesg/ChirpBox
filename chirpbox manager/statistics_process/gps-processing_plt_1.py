import binascii
import os
os.getcwd()
import numpy as np
import matplotlib.dates as dates
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
import random
import pandas as pd
pd.plotting.register_matplotlib_converters()
import itertools
import re
from collections import Counter
from matplotlib.ticker import MultipleLocator
from matplotlib.lines import Line2D
from colors_lib import cnames
import datetime
import matplotlib.image as image
from matplotlib.offsetbox import (TextArea, DrawingArea, OffsetImage,
                                  AnnotationBbox)
file_name = 'gps_sf12_8nodes(4_5_9_11_12_13_16_19)_utc'

df = pd.read_csv(file_name+'.csv')
txt_file_name =  file_name[0:-4] + '.txt'

uinx_time = df.iloc[:,[4]].values.tolist()

start_time = min(uinx_time)[0]
end_time = max(uinx_time)[0]  # extend end_time 100s so that gateway can receive packet
print(start_time)
print(end_time)

MAX_NODE_NUMBER = 21

###############  csv file ###############
x = [[] for i in range(MAX_NODE_NUMBER)]
y = [[] for i in range(MAX_NODE_NUMBER)]
node_id_count = [0] * MAX_NODE_NUMBER
for i,date in enumerate(df.iterrows()):
    try:
        if df.iloc[[i],[1]].values <= MAX_NODE_NUMBER - 1:
            node_id = df.iloc[[i],[1]].values.tolist()[0][0]
            node_channel = df.iloc[[i],[3]].values.tolist()[0][0]
            node_uinx_time = df.iloc[[i],[4]].values[0][0]
            x[node_id].append( node_uinx_time )
            y[node_id].append( node_channel )
            node_id_count[node_id] = node_id_count[node_id] + 1
    except ValueError as e:
        print(df.iloc[i])

for i in range(MAX_NODE_NUMBER):
    for j in range(0,len(x[i])):
        x[i][j] = x[i][j] - start_time

#print(x)
#print(y)

result = Counter([i for item in y for i in item]) # convert 2-D to 1-D list
#print(result)

result = list(result)
result.sort()

y_channel_num = y

for i in range(MAX_NODE_NUMBER):
    for j in range(0,len(y[i])):
        y_channel_num[i][j] =  result.index(y[i][j])

##############  end of csv ####################


##############  txt file ######################
gateway_unix_time = []
gateway_channel = []
gateway_node_id = []

sourcesessionis = open(txt_file_name).read()
temp = sourcesessionis

reg = r'indexed_at_ms":(.{13})'
wordreg = re.compile(reg)
wordreglist = re.findall(wordreg,temp)
for word in wordreglist:
    gateway_unix_time.append(int(word))
    #print(word)
reg = r'deviceName\":\"(.{8})'
wordreg = re.compile(reg)
wordreglist = re.findall(wordreg,temp)
for word in wordreglist:
    gateway_node_id.append(word)
    #print(word)
reg = r'frequency\":(.{9})'
wordreg = re.compile(reg)
wordreglist = re.findall(wordreg,temp)
for word in wordreglist:
    gateway_channel.append(int(word))
    #print(word)

print(gateway_unix_time)
print(gateway_node_id)
print(gateway_channel)

node_lib = ['0x550033', '0x420029', '0x38001E', '0x1E0030', '0x26003E', '0x350017', '0x4A002D',
           '0x420020', '0x530045', '0X1D002B', '0x4B0027', '0x440038', '0x520049', '0x4B0023',
           '0X20003D', '0x360017', '0X30003C', '0x210027', '0X1C0040', '0x250031', '0x39005F']
gateway_unix_time_New = []
gateway_node_id_New = []
gateway_channel_New = []


# filter gateway packet accroding duration time from start_time to  end_time
for i in range(0,len(gateway_unix_time)):
    if (int(gateway_unix_time[i]/1000) - start_time >= 0) and (gateway_unix_time[i]/1000 <= end_time):
        gateway_unix_time_New.append(int(gateway_unix_time[i]/1000) - start_time - 3)
        gateway_node_id_New.append(node_lib.index(gateway_node_id[i]))
        gateway_channel_New.append(result.index(gateway_channel[i]/1000))

##############  end of txt #########################

###############################################
def get_index1(lst=None, item=''):
     return [index for (index,value) in enumerate(lst) if value == item]
##################  polt   #############################
    ### time vs channel ###

# plt.figure()
fig, ax = plt.subplots(figsize=(16, 9))

color = [''] * MAX_NODE_NUMBER
for i in range(MAX_NODE_NUMBER):
    gateway_node_id_index = []
    gateway_node_id = []
    gateway_channel = []
    gateway_unix_time = []
    gateway_node_id_index = get_index1(gateway_node_id_New,i)
    for j in range(len(gateway_node_id_index)):
        gateway_unix_time.append(gateway_unix_time_New[gateway_node_id_index[j]])
        gateway_channel.append(gateway_channel_New[gateway_node_id_index[j]])
    # while True:
    #     color_temp = cnames[random.randint(0,len(cnames)-1)] # cnames[i]
    #     R = int(color_temp[1:3],16)
    #     G = int(color_temp[3:5],16)
    #     B = int(color_temp[5:7],16)
    #     if 0.299 * R + 0.587 * G + 0.114 * B <= 192 and (color_temp not in color):
    #         color[i] = color_temp
    #         break
    color_temp = cnames[i]
    color[i] = color_temp

    plt_handler = plt.plot( x[i] , y[i] , linestyle='none',marker='s', markersize = 10, alpha=0.8, label=str(i),color = color[i], markeredgewidth=0, markeredgecolor=color[i], markerfacecolor=color[i])

    plt.plot( gateway_unix_time , gateway_channel , linestyle='none',marker="|", markersize = 30, alpha=1, color=color[i], markeredgewidth=2, markeredgecolor=color[i], markerfacecolor=color[i])

# plt.legend()

ax = plt.gca()
ax.set_aspect('auto')
ax.spines['bottom'].set_linewidth(1.5)
ax.spines['top'].set_linewidth(1.5)
ax.spines['left'].set_linewidth(1.5)
ax.spines['right'].set_linewidth(1.5)
ax.tick_params(direction='out', length=10, width=2)
# ax.set_ylim(5,6)

colors = color
lines = [Line2D([0], [0], color=c, linestyle='none',marker='s', markersize = 10, alpha=0.8, label=str(i), markeredgewidth=0, markeredgecolor=c, markerfacecolor=c) for c in colors]
labels = list(range(0, 21))
# plt.legend(lines, labels)

legend = plt.legend(lines, labels, loc='center right', bbox_to_anchor=(1.09, 0.5), edgecolor='k',fontsize = 15, fancybox=True, handlelength=1, handleheight=1)
# legend = plt.legend(loc='center right', bbox_to_anchor=(1.11, 0.5), edgecolor='k',fontsize = 16, fancybox=True)
legend.set_title("Node ID",prop={'size':15})
legend.get_frame().set_linewidth(2)
legend.get_frame().set_edgecolor("k")
ax.set_xlim(0, 3600)
x_list = [0, 600, 1200, 1800, 2400, 3000, 3600]
ax.set_xticks(x_list)

config_label_name_file = [0, 600, 1200, 1800, 2400, 3000, 3600]
ax.set_xticklabels(config_label_name_file, rotation=0, fontsize=34)
plt.xlabel('Time (s)',fontsize=50)             #设置x，y轴的标签
plt.ylabel('Channel ID',fontsize=50)
ax.tick_params(axis="both", labelsize=40, length=10, width=2)

# plt.yticks([0, 1, 2, 3, 4, 5, 6, 7],
#           [r'$Channel\ 0$', r'$Channel\ 1$', r'$Channel\ 2$', r'$Channel\ 3$', r'$Channel\ 4$', r'$Channel\ 5$',r'$Channel\ 6$',r'$Channel\ 7$'])

plt.yticks([0, 1, 2, 3, 4, 5, 6, 7],
          [r'1', r'2', r'3', r'4', r'5',r'6',r'7',r'8'])

# plt.legend( bbox_to_anchor=(1,0.5), loc='center left')

# ax.grid(True)
# minorLocator = MultipleLocator(1)
# ax.xaxis.set_minor_locator(minorLocator)

# plt.xticks(np.arange(min(gateway_unix_time_New), max(gateway_unix_time_New)+1, 1.0))
# plt.gca().xaxis.grid(color='gray', linestyle='dashed')
# plt.gca().yaxis.grid(color='gray', linestyle='dashed')
# plt.show()
figure_name = "lorawan" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.pdf'
plt.tight_layout()

plt.savefig(figure_name, dpi = 3600)

    ##### PDR figure ######

def to_percent(temp, position):
    return '%1.0f'%(100*temp) + '%'

plt.figure()
PDR = []
gateway_value_counts_result = pd.value_counts(gateway_node_id_New).sort_index().tolist()

for i in range(MAX_NODE_NUMBER):
    PDR.append(gateway_value_counts_result[i]/node_id_count[i])
plt.bar( range(MAX_NODE_NUMBER) , PDR, width=0.5)

plt.gca().yaxis.set_major_formatter(FuncFormatter(to_percent))

plt.xticks(range(MAX_NODE_NUMBER))
plt.xlabel('Node ID')
plt.ylabel('Packet Delivery Ratio (PDR)')
print(PDR)
plt.show()



'''
def plot_with_interval(min_interval_max, time_list, ch_list):
    seconds_interval = np.arange(0, min_interval_max*60+1, 10).tolist()

    # x = [datetime.datetime.strptime('15 Aug 2020', '%d %b %Y') + datetime.timedelta(seconds=i) for i in seconds_interval]
    # y = [i+random.gauss(0,1) for i,_ in enumerate(x)]
    # print(x)
    plt.figure()
    for i in range(21):
        x = time_list[i]
        y = ch_list[i]
        print(x)
        print(y)
        # xformatter = dates.DateFormatter('%M:%S')
        # plot
        # ax.xaxis.set_major_formatter(xformatter)
        # ax.set_xlim([pd.to_datetime('2020-08-15 00:00:00'), pd.to_datetime('2020-08-15 00:10:00')])

        plt.plot(x,y, linestyle='none',marker='o', markersize = 1)
    plt.show()



dates = []
time_list = []
for i, date in enumerate(df.iterrows()):
    try:
        time = datetime.datetime(year =2020, month = 8, day = 15, hour=(df.iloc[[i],[4]].values), minute=df.iloc[[i],[5]].values, second=df.iloc[[i],[6]].values)
        dates.append(time)
        print(i, df.iloc[[i],[6]].values)
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
'''