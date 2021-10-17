from matplotlib import pyplot as plt

with open('./sd_analysis_data.txt') as f:
    y = f.read()[:-1].split()
    y_int_sd = [int(e) for e in y]

with open('./cpp_analysis_data.txt') as f:
    y = f.read()[:-1].split()
    y_int_cpp = [int(e) for e in y]

with open('./sd_time_data.txt') as f:
    x = f.read()[:-1].split()
    x_val_sd = [float(e) for e in x]

with open('./cpp_time_data.txt') as f:
    x = f.read()[:-1].split()
    x_val_cpp = [float(e) for e in x]

plt.plot(x_val_cpp, y_int_cpp)
plt.plot(x_val_sd, y_int_sd)
plt.show()