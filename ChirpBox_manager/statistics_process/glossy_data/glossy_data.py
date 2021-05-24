
import statistics



def glossy_process(filename):
    with open(filename, 'r') as f:
        previous_line = ''
        p_previous_line = ''
        ENERGEST = [[0 for x in range(3)] for y in range(21)]
        Average_latency = [0 for x in range(21)]
        glossy_cnt = [[] for y in range(21)]
        reliability = [0 for x in range(21)]

        for line in f:
            if (line.startswith('Node')):
                tmp = line.split()
                node_id = int(tmp[1][:2])
                # print(node_id)
            if (line.startswith('Value_1:') and (previous_line.startswith('Value:')) and (p_previous_line.startswith('ENERGEST:'))):
                tmp = line.split()
                if (ENERGEST[node_id][0] == 0):
                    ENERGEST[node_id] = [int(previous_line.split()[0][6:]), int(tmp[0][8:]), int(tmp[1][:4])]
            if (line.startswith('Value:') and (previous_line.startswith('Average la'))):
                tmp = line.split()
                if (Average_latency[node_id] == 0):
                    Average_latency[node_id] = int(tmp[0][6:])
            if (line.startswith('Value:') and (previous_line.startswith('reliabilit'))):
                tmp = line.split()
                if (reliability[node_id] == 0):
                    reliability[node_id] = int(tmp[0][6:])/1e3
            if (line.startswith('Value:') and (previous_line.startswith('get_relay_'))):
                tmp = line.split()
                # print(tmp)
                glossy_cnt[node_id].append(int(tmp[0][6:]))
            p_previous_line = previous_line
            previous_line = line
        # print(glossy_cnt)
        # print(ENERGEST, Average_latency, reliability, glossy_cnt)
        return (ENERGEST, Average_latency, reliability, glossy_cnt)


def glossy_energy(ENERGEST,Average_latency,packet_length,glossy_cnt,reliability):
    energy_nodes = []
    glossy_cnt_noddes = []
    energy_list = [80.9633, 207.37644, 181.71642]
    for i in range(len(ENERGEST)):
        energy_nodes.append(energy_list[0]*ENERGEST[i][0]+energy_list[1]*ENERGEST[i][1]+energy_list[2]*ENERGEST[i][2])
        glossy_cnt_noddes.append(statistics.mean(glossy_cnt[i]))

    energy_node_total_mean = statistics.mean(energy_nodes)
    energy_node_total_stdev = statistics.stdev(energy_nodes)
    reliability_mean = statistics.mean(reliability)
    min(reliability)
    Average_latency[:] = [x / packet_length for x in Average_latency]
    print(str(statistics.mean(Average_latency))+','+str(max(Average_latency))+','+str(reliability_mean)+','+str(min(reliability))+','+str(energy_nodes[2])+','+str(energy_nodes[4])+','+str(energy_node_total_mean))


filename = "D:\TP\Study\wesg\Chirpbox\Chirpbox_manager\statistics_process\glossy_data\glossy_ct11_version_used_sf7_slot_num80_payload_len_11(20200914214701085605)_data.txt"

(ENERGEST, Average_latency, reliability, glossy_cnt) = glossy_process(filename)
glossy_energy(ENERGEST,Average_latency,min(Average_latency),glossy_cnt,reliability)

