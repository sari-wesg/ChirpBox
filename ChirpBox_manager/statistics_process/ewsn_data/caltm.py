list_1 = [9.070526,9.18446,9.10344827,9.0801896,9.14627,9.117853]
list_2 = [950,959,957,949,963,957]
list1= []
for i in range(len(list_1)):
    list1.append(list_1[i] * list_2[i]/1e2)
print(list1)