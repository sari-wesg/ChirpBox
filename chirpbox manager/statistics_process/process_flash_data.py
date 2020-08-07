import enum
import math
import matplotlib.pyplot as plt
import numpy as np
import pandas
import datetime
from cal_task_time import dissem_total_time
import seaborn as sns

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
    figure_name = "coldata_save//" + string_name + "_" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.png'
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

    # add vertical line
    # yposition = 2.77
    # plt.axvline(y=yposition, color='k', linestyle='-.',linewidth=0.5)

    plt.axhline(y=2.77, color='r', linestyle='--')
    plt.text(7.5, 2.6, r'Effective Duty Cycle',fontsize=18,fontname="Arial")

    ax = plt.gca()
    ax.set_aspect('auto')
    ax.spines['bottom'].set_linewidth(1.5)
    ax.spines['top'].set_linewidth(1.5)
    ax.spines['left'].set_linewidth(1.5)
    ax.spines['right'].set_linewidth(1.5)
    ax.tick_params(direction='out', length=10, width=2)

    figure_name = "coldata_save//" + string_name + "_" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.png'
    plt.tight_layout()
    plt.savefig(figure_name, dpi = 300)
    plt.show()


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
    channel_data_1_array = np.array(channel_data_1)
    channel_data_1_array = channel_data_1_array / (time_in_task * 1e6) * 1e2
    channel_data_1 = channel_data_1_array.tolist()
    # print(channel_data_1)
    plot_with_node_num_list_len_duty_cycle(channel_data_1, node_num, stats_lbt, filename + "channel_1")

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


