from process_flash_data import *
from cal_task_time import task_slot, CHIRP_TASK


filename = "all_to_all//all_to_all_used_sf7_slot_num60_payload_len_8(20200808164302868686).txt"
node_num = 21
f_data_len = 120
Matrix_data = coldata_to_list(filename, int(node_num), int(f_data_len - 8))

rx_time, tx_time, channel_data, task_try_num = matrix_to_type_data(Matrix_data, node_num, filename)

radio_on_time = [0] * len(rx_time)
# print(rx_time, tx_time)
for i in range(0, len(rx_time)):
    radio_on_time[i] = (rx_time[i] + tx_time[i]) / 1e6
radio_std = statistics.stdev(radio_on_time)
radio_mean = statistics.mean(radio_on_time)

# print(radio_on_time, radio_mean, radio_std)
task_slot(CHIRP_TASK.CHIRP_VERSION, 11, 60, 7, 21, 21)