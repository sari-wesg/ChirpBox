
import statistics



def glossy_process(filename):
    with open(filename, 'r') as f:
        previous_line = ''
        p_previous_line = ''
        rx_count = [0 for x in range(21)]

        for line in f:
            if (line.startswith('Node')):
                tmp = line.split()
                node_id = int(tmp[1][:2])
            if (line.startswith('Value:') and (previous_line.startswith('lora_rx_co'))):
                tmp = line.split()
                if (rx_count[node_id] == 0):
                    rx_count[node_id] = int(tmp[0][6:9])
            p_previous_line = previous_line
            previous_line = line
        print(rx_count)
        return (rx_count)


def glossy_energy(time_s):
    energy_list = [181.71642]
    energy = energy_list[0] * time_s
    print(energy)

filename = "D:\TP\Study\wesg\Chirpbox\chirpbox manager\statistics_process\lorawan\lorawan_6.txt"

rx_count = glossy_process(filename)

max_count = max(rx_count)
rece_count = [x / max_count for x in rx_count]
# print(rece_count)
rece_count = [i for i in rece_count if i != 0]
# print(rece_count)
reliability_mean = statistics.mean(rece_count)
print(reliability_mean*1e6)
glossy_energy(957)