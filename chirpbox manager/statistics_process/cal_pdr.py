
import numpy as np
import matplotlib.pyplot as plt

real_data = [67, 83, 75, 73, 76, 78, 72, 68, 73, 73, 69, 84, 69, 70, 78, 77, 85, 79, 80, 83, 79]
rx_list = [57, 77, 71, 66, 68, 32, 66, 62, 67, 59, 64, 75, 43, 62, 75, 70, 71, 75, 75, 70, 74]

# real_data = [89, 1, 2, 3, 89, 91, 1, 2, 3, 89, 1, 90, 89, 89, 1, 2, 90, 1, 2, 89, 1]
# rx_list = [0, 0, 0, 0, 85, 79, 0, 0, 0, 87, 0, 64, 85, 80, 0, 0, 84, 0, 0, 83, 0]
x = np.arange(0, 21, 1)
print(x)
pdr_list = []
for i in range(len(real_data)):
    pdr_list.append((rx_list[i] / real_data[i]) * 100)
print(pdr_list)

plt.plot(x, pdr_list, linestyle='None', marker='o', markersize = 10)
ax = plt.gca()
ax.set_ylim(5,100)
plt.show()

# 4, 5, 9, 11, 12, 13, 16, 19