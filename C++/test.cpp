#include <vector>
#include <iostream>
#include "signal_processing.h"
#include "csv_reader.h"
#include <string>
#include <thread>
#include <future>         // std::async, std::future
#include <iomanip>      // std::setprecision
#include<fstream>

using namespace std;

// Definitions used for testing purposes and easy printing of arrays
#define print_vec(vec) for(auto e: vec) {cout << e << " ";} cout << endl;
#define print_point(point, size) for(int i=0;i<min(size, 100);i++) {cout << setprecision(9) << point[i] << " ";} cout << endl;

// Complete buffer size (combination of buffer 1 + buffer 2) -> 0.8*COMPLETE_BUFFER_SIZE == Buffer1 -> 1 second
#define COMPLETE_BUFFER_SIZE 2500
#define SINGLE_BUFFER_SIZE int(0.8*COMPLETE_BUFFER_SIZE)
#define THRESHOLD 7

// Headers of defined functions
double* test_filter(double* dp_rawSignal, int i_size);
void compare_matlab(double* dp_filteredSignal, int i_size);
void test_analysis(double* dp_rawSignal, int i_size);
void generate_sdCardPlotData(int i_size);
void ADCReader(double* dp_rawSignal,  double* dp_audioBuffer, int i_buffer1Head, 
                int i_buffer2Head, int i_buffer1Tail, int i_buffer2Tail, int i_CompleteDataSize);
int Filter(double* buffer, int head, int i_generalLocation);

// Global variables - might need to be initiliazed elsewhere
vector<vector<double>> vvd_sdCard; // 2D array simulates what SD card will look like: first element of each array is the index in the general signal (timestamp)
int i_sdCardIndex = 0; // index of the sd card save location
double d_filteredPrev1 = 0; // before last element of previously filtered buffer
double d_filteredPrev2 = 0; // last element of previously filtered buffer
double u16_filteredPrev1 = 0; // before last element of previously filtered buffer
double u16_filteredPrev2 = 0; // last element of previously filtered buffer
double d_rawPrev1 = 0; // before last element of previous buffer before filtering
double d_rawPrev2 = 0; // last element of previous buffer before filtering
int i_vocalisationDetected = 0; // result of the analysis to decide if we want to save or not 
int i_mergeState = 0; // selects if we need to merge current buffer with previous one: THREE STATES -> YES, YES/NO, NO. This is done in order to save instances together when there is only one buffer of difference of no
int i_startCopyIndex = (int)((0.6*COMPLETE_BUFFER_SIZE+0.5)); // selects the beginning of the last 25% of the SINGLE BUFFER (which is 80% of the BUFFER)
double* dp_filteredSignalAfterBuffer = (double *)malloc(sizeof(double)*262144); 
int i_filteredSignalAfterBufferIndex = 0; 
int b_filteredSignalAfterBufferIndex = false; 
int d_notchedSignalPrev1 = 0;
int d_notchedSignalPrev2 = 0;
int d_notchedReferenceSignalPrev1 = 0;
int d_notchedReferenceSignalPrev2 = 0;

/**
 * @brief Main function that initiliazes everything
 * 
 * @return  
 */
int main(){
    double* dp_audioBuffer = (double *)malloc(sizeof(double)*COMPLETE_BUFFER_SIZE); // Complete buffer allocated for double values 
    int i_buffer1Head = 0; // integer indicating the head of the first buffer
    int i_buffer2Head = 0.2*COMPLETE_BUFFER_SIZE; // integer indicating the head of the second buffer
    int i_buffer1Tail = 0.8*COMPLETE_BUFFER_SIZE; // integer indicating the tail of the first buffer
    int i_buffer2Tail = COMPLETE_BUFFER_SIZE; // integer indicating the tail of the second buffer

    vector<double> vd_data = read_csv("./res/data.csv", 1); // vector containing all the testing data  
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
    // this_thread::sleep_for(1s);
    // chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    double* dp_filteredSignal = filter_signal(dp_rawSignal, i_size, 0, 0);
    // chrono::steady_clock::time_point end1 = chrono::steady_clock::now();
    // cout << "Filtering function took " << chrono::duration_cast<chrono::microseconds>(end1 - begin).count() << "muS" << endl;
    return dp_filteredSignal;
}

/**
 * @brief compares the filtered array to the correct values extracted from MATLAB -> prints a comparision eleemnt by element
 * 
 * @param dp_filteredSignal array that contains the values to compare with MATLAB
 * @param i_size size of comparision points -> shouldn't exceed max size
 */
