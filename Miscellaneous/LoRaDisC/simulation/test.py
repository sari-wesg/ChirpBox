
# %%
from grpc import Channel
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from matplotlib.ticker import MultipleLocator


# %%

# parameters:
end_point = 40
# TODO:
interval = 9
slot_length = 3 # slot length < interval
start_point = [0, 1, 2, 2, 2, 5, 6, 7, 8, 5] # start_point in [0, interval)
channel_id = [0, 1, 2, 3, 4, 5, 6, 7, 8, 6]
node_num = len(start_point)
# channel_id = [0] * node_num




fig, ax = plt.subplots(figsize=(16, 9))

for node in range(node_num):
    list_begin = list(range(start_point[node], end_point, interval))
    list_end = list(range(start_point[node]+slot_length, end_point+1, interval))

    if (end_point - list_begin[-1]) < slot_length:
        list_end.append(end_point)
    if (list_begin[0]) > interval - slot_length:
        list_end.append(list_begin[0] - (interval - slot_length))
        list_begin.append(0)

    # print(list_begin)
    # print(list_end)

    begin = list_begin
    end = list_end
    df = pd.DataFrame(np.array([begin, end]).T)
    df.columns =['begin', 'end']

    for x_1 , x_2 in zip(df['begin'].values ,df['end'].values):
        ax.add_patch(plt.Rectangle((x_1,channel_id[node]),x_2-x_1,1))

ax.autoscale()
ax.set_ylim(0,8)
ax.set_xlim(0,end_point)

plt.gca().xaxis.set_major_locator(MultipleLocator(slot_length))
plt.gca().xaxis.set_minor_locator(MultipleLocator(1))

plt.grid()
plt.grid(True, 'minor', color='#ddddee') # use a lighter color

plt.show()
# %%

# %%
