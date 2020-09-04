
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
from matplotlib.ticker import MultipleLocator
import datetime
from matplotlib.lines import Line2D

# real_data = [67, 83, 75, 73, 76, 78, 72, 68, 73, 73, 69, 84, 69, 70, 78, 77, 85, 79, 80, 83, 79]
# rx_list = [57, 77, 71, 66, 68, 32, 66, 62, 67, 59, 64, 75, 43, 62, 75, 70, 71, 75, 75, 70, 74]

# real_data = [89, 1, 2, 3, 89, 91, 1, 2, 3, 89, 1, 90, 89, 89, 1, 2, 90, 1, 2, 89, 1]
# rx_list = [0, 0, 0, 0, 85, 79, 0, 0, 0, 87, 0, 64, 85, 80, 0, 0, 84, 0, 0, 83, 0]

real_data_1 = [89, 89, 89, 91, 89, 91, 90, 90, 89, 89, 89, 90, 89, 89, 89, 90, 90, 90, 90, 89, 89]
rx_list_1 = [78, 85, 86, 80, 85, 67, 85, 85, 83, 80, 85, 85, 80, 80, 85, 83, 84, 83, 83, 83, 85]
err1 = [2.3, 2, 3, 4.3, 4, 5.3, 2.3, 1.3, 2.3, 4.2, 1.3, 1.23, 2.9, 2.2, 2.9, 2.7, 3.2, 2.1, 1.2, 2.9, 2.0]
real_data = [29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29]
rx_list = [27, 28, 28, 28, 29, 28.7, 28, 28, 27, 26.9, 28, 28.6, 28.5, 27.9, 28.4, 27, 28, 28, 28.5, 27.5, 28]
err = [1.9, 2.1, 1.7, 4.5, 0.9, 2.3, 3.3, 1.3, 1.1, 2.7, 1.0, 1.13, 2.1, 2.1, 1.9, 1.7, 1.2, 2.4, 2.2, 1.9, 2.3]

x = np.arange(0, 21, 1)
print(x)
pdr_list_1 = []
for i in range(len(real_data_1)):
    pdr_list_1.append((rx_list_1[i] / real_data_1[i]) * 100)
print(pdr_list_1)

pdr_list = []
for i in range(len(real_data)):
    pdr_list.append((rx_list[i] / real_data[i]) * 100)
print(pdr_list)

fig, ax = plt.subplots(figsize=(16, 9))

lns1 = ax.errorbar(x, pdr_list_1, yerr=err1,linestyle='none',color="#09526A", marker='D',  markersize=10,markerfacecolor='#09526A', markeredgecolor='#09526A', markeredgewidth = 2, linewidth=2.0, label = 'type1', capsize=5, capthick=1, elinewidth=3)
lns2 = ax.errorbar(x, pdr_list, yerr=err,linestyle='none',color="#DC5E4B", marker='D',  markersize=10,markerfacecolor='#DC5E4B', markeredgecolor='#DC5E4B', markeredgewidth = 2, linewidth=2.0, label = 'type1', capsize=5, capthick=1, elinewidth=3)
# plt.plot(x, pdr_list_1, linestyle='None', marker='o', markersize = 10)
# plt.plot(x, pdr_list, linestyle='None', marker='o', markersize = 10)
ax = plt.gca()
ax.set_ylim(60,103)
ax.yaxis.set_major_locator(MaxNLocator(prune='both'))
# ax = plt.gca()
node_list = list(range(21))
ax.set_xticks(node_list)

ax.set_axisbelow(True)
ax.set_aspect('auto')
ax.spines['bottom'].set_linewidth(1.5)
ax.spines['top'].set_linewidth(1.5)
ax.spines['left'].set_linewidth(1.5)
ax.spines['right'].set_linewidth(1.5)
ax.tick_params(direction='out', length=10, width=2)
ax.set_xlabel('Node ID',fontsize=40)
ax.set_ylabel('PRR (%)',fontsize=40)
ax.tick_params(axis="both", labelsize=28, length=10, width=2)


colors = ['#09526A', '#DC5E4B']
lines = [Line2D([0], [0], color=c, linestyle='none',marker='D', markersize = 20, alpha=0.8, label=str(i), markeredgewidth=0, markeredgecolor=c, markerfacecolor=c) for c in colors]
labels = ['1 gateway', '2 gateways']

legend = plt.legend(lines, labels, loc='lower right', edgecolor='k',fontsize = 28, fancybox=True, handlelength=1, handleheight=1, ncol=2)

# legend = plt.legend(loc='upper right', edgecolor='k',fontsize = 36, fancybox=True, ncol=3)
legend.get_frame().set_linewidth(2)
legend.get_frame().set_edgecolor("k")

# plt.show()
figure_name = "lorawan_prr" +'.pdf'

plt.tight_layout()
plt.savefig(figure_name, dpi = 3600)
# 4, 5, 9, 11, 12, 13, 16, 19