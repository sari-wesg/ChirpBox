import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
import numpy as np
import datetime

data = {'Late afternoon': {0: 17, 1: 37, 2: 38, 3: 36, 4: 36, 5: 32, 6: 38, 7: 34, 8: 45, 9: 38, 10: 47, 11:
37, 12: 36, 13: 36, 14: 44, 15: 39, 16: 34, 17: 33, 18: 35, 19: 41, 20: 40}, 'Evening': {0: 16, 1: 26, 2: 26, 3: 26, 4: 26, 5: 22, 6: 28, 7: 26, 8: 35, 9: 31, 10: 28, 11:
27, 12: 27, 13: 29, 14: 29, 15: 24, 16: 28, 17: 24, 18: 29, 19: 27, 20: 27}, 'Midnight': {0: 17, 1: 23, 2: 24, 3: 23, 4: 24, 5: 19, 6: 24, 7: 23, 8: 31, 9: 27, 10: 25, 11:
23, 12: 23, 13: 26, 14: 26, 15: 21, 16: 25, 17: 21, 18: 26, 19: 24, 20: 24}}

df = pd.DataFrame(data)
# fig, ax = plt.subplots(figsize=(21, 9))
fig, ax = plt.subplots(figsize=(36, 9))

# ax.tick_params(axis="both", labelsize=24, length=10, width=2)

# typically 3 to 4 p.m. local time. By this time, the sun's heat has built up since noon and more heat is present at the surface than is leaving it
# While you can typically expect the air temperature to drop as the evening and nighttime hours wear on, the lowest temperatures don't tend to happen until just before sunrise. 
df.plot(kind='bar', color = ["#DC5E4B", "#F39879", "#F4C3AB"], width=0.7, ax =ax)

ax = plt.gca()
ax.set_aspect('auto')
ax.spines['bottom'].set_linewidth(1.5)
ax.spines['top'].set_linewidth(1.5)
ax.spines['left'].set_linewidth(1.5)
ax.spines['right'].set_linewidth(1.5)
ax.tick_params(direction='out', length=10, width=2)

x_positions = np.linspace(0, 21, 22)
plt.xticks(x_positions, fontsize=36, rotation=0)
plt.yticks(fontsize=36)
# ax.set_xlim(0,20)
ax.set_ylim(0,50)

# plt.legend()
plt.xlabel('Node ID',fontsize=40)
plt.ylabel('Temperature ($^\circ$C)',fontsize=40)

legend = plt.legend(loc='upper right', edgecolor='k',fontsize = 36, fancybox=True, ncol=3)
legend.get_frame().set_linewidth(2)
legend.get_frame().set_edgecolor("k")

figure_name = "temp_" + datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + '.pdf'
plt.tight_layout()

# plt.show()
plt.savefig(figure_name, dpi = 300)
