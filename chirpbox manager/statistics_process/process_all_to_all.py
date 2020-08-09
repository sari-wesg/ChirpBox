from process_flash_data import *
from cal_task_time import task_slot, CHIRP_TASK



def all_to_all_radio_time(filename, node_num):
    f_data_len = 120
    Matrix_data = coldata_to_list(filename, int(node_num), int(f_data_len - 8))
    rx_time, tx_time, channel_data, task_try_num = matrix_to_type_data(Matrix_data, node_num, filename)
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
    # print(radio_mean, radio_std)
    return ([radio_rx_mean, radio_rx_stdev, radio_tx_mean, radio_tx_stdev])

# filename = "all_to_all//all_to_all_used_sf7_slot_num60_payload_len_8(20200808164302868686).txt"

# all_to_all_radio_time(filename, 21)

# arrange time == version time (sf 12)
# task_slot(CHIRP_TASK.MX_ARRANGE, 12, 84, 12, 21, 21)
# task_slot(CHIRP_TASK.CHIRP_VERSION, 11, 84, 12, 21, 21)