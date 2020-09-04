import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
import numpy as np
import datetime

# ['fffff', '7FFF', '3FF', '1F']
# ['1fffff', '16FB9F', '14B38D', '1385']

# 20 nodes 41 s 15 s 1105
# 15 nodes 39.3 s 9.3 s 962.7
# 10 nodes 36.36 s 6.91 s 858.49
# 5 nodes 30.52 s 3.86 s 683.74

df = pd.read_csv('dissem_col_nodes_sf7_60kb_2kb.csv')
print(df)
dissem_df = df.loc[df['dis'] == 1]
coll_df = df.loc[df['dis'] == 0]
dissem_df_x = dissem_df['node']
dissem_df_y = dissem_df['time']
col_df_x = coll_df['node']
col_df_y = coll_df['time']
plt.plot(dissem_df_x, dissem_df_y,linestyle='None', marker='o', markersize = 10)
plt.plot(col_df_x, col_df_y,linestyle='None', marker='o', markersize = 10)
plt.show()