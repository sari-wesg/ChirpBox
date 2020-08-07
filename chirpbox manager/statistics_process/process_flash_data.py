import enum
import math
import matplotlib.pyplot as plt
import numpy as np
import pandas
import datetime
from cal_task_time import dissem_total_time
import seaborn as sns
import pandas as pd

class STATE(enum.Enum):
    WAITING_FOR_R = 1
    WAITING_FOR_F = 2

stats_len_write = 16
stats_len = 15
stats_all_len = stats_len_write * 2
stats_lbt = 10
stats_total_len = stats_all_len + stats_lbt * 2

def coldata_to_list(filename, node_num, value_row):
    Matrix_data = [[0 for x in range(stats_total_len)] for y in range(node_num)]
    previous_line = ''
    row_seq = 0
    with open(filename, 'r') as f:
        for line in f:
            if (line.startswith('f') and (previous_line.startswith('r '))):
                node_id_hex = previous_line[2:4].strip() + ' '
                node_id = int(node_id_hex, 16)
                if (node_id == 0):
                    row_seq = row_seq + 1
                tmp = line.split()
                for cnt in range(int(value_row / 4)):
                    value = int(tmp[cnt * 4 + 1], base = 16) + int(tmp[cnt * 4 + 2], base = 16) * 256  + int(tmp[cnt * 4 + 3], base = 16) * 65536  + int(tmp[cnt * 4 + 4], base = 16) * 16777216
                    if ((row_seq - 1) * int(value_row / 4) + cnt < stats_total_len):
                        Matrix_data[node_id][(row_seq - 1) * int(value_row / 4) + cnt] = value
            previous_line = line
        return Matrix_data

def data_to_stats_list(Matrix_data, node_num, stats_pos, list_len, stats_list):
    for i in range(int(list_len)):
        for node_id in range(int(node_num)):
            stats_list[i][node_id] = Matrix_data[node_id][stats_pos + i]
            if ((i == 0) and (Matrix_data[node_id][stats_pos + i + 1] != 0)):
                stats_list[i][node_id] = stats_list[i][node_id] / Matrix_data[node_id][stats_pos + i + 1]

def plot_with_node_num(stats_list, node_num, string_name):
    max_value=max(stats_list)#max value
    # plt.xlim(0,node_num)  #  设置x轴刻度范围
    plt.ylim(0,max_value + 1)

    plt.xlabel('node_id',fontsize=28)
    plt.ylabel('value',fontsize=28)
    labels = (np.linspace(0,node_num,node_num + 1,endpoint=True))
    plt.xticks(np.linspace(0,node_num,node_num + 1,endpoint=True),labels, rotation=0, fontsize=28,fontname="Arial")
    plt.yticks(fontsize=28)
    fig=plt.plot(stats_list,linewidth=3,color='k', linestyle='-',marker='o',
                markersize=8,markerfacecolor='none')
    ax = plt.gca()
    ax.set_aspect('auto')
    ax.spines['bottom'].set_linewidth(1.5)
    ax.spines['top'].set_linewidth(1.5)
    ax.spines['left'].set_linewidth(1.5)
    ax.spines['right'].set_linewidth(1.5)
    plt.subplots_adjust(left=0.22, right=0.96, top=0.95, bottom=0.15)
    figure_name = "coldata_save//" + string_name + "_" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.pdf'
    plt.tight_layout()
    plt.savefig(figure_name)
    plt.show()


