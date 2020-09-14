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
                    # print(node_id,cnt)
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

    return (idle_list, arrange_list, task_list, sum(task_list)/1e6)

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

def get_energy_list(task_id, filename):
    Matrix_data = coldata_to_list(filename, 21, (232 - 8))
    energy_node_total = []
    total_time_node_all = []
    task_time_node_all = []
    for i in range(1,21):
        idle_list, arrange_list, task_list, task_time = Matrix_data_list_to_task_energy(Matrix_data,i,task_id)
        energy_node, total_time_node = task_list_to_energy(idle_list, arrange_list, task_list, task_id)
        # if (((i != 5) and (i != 8) and (i != 9) and (i != 11) and (i != 99) and (i != 19) and (i != 3) and (i < 197))):
        # if (((i == 2) or (i == 6) or (i == 7) or (i == 9) or (i == 10) or (i == 12) or (i == 13) or (i == 14) or (i == 15)  or (i == 18) and (i < 197))):
        # if (((i == 2) or (i == 99) or (i == 6) or (i == 10) or (i == 15) and (i < 197))):
        if (((i != 5)) and ((i != 9))):
        # if (((i != 5))):
            energy_node_total.append(energy_node)
            total_time_node_all.append(total_time_node)
            task_time_node_all.append(task_time)

    energy_node_total_mean = statistics.mean(energy_node_total)
    energy_node_total_stdev = statistics.stdev(energy_node_total)
    total_time_node_all_mean = statistics.mean(total_time_node_all)
    print(energy_node_total,statistics.mean(task_time_node_all))
    print(total_time_node_all_mean,total_time_node_all)
    print(str((int(total_time_node_all_mean)))+','+str(energy_node_total[1])+','+str(energy_node_total[3])+','+str(energy_node_total_mean)+','+str(energy_node_total_stdev))

filename = "D:\TP\Study\wesg\Chirpbox\chirpbox manager\statistics_process\ewsn_data\sf_no_lbt\olbt_sf11_collect_data_command_len_48_used_sf11used_tp0command_len48_slot_num70startaddress_0807E000end_address0807E266(20200913192201859264).txt"
# get_energy_list(0, filename)
get_energy_list(1, filename)
