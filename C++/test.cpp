#include <vector>
#include <iostream>
#include "noise_filter.h"
#include "csv_reader.h"
#include <string>
#include <chrono>
#include <thread>
#include <future>         // std::async, std::future

using namespace std;

#define print_vec(vec) for(auto e: vec) {cout << e << " ";} cout << endl;
#define print_point(point, size) for(int i=0;i<min(size, 100);i++) {cout << point[i] << " ";} cout << endl;
#define BUFFER_SIZE 10

double* test_filter(double* raw_signal, long size);
void compare_matlab(vector<double> vectored_signal,double* filtered_signal, int size);
void ADCReader(double* raw_signal,  double* audioBuffer1, double* audioBuffer2, int buffer1_head, 
                int buffer2_head, int buffer1_tail, int buffer2_tail, long size);
double* Filter(double* buffer, long size);

int main(){
    double* audioBuffer1 = (double *)malloc(sizeof(double)*BUFFER_SIZE);
    double* audioBuffer2 = (double *)malloc(sizeof(double)*BUFFER_SIZE);

    int buffer1_head = 0;
    int buffer2_head = -(BUFFER_SIZE/4);
    int buffer1_tail = BUFFER_SIZE;
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
    future<void> readerThread = async(launch::async, ADCReader, raw_signal, audioBuffer1, audioBuffer2,  buffer1_head, 
                buffer2_head, buffer1_tail, buffer2_tail, size);

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
    double precision = 0.0000001;
    for (int i =0;i<min(size,1000);i++){
        cout << "Volt: " << vectored_signal[i] << " - C++ FIR: " << filtered_signal[i] 
        << " - MATLAB FIR: " << matlab_filtered_signal[i] << " - Equal? " 
        << (abs(filtered_signal[i]-matlab_filtered_signal[i]<precision) ? "True" : "False") << endl;
    }
}

void ADCReader(double* raw_signal,  double* audioBuffer1, double* audioBuffer2, int buffer1_head, 
                int buffer2_head, int buffer1_tail, int buffer2_tail, long size)
{
    bool swap = false;
    future<double*> filterThread1 = async(launch::deferred, Filter, audioBuffer1, size);
    future<double*> filterThread2 = async(launch::deferred, Filter, audioBuffer2, size);
    cout << "ADC Reader thread " << this_thread::get_id() << endl;
    for(int i = 0; i < size; i++){
        cout << i << "  " << size << "  " << buffer1_head << "  " << buffer2_head << endl;
        audioBuffer1[max(buffer1_head, 0)] = raw_signal[i];
        buffer1_head++;
        audioBuffer2[max(buffer2_head, 0)] = raw_signal[i];
        buffer2_head++;
        if (buffer1_head == BUFFER_SIZE){
            cout << "Buffer 1 full. Send to filtering" << endl;
            buffer1_head = -(BUFFER_SIZE/4); //add the delay of 0.25s
            double* filteredBuffer = filterThread1.get(); 
            print_point(filteredBuffer, 10)
        }
        if (buffer2_head == BUFFER_SIZE){
            cout << "Buffer 2 full. Send to filtering" << endl;
            buffer2_head = -(BUFFER_SIZE/4); //add the delay of 0.25s
            double* filteredBuffer = filterThread2.get(); 
            print_point(filteredBuffer, 10)
        }
        this_thread::sleep_for(0.0005s);    
    }
}

double* Filter(double* buffer, long size)
{
    cout << "Filter thread " << this_thread::get_id() << endl;
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    double* filtered_signal = filter_signal(buffer, size);
    chrono::steady_clock::time_point end1 = chrono::steady_clock::now();
    cout << "Filtering function took " << chrono::duration_cast<chrono::microseconds>(end1 - begin).count() << "muS" << endl;
    // send to analysis
    // add to sd card
    return filtered_signal;
}