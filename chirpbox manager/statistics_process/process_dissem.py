from process_flash_data import *
from process_all_to_all import all_to_all_radio_time
import numpy as np
import seaborn as sns

node_num = 21
""" ---------------------------------------------------------------- """
# back config time:
all_to_all_filename = "all_to_all//all_to_all_used_sf7_slot_num60_payload_len_8(20200808164302868686).txt"
back_radio = all_to_all_radio_time(all_to_all_filename, node_num)
# print(back_radio)
# all_to_all_filename = "all_to_all//all_to_all_used_sf7_slot_num120_payload_len_8(20200808170257200810).txt"
# back_radio_mean, back_radio_std = all_to_all_radio_time(all_to_all_filename, node_num)


""" ---------------------------------------------------------------- """
file_list = ["dissem_data//disseminate_command_len_200_used_sf7_generate_size24_slot_num110_bitmap1FFFFF_FileSize102400_dissem_back_sf7_dissem_back_slot60(20200809004200169286).txt", "dissem_data//disseminate_command_len_200_used_sf7_generate_size24_slot_num100_bitmap1FFFFF_FileSize102400_dissem_back_sf7_dissem_back_slot60(20200808234057951666).txt", "dissem_data//disseminate_command_len_200_used_sf7_generate_size16_slot_num80_bitmap1FFFFF_FileSize102400_dissem_back_sf7_dissem_back_slot60(20200808224432365450).txt"]
dissem_config_list = [[7, 200, 110, 24, 7, 60], [7, 200, 100, 24, 7, 60], [7, 200, 80, 16, 7, 60]]
config_label_name = ["24-110", "24-100", "16-80"]
dissem_result = dissem_files(21, file_list, dissem_config_list)
# print(dissem_result)

""" ---------------------------------------------------------------- """
dissem_plt_result = []
for i in range(len(dissem_result)):
    radio_rx_total = dissem_result[i][0] * dissem_result[i][5] + back_radio[0] * (dissem_result[i][5] - 1)
    radio_rx_std = np.sqrt(np.square(dissem_result[i][1] * dissem_result[i][5]) + np.square(back_radio[1] * (dissem_result[i][5] - 1)))
    radio_tx_total = dissem_result[i][2] * dissem_result[i][5] + back_radio[2] * (dissem_result[i][5] - 1)
    radio_tx_std = np.sqrt(np.square(dissem_result[i][3] * dissem_result[i][5]) + np.square(back_radio[3] * (dissem_result[i][5] - 1)))
    dissem_plt_result.append([radio_rx_total, radio_rx_std, radio_tx_total, radio_tx_std, radio_rx_total + radio_tx_total, (radio_rx_total + radio_tx_total) / dissem_result[i][4], dissem_result[i][4]])
    print(radio_rx_total, radio_rx_std, radio_tx_total, radio_tx_std, radio_rx_total + radio_tx_total, dissem_result[i][4], dissem_result[i][5])

print(dissem_plt_result)
dissem_plt_pd = [[0]*7]*len(dissem_plt_result)
for i in range(0, len(dissem_plt_result)):
    dissem_plt_pd[i] = [i, dissem_plt_result[i][0], dissem_plt_result[i][1], dissem_plt_result[i][2], dissem_plt_result[i][3], dissem_plt_result[i][6], dissem_plt_result[i][5]]
df = pd.DataFrame(dissem_plt_pd,columns=['id', 'RX time','RX time std','TX time','TX time std','Total time','energy'])

df_bar = df[0:6].copy()

fig, ax = plt.subplots(figsize=(16,9))
sns.set_palette("PuBuGn_d") 
df[['RX time', 'TX time']].plot.bar(stacked=True, yerr=df[['RX time std', 'TX time std']].values.T, width=0.3, ax=ax, error_kw=dict(ecolor='k', lw=0.2, markersize='1', capsize=5, capthick=1, elinewidth=2))
df[['Total time']].plot.bar(stacked=True, width=0.3, fc=(1,0,0,0), ec=(0,0,0,1), ax=ax, linewidth=2)

ax2 = ax.twinx()
ax2.plot(df['energy'],linewidth=3,color='k', linestyle='-',marker='o', markersize=8,markerfacecolor='none')
# config ticks
# ax.set_xticks(fontsize=28)
# ax.set_yticks(fontsize=28)
ax.tick_params(axis="both", labelsize=24, length=10, width=2)

# change font size of the scientific notation in matplotlib
ax.yaxis.offsetText.set_fontsize(24)

ax.set_xticklabels(config_label_name, rotation=0)
ax.set_xlabel('Configs',fontsize=28)
ax.set_ylabel('Radio on time (s)',fontsize=28)
ax2.set_ylabel('Energy',fontsize=28)
ax2.tick_params(axis="both", labelsize=24, length=10, width=2)

legend = ax.legend(loc='upper left', edgecolor='k',fontsize = 16, fancybox=True)
legend.set_title("Config",prop={'size':16})
legend.get_frame().set_linewidth(1.5)
legend.get_frame().set_edgecolor("k")
# ax.set_ylim(0,1500)
ax2.set_ylim(0.4,1)


ax = plt.gca()
ax.set_aspect('auto')
ax.spines['bottom'].set_linewidth(1.5)
ax.spines['top'].set_linewidth(1.5)
ax.spines['left'].set_linewidth(1.5)
ax.spines['right'].set_linewidth(1.5)
ax.tick_params(direction='out', length=10, width=2)

# plt config
figure_name = "coldata_save//" + "dissem" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.pdf'

plt.tight_layout()
plt.savefig(figure_name, dpi = 300)

# plt.show()