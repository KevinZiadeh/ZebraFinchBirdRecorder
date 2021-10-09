#include <stdint.h>
#include <windows.h>
#include <iostream>

using namespace std;

double* filter_signal(uint16_t* u16_bufferSignal, int i_singleBufferSize, double d_filteredPrev1, double d_filteredPrev2){
    // coefficients used for the filtering. Online
    double a0 = 0.8948577513857248;
    double a1 = -1.7897155027714495;
    double a2 = 0.8948577513857248;
    double b1 = -1.7786300789392977;
    double b2 = 0.8008009266036016;

    double* dp_filteredSignal = (double *)malloc(sizeof(double)*i_singleBufferSize); // filtered buffer
    // Initial values set to 0
    dp_filteredSignal[0] = d_filteredPrev1;
    dp_filteredSignal[0] = d_filteredPrev2;
 
    for (int i =2; i< i_singleBufferSize; i++){
        dp_filteredSignal[i] = (((a0*u16_bufferSignal[i] + a1*u16_bufferSignal[i-1] + a2*u16_bufferSignal[i-2])*3.3)/(double)4096) - b1*dp_filteredSignal[i-1] - b2*dp_filteredSignal[i-2];
    }

    return dp_filteredSignal;
}

double* filter_signal(double* dp_bufferSignal, int i_singleBufferSize, double d_filteredPrev1, double d_filteredPrev2){    
    // coefficients used for the filtering. Online
    double a0 = 0.8948577513857248;
    double a1 = -1.7897155027714495;
    double a2 = 0.8948577513857248;
    double b1 = -1.7786300789392977;
    double b2 = 0.8008009266036016;

    double* dp_filteredSignal = (double*)malloc(sizeof(double)*i_singleBufferSize); // filtered buffer
    // Initial values set to 0
    dp_filteredSignal[0] = d_filteredPrev1;
    dp_filteredSignal[1] = d_filteredPrev2;

    for (int i=2; i<i_singleBufferSize; i++){
        dp_filteredSignal[i] = a0*dp_bufferSignal[i] + a1*dp_bufferSignal[i-1] + a2*dp_bufferSignal[i-2] - b1*dp_filteredSignal[i-1] - b2*dp_filteredSignal[i-2];
    }

    return dp_filteredSignal;
}