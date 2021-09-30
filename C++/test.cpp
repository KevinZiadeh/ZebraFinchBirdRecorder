#include <vector>
#include <iostream>
#include "noise_filter.h"
#include "csv_reader.h"
#include <string>
#include <chrono>
#include <thread>

using namespace std;

#define BUFFER_SIZE 4096

float* test_filter(float* raw_signal, long size);
void compare_matlab(vector<float> vectored_signal,float* filtered_signal, int size);

int main(){
    float* audioBuffer1 = (float *)malloc(sizeof(float)*BUFFER_SIZE);
    float* audioBuffer2 = (float *)malloc(sizeof(float)*BUFFER_SIZE);

    int buffer1_head = 0;
    int buffer2_head = 0;
    int buffer1_tail = BUFFER_SIZE;
    int buffer2_tail = BUFFER_SIZE;

    vector<float> data = read_csv("data.csv");
    float* raw_signal = (float *)malloc(sizeof(float)*data.size());
    for (long i =0; i<data.size();i++){
        raw_signal[i] = data[i];
    }
    long size = data.size();


    float* filtered_signal = test_filter(raw_signal, size);
    this_thread::sleep_for(1s);
    compare_matlab(data, filtered_signal, size);

    return 0;
}

float* test_filter(float* raw_signal, long size){
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    float* filtered_signal = filter_signal(raw_signal, size);
    std::chrono::steady_clock::time_point end1 = std::chrono::steady_clock::now();
    std::cout << "Filtering function took " << std::chrono::duration_cast<std::chrono::microseconds>(end1 - begin).count() << "muS" << std::endl;
    return filtered_signal;
}

void compare_matlab(vector<float> vectored_signal,float* filtered_signal, int size){

    for (int i =0;i<size;i++){
        cout << "Volt: " << vectored_signal[i] << " - FIR: " << filtered_signal[i] << endl;
    }

}