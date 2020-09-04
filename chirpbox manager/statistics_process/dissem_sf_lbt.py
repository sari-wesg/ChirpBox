import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
import numpy as np
import datetime


y1 = [265, 388.7, 485.14, 863.34, 2115.46]
y2 = [257, 380, 465, 819.5, 1907]
y3 = [330, 536.25, 754, 1827, 3952]
y4 = [318, 529, 727.48, 1732.5, 3453.8]
x = [7,8,9,10,11]


df = pd.read_csv('dissem_col_sf_lbt_5kb_2kb.csv')
print(df)
# dissem_df = df.loc[df['dis'] == 1]
# coll_df = df.loc[df['dis'] == 0]
# dissem_df_x = dissem_df['position']
# dissem_df_y = dissem_df['time']
# col_df_x = coll_df['position']
# col_df_y = coll_df['time']
plt.plot(x, y1,linestyle='None', marker='o', markersize = 10)
plt.plot(x, y2,linestyle='None', marker='o', markersize = 10)
plt.plot(x, y3,linestyle='None', marker='o', markersize = 10)
plt.plot(x, y4,linestyle='None', marker='o', markersize = 10)

plt.show()