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
#define print_point(point, i_size) for(int i=0;i<min(size, 100);i++) {cout << setprecision(9) << point[i] << " ";} cout << endl;
#define BUFFER_SIZE 128

double* dp_sdCard = (double *)malloc(sizeof(double)*262144);
int i_sdCardIndex = 0;
double d_filteredPrev1 = 0;
double d_filteredPrev2 = 0;
double d_rawPrev1 = 0;
double d_rawPrev2 = 0;

double* test_filter(double* dp_rawSignal, int i_size);
void compare_matlab(double* dp_filteredSignal, int i_size);
void ADCReader(double* dp_rawSignal,  double* dp_audioBuffer, int i_buffer1Head, 
                int i_buffer2Head, int i_buffer1Tail, int i_buffer2Tail, int i_size);
double* Filter(double* buffer, int head, int i_size);

int main(){
    double* dp_audioBuffer = (double *)malloc(sizeof(double)*BUFFER_SIZE);

    int i_buffer1Head = 0;
    int i_buffer2Head = 0.2*BUFFER_SIZE;
    int i_buffer1Tail = 0.8*BUFFER_SIZE;
    int i_buffer2Tail = BUFFER_SIZE;

    vector<double> data = read_csv("data.csv", 1);
    double* dp_rawSignal = (double *)malloc(sizeof(double)*data.size());
    for (long i =0; i<data.size();i++){
        dp_rawSignal[i] = data[i];
    }
    int i_size = data.size();

    // double* dp_filteredSignal = test_filter(dp_rawSignal, i_size);
    // this_thread::sleep_for(1s);
    // compare_matlab(data, dp_filteredSignal, i_size);


    cout << endl << endl << "Main thread " << this_thread::get_id() << endl;
    this_thread::sleep_for(2s);
    future<void> readerThread = async(launch::async, ADCReader, dp_rawSignal, dp_audioBuffer,  i_buffer1Head, 
                i_buffer2Head, i_buffer1Tail, i_buffer2Tail, i_size);

    return 0;
}

double* test_filter(double* dp_rawSignal, int i_size){
    this_thread::sleep_for(1s);
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    double* dp_filteredSignal = filter_signal(dp_rawSignal, i_size, d_filteredPrev1, d_filteredPrev2);
    chrono::steady_clock::time_point end1 = chrono::steady_clock::now();
    cout << "Filtering function took " << chrono::duration_cast<chrono::microseconds>(end1 - begin).count() << "muS" << endl;
    return dp_filteredSignal;
}

void compare_matlab(double* dp_filteredSignal, int i_size){
    vector<double> vd_matlabFilteredSignal = read_csv("matlab_firfiltered.csv", 0);
    double d_precision = 0.0000001;
    for (int i =0;i<min(i_size,1000);i++){
        cout << i << " - C++ FIR: " << dp_filteredSignal[i] 
        << " - MATLAB FIR: " << vd_matlabFilteredSignal[i] << " - Equal? " 
        << (abs(dp_filteredSignal[i]-vd_matlabFilteredSignal[i]<d_precision) ? "True" : "False") << endl;
    }
}

void ADCReader(double* dp_rawSignal,  double* dp_audioBuffer, int i_buffer1Head, 
                int i_buffer2Head, int i_buffer1Tail, int i_buffer2Tail, int i_size)
{
    int i_head = -1;
    int i_iter = 0;
    int i_singleBufferSize = 0.8*BUFFER_SIZE;
    cout << "ADC Reader thread " << this_thread::get_id() << endl;
    // can also do i%(BUFFER_SIZE-1)
    for(int i = 0; i < i_size; i++){
        dp_audioBuffer[i_iter] = dp_rawSignal[i];
        if (i_iter == i_buffer1Tail){
            cout << "Buffer 1 full. Send to filtering" << endl;
            i_head = i_buffer1Head;
            i_buffer1Head = ((int)(i_buffer1Head+0.4*BUFFER_SIZE)%BUFFER_SIZE);
            i_buffer1Tail = ((int)(i_buffer1Tail+0.4*BUFFER_SIZE)%BUFFER_SIZE);
            future<double*> filterThread = async(launch::deferred, Filter, dp_audioBuffer, i_head, i_singleBufferSize);
            double* filteredBuffer = filterThread.get(); 
        }
        if (i_iter == i_buffer2Tail){
            cout << "Buffer 2 full. Send to filtering" << endl;
            i_head = i_buffer2Head;
            i_buffer2Head = ((int)(i_buffer2Head+0.4*BUFFER_SIZE)%BUFFER_SIZE);
            i_buffer2Tail = ((int)(i_buffer2Tail+0.4*BUFFER_SIZE)%BUFFER_SIZE);
            future<double*> filterThread = async(launch::deferred, Filter, dp_audioBuffer, i_head, i_singleBufferSize);
            double* filteredBuffer = filterThread.get(); 
        }
        i_iter++;
        i_iter = i_iter%(BUFFER_SIZE+1);
        if (i == 1000) break;
        this_thread::sleep_for(0.0005s);    
    }
    // print_point(sd_card, 35);
    compare_matlab(dp_sdCard, 262144);
}

double* Filter(double* buffer, int head, int i_size)
{
    // cout << "Filter thread " << this_thread::get_id() << " with head " << head << endl;
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    double* buffer_signal = (double *)malloc(sizeof(double)*i_size);
    // print_point(buffer, BUFFER_SIZE);
    for (int i=0; i<i_size;i++){
        buffer_signal[i] = buffer[(head+i)%(BUFFER_SIZE+1)];
    }
    // cout << head << endl;
    // print_point(buffer_signal, i_size);
    double* dp_filteredSignal = filter_signal(buffer_signal, i_size, d_filteredPrev1, d_filteredPrev2);
    chrono::steady_clock::time_point end1 = chrono::steady_clock::now();
    // cout << "Filtering function took " << chrono::duration_cast<chrono::microseconds>(end1 - begin).count() << "muS" << endl;
    // send to analysis
    // add to sd card
    d_rawPrev1 = buffer[i_size-2];
    d_rawPrev1 = buffer[i_size-1];
    d_filteredPrev1 = dp_filteredSignal[i_size-2];
    d_filteredPrev2 = dp_filteredSignal[i_size-1];

    for(int i=0;i<i_size;i++) {
        dp_sdCard[i_sdCardIndex] = dp_filteredSignal[i];
        i_sdCardIndex++;
    }
    return dp_filteredSignal;
}