#include <vector>
#include <iostream>
#include "noise_filter.h"
#include "csv_reader.h"
#include <string>
#include <chrono>
#include <thread>

using namespace std;

#define print_vec(vec) for(auto e: vec) {cout << e << " ";} cout << endl;
#define BUFFER_SIZE 4096

double* test_filter(double* raw_signal, long size);
void compare_matlab(vector<double> vectored_signal,double* filtered_signal, int size);

int main(){
    double* audioBuffer1 = (double *)malloc(sizeof(double)*BUFFER_SIZE);
    double* audioBuffer2 = (double *)malloc(sizeof(double)*BUFFER_SIZE);

    int buffer1_head = 0;
    int buffer2_head = 0;
    int buffer1_tail = BUFFER_SIZE;
    int buffer2_tail = BUFFER_SIZE;

    vector<double> data = read_csv("data.csv", 1);
    double* raw_signal = (double *)malloc(sizeof(double)*data.size());
    for (long i =0; i<data.size();i++){
        raw_signal[i] = data[i];
    }
    int size = data.size();

    double* filtered_signal = test_filter(raw_signal, size);
    this_thread::sleep_for(1s);
    compare_matlab(data, filtered_signal, size);

    return 0;
}

double* test_filter(double* raw_signal, long size){
    this_thread::sleep_for(1s);
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    double* filtered_signal = filter_signal(raw_signal, size);
    chrono::steady_clock::time_point end1 = chrono::steady_clock::now();
    cout << "Filtering function took " << chrono::duration_cast<chrono::microseconds>(end1 - begin).count() << "muS" << endl;
    return filtered_signal;
}

void compare_matlab(vector<double> vectored_signal, double* filtered_signal, int size){

    vector<double> matlab_filtered_signal = read_csv("matlab_firfiltered.csv", 0);
    double precision = 0.0000000001;
    for (int i =0;i<size;i++){
        cout << "Volt: " << vectored_signal[i] << " - C++ FIR: " << filtered_signal[i] 
        << " - MATLAB FIR: " << matlab_filtered_signal[i] << " - Equal? " 
        << (abs(filtered_signal[i]-matlab_filtered_signal[i]<precision) ? "True" : "False") << endl;
    }

}