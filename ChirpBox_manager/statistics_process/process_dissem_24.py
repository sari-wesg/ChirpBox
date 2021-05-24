from process_flash_data import *
from process_all_to_all import all_to_all_radio_time
import numpy as np
import seaborn as sns
import matplotlib.patches as mpatches
# from matplotlib.lines import Line2D
import matplotlib.lines as mlines
from matplotlib.ticker import MultipleLocator

node_num = 21
""" ---------------------------------------------------------------- """
# back config time:
all_to_all_filename = "all_to_all//all_to_all_used_sf7_slot_num60_payload_len_8(20200808164302868686).txt"
back_radio_60 = all_to_all_radio_time(all_to_all_filename, node_num)
all_to_all_filename = "all_to_all//all_to_all_used_sf7_slot_num80_payload_len_8(20200809183756428083).txt"
back_radio_80 = all_to_all_radio_time(all_to_all_filename, node_num)
# print(back_radio)
# all_to_all_filename = "all_to_all//all_to_all_used_sf7_slot_num120_payload_len_8(20200808170257200810).txt"
# back_radio_mean, back_radio_std = all_to_all_radio_time(all_to_all_filename, node_num)


""" ---------------------------------------------------------------- """
# group1: same SF (7), different file size and configs
# group2: different SF and configs, same file size
file_list = ["dissem_time//disseminate_command_len_232_used_sf7_generate_size16_slot_num80_bitmap1FFFFF_FileSize5120_dissem_back_sf7_dissem_back_slot80(20200811042957555476).txt","dissem_time//disseminate_command_len_232_used_sf7_generate_size24_slot_num120_bitmap1FFFFF_FileSize5120_dissem_back_sf7_dissem_back_slot80(20200810110858799047).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size32_slot_num150_bitmap1FFFFF_FileSize5120_dissem_back_sf7_dissem_back_slot80(20200810114011399513).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size16_slot_num80_bitmap1FFFFF_FileSize15360_dissem_back_sf7_dissem_back_slot80(20200811044715623570).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size24_slot_num120_bitmap1FFFFF_FileSize15360_dissem_back_sf7_dissem_back_slot80(20200810102002107092).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size32_slot_num150_bitmap1FFFFF_FileSize15360_dissem_back_sf7_dissem_back_slot80(20200810104839237604).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size16_slot_num80_bitmap1FFFFF_FileSize40960_dissem_back_sf7_dissem_back_slot80(20200811051742210983).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size24_slot_num120_bitmap1FFFFF_FileSize40960_dissem_back_sf7_dissem_back_slot80(20200810022219634248).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size32_slot_num150_bitmap1FFFFF_FileSize40960_dissem_back_sf7_dissem_back_slot80(20200810031551203649).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size16_slot_num80_bitmap1FFFFF_FileSize61440_dissem_back_sf7_dissem_back_slot80(20200811071707960877).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size24_slot_num120_bitmap1FFFFF_FileSize61440_dissem_back_sf7_dissem_back_slot80(20200809221130382056).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size32_slot_num150_bitmap1FFFFF_FileSize61440_dissem_back_sf7_dissem_back_slot80(20200809231454340618).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size16_slot_num80_bitmap1FFFFF_FileSize102400_dissem_back_sf7_dissem_back_slot80(20200811074529606211).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size24_slot_num120_bitmap1FFFFF_FileSize102400_dissem_back_sf7_dissem_back_slot80(20200809235533700909).txt", "dissem_time//disseminate_command_len_232_used_sf7_generate_size32_slot_num150_bitmap1FFFFF_FileSize102400_dissem_back_sf7_dissem_back_slot80(20200810013548853290).txt"]
dissem_config_list = [[7, 232, 80, 16, 7, 80], [7, 232, 120, 24, 7, 80], [7, 232, 150, 32, 7, 80], [7, 232, 80, 16, 7, 80], [7, 232, 120, 24, 7, 80], [7, 232, 150, 32, 7, 80], [7, 232, 80, 16, 7, 80], [7, 232, 120, 24, 7, 80], [7, 232, 150, 32, 7, 80], [7, 232, 80, 16, 7, 80], [7, 232, 120, 24, 7, 80], [7, 232, 150, 32, 7, 80], [7, 232, 80, 16, 7, 80], [7, 232, 120, 24, 7, 80], [7, 232, 150, 32, 7, 80]]
config_label_name = ["16", "24", "32","16",  "24", "32","16",  "24", "32", "16", "24", "32", "16", "24", "32"]
config_label_name_file = ["5", "15", "40", "60", "100"]
dissem_result = dissem_files(21, file_list, dissem_config_list)
# print(dissem_result)
type_dissem = 3

