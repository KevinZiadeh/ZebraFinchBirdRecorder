from matplotlib import pyplot as plt

with open('./cpp_analysis_data.txt') as f:
    y = f.read()[:-1].split()
    y_int = [int(e) for e in y]
with open('./cpp_time_data.txt') as f:
    x = f.read()[:-1].split()
    x_val = [float(e) for e in x]


print(x_val)
print(y_int)

plt.plot(x_val, y_int)
plt.show()