import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
import numpy as np
import datetime

# 4 2 1 6 5 3
# 39
# 107
# 406

# 71
# 69
# 76
# 67
# 74
# 73
df = pd.read_csv('dissem_col_position_sf7_60kb_2kb.csv')
print(df)
dissem_df = df.loc[df['dis'] == 1]
coll_df = df.loc[df['dis'] == 0]
dissem_df_x = dissem_df['position']
dissem_df_y = dissem_df['time']
col_df_x = coll_df['position']
col_df_y = coll_df['time']
plt.plot(dissem_df_x, dissem_df_y,linestyle='None', marker='o', markersize = 10)
plt.plot(col_df_x, col_df_y,linestyle='None', marker='o', markersize = 10)
plt.show()