void compare_matlab(double* dp_filteredSignal, int i_size){
    vector<double> vd_matlabFilteredSignal = read_csv("./res/matlab_firfiltered_data.csv", 0);
    double d_precision = 0.0000001;
    int i_numFalse = 0;
    for (int i =0;i<i_size;i++){
        // cout << i << " - C++ FIR: " << dp_filteredSignal[i] 
        // << " - MATLAB FIR: " << vd_matlabFilteredSignal[i] << " - Equal? " 
        // << (abs(dp_filteredSignal[i]-vd_matlabFilteredSignal[i])<d_precision ? "True" : "False") << endl;
        i_numFalse += (int)(abs(dp_filteredSignal[i]-vd_matlabFilteredSignal[i])>=d_precision);
    }
    cout << "Number of filtered elements that aren't equal to the MATLAB filer is " << i_numFalse << endl;
}

/**
 * @brief compares the filtered array to the correct values extracted from MATLAB -> prints a comparision eleemnt by element
 * 
 * @param dp_rawSignal array that indicates the complete raw signal for filtering 
 * @param vvd_sdCard array that indicates the values saved in the SD card
 * @param i_size size of comparision points -> shouldn't exceed max size
 */
void compare_filter_buffer_nobuffer(double* dp_rawSignal, double* dp_filteredSignalBuffered, int i_size){
    double* dp_filteredSignal = test_filter(dp_rawSignal, i_size);
    double d_precision = 0.0000001;
    int i_numFalse = 0;
    for (int i =0;i<i_size;i++){
        // cout << i << " - C++ Complete: " << dp_filteredSignal[i] 
        // << " - C++ Buffer: " << dp_filteredSignalBuffered[i] << " - Equal? " 
        // << (abs(dp_filteredSignal[i]-dp_filteredSignalBuffered[i])<d_precision ? "True" : "False") << endl;
        i_numFalse += (int)(abs(dp_filteredSignal[i]-dp_filteredSignalBuffered[i])>=d_precision);
    }
    cout << "Number of filtered elements using buffer implementation that are not equal to no buffer implementation is " << i_numFalse << endl;
}

/**
 * @brief function that tests the analysis implementation
 * 
 * @param dp_filteredSignal array that contains the filtered signal
 * @param i_size size of the array
 */
void test_analysis(double* dp_filteredSignal, int i_size){
    int i_chunkSize = COMPLETE_BUFFER_SIZE*0.8; // size of the subset of the signal to analyse
    double* dp_chunkedSignal = (double *)malloc(sizeof(double)*i_chunkSize);
    string s_cppAnalysisFile = "./res/cpp_analysis_data.txt";
    string s_cppTimeFile = "./res/cpp_time_data.txt";
    ofstream fout1; 
    ofstream fout2; 
    fout1.open(s_cppAnalysisFile, std::ofstream::out);
    fout2.open(s_cppTimeFile, std::ofstream::out);
    int i = 0;
    string s_notchedSignalPath = "./res/cpp_notched_signal.txt"; // to clear the files before using them
    string s_referenceSignalPath = "./res/cpp_reference_signal.txt"; // to clear the files before using them
    string s_firfilteredSignalPath = "./res/cpp_firfiltered_signal.txt";
    ofstream temp; 
    temp.open(s_notchedSignalPath, std::ofstream::out);
    temp.open(s_referenceSignalPath, std::ofstream::out);
    temp.open(s_firfilteredSignalPath, std::ofstream::out);
    while (i < i_size){
        for (int j = 0; j < i_chunkSize; j++){
            dp_chunkedSignal[j] = dp_filteredSignal[i+j];
        }
        double* dp_analysisResult = analyze_signal(dp_chunkedSignal, i_chunkSize, d_notchedSignalPrev1, d_notchedSignalPrev2, d_notchedReferenceSignalPrev1, d_notchedReferenceSignalPrev2, THRESHOLD);
        i_vocalisationDetected = (int)dp_analysisResult[4];
        d_notchedSignalPrev1 = dp_analysisResult[0];
        d_notchedSignalPrev2 = dp_analysisResult[1]; 
        d_notchedReferenceSignalPrev1 = dp_analysisResult[2];
        d_notchedReferenceSignalPrev2 = dp_analysisResult[3];        
        fout1<<i_vocalisationDetected<<endl;
        fout2<<i*0.000125<<endl;
        fout1<<(int)i_vocalisationDetected<<endl;
        fout2<<(i+(i_chunkSize/4))*0.000125<<endl;
        i += COMPLETE_BUFFER_SIZE*0.2;
    }
    // for (int i = 0; i < 4*i_size/i_chunkSize; i++){
    //     for (int j = 0; j < i_chunkSize; j++){
    //         dp_chunkedSignal[j] = dp_filteredSignal[(int)(i*(i_chunkSize)/4)+j];
    //     }
    //     int i_analysisResult = (int)analyze_signal(dp_chunkedSignal, i_chunkSize,0, 0, 0, 0, THRESHOLD)[4];
    //     fout1<<(int)i_analysisResult<<endl;
    //     fout2<<(i*(i_chunkSize)/4)*0.000125<<endl;
    //     fout1<<(int)i_analysisResult<<endl;
    //     fout2<<((i*(i_chunkSize)/4)+(i_chunkSize/4))*0.000125<<endl;
    // }
    // fout1.close();
    // fout2.close();
}


