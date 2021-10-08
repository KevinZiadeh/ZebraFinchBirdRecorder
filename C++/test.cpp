#include <vector>
#include <iostream>
#include "noise_filter.h"
#include "csv_reader.h"
#include <string>
#include <chrono>
#include <thread>
#include <future>         // std::async, std::future
#include <iomanip>      // std::setprecision

using namespace std;

#define print_vec(vec) for(auto e: vec) {cout << e << " ";} cout << endl;
#define print_point(point, size) for(int i=0;i<min(size, 100);i++) {cout << setprecision(9) << point[i] << " ";} cout << endl;
#define BUFFER_SIZE 64

double* sd_card = (double *)malloc(sizeof(double)*262144);
int sd_card_index = 0;
double prev1 = 0;
double prev2 = 0;

double* test_filter(double* raw_signal, long size);
void compare_matlab(double* filtered_signal, int size);
// void compare_matlab(vector<double> vectored_signal,double* filtered_signal, int size);
void ADCReader(double* raw_signal,  double* audioBuffer, int buffer1_head, 
                int buffer2_head, int buffer1_tail, int buffer2_tail, long size);
double* Filter(double* buffer, int head, int size);

int main(){
    double* audioBuffer = (double *)malloc(sizeof(double)*BUFFER_SIZE);

    int buffer1_head = 0;
    int buffer2_head = 0.2*BUFFER_SIZE;
    int buffer1_tail = 0.8*BUFFER_SIZE;
    int buffer2_tail = BUFFER_SIZE;

    vector<double> data = read_csv("data.csv", 1);
    double* raw_signal = (double *)malloc(sizeof(double)*data.size());
    for (long i =0; i<data.size();i++){
        raw_signal[i] = data[i];
    }
    int size = data.size();

    // double* filtered_signal = test_filter(raw_signal, size);
    // this_thread::sleep_for(1s);
    // compare_matlab(data, filtered_signal, size);


    cout << endl << endl << "Main thread " << this_thread::get_id() << endl;
    this_thread::sleep_for(2s);
    future<void> readerThread = async(launch::async, ADCReader, raw_signal, audioBuffer,  buffer1_head, 
                buffer2_head, buffer1_tail, buffer2_tail, size);

    return 0;
}

double* test_filter(double* raw_signal, long size){
    this_thread::sleep_for(1s);
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    double* filtered_signal = filter_signal(raw_signal, size, prev1, prev2);
    chrono::steady_clock::time_point end1 = chrono::steady_clock::now();
    cout << "Filtering function took " << chrono::duration_cast<chrono::microseconds>(end1 - begin).count() << "muS" << endl;
    return filtered_signal;
}

void compare_matlab(double* filtered_signal, int size){
// void compare_matlab(vector<double> vectored_signal, double* filtered_signal, int size){
    vector<double> matlab_filtered_signal = read_csv("matlab_firfiltered.csv", 0);
    double precision = 0.0000001;
    for (int i =0;i<min(size,1000);i++){
        cout << i << " - C++ FIR: " << filtered_signal[i] 
        << " - MATLAB FIR: " << matlab_filtered_signal[i] << " - Equal? " 
        << (abs(filtered_signal[i]-matlab_filtered_signal[i]<precision) ? "True" : "False") << endl;
    }
}

void ADCReader(double* raw_signal,  double* audioBuffer, int buffer1_head, 
                int buffer2_head, int buffer1_tail, int buffer2_tail, long size)
{
    bool swap = false;
    int head = -1;
    int iter = 0;
    int single_buffer_size = 0.8*BUFFER_SIZE;
    cout << "ADC Reader thread " << this_thread::get_id() << endl;
    // can also do i%(BUFFER_SIZE-1)
    for(int i = 0; i < size; i++){
        audioBuffer[iter] = raw_signal[i];
        if (iter == buffer1_tail){
            cout << "Buffer 1 full. Send to filtering" << endl;
            head = buffer1_head;
            buffer1_head = ((int)(buffer1_head+0.4*BUFFER_SIZE)%BUFFER_SIZE);
            buffer1_tail = ((int)(buffer1_tail+0.4*BUFFER_SIZE)%BUFFER_SIZE);
            future<double*> filterThread = async(launch::deferred, Filter, audioBuffer, head, single_buffer_size);
            double* filteredBuffer = filterThread.get(); 
        }
        if (iter == buffer2_tail){
            cout << "Buffer 2 full. Send to filtering" << endl;
            head = buffer2_head;
            buffer2_head = ((int)(buffer2_head+0.4*BUFFER_SIZE)%BUFFER_SIZE);
            buffer2_tail = ((int)(buffer2_tail+0.4*BUFFER_SIZE)%BUFFER_SIZE);
            future<double*> filterThread = async(launch::deferred, Filter, audioBuffer, head, single_buffer_size);
            double* filteredBuffer = filterThread.get(); 
        }
        iter++;
        iter = iter%(BUFFER_SIZE+1);
        if (i == 1000) break;
        this_thread::sleep_for(0.0005s);    
    }
    // print_point(sd_card, 35);
    compare_matlab(sd_card, 262144);
}

double* Filter(double* buffer, int head, int size)
{
    // cout << "Filter thread " << this_thread::get_id() << " with head " << head << endl;
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    double* buffer_signal = (double *)malloc(sizeof(double)*size);
    // print_point(buffer, BUFFER_SIZE);
    for (int i=0; i<size;i++){
        buffer_signal[i] = buffer[(head+i)%(BUFFER_SIZE+1)];
    }
    // cout << head << endl;
    // print_point(buffer_signal, size);
    double* filtered_signal = filter_signal(buffer_signal, size, prev1, prev2);
    chrono::steady_clock::time_point end1 = chrono::steady_clock::now();
    // cout << "Filtering function took " << chrono::duration_cast<chrono::microseconds>(end1 - begin).count() << "muS" << endl;
    // send to analysis
    // add to sd card
    prev1 = filtered_signal[size-2];
    prev2 = filtered_signal[size-1];

    for(int i=0;i<size;i++) {
        sd_card[sd_card_index] = filtered_signal[i];
        sd_card_index++;
    }
    return filtered_signal;
}