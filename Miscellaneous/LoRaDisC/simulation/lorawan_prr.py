import pandas as pd
import statistics

df = pd.read_csv("C:\\Users\\tecop\\Desktop\\prr_lorawan.csv")
Node_num = 21
item_num = 10
sf_prr_result = []
for k in range(len([7, 8, 9, 10, 11, 12])):
    prr_result = []
    for i in range(Node_num):
        list_prr = df.iloc[:,[i]].values.tolist()
        list_prr = sum(list_prr, [])[k*item_num:k*item_num+item_num]
        prr_result.append(statistics.mean(list_prr))
    sf_prr_result.append(prr_result)
print(sf_prr_result)


