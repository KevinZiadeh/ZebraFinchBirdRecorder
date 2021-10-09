#include <vector>
#include <iostream>
#include "noise_filter.h"
#include "csv_reader.h"
#include <string>
#include <thread>
#include <future>         // std::async, std::future
#include <iomanip>      // std::setprecision

using namespace std;

// Definitions used for testing purposes and easy printing of arrays
#define print_vec(vec) for(auto e: vec) {cout << e << " ";} cout << endl;
#define print_point(point, size) for(int i=0;i<min(size, 100);i++) {cout << setprecision(9) << point[i] << " ";} cout << endl;

// Complete buffer size (combination of buffer 1 + buffer 2) -> 0.8*BUFFER_SIZE == Buffer1 -> 1 second
#define BUFFER_SIZE 5120

// Headers of defined functions
double* test_filter(double* dp_rawSignal, int i_size);
void compare_matlab(double* dp_filteredSignal, int i_size);
void ADCReader(double* dp_rawSignal,  double* dp_audioBuffer, int i_buffer1Head, 
                int i_buffer2Head, int i_buffer1Tail, int i_buffer2Tail, int i_CompleteDataSize);
double* Filter(double* buffer, int head, int i_size);

// Global variables - might need to be initiliazed elsewhere
double* dp_sdCard = (double *)malloc(sizeof(double)*262144); // simulates what SD card will look like
int i_sdCardIndex = 0; // index of the sd card save location
double d_filteredPrev1 = 0; // before last element of previously filtered buffer
double d_filteredPrev2 = 0; // last element of previously filtered buffer
double u16_filteredPrev1 = 0; // before last element of previously filtered buffer
double u16_filteredPrev2 = 0; // last element of previously filtered buffer
double d_rawPrev1 = 0; // before last element of previous buffer before filtering
double d_rawPrev2 = 0; // last element of previous buffer before filtering
bool b_mergeBuffer = false; // selects if we need to merge current buffer with previous one


/**
 * @brief Main function that initiliazes everything
 * 
 * @return  
 */
int main(){
    double* dp_audioBuffer = (double *)malloc(sizeof(double)*BUFFER_SIZE); // Complete buffer allocated for double values 
    int i_buffer1Head = 0; // integer indicating the head of the first buffer
    int i_buffer2Head = 0.2*BUFFER_SIZE; // integer indicating the head of the second buffer
    int i_buffer1Tail = 0.8*BUFFER_SIZE; // integer indicating the tail of the first buffer
    int i_buffer2Tail = BUFFER_SIZE; // integer indicating the tail of the second buffer

    vector<double> vd_data = read_csv("data.csv", 1); // vector containing all the testing data  
    double* dp_rawSignal = (double *)malloc(sizeof(double)*vd_data.size()); // array containing all the testing data. This is done for compatibility 
    // populating the rawSignal array
    for (long i =0; i<vd_data.size();i++){
        dp_rawSignal[i] = vd_data[i];
    }
    int i_CompleteDataSize = vd_data.size(); // total number of testing data points

    // launch thread that reads data asynchronously continuously 
    future<void> readerThread = async(launch::async, ADCReader, dp_rawSignal, dp_audioBuffer,  i_buffer1Head, 
                i_buffer2Head, i_buffer1Tail, i_buffer2Tail, i_CompleteDataSize);
    return 0;
}

/**
 * @brief function that tests the filter implementation
 * 
 * @param dp_rawSignal array that indicates the raw signal for filtering
 * @param i_size size of the array
 * @return double* to the filtered array
 */
double* test_filter(double* dp_rawSignal, int i_size){
    this_thread::sleep_for(1s);
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    double* dp_filteredSignal = filter_signal(dp_rawSignal, i_size, 0, 0);
    chrono::steady_clock::time_point end1 = chrono::steady_clock::now();
    cout << "Filtering function took " << chrono::duration_cast<chrono::microseconds>(end1 - begin).count() << "muS" << endl;
    return dp_filteredSignal;
}

/**
 * @brief compares the filtered array to the correct values extracted from MATLAB -> prints a comparision eleemnt by element
 * 
 * @param dp_filteredSignal array that contains the values to compare with MATLAB
 * @param i_size size of comparision points -> shouldn't exceed max size
 */
void compare_matlab(double* dp_filteredSignal, int i_size){
    vector<double> vd_matlabFilteredSignal = read_csv("matlab_firfiltered.csv", 0);
    double d_precision = 0.0000001;
    for (int i =0;i<i_size;i++){
        cout << i << " - C++ FIR: " << dp_filteredSignal[i] 
        << " - MATLAB FIR: " << vd_matlabFilteredSignal[i] << " - Equal? " 
        << (abs(dp_filteredSignal[i]-vd_matlabFilteredSignal[i])<d_precision ? "True" : "False") << endl;
    }
}

/**
 * @brief compares the filtered array to the correct values extracted from MATLAB -> prints a comparision eleemnt by element
 * 
 * @param dp_rawSignal array that indicates the complete raw signal for filtering 
 * @param dp_sdCard array that indicates the values saved in the SD card
 * @param i_size size of comparision points -> shouldn't exceed max size
 */
