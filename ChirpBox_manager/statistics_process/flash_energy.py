flash_list = [0x31, 0x49, 0x83, 0x00, 0x9e, 0xd8, 0x3a, 0x00, 0x00, 0x51, 0x25, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x8b, 0x3c, 0x00, 0xe7, 0x63, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x85, 0xc1, 0x31, 0x01, 0x4a, 0x32, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x85, 0x69, 0x00, 0x0f, 0x29, 0x53, 0x03, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x18, 0xd7, 0x16, 0x01, 0x03, 0x1c, 0xe9, 0x14, 0x00, 0x00, 0x00, 0x00, 0x66, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0xac, 0x00, 0x00, 0xba, 0x03, 0x00, 0x00, 0xac, 0x6e, 0xdd, 0x09, 0x10, 0xed, 0x0e, 0x35, 0x00, 0x00, 0x00, 0x00
]

idle_list = []
arrange_list = []
task_list = []
len_flash = len(flash_list)
for i in range(int(len_flash / 4)):
    hex_value = int(flash_list[i*4]) * 1 + int(flash_list[i*4+1]) * 256 + int(flash_list[i*4+2]) * 256 * 256 + int(flash_list[i*4+3]) * 256 * 256 * 256
    count = int(i / 16)
    count_1 = i % 16
    if((count == 0) and (count_1 < 10)):
        idle_list.append(hex_value / 1e6)
    elif((count == 1) and (count_1 < 10)):
        arrange_list.append(hex_value / 1e6)
    elif((count == 2) and (count_1 < 10)):
        task_list.append(hex_value / 1e6)

    # print(hex_value, i, count, count_1)
# print(idle_list, arrange_list, task_list)

for i in range(len(task_list)):
    task_list[i] = task_list[i] + arrange_list[i] + idle_list[i]

energy_total = 0
energy_list_14 = [80.9633, 53.5069, 18.837, 47.7732, 72.84508, 62.39981, 80.9633, 287.6562, 181.71642, 0]
energy_list_0 = [80.9633, 53.5069, 18.837, 47.7732, 72.84508, 62.39981, 80.9633, 207.37644, 181.71642, 0]

for i in range(len(idle_list)):
    energy_total = energy_total + idle_list[i] * energy_list_14[i]
for i in range(len(arrange_list)):
    energy_total = energy_total + arrange_list[i] * energy_list_14[i]
for i in range(len(task_list)):
    energy_total = energy_total + task_list[i] * energy_list_0[i]

total_time = 0
for i in range(len(task_list)):
    total_time = total_time + task_list[i]

print(energy_total)
print(total_time, energy_total)
