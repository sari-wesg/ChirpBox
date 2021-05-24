import statistics

stats_total_len = int(40/4)
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

def get_lbt_list(filename, dis_value, col_value, len_row):
    node_num = 2
    Matrix_data = coldata_to_list(filename, node_num, (len_row - 8))
    channel_list_dis_all = [[0 for x in range(8)] for y in range(node_num)]
    channel_list_col_all = [[0 for x in range(8)] for y in range(node_num)]

    for i in range(0,node_num):
        c_list_d, c_list_c = Matrix_data_list_to_channel(Matrix_data,i, dis_value, col_value )
        channel_list_dis_all[i] = c_list_d
        channel_list_col_all[i] = c_list_c
        # if (((i != 5))):

    print(channel_list_dis_all)
    print('----------------------')
    print(channel_list_col_all)

def Matrix_data_list_to_channel(Matrix_data,k, dis_total, col_total):
    # task_list = Matrix_data[i][]
    print(Matrix_data)
    channel_list_dis = []
    channel_list_col = []
    for i in range(8):
        channel_list_dis.append(Matrix_data[k][32+i])
        channel_list_col.append(Matrix_data[k][32+8+i])
    # print (channel_list_dis, channel_list_col)
    channel_list_dis[:] = [x / (dis_total*1e4) for x in channel_list_dis]
    channel_list_col[:] = [x / (col_total*1e4) for x in channel_list_col]

    return (channel_list_dis, channel_list_col)

def list_time(Matrix_data_2, Matrix_data_1, node_num):
    Matrix_data = [[0 for x in range(10)] for y in range(node_num)]
    for i in range(len(Matrix_data_2)):
        for k in range(10):
            Matrix_data[i][k] = (Matrix_data_2[i][k] - Matrix_data_1[i][k]) / 1e6
    return Matrix_data


filename_2 ="D:\TP\Study\wesg\Chirpbox\Chirpbox_manager\disseminate_command_len_232_used_sf7used_tp14_generate_size16_slot_num80_bitmap1fffff_FileSize62284_dissem_back_sf7_dissem_back_slot80(20200917121600717151).txt"
filename_1 ="D:\TP\Study\wesg\Chirpbox\Chirpbox_manager\disseminate_command_len_232_used_sf7used_tp14_generate_size16_slot_num80_bitmap1fffff_FileSize62284_dissem_back_sf7_dissem_back_slot80(20200917114400731840).txt"

node_num = 21
Matrix_data_1 = coldata_to_list(filename_1, node_num, (48 - 8))
Matrix_data_2 = coldata_to_list(filename_2, node_num, (48 - 8))
Matrix_data = list_time(Matrix_data_2, Matrix_data_1, node_num)
print(Matrix_data_1)
print(Matrix_data_2)
print(Matrix_data)
