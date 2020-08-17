import math
import enum

""" enum in chirpbox """
class CHIRP_TASK(enum.Enum):
	CHIRP_START         = 1
	MX_DISSEMINATE      = 2
	MX_COLLECT          = 3
	CHIRP_CONNECTIVITY  = 4
	CHIRP_TOPO          = 5
	CHIRP_SNIFF         = 6
	CHIRP_VERSION       = 7

	MX_ARRANGE          = 8

	MX_TASK_FIRST       = 1
	MX_TASK_LAST        = 8

LBT_DELAY_IN_US = 10000
CHANNEL_ALTER   = 2
""" function in mixer """
def PADDING_SIZE(x):
    return (4 - x % 4) % 4

""" function for LoRa """
def filter_SF(arg):
    switcher = {
        7: lambda: 125000,
        8: lambda: 250000,
        9: lambda: 250000,
    }
    return switcher.get(arg, lambda: "Invalid arg")()

def lora_packet_time(sf, bandwidth, pktLen):
    cr = 1
    preamble_len = 8
    Crc_on = 1
    FixLen = 0
    # chose bandwidth
    bw = filter_SF(bandwidth)

    if( ( ( bandwidth == 7 ) and ( ( sf == 11 ) or ( sf == 12 ) ) ) or
    ( ( bandwidth == 8 ) and ( sf == 12 ) ) ):
        LowDatarateOptimize = 1
    else:
        LowDatarateOptimize = 0

    rs = (1e3 * bw) / ( 1 << sf )
    ts = 1e9 / rs
    tmp = (math.ceil( ( 8 * pktLen - 4 * sf + 28 + 16 * Crc_on - (20 if ( FixLen > 0 ) else 0) ) / ( 4 * (sf - (2 if ( LowDatarateOptimize > 0 ) else 0))) ) * ( cr + 4 ))
    tPreamble = (preamble_len + 4) * ts + ts / 4
    nPayload = 8 + (tmp if ( tmp > 0 ) else 0)
    tPayload = nPayload * ts
    tOnAir = tPreamble + tPayload
    rs = int((bw) / ( 1 << sf ))
    ts = int(1e6 / rs)
    symbol_time = ts
    # print("tOnAir", tOnAir, "symbol_time", symbol_time, rs)
    return (tOnAir, symbol_time)

""" function in chirpbox """
def chirp_mx_packet_config(mx_num_nodes, mx_generation_size, mx_payload_size):
    coding_vector_len = int((mx_generation_size + 7) / 8)
    payload_len = int(mx_payload_size)
    info_vector_len = coding_vector_len
    _padding_2_len = (0 if ( 0 > (PADDING_SIZE(coding_vector_len) + PADDING_SIZE(mx_payload_size) - PADDING_SIZE(coding_vector_len))) else (PADDING_SIZE(coding_vector_len) + PADDING_SIZE(mx_payload_size) - PADDING_SIZE(coding_vector_len)))
    phy_payload_size = 8 + coding_vector_len + payload_len + info_vector_len

    print("coding_vector_len", coding_vector_len, "payload_len", payload_len, "info_vector_len", info_vector_len, "phy_payload_size", phy_payload_size)
    return phy_payload_size


def chirp_mx_slot_config(mx_slot_length_in_us, mx_round_length, period_time_us_plus):
    mx_period_time_us = mx_slot_length_in_us * mx_round_length + period_time_us_plus
    mx_period_time_s = int((mx_period_time_us + 1000000 - 1) / 1000000)
    # print("mx_period_time_s", mx_period_time_s, mx_slot_length_in_us)
    return mx_period_time_s


""" function in script """
def task_slot(task, payload_len, slot_num, sf, default_generate_size, node_num):
    mx_num_nodes = node_num
    mx_generation_size = 0
    if (task == CHIRP_TASK.MX_ARRANGE):
        mx_generation_size = mx_num_nodes
        payload_len = 12
        slot_num = mx_num_nodes * 4
        sf = 11
    elif (task == CHIRP_TASK.CHIRP_START):
        mx_generation_size = mx_num_nodes
        payload_len = 25
    elif(task == CHIRP_TASK.MX_DISSEMINATE):
        mx_generation_size = default_generate_size
    elif(task == CHIRP_TASK.MX_COLLECT):
        mx_generation_size = mx_num_nodes
    elif(task == CHIRP_TASK.CHIRP_CONNECTIVITY):
        mx_generation_size = mx_num_nodes
        payload_len = 15
    elif(task == CHIRP_TASK.CHIRP_TOPO):
        mx_generation_size = mx_num_nodes
    elif(task == CHIRP_TASK.CHIRP_SNIFF):
        mx_generation_size = mx_num_nodes
        # TODO:
        sniff_nodes_num = 1
        payload_len = 8 + sniff_nodes_num * 5
    elif(task == CHIRP_TASK.CHIRP_VERSION):
        mx_generation_size = mx_num_nodes
        payload_len = 11
    phy_payload_size = chirp_mx_packet_config(mx_num_nodes, mx_generation_size, payload_len + 2)
    print(phy_payload_size)
    packet_time, symbol_time_us = lora_packet_time(sf, 7, phy_payload_size)
    lbt_detect_duration_us = (6 * symbol_time_us if ( 6 * symbol_time_us > LBT_DELAY_IN_US ) else LBT_DELAY_IN_US)
    print("lbt_detect_duration_us", lbt_detect_duration_us, symbol_time_us)
    print(packet_time)
    chirp_mx_slot_time = chirp_mx_slot_config(packet_time + 100000 + lbt_detect_duration_us * CHANNEL_ALTER, slot_num, 1500000)
    return chirp_mx_slot_time


def dissem_total_time(send_sf, send_payload, send_slot, send_generate, back_sf, back_slot, try_num, node_num):
    dissem_time = task_slot(CHIRP_TASK.MX_DISSEMINATE, send_payload, send_slot, send_sf, send_generate, node_num)
    dissem_time_total = try_num * dissem_time
    dissem_col_time = task_slot(CHIRP_TASK.MX_COLLECT, 8, back_slot, back_sf, node_num, node_num)
    dissem_col_time_total = (try_num - 1) * dissem_col_time
    dissem_total_time = dissem_time_total + dissem_col_time_total
    print(dissem_time, dissem_time_total, dissem_col_time, dissem_col_time_total, dissem_total_time)
    return (dissem_time, dissem_total_time, dissem_time_total)

def effective_round(send_payload, send_generate, file_size):
    effective_length = send_payload - 8
    fragment_size = send_generate * effective_length
    effective_round = math.ceil((file_size + fragment_size - 1) / fragment_size) + 1
    return effective_round


task_payload_len = 200
task_slot_num = 90
task_sf = 7
task_generate_size = 12
task_back_sf = 7
task_back_slot = 80
task_try_num = 4
node_num = 21

# print(dissem_total_time(task_sf, task_payload_len, task_slot_num, task_generate_size, task_back_sf, task_back_slot, task_try_num, node_num))
print(task_slot(CHIRP_TASK.MX_DISSEMINATE, 232, 80, 7, 17, 21))