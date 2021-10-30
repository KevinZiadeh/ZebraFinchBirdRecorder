from matplotlib import pyplot as plt

with open('./res/matlab_analysis_data.csv') as f:
# with open('./res/sd_analysis_data.txt') as f:
    y = f.read()[:-1].split()
    y_int_sd = [int(e) for e in y]

with open('./res/sd_analysis_data.txt') as f:
# with open('./res/cpp_analysis_data.txt') as f:
    y = f.read()[:-1].split()
    y_int_cpp = [int(e) for e in y]

with open('./res/matlab_time_data.csv') as f:
# with open('./res/sd_time_data.txt') as f:
    x = f.read()[:-1].split()
    x_val_sd = [float(e) for e in x]

with open('./res/sd_time_data.txt') as f:
# with open('./res/cpp_time_data.txt') as f:
    x = f.read()[:-1].split()
    x_val_cpp = [float(e) for e in x]

plt.plot(x_val_sd, y_int_sd, color='red')
plt.plot(x_val_cpp, y_int_cpp, color='green', linestyle='dashed')
plt.show()