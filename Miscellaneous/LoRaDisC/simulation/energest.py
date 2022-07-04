energest_value = [80.96, 53.51, 207.37, 181.72]
# energest_value = [80.96, 53.51, 287.65, 181.72]

Energest_LoRaWAN = [[805664, 67464447, 3856292, 6341369]]
Energest_LoRaDisC = [[805664, 67464447, 3856292, 6341369], [805664, 67464447, 3856292, 6341369]]
LoRaWAN_packet_count = 3
LoRaDisC_packet_count = 3

def calculate_energest_J():
    lorawan_node_num = len(Energest_LoRaWAN)
    loradisc_node_num = len(Energest_LoRaDisC)

    energest_lorawan = 0
    for i in range(lorawan_node_num):
        energest_node = Energest_LoRaWAN[i][0] * energest_value[0] + Energest_LoRaWAN[i][1] * energest_value[1] + Energest_LoRaWAN[i][2] * energest_value[2] + Energest_LoRaWAN[i][3] * energest_value[3]
        energest_lorawan += energest_node
    energest_lorawan = energest_lorawan / 1e9 / lorawan_node_num

    energest_loradisc = 0
    for i in range(loradisc_node_num):
        energest_node = Energest_LoRaDisC[i][0] * energest_value[0] + Energest_LoRaDisC[i][1] * energest_value[1] + Energest_LoRaDisC[i][2] * energest_value[2] + Energest_LoRaDisC[i][3] * energest_value[3]
        energest_loradisc += energest_node
    energest_loradisc = energest_loradisc / 1e9 / loradisc_node_num

    return (energest_lorawan, energest_loradisc)


energest_lorawan, energest_loradisc = calculate_energest_J()
print(energest_lorawan, energest_loradisc)