""" ---------------------------------------------------------------- """
dissem_plt_result = []
for i in range(len(dissem_result)):
    # if (i % 2 == 0):
    #     back_radio = back_radio_60
    # else:
    back_radio = back_radio_80
    radio_rx_total = dissem_result[i][0] * dissem_result[i][5] + back_radio[0] * (dissem_result[i][5] - 1)
    radio_rx_std = np.sqrt(np.square(dissem_result[i][1] * dissem_result[i][5]) + np.square(back_radio[1] * (dissem_result[i][5] - 1)))
    radio_tx_total = dissem_result[i][2] * dissem_result[i][5] + back_radio[2] * (dissem_result[i][5] - 1)
    radio_tx_std = np.sqrt(np.square(dissem_result[i][3] * dissem_result[i][5]) + np.square(back_radio[3] * (dissem_result[i][5] - 1)))
    dissem_plt_result.append([radio_rx_total, radio_rx_std, radio_tx_total, radio_tx_std, radio_rx_total + radio_tx_total, ((radio_rx_total + radio_tx_total) / dissem_result[i][4]) * 100, dissem_result[i][4]])
    print(radio_rx_total, radio_rx_std, radio_tx_total, radio_tx_std, radio_rx_total + radio_tx_total, dissem_result[i][4], dissem_result[i][5])

print("dissem_plt_result", dissem_plt_result)
dissem_plt_pd = [[0]*7]*len(dissem_plt_result)
for i in range(0, len(dissem_plt_result)):
    dissem_plt_pd[i] = [i, dissem_plt_result[i][0], dissem_plt_result[i][1], dissem_plt_result[i][2], dissem_plt_result[i][3], dissem_plt_result[i][6], dissem_plt_result[i][5]]
df = pd.DataFrame(dissem_plt_pd,columns=['id', 'RX time','RX time std','TX time','TX time std','Total time','energy'])

df_bar = df[0:len(df)].copy()
df1 = df_bar.iloc[[0,3,6, 9, 12],:]
df2 = df_bar.iloc[[1,4,7, 10, 13],:]
df3 = df_bar.iloc[[2,5,8, 11, 14],:]
print(df)
# print(df2)
fig, ax = plt.subplots(figsize=(16, 9))
# sns.set_palette(["#344146", "#7BBAD2"])
sns.set_palette(sns.color_palette("muted"))
# df[['RX time', 'TX time']].plot.bar(stacked=True, yerr=df[['RX time std', 'TX time std']].values.T, width=0.3, ax=ax, error_kw=dict(ecolor='k', lw=0.2, markersize='1', capsize=5, capthick=1, elinewidth=2))
# df[['Total time']].plot.bar(stacked=True, width=0.3, fc=(1,0,0,0), ec=(0,0,0,1), ax=ax, linewidth=2)

df1[['RX time', 'TX time']].plot.bar(position=1.5, stacked=True, yerr=df1[['RX time std', 'TX time std']].values.T, width=0.25, ax=ax, error_kw=dict(ecolor='k', lw=0.2, markersize=10, capsize=5, capthick=1, elinewidth=2), align='center', color = ["#618A8A", "#39F3BB"])
df1[['Total time']].plot.bar(position=1.5, stacked=True, width=0.25, fc=(1,0,0,0), ec=(0,0,0,1), ax=ax, linewidth=2, label='_nolegend_')

df2[['RX time', 'TX time']].plot.bar(position=0.5, stacked=True, yerr=df2[['RX time std', 'TX time std']].values.T, width=0.25, ax=ax, error_kw=dict(ecolor='k', lw=0.2, markersize=10, capsize=5, capthick=1, elinewidth=2), align='center', color = ["#15607A", "#18A1CD"])
df2[['Total time']].plot.bar(position=0.5, stacked=True, width=0.25, fc=(1,0,0,0), ec=(0,0,0,1), ax=ax, linewidth=2, label='_nolegend_')

df3[['RX time', 'TX time']].plot.bar(position=-0.5, stacked=True, yerr=df3[['RX time std', 'TX time std']].values.T, width=0.25, ax=ax, error_kw=dict(ecolor='k', lw=0.2, markersize=10, capsize=5, capthick=1, elinewidth=2), align='center', color = ["#233A54", "#82878C"])
df3[['Total time']].plot.bar(position=-0.5, stacked=True, width=0.25, fc=(1,0,0,0), ec=(0,0,0,1), ax=ax, linewidth=2, label='_nolegend_')


ax = plt.gca()
ax.set_axisbelow(True)
ax.grid(True)
minorLocator = MultipleLocator(1)
ax.xaxis.set_minor_locator(minorLocator)
plt.gca().yaxis.grid(color='gray', linestyle='dashed')
ax2 = ax.twinx()
# ax2.plot(df['energy'],linestyle='None',marker=['s', 'D'], markersize=8,markerfacecolor='None', markeredgecolor=['r','purple'])

df_energy = []
for i in range((int)(len(file_list) / type_dissem)):
    df_energy.append([df.iloc[i * type_dissem,6], df.iloc[i * type_dissem+1,6], df.iloc[i * type_dissem+2,6]])
