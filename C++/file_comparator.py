precision = 0.0000001

sd_firfiltered_signal = []
with open('./res/sd_firfiltered_data.txt', 'r') as f:
    for line in f:
        sd_firfiltered_signal.append(float(line))

cpp_firfiltered_signal = []
with open('./res/cpp_firfiltered_signal.txt', 'r') as f:
    for line in f:
        cpp_firfiltered_signal.append(float(line))

matlab_firfiltered_signal = []
with open('./res/matlab_firfiltered_data.csv', 'r') as f:
    for line in f:
        matlab_firfiltered_signal.append(float(line))

cpp_notched_signal = []
with open('./res/cpp_notched_signal.txt', 'r') as f:
    for line in f:
        cpp_notched_signal.append(float(line))

matlab_notched_signal = []
with open('./res/matlab_notched_signal.csv', 'r') as f:
    for line in f:
        matlab_notched_signal.append(float(line))

cpp_reference_signal = []
with open('./res/cpp_reference_signal.txt', 'r') as f:
    for line in f:
        cpp_reference_signal.append(float(line))

matlab_reference_signal = []
with open('./res/matlab_reference_signal.csv', 'r') as f:
    for line in f:
        matlab_reference_signal.append(float(line))

def check_equality(arr1, arr2, precision):
    not_equal = 0
    length = min(len(arr1), len(arr2))
    index = -1
    for i in range(length):
        # not_equal += int((arr1[i] - arr2[i])>=precision)
        # print(str(arr1[i]) + ' -- ' + str(arr2[i]) + ' ->> ' + str((arr1[i] - arr2[i])<precision))
        if not abs(arr1[i] - arr2[i])<precision:
            # print(f"position {i} values are {arr1[i]} and {arr2[i]}" )
            index = i if index < 0 else index
            not_equal += 1
    return f"The number of elements at precision {precision} that are not equal that is {not_equal} -> first index {index}"

print("sd matlab firfiltered -> " + check_equality(sd_firfiltered_signal, matlab_firfiltered_signal, precision))
print("cpp matlab firfiltered -> " + check_equality(cpp_firfiltered_signal, matlab_firfiltered_signal, precision))
print("sd cpp firfiltered -> " + check_equality(sd_firfiltered_signal, cpp_firfiltered_signal, precision))
print("cpp matlab notched -> " + check_equality(cpp_notched_signal, matlab_notched_signal, precision))
print("cpp matlab reference -> " + check_equality(cpp_reference_signal, matlab_reference_signal, precision))