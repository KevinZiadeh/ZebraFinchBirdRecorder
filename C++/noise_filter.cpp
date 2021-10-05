// #include <vector>
#include <stdint.h>
#include <windows.h>
#include <iostream>

using namespace std;

#define print_vec(vec) for(auto e: vec) {cout << e << " ";} cout << endl;

// function to filter the noise out of the signal 
// INPUT:   pointer to the raw signal
//          size of the raw signal
// OUTPUT:  pointer to the filtered signal
uint16_t* filter_signal(uint16_t* raw_signal, int size){
// coefficients used for the filtering. Online
    double a0 = 0.8948577513857248;
    double a1 = -1.7897155027714495;
    double a2 = 0.8948577513857248;
    double b1 = -1.7786300789392977;
    double b2 = 0.8008009266036016;

// new filtered signal
    uint16_t* filtered_signal = (uint16_t *)malloc(2*size);
    // Initial values set to 0
    filtered_signal[0] = (uint16_t)0;
    filtered_signal[0] = (uint16_t)0;

   for (int i =2; i< size; i++){
        filtered_signal[i] = a0*raw_signal[i] + a1*raw_signal[i-1] + a2*raw_signal[i-2] - b1*filtered_signal[i-1] - b2*filtered_signal[i-2];
    }

    return filtered_signal;
}

double* filter_signal(double* raw_signal, int size){
    
// coefficients used for the filtering. Online
    double a0 = 0.8948577513857248;
    double a1 = -1.7897155027714495;
    double a2 = 0.8948577513857248;
    double b1 = -1.7786300789392977;
    double b2 = 0.8008009266036016;

// new filtered signal
    double* filtered_signal = (double*)malloc(sizeof(double)*size);
    // Initial values set to 0
    filtered_signal[0] = (double)0;
    filtered_signal[0] = (double)0;

   for (int i=2; i<size; i++){
        cout << i << "  " << size << endl;
        filtered_signal[i] = a0*raw_signal[i] + a1*raw_signal[i-1] + a2*raw_signal[i-2] - b1*filtered_signal[i-1] - b2*filtered_signal[i-2];
    }

    return filtered_signal;
}