df_energy_frame = pd.DataFrame(df_energy,columns=['type1', 'type2', 'type3'])
lns1 = ax2.plot(ax.get_xticks(),
        df_energy_frame['type1'].values,
        linestyle='--',color="blue",
        marker='o', markersize = 10, linewidth=2.0, label = 'type1')

lns2 = ax2.plot(ax.get_xticks(),
        df_energy_frame['type2'].values,
        linestyle='--',color="k",
        marker='o', markersize = 10, linewidth=2.0, label = 'type2')

lns3 = ax2.plot(ax.get_xticks(),
        df_energy_frame['type3'].values,
        linestyle='--',color="r",
        marker='o', markersize = 10, linewidth=2.0, label = 'type3')
# config ticks
# ax.set_xticks(fontsize=28)
ax.yaxis.set_ticks(np.arange(0, 3600+1, 600))

ax.tick_params(axis="both", labelsize=24, length=10, width=2)

# change font size of the scientific notation in matplotlib
ax.yaxis.offsetText.set_fontsize(24)

# Hide major tick labels
# ax.set_xticklabels(config_label_name_file, minor=True, rotation=0)
# ax2.set_xticklabels('')

# Customize minor tick labels
print(ax.get_xticks())
x_list = [x - 0.6 for x in ax2.get_xticks()]
print(x_list)

ax.set_xticks(x_list,      minor=True)

ax2.set_xticks(x_list,      minor=True)
# ax.set_xticklabels('')
ax.set_xticklabels(config_label_name_file, rotation=0)
ax.set_xlabel('Firmware size (kB)',fontsize=28)
ax.set_ylabel('Time (s)',fontsize=28)
ax2.set_ylabel('Average energy',fontsize=28)
ax2.tick_params(axis="both", labelsize=24, length=10, width=2)


# RX = mpatches.Patch(color='#344146', label='RX time')
# TX = mpatches.Patch(color='#7BBAD2', label='TX time')
# TOTAL = mpatches.Patch(color='red', label='Total time', fc=(1,0,0,0), ec=(0,0,0,1), linewidth=2)
# legend = plt.legend(handles=[RX,TX,TOTAL], loc='upper left', edgecolor='k',fontsize = 20, fancybox=True)
hand, labl = ax.get_legend_handles_labels()
handout=[]
lablout=[]
total_time_added = 0
for h,l in zip(hand,labl):
    print(l)
    if (l == 'RX time'):
        lablout.append(l)
        handout.append(h)

lablout = ["Config 1:", "Config 2:", "Config 3:", ""] + lablout
handout = [plt.plot([],marker="", ls="", markersize = 1)[0]] * 4 + handout
for h,l in zip(hand,labl):
    if (l == 'Total time'):
        lablout.append(l)
        handout.append(h)
        break

for h,l in zip(hand,labl):
    print(l)
    if (l == 'TX time'):
        lablout.append(l)
        handout.append(h)
print(lablout)

legend = ax.legend(handout, lablout, loc='upper left', edgecolor='k',fontsize = 20, fancybox=True, ncol=3, handletextpad=1, handlelength=2)
# legend.set_title("Config",prop={'size':20})
legend.get_frame().set_linewidth(2)
legend.get_frame().set_edgecolor("k")
for vpack in legend._legend_handle_box.get_children()[:1]:
    for hpack in vpack.get_children():
        hpack.get_children()[0].set_width(0)

energy_1 = mlines.Line2D([], [], color='blue', linestyle='--',marker='o', markersize=10, label='Config 1')
energy_2 = mlines.Line2D([], [], color='k', linestyle='--',marker='o', markersize=10, label='Config 2')
energy_3 = mlines.Line2D([], [], color='r', linestyle='--',marker='o', markersize=10, label='Config 3')
legend = plt.legend(handles=[energy_1,energy_2,energy_3], loc='upper right', edgecolor='k',fontsize = 20, fancybox=True)

# legend.set_title("Config",prop={'size':20})
legend.get_frame().set_linewidth(2)
legend.get_frame().set_edgecolor("k")
ax.set_ylim(0,3600)
ax2.set_ylim(0,100)
y_value=['{:,.2f}'.format(x) + '%' for x in ax2.get_yticks()]
ax2.set_yticklabels(y_value, fontsize=28)


ax = plt.gca()
ax.set_aspect('auto')
ax.spines['bottom'].set_linewidth(1.5)
ax.spines['top'].set_linewidth(1.5)
ax.spines['left'].set_linewidth(1.5)
ax.spines['right'].set_linewidth(1.5)
ax.tick_params(direction='out', length=10, width=2)
# ax.grid(which='both', axis='both', linestyle='--')
ax.set_xticklabels(config_label_name_file)
plt.setp(ax.get_xticklabels(), rotation=30)

# plt config
figure_name = "coldata_save//" + "dissem" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.pdf'

plt.tight_layout()
plt.savefig(figure_name, dpi = 3600)

# plt.show()