def plot_with_node_num_list_len_duty_cycle(stats_list, node_num, list_len, string_name):

    binned_data = np.array(stats_list).T

    x_positions = np.linspace(0, list_len - 1, 10)
    number_of_groups = binned_data.shape[0]
    fill_factor =  .6  # ratio of the groups width
                    # relatively to the available space between ticks
    bar_width = np.diff(x_positions).min()/number_of_groups * fill_factor

    # colors = ['red','yellow', 'blue']
    # labels = ['red flowers', 'yellow flowers', 'blue flowers']
    plt.figure(figsize=(16,9))
    for i, groupdata in enumerate(binned_data):
        bar_positions = x_positions - number_of_groups*bar_width/2 + (i + 0.2)*bar_width
        fig = plt.bar(bar_positions, groupdata, bar_width,
                align='center',
                linewidth=1, edgecolor='k', alpha=0.8)
        # plt.bar(bar_positions, groupdata, bar_width,
        #         align='center',
        #         linewidth=1, edgecolor='k',
        #         color=colors[i], alpha=0.7,
        #         label=labels[i])
    plt.xticks(x_positions, fontsize=28)
    plt.yticks(fontsize=28)

    # plt.legend()
    plt.xlabel('channel length',fontsize=28)
    plt.ylabel('time in duty cycle',fontsize=28)

    plt.ylim(0,3)

    plt.axhline(y=2.77, color='r', linestyle='--')
    plt.text(7.5, 2.6, r'Effective Duty Cycle',fontsize=18,fontname="Arial")

    ax = plt.gca()
    ax.set_aspect('auto')
    ax.spines['bottom'].set_linewidth(1.5)
    ax.spines['top'].set_linewidth(1.5)
    ax.spines['left'].set_linewidth(1.5)
    ax.spines['right'].set_linewidth(1.5)
    ax.tick_params(direction='out', length=10, width=2)

    figure_name = "coldata_save//" + string_name + "_" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.pdf'
    plt.tight_layout()
    plt.savefig(figure_name, dpi = 300)
    plt.show()


def lbt_figure_sns(node_num, stats_lbt, channel_data, string_name, total):
    array_lbt = np.array(channel_data).T
    lbt_list = [[0]*3]*node_num * stats_lbt
    lbt_count = 0
    for node_lbt in range(0, node_num):
        for channel_lbt in range(0, stats_lbt):
            lbt_list[lbt_count] = [node_lbt, channel_lbt, array_lbt[node_lbt][channel_lbt]]
            lbt_count = lbt_count + 1
    df = pd.DataFrame(lbt_list,columns=['node_id','channel_id','duty_cycle'])

    # figure config
    plt.figure(figsize=(16,9))

    # seaborn
    if (total == 1):
        # ax = sns.barplot(y='duty_cycle',x='channel_id',data=df,hue='node_id', palette="muted")
        ax = sns.barplot(y='duty_cycle',x='channel_id',data=df,hue='node_id', palette=sns.color_palette('PuBuGn_d', n_colors=node_num, desat=1))
        # config ticks
        plt.xticks(fontsize=28)
        plt.yticks(fontsize=28)

        plt.xlabel('Channels',fontsize=28)
        plt.ylabel('Tx Total Time (s)',fontsize=28)

        plt.ylim(0,100)

        # plt.axhline(y=100, color='k', linestyle='--')
        # plt.text(7, 100, r'TX period limit',fontsize=18,fontname="Arial")

    else:
        # ax = sns.barplot(y='duty_cycle',x='channel_id',data=df,hue='node_id', palette=sns.color_palette("BrBG", 22))
        ax = sns.barplot(y='duty_cycle',x='channel_id',data=df,hue='node_id', palette=sns.color_palette('PuBuGn_d', n_colors=node_num, desat=1))

        # config ticks
        plt.xticks(fontsize=28)
        # plt.yticks(fontsize=28)
        y_value=['{:,.2f}'.format(x) + '%' for x in ax.get_yticks()]
        ax.set_yticklabels(y_value, fontsize=28)

        plt.xlabel('Channels',fontsize=28)
        plt.ylabel('TX duty cycle',fontsize=28)

        plt.ylim(0,3)

        plt.axhline(y=2.77, color='k', linestyle='--')
        plt.text(6.5, 2.6, r'Effective duty cycle limit',fontsize=24,fontname="Arial")

    ax = plt.gca()
    ax.set_aspect('auto')
    ax.spines['bottom'].set_linewidth(1.5)
    ax.spines['top'].set_linewidth(1.5)
    ax.spines['left'].set_linewidth(1.5)
    ax.spines['right'].set_linewidth(1.5)
    ax.tick_params(direction='out', length=10, width=2)

    # plt config
    figure_name = "coldata_save//" + string_name + "_" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.pdf'
    # plt.legend(loc='best',edgecolor='k',fontsize=10)
    legend = plt.legend(loc='center right', bbox_to_anchor=(1.1, 0.5),title="Node ID", edgecolor='k',fontsize = 16, fancybox=True)
    legend.get_frame().set_linewidth(2)
    legend.get_frame().set_edgecolor("k")

    plt.tight_layout()
    plt.savefig(figure_name, dpi = 300)
    # plt.show()

