import enum
import math
import matplotlib.pyplot as plt
plt.rcParams.update({'figure.max_open_warning': 0})
import numpy as np
import pandas
import datetime
from cal_task_time import *
import seaborn as sns
import pandas as pd
import statistics
import csv
class STATE(enum.Enum):
    WAITING_FOR_R = 1
    WAITING_FOR_F = 2

stats_len_write = 16
stats_len = 15
stats_all_len = stats_len_write * 2
stats_lbt = 9
stats_total_len = stats_all_len + (stats_lbt + 1) * 2

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

    plt.xlabel('node_id',fontsize=40)
    plt.ylabel('value',fontsize=40)
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
    # plt.show()


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
    plt.xticks(x_positions, fontsize=32)
    plt.yticks(fontsize=32)

    # plt.legend()
    plt.xlabel('channel length',fontsize=40)
    plt.ylabel('time in duty cycle',fontsize=40)

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
    # plt.show()


def lbt_figure_sns(node_num, stats_lbt, channel_data, string_name, total):
    array_lbt = np.array(channel_data).T
    lbt_list = [[0]*3]*node_num * stats_lbt
    lbt_count = 0
    for node_lbt in range(0, node_num):
        for channel_lbt in range(0, stats_lbt):
            lbt_list[lbt_count] = [node_lbt, channel_lbt + 1, array_lbt[node_lbt][channel_lbt]]
            lbt_count = lbt_count + 1
    df = pd.DataFrame(lbt_list,columns=['node_id','channel_id','duty_cycle'])

    # figure config
    plt.figure(figsize=(16,9))

    # seaborn
    if (total == 1):
        # ax = sns.barplot(y='duty_cycle',x='channel_id',data=df,hue='node_id', palette="muted")
        ax = sns.barplot(y='duty_cycle',x='channel_id',data=df,hue='node_id', palette=sns.color_palette('PuBuGn_d', n_colors=node_num, desat=1))
        # config ticks
        plt.xticks(fontsize=32)
        plt.yticks(fontsize=32)

        plt.xlabel('Channels',fontsize=40)
        plt.ylabel('Transmission time (s)',fontsize=40)

        plt.ylim(0,100)

        # plt.axhline(y=100, color='k', linestyle='--')
        # plt.text(7, 100, r'TX period limit',fontsize=18,fontname="Arial")

    else:
        # ax = sns.barplot(y='duty_cycle',x='channel_id',data=df,hue='node_id', palette=sns.color_palette("BrBG", 22))
        ax = sns.barplot(y='duty_cycle',x='channel_id',data=df,hue='node_id', palette=sns.color_palette('PuBuGn_d', n_colors=node_num, desat=1))

        # config ticks
        plt.ylim(0,3)

        plt.xticks(fontsize=32)
        # plt.yticks(fontsize=28)
        y_value=['{:,.2f}'.format(x) + '%' for x in ax.get_yticks()]
        ax.set_yticklabels(y_value, fontsize=32)

        plt.xlabel('Channels',fontsize=40)
        plt.ylabel('TX duty cycle',fontsize=40)

        # plt.ylim(0,3)

        plt.axhline(y=2.77, color='k', linestyle='--')
        plt.text(5.5, 2.6, r'Maximum duty cycle',fontsize=32,fontname="Arial")

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
    legend = plt.legend(loc='center right', bbox_to_anchor=(1.1, 0.5), edgecolor='k',fontsize = 16, fancybox=True)
    legend.set_title("Node ID",prop={'size':16})
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
    plt.xticks(fontsize=32)
    plt.yticks(fontsize=32)

    # change font size of the scientific notation in matplotlib
    ax.yaxis.offsetText.set_fontsize(32)

    plt.xlabel('Channels',fontsize=40)
    plt.ylabel('Time (s)',fontsize=40)

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
    legend = plt.legend(loc='upper right', edgecolor='k',fontsize = 16, fancybox=True)
    legend.set_title("Node ID",prop={'size':16})
    legend.get_frame().set_linewidth(1.5)
    legend.get_frame().set_edgecolor("k")

    plt.tight_layout()
    plt.savefig(figure_name, dpi = 300)
    # plt.show()

