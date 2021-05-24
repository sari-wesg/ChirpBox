import statistics
list1 = [7,1,5,5,50,0,20,0,53.844877,0,0,0,0,5903.975432,6135.201395,1429468,32154521,155920,20104968,0,5534.47735,1]
list2 = [7,1,5,5,50,0,20,62,237.207114,0,0,0,0,35034.08463,33417.85596,9322613,137943919,26267664,117517795,953125,31512.23305,3.45]
# print(len(list1))
energy = list1[20] * list1[21] + list2[20] * list2[21]
time = list1[8] * list1[21] + list2[8] * list2[21]
tx_time = list1[17] * list1[21] + list2[17] * list2[21]
energy_real_2 = list1[13] * list1[21] + list2[13] * list2[21]
energy_real_4 = list1[14] * list1[21] + list2[14] * list2[21]
print(energy,int(time))
print(energy_real_2)
print(energy_real_4)
print(tx_time/1e6)

110125.96662300001,110125.96662300001,0,0,0,121820.0263956,110525.8574578
