// #include <vector>
#include <stdint.h>
#include <windows.h>

// function to filter the noise out of the signal 
// INPUT:   pointer to the raw signal
//          size of the raw signal
// OUTPUT:  pointer to the filtered signal
uint16_t* filter_signal(uint16_t* raw_signal, int size){
// coefficients used for the filtering. Online
    float a0 = 0.8948577513857248;
    float a1 = -1.7897155027714495;
    float a2 = 0.8948577513857248;
    float b1 = -1.7786300789392977;
    float b2 = 0.8008009266036016;

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

float* filter_signal(float* raw_signal, int size){
// coefficients used for the filtering. Online
    float a0 = 0.8948577513857248;
    float a1 = -1.7897155027714495;
    float a2 = 0.8948577513857248;
    float b1 = -1.7786300789392977;
    float b2 = 0.8008009266036016;

// new filtered signal
    float* filtered_signal = (float*)malloc(sizeof(float)*size);
    // Initial values set to 0
    filtered_signal[0] = (float)0;
    filtered_signal[0] = (float)0;

   for (int i =2; i< size; i++){
        filtered_signal[i] = a0*raw_signal[i] + a1*raw_signal[i-1] + a2*raw_signal[i-2] - b1*filtered_signal[i-1] - b2*filtered_signal[i-2];
    }

    return filtered_signal;
}