def matrix_to_type_data(Matrix_data, node_num, filename):
    print(Matrix_data)
    print(len(Matrix_data))
    list_test = [38, 1, 38, 38, 0, 4506309, 1, 4506309, 4506309, 0, 873683, 1, 873683, 873683, 0, 0, 582, 11, 40, 60, 0, 193637478, 11, 16353727, 18606317, 0, 41012384, 11, 2734283, 4687392, 0, 0, 873678, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6640926, 8592447, 4296606, 2734280, 3515198, 4686825, 3124324, 2734347]
    print(len(list_test))

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
    if ((stats_lbt % 2)==1):
        channel_data_2_pos = channel_data_2_pos + 1

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
    print(slot_data_2)
    print(rx_data_2)
    print(tx_data_2)
    print(channel_data_2)
    try_num = slot_data_2[1][0]
    return [rx_data_2[0], tx_data_2[0], channel_data_2, try_num]

def print_in_hex(matrix_data):
    for i in range(len(matrix_data)):
        print(i, '\n')
        for k in range(len(matrix_data[0])):
            print(hex((matrix_data[i][k])), '', end = '')
        print('\n')

def plot_dissem_lbt_radio(rx_time, tx_time, channel_data, task_try_num, time_in_task, dissem_config, node_num):
    dissem_config_name = ''.join(str(e) for e in dissem_config)
    # 1, plt radio on time
    # rx_tx_one_dissem(node_num, rx_time, tx_time, dissem_config_name + "radio_on")

    # 2, plt lbt total time
    channel_data_1_array = np.array(channel_data)
    channel_data_1_array = channel_data_1_array / (1e6)
    channel_data_1_temp = channel_data_1_array.tolist()
    lbt_figure_sns(node_num, stats_lbt, channel_data_1_temp, dissem_config_name + "channel_total", 1)
    # 3, plt lbt duty cycle
    channel_data_1_array = np.array(channel_data)
    channel_data_1_array = channel_data_1_array / (time_in_task * 1e6) * 1e2
    channel_data_1_temp = channel_data_1_array.tolist()
    # print("channel_data_1_temp", channel_data_1_temp)
    # for k in range(9):
    #     for i in range(21):
    #         if ((i == 0) or (i == 6) or (i == 7) or (i == 8) or (i == 9) or (i == 13) or (i == 14)):
    #             channel_data_1_temp[k][i] = channel_data_1_temp[k][i] * 1.15
    #         elif ((i == 11) or (i == 12) or (i == 16) or (i == 20)):
    #             channel_data_1_temp[k][i] = channel_data_1_temp[k][i] * 0.8
    lbt_figure_sns(node_num, stats_lbt, channel_data_1_temp, dissem_config_name + "channel_duty_cycle", 0)

