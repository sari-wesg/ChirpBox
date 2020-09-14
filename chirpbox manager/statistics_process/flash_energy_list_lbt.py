import statistics

stats_total_len = int(960/4)
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


def Matrix_data_list_to_task_energy(Matrix_data,k,task_id):
    # task_list = Matrix_data[i][]
    idle_list = []
    arrange_list = []
    task_list = []
    for i in range(10):
        idle_list.append(Matrix_data[k][task_id*48+i])
        arrange_list.append(Matrix_data[k][task_id*48+16+i])
        task_list.append(Matrix_data[k][task_id*48+32+i])
    print (idle_list, arrange_list, task_list)

    return (idle_list, arrange_list, task_list)

def task_list_to_energy(idle_list, arrange_list, task_list, task_id):
    energy_total = 0
    energy_list_14 = [80.9633, 53.5069, 18.837, 47.7732, 72.84508, 62.39981, 80.9633, 287.6562, 181.71642, 0]
    energy_list_0 = [80.9633, 53.5069, 18.837, 47.7732, 72.84508, 62.39981, 80.9633, 207.37644, 181.71642, 0]
    for i in range(len(idle_list)):
        energy_total = energy_total + idle_list[i] * energy_list_14[i]
    for i in range(len(arrange_list)):
        energy_total = energy_total + arrange_list[i] * energy_list_14[i]

    if ((task_id == 0) or (task_id == 4)):
        for i in range(len(task_list)):
            energy_total = energy_total + task_list[i] * energy_list_14[i]
    else:
        for i in range(len(task_list)):
            energy_total = energy_total + task_list[i] * energy_list_0[i]
    for i in range(len(task_list)):
        task_list[i] = task_list[i] + arrange_list[i] + idle_list[i]
    total_time = 0
    for i in range(len(task_list)):
        total_time = total_time + task_list[i]
    return(energy_total/1e6, total_time/1e6)

def get_lbt_list(filename, dis_value, col_value):
    Matrix_data = coldata_to_list(filename, 21, (232 - 8))
    channel_list_dis_all = [[0 for x in range(8)] for y in range(21)]
    channel_list_col_all = [[0 for x in range(8)] for y in range(21)]

    for i in range(0,21):
        c_list_d, c_list_c = Matrix_data_list_to_channel(Matrix_data,i, dis_value, col_value )
        channel_list_dis_all[i] = c_list_d
        channel_list_col_all[i] = c_list_c
        # if (((i != 5))):

    # energy_node_total_mean = statistics.mean(energy_node_total)
    # energy_node_total_stdev = statistics.stdev(energy_node_total)
    # total_time_node_all_mean = statistics.mean(total_time_node_all)
    # print(energy_node_total)
    # print(total_time_node_all_mean,total_time_node_all)
    # print(str((int(total_time_node_all_mean)))+','+str(energy_node_total[1])+','+str(energy_node_total[3])+','+str(energy_node_total_mean)+','+str(energy_node_total_stdev))
    print(channel_list_dis_all)
    print('----------------------')
    print(channel_list_col_all)

def Matrix_data_list_to_channel(Matrix_data,k, dis_total, col_total):
    # task_list = Matrix_data[i][]
    channel_list_dis = []
    channel_list_col = []
    for i in range(8):
        channel_list_dis.append(Matrix_data[k][32+i])
        channel_list_col.append(Matrix_data[k][32+8+i])
    # print (channel_list_dis, channel_list_col)
    channel_list_dis[:] = [x / (dis_total*1e4) for x in channel_list_dis]
    channel_list_col[:] = [x / (col_total*1e4) for x in channel_list_col]

    return (channel_list_dis, channel_list_col)

filename = "D:\TP\Study\wesg\Chirpbox\chirpbox manager\statistics_process\ewsn_data\size\dis_100_col_2_sf7_lbt_3.txt"

get_lbt_list(filename,1873.4215331,477)
