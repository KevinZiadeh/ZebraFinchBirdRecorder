#include <vector>
#include <stdint.h>

#define BUFFER_SIZE 4096

std::vector<float> filter_signal(std::vector<float> raw_signal){
    float a0 = 0.8948577513857248;
    float a1 = -1.7897155027714495;
    float a2 = 0.8948577513857248;
    float b1 = -1.7786300789392977;
    float b2 = 0.8008009266036016;

    std::vector<float> filtered_signal;

    for (int i =0; i< raw_signal.size(); i++){
        filtered_signal.push_back(a0*raw_signal[i] + a1*raw_signal[i-1] + a2*raw_signal[i-2] - b1*filtered_signal[i-1] - b2*filtered_signal[i-2]);
    }
    return filtered_signal;
}

uint16_t* filter_signal(uint16_t* raw_signal, int size){
    float a0 = 0.8948577513857248;
    float a1 = -1.7897155027714495;
    float a2 = 0.8948577513857248;
    float b1 = -1.7786300789392977;
    float b2 = 0.8008009266036016;

    uint16_t* filtered_signal = (uint16_t *)malloc(2*BUFFER_SIZE);
    filtered_signal[0] = (uint16_t)0;
    filtered_signal[0] = (uint16_t)0;

   for (int i =2; i< size; i++){
        filtered_signal[i] = a0*raw_signal[i] + a1*raw_signal[i-1] + a2*raw_signal[i-2] - b1*filtered_signal[i-1] - b2*filtered_signal[i-2];
    }

    return filtered_signal;
}