def dissem_files(node_num, file_list, dissem_config_list):
    f_data_len = 120
    dissem_result = []
    dissem_round = []
    for i in range(0, len(file_list)):
        filename = file_list[i]
        Matrix_data = coldata_to_list(filename, int(node_num), int(f_data_len - 8))
        rx_time, tx_time, channel_data, task_try_num = matrix_to_type_data(Matrix_data, node_num, filename)
        dissem_config = dissem_config_list[i]
        task_time = dissem_total_time(dissem_config[0], dissem_config[1], dissem_config[2], dissem_config[3], dissem_config[4], dissem_config[5], task_try_num, node_num)

        time_in_task = task_time[1]
        # TODO:
        # file_size = 1024 * int(dissem_config_list[i][6])
        # effective_num = effective_round(dissem_config_list[i][1], dissem_config_list[i][3], file_size)
        # need_round_num = task_try_num - 1
        # dissem_round.append([dissem_config_list[i][2], effective_num / need_round_num])

        # plot figures
        # print("channel_data", channel_data)
        # change_list = [0.8, 0.66, 0.6, 0.83, 1]
        # if (1):
        #     change_list_id = i
        #     for k in range(9):
        #         for node_index in range(node_num):
        #             # print(node_index, k, change_list[change_list_id])
        #             channel_data[k][node_index] = channel_data[k][node_index] * change_list[change_list_id]
        #     task_try_num = task_try_num * change_list[change_list_id]
        #     time_in_task = time_in_task * change_list[change_list_id]
            # print("-------channel_data", channel_data, task_try_num)
        # plot_dissem_lbt_radio(rx_time, tx_time, channel_data, task_try_num, time_in_task, dissem_config, node_num)

        # radio_on_time = [0] * len(rx_time)
        # for i in range(0, len(rx_time)):
        #     radio_on_time[i] = (rx_time[i] + tx_time[i]) / 1e6
        # radio_std = statistics.stdev(radio_on_time)
        # radio_mean = statistics.mean(radio_on_time)
        rx_time[:] = [x / 1e6 for x in rx_time]
        tx_time[:] = [x / 1e6 for x in tx_time]
        radio_rx_mean = statistics.mean(rx_time)
        radio_rx_stdev = statistics.stdev(rx_time)
        radio_tx_mean = statistics.mean(tx_time)
        radio_tx_stdev = statistics.stdev(tx_time)
        # print (radio_rx_mean, radio_rx_stdev, radio_tx_mean, radio_tx_stdev, task_time[1], task_try_num)
        dissem_result.append([radio_rx_mean, radio_rx_stdev, radio_tx_mean, radio_tx_stdev, time_in_task, task_try_num])
    # TODO:
    # df = pd.DataFrame(dissem_round,columns=['slot_num', 'pdr'])
    # save_SF = 7
    # save_packet = 16
    # save_tp = 0
    # save_csv_name = "pdr_save//" + str(save_SF) + "_" + str(save_packet) + "_" + str(save_tp) + ".csv"
    # df.to_csv(save_csv_name, encoding='utf-8', index=False)
    return (dissem_result)


def coll_files(node_num, file_list, coll_config):
    f_data_len = 120
    dissem_result = []
    dissem_round = []
    for i in range(0, len(file_list)):
        filename = file_list[i]
        Matrix_data = coldata_to_list(filename, int(node_num), int(f_data_len - 8))
        rx_time, tx_time, channel_data, task_try_num = matrix_to_type_data(Matrix_data, node_num, filename)

        round_time = task_slot(CHIRP_TASK.MX_COLLECT, 232, 60, 7, node_num, node_num)
        time_in_task = task_try_num * round_time
        print("total_time", time_in_task, task_try_num)

        plot_dissem_lbt_radio(rx_time, tx_time, channel_data, task_try_num, time_in_task, coll_config, node_num)

        rx_time[:] = [x / 1e6 for x in rx_time]
        tx_time[:] = [x / 1e6 for x in tx_time]
        radio_rx_mean = statistics.mean(rx_time)
        radio_rx_stdev = statistics.stdev(rx_time)
        radio_tx_mean = statistics.mean(tx_time)
        radio_tx_stdev = statistics.stdev(tx_time)
        print (radio_rx_mean * task_try_num, radio_rx_stdev, radio_tx_mean * task_try_num, radio_tx_stdev, task_try_num, radio_rx_mean * task_try_num+ radio_tx_mean * task_try_num)
        # dissem_result.append([radio_rx_mean, radio_rx_stdev, radio_tx_mean, radio_tx_stdev, time_in_task, task_try_num])

    return (dissem_result)


# const:
# TODO:
node_num = 21
file_list = ["collect_data_command_len_136_used_sf7used_tp14command_len136_slot_num60startaddress_0807E000end_address0807E808(20200815234429736613).txt"]
coll_config = [[0]]
coll_files(node_num, file_list, coll_config)

# print(len(file_list), len(dissem_config_list))
# dissem_files(21, file_list, dissem_config_list)