void compare_buffer_nobuffer(double* dp_rawSignal, double* dp_sdCard, int i_size){
    double* dp_filteredSignal = test_filter(dp_rawSignal, i_size);
    double d_precision = 0.0000001;
    for (int i =400;i<i_size;i++){
        cout << i << " - C++ Complete: " << dp_filteredSignal[i] 
        << " - C++ Buffer: " << dp_sdCard[i] << " - Equal? " 
        << (abs(dp_filteredSignal[i]-dp_sdCard[i])<d_precision ? "True" : "False") << endl;
    }
}

/**
 * @brief simulates the continuous reading from the ADC
 * 
 * @param dp_rawSignal array that indicates the complete raw signal for filtering
 * @param dp_audioBuffer array that indicates the complete buffer allocated for double values
 * @param i_buffer1Head head of the first buffer
 * @param i_buffer2Head head of the second buffer
 * @param i_buffer1Tail tail of the first buffer
 * @param i_buffer2Tail tail of the second buffer
 * @param i_CompleteDataSize total number of testing data points
 */
void ADCReader(double* dp_rawSignal,  double* dp_audioBuffer, int i_buffer1Head, int i_buffer2Head, int i_buffer1Tail, int i_buffer2Tail, int i_CompleteDataSize)
{
    int i_head = -1; // helper integer to specify head of buffer for filtering
    int i_iter = 0; // iterator for the index of the buffer
    const int i_singleBufferSize = 0.8*BUFFER_SIZE; // size of each buffer, whose combination and overlap is BUFFER_SIZE
    // Reads data sequentially from array containing all the data -> simulates reading from ADC
    // We lose a maximum of 0.2*BUFFER_SIZE of data at the end
    for(int i = 0; i < i_CompleteDataSize; i++){
        dp_audioBuffer[i_iter] = dp_rawSignal[i];
        if (i_iter == i_buffer1Tail){ // when buffer 1 is full
            i_head = i_buffer1Head;
            i_buffer1Head = ((int)(i_buffer1Head+0.4*BUFFER_SIZE)%(BUFFER_SIZE+1)); // changes head position on the first buffer to start saving 50% later 
            i_buffer1Tail = ((int)(i_buffer1Tail+0.4*BUFFER_SIZE)%BUFFER_SIZE); // changes tail position on the first buffer to end 50% later
            future<double*> filterThread = async(launch::deferred, Filter, dp_audioBuffer, i_head, i_singleBufferSize); // launch thread that filters the buffer 
            double* filteredBuffer = filterThread.get(); 
        }
        if (i_iter == i_buffer2Tail){ // when buffer 2 is full
            i_head = i_buffer2Head;
            i_buffer2Head = ((int)(i_buffer2Head+0.4*BUFFER_SIZE)%(BUFFER_SIZE+1)); // changes head position on the second buffer to start saving 50% later
            i_buffer2Tail = ((int)(i_buffer2Tail+0.4*BUFFER_SIZE)%BUFFER_SIZE); // changes tail position on the second buffer to end 50% later
            future<double*> filterThread = async(launch::deferred, Filter, dp_audioBuffer, i_head, i_singleBufferSize); // launch thread that filters the buffer
            double* filteredBuffer = filterThread.get(); 
        }
        i_iter++;
        i_iter = i_iter%(BUFFER_SIZE+1);
        this_thread::sleep_for(0.0005s);    
    }
}

/**
 * @brief simulated the filtering, analysis and adding to the SD card
 * 
 * @param dp_buffer buffer that mimicks a circular queue to be filtered and analyzed
 * @param i_head beginning position to read from the buffer 
 * @param i_singleBufferSize size of a single buffer - proportional to BUFFER_SIZE
 * @return double* to the filtered buffer 
 */
double* Filter(double* dp_buffer, int i_head, int i_singleBufferSize){
    double* dp_bufferSignal = (double *)malloc(sizeof(double)*i_singleBufferSize); // buffer to be analyzed
    // populate correct values starting from specified and looping back as if circular queue 
    for (int i=0; i<i_singleBufferSize;i++){
        dp_bufferSignal[i] = dp_buffer[(i_head+i)%(BUFFER_SIZE+1)]; // implements circular queue
    }
    double* dp_filteredSignal = filter_signal(dp_bufferSignal, i_singleBufferSize, d_filteredPrev1, d_filteredPrev2); // filtered signal
    d_filteredPrev1 = dp_filteredSignal[i_singleBufferSize-2];
    d_filteredPrev2 = dp_filteredSignal[i_singleBufferSize-1];

    // add to sd card will be done after setting merge buffer by analysis, 
    // but for testing purposes, we want to save the complete first buffer
    // then start appending
    int i_startCopyIndex = (int)((b_mergeBuffer*0.6*BUFFER_SIZE+0.5));
    for(int i=i_startCopyIndex;i<i_singleBufferSize;i++) {
        dp_sdCard[i_sdCardIndex] = dp_filteredSignal[i];
        i_sdCardIndex++;
    }
    b_mergeBuffer = true;

    return dp_filteredSignal;
}