def rx_tx_one_dissem(node_num, rx_time, tx_time, string_name):
    radio_list = [[0]*3]*node_num
    for node_radio in range(0, node_num):
        radio_list[node_radio] = [node_radio, rx_time[node_radio], tx_time[node_radio]]
    df = pd.DataFrame(radio_list,columns=['node_id','RX time','TX time'])


    sns.set(context=None, style=None, palette=sns.color_palette('PuBuGn_d', n_colors=2, desat=1), font_scale=1, color_codes=False, rc=None)
    ax = df.set_index('node_id').plot(kind='bar', stacked=True, figsize=(16,9))

    # config ticks
    plt.xticks(fontsize=28)
    plt.yticks(fontsize=28)

    # change font size of the scientific notation in matplotlib
    ax.yaxis.offsetText.set_fontsize(24)

    plt.xlabel('Channels',fontsize=28)
    plt.ylabel('Tx Total Time (s)',fontsize=28)

    ax = plt.gca()
    ax.set_aspect('auto')
    ax.spines['bottom'].set_linewidth(1.5)
    ax.spines['top'].set_linewidth(1.5)
    ax.spines['left'].set_linewidth(1.5)
    ax.spines['right'].set_linewidth(1.5)
    ax.tick_params(direction='out', length=10, width=2)

    # plt config
    figure_name = "coldata_save//" + string_name + "_" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.pdf'
    # plt.legend(loc='best',edgecolor='k',fontsize=10)
    legend = plt.legend(loc='upper right', title="Node ID", edgecolor='k',fontsize = 16, fancybox=True)
    legend.get_frame().set_linewidth(1.5)
    legend.get_frame().set_edgecolor("k")

    plt.tight_layout()
    plt.savefig(figure_name, dpi = 300)
    # plt.show()