/**
 * @brief generate data to plot sd card info
 * 
 * @param i_size size of the complete input data 
 */
void generate_sdCardPlotData(int i_size){
    string s_sdAnalysisFile = "./res/sd_analysis_data.txt";
    string s_sdTimeFile = "./res/sd_time_data.txt";
    ofstream fout1; 
    ofstream fout2; 
    fout1.open(s_sdAnalysisFile, std::ofstream::out);
    fout2.open(s_sdTimeFile, std::ofstream::out);    
    int i_SDcardSample = 0; // which sample of the SD card we are using
    int i = 0;

    fout2 << -0.000125 << endl;
    fout1 << 0 << endl;
    for (auto row: vvd_sdCard){
        fout2 << (row[0])*0.000125 << endl;
        fout1 << 0 << endl;
        fout2 << row[0]*0.000125 << endl;
        fout1 << 1 << endl;
        fout2 << (row[0]+row.size())*0.000125 << endl;
        fout1 << 1 << endl;
        fout2 << (row[0]+row.size())*0.000125 << endl;
        fout1 << 0 << endl;
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
    // Reads data sequentially from array containing all the data -> simulates reading from ADC
    // We lose a maximum of 0.2*COMPLETE_BUFFER_SIZE of data at the end
    for(int i = 0; i < i_CompleteDataSize; i++){
        dp_audioBuffer[i_iter] = dp_rawSignal[i];
        if (i_iter == i_buffer1Tail){ // when buffer 1 is full
            i_head = i_buffer1Head;
            i_buffer1Head = ((int)(i_buffer1Head+0.4*COMPLETE_BUFFER_SIZE)%(COMPLETE_BUFFER_SIZE+1)); // changes head position on the first buffer to start saving 50% later 
            i_buffer1Tail = ((int)(i_buffer1Tail+0.4*COMPLETE_BUFFER_SIZE)%COMPLETE_BUFFER_SIZE); // changes tail position on the first buffer to end 50% later
            future<int> filterThread = async(launch::deferred, Filter, dp_audioBuffer, i_head, i-0.8*COMPLETE_BUFFER_SIZE); // launch thread that filters the buffer 
            int filteredBuffer = filterThread.get(); 
        }
        if (i_iter == i_buffer2Tail){ // when buffer 2 is full
            i_head = i_buffer2Head;
            i_buffer2Head = ((int)(i_buffer2Head+0.4*COMPLETE_BUFFER_SIZE)%(COMPLETE_BUFFER_SIZE+1)); // changes head position on the second buffer to start saving 50% later
            i_buffer2Tail = ((int)(i_buffer2Tail+0.4*COMPLETE_BUFFER_SIZE)%COMPLETE_BUFFER_SIZE); // changes tail position on the second buffer to end 50% later
            future<int> filterThread = async(launch::deferred, Filter, dp_audioBuffer, i_head, i-0.8*COMPLETE_BUFFER_SIZE); // launch thread that filters the buffer
            int filteredBuffer = filterThread.get(); 
        }
        i_iter++;
        i_iter = i_iter%(COMPLETE_BUFFER_SIZE+1);
    }

    string s_firfilteredSignalPath = "./res/sd_firfiltered_data.txt";
    ofstream fout1; 
    fout1.open(s_firfilteredSignalPath, std::ofstream::out);
    for (int i=0; i<i_CompleteDataSize; i++){
        fout1 << dp_filteredSignalAfterBuffer[i] << endl;
    }

    compare_filter_buffer_nobuffer(dp_rawSignal, dp_filteredSignalAfterBuffer, i_CompleteDataSize);
    compare_matlab(dp_filteredSignalAfterBuffer, i_CompleteDataSize);
    // test_analysis(dp_filteredSignalAfterBuffer, i_CompleteDataSize);
    generate_sdCardPlotData(i_CompleteDataSize);
}

/**
 * @brief simulated the filtering, analysis and adding to the SD card
 * 
 * @param dp_buffer buffer that mimicks a circular queue to be filtered and analyzed
 * @param i_head beginning position to read from the buffer 
 * @param SINGLE_BUFFER_SIZE size of a single buffer - proportional to COMPLETE_BUFFER_SIZE
 * @return double* to the filtered buffer 
 */
int Filter(double* dp_buffer, int i_head, int i_generalLocation){
    double* dp_bufferSignal = (double *)malloc(sizeof(double)*SINGLE_BUFFER_SIZE); // buffer to be analyzed
    // populate correct values starting from specified and looping back as if circular queue 
    for (int i=0; i<SINGLE_BUFFER_SIZE;i++){
        dp_bufferSignal[i] = dp_buffer[(i_head+i)%(COMPLETE_BUFFER_SIZE+1)]; // implements circular queue
    }

    double* dp_filteredSignal = filter_signal(dp_bufferSignal, SINGLE_BUFFER_SIZE, d_filteredPrev1, d_filteredPrev2); // filtered signal
    d_filteredPrev1 = dp_filteredSignal[int(SINGLE_BUFFER_SIZE*0.25)+0];
    d_filteredPrev2 = dp_filteredSignal[int(SINGLE_BUFFER_SIZE*0.25)+1];

    // tests that the filtering and mergind is equal to filtering of complete data
    // to be deleted later
    int i_filteredSignalAfterBufferStartIndex = (int)((b_filteredSignalAfterBufferIndex*0.6*COMPLETE_BUFFER_SIZE+0.5));
    for(int i=i_filteredSignalAfterBufferStartIndex;i<SINGLE_BUFFER_SIZE;i++) {
        dp_filteredSignalAfterBuffer[i_filteredSignalAfterBufferIndex] = dp_filteredSignal[i];
        i_filteredSignalAfterBufferIndex++;
    }
    b_filteredSignalAfterBufferIndex = true;

    string s_firfilteredSignalPath = "./res/cpp_firfiltered_signal.txt";
    ofstream fout2;
    fout2.open(s_firfilteredSignalPath, std::ofstream::out  | std::ofstream::app);
    for (int i=0; i<SINGLE_BUFFER_SIZE*0.25; i++){
        fout2 << setprecision(16) << dp_filteredSignal[i] << endl;
    }


    double* dp_analysisResult = analyze_signal(dp_filteredSignal, SINGLE_BUFFER_SIZE, 0, 0, 0, 0, THRESHOLD);
    i_vocalisationDetected = (int)dp_analysisResult[4];
    d_notchedSignalPrev1 = dp_analysisResult[0];
    d_notchedSignalPrev2 = dp_analysisResult[1]; 
    d_notchedReferenceSignalPrev1 = dp_analysisResult[2];
    d_notchedReferenceSignalPrev2 = dp_analysisResult[3];

    vector<double> instance;
    if (((2*i_vocalisationDetected)-1) + i_mergeState < 1) {
        i_mergeState = 0;
        return i_mergeState;
    }
    if (i_mergeState == 0 && ((2*i_vocalisationDetected)-1) == 1){
        vector<double> instance;
        i_mergeState = 2;
        instance.push_back(i_generalLocation);
        for(int i=0;i<(0.25*SINGLE_BUFFER_SIZE);i++) {
            instance.push_back(dp_filteredSignal[i]);
        }
        vvd_sdCard.push_back(instance);
    } else{
        for(int i=0;i<(0.25*SINGLE_BUFFER_SIZE);i++) {
            // select last element in the sd_card and add to it
            vvd_sdCard.back().push_back(dp_filteredSignal[i]);
        }
        i_mergeState = min(i_mergeState+((2*i_vocalisationDetected)-1), 2);
    }

    // if (i_vocalisationDetected){
    //     if (i_mergeState == 0){
    //         vector<double> instance;
    //         instance.push_back(i_generalLocation);
    //         for(int i=0;i<(0.25*SINGLE_BUFFER_SIZE);i++) {
    //             instance.push_back(dp_filteredSignal[i]);
    //         }
    //         vvd_sdCard.push_back(instance);
    //         i_mergeState = 1;
    //     }
    //     else{
    //         for(int i=0;i<(0.25*SINGLE_BUFFER_SIZE);i++) {
    //             vvd_sdCard.back().push_back(dp_filteredSignal[i]);
    //         }
    //     }
    // } else {
    //     i_mergeState = 0;
    // }


    return i_mergeState;
}