def matrix_to_type_data(Matrix_data, node_num, filename, time_in_round, time_in_task):
    global stats_lbt
    slot_data_1 = [[0 for x in range(node_num)] for y in range(5)]
    rx_data_1 = [[0 for x in range(node_num)] for y in range(4)]
    tx_data_1 = [[0 for x in range(node_num)] for y in range(4)]
    slot_data_2 = [[0 for x in range(node_num)] for y in range(5)]
    rx_data_2 = [[0 for x in range(node_num)] for y in range(4)]
    tx_data_2 = [[0 for x in range(node_num)] for y in range(4)]
    channel_data_1 = [[0 for x in range(node_num)] for y in range(int(stats_lbt))]
    channel_data_2 = [[0 for x in range(node_num)] for y in range(int(stats_lbt))]

    slot_1_pos = 0
    rx_1_pos = 5
    tx_1_pos = 10
    slot_2_pos = 16
    rx_2_pos = 21
    tx_2_pos = 26

    channel_data_1_pos = 32
    channel_data_2_pos = 32 + stats_lbt

    data_to_stats_list(Matrix_data, node_num, slot_1_pos, 5, slot_data_1)
    data_to_stats_list(Matrix_data, node_num, rx_1_pos, 4, rx_data_1)
    data_to_stats_list(Matrix_data, node_num, tx_1_pos, 4, tx_data_1)
    data_to_stats_list(Matrix_data, node_num, slot_2_pos, 5, slot_data_2)
    data_to_stats_list(Matrix_data, node_num, rx_2_pos, 4, rx_data_2)
    data_to_stats_list(Matrix_data, node_num, tx_2_pos, 4, tx_data_2)

    for i in range(int(stats_lbt)):
        for node_id in range(int(node_num)):
            channel_data_1[i][node_id] = Matrix_data[node_id][channel_data_1_pos + i]
    for i in range(int(stats_lbt)):
        for node_id in range(int(node_num)):
            channel_data_2[i][node_id] = Matrix_data[node_id][channel_data_2_pos + i]

    tx_time = 0
    for i in range(int(stats_lbt)):
        tx_time = tx_time + channel_data_1[i][0]


    # plot_with_node_num(slot_data_1[0], node_num, filename + "slot1_1")
    # plot_with_node_num(slot_data_1[4], node_num, filename + "slot_fail_1")
    # plot_with_node_num(rx_data_1[0], node_num, filename + "rx_1")
    # plot_with_node_num(tx_data_1[0], node_num, filename + "tx_1")
    # 1, plt radio on time
    rx_tx_one_dissem(node_num, rx_data_1[0], tx_data_1[0], filename + "radio_on")

    # 2, plt lbt total time
    channel_data_1_array = np.array(channel_data_1)
    channel_data_1_array = channel_data_1_array / (1e6)
    channel_data_1_temp = channel_data_1_array.tolist()
    lbt_figure_sns(node_num, stats_lbt, channel_data_1_temp, filename + "channel_total", 1)
    # 3, plt lbt duty cycle
    channel_data_1_array = np.array(channel_data_1)
    channel_data_1_array = channel_data_1_array / (time_in_task * 1e6) * 1e2
    channel_data_1_temp = channel_data_1_array.tolist()
    lbt_figure_sns(node_num, stats_lbt, channel_data_1_temp, filename + "channel_duty_cycle", 0)
    # plot_with_node_num_list_len_duty_cycle(channel_data_1, node_num, stats_lbt, filename + "channel_1")

    # plot_with_node_num(slot_data_2[0], node_num, filename + "slot1_2")
    # plot_with_node_num(slot_data_2[4], node_num, filename + "slot_fail_2")
    # plot_with_node_num(rx_data_2[0], node_num, filename + "rx_2")
    # plot_with_node_num(tx_data_2[0], node_num, filename + "tx_2")
    # plot_with_node_num_list_len_duty_cycle(channel_data_2, node_num, stats_lbt, filename + "channel_2")

    # print(slot_data_1)
    # print(rx_data_1)
    # print(tx_data_1)
    # print(channel_data_1)
    # print(slot_data_2)
    # print(rx_data_2)
    # print(tx_data_2)
    # print(channel_data_2)

def print_in_hex(matrix_data):
    for i in range(len(matrix_data)):
        print(i, '\n')
        for k in range(len(matrix_data[0])):
            print(hex((matrix_data[i][k])), '', end = '')
        print('\n')


# const:
node_num = 22
f_data_len = 120
filename = "disseminate_command_len_200_used_sf7_generate_size4_slot_num40_bitmap3FFFFF_FileSize2048_dissem_back_sf7_dissem_back_slot120(20200806220358213616).txt"
Matrix_data = coldata_to_list(filename, int(node_num), int(f_data_len - 8))

# print_in_hex(Matrix_data)

task_payload_len = 200
task_slot_num = 40
task_sf = 7
task_generate_size = 4
task_back_sf = 7
task_back_slot = 120
task_try_num = 7

task_time = dissem_total_time(task_sf, task_payload_len, task_slot_num, task_generate_size, task_back_sf, task_back_slot, task_try_num, node_num)
time_in_round = task_time[0]
time_in_task = task_time[1]
# print(time_in_round, time_in_task)
matrix_to_type_data(Matrix_data, node_num, filename, time_in_round, time_in_task)


# People_List = [['Jon','Smith',21],['Mark','Brown',38],['Maria','Lee',42],['Jill','Jones',28],['Jack','Ford',55]]

# df = pd.DataFrame(People_List,columns=['First_Name','Last_Name','Age'])
# print(df)

