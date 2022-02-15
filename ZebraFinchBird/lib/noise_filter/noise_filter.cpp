// #include <vector>
#include "Arduino.h"

float* filter_signal(uint16_t* &u16_bufferSignal, int &i_singleBufferSize, float &f_filterepfrev1, float &f_filterepfrev2){
    // coefficients used for the filtering. Online
    float a0 = 0.8948577513857248;
    float a1 = -1.7897155027714495;
    float a2 = 0.8948577513857248;
    float b1 = -1.7786300789392977;
    float b2 = 0.8008009266036016;

    float* pf_filteredSignal = (float *)malloc(sizeof(float)*i_singleBufferSize); // filtered buffer
    // Initial values set to 0
    pf_filteredSignal[0] = f_filterepfrev1;
    pf_filteredSignal[0] = f_filterepfrev2;
 
    for (int i =2; i< i_singleBufferSize; i++){
        pf_filteredSignal[i] = (((a0*u16_bufferSignal[i] + a1*u16_bufferSignal[i-1] + a2*u16_bufferSignal[i-2])*3.3)/(float)4096) - b1*pf_filteredSignal[i-1] - b2*pf_filteredSignal[i-2];
    }
    return pf_filteredSignal;
}

double* filter_signal(double* &dp_bufferSignal, int i_singleBufferSize, double &d_filteredPrev1, double &d_filteredPrev2){    
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

float* analyze_signal(float* &pf_filteredSignal, int &i_signalSize, float &f_notchedSignalPrev1, float &f_notchedSignalPrev2, float &f_notchedReferenceSignalPrev1, float &f_notchedReferenceSignalPrev2, float &f_threshhold){    
    // First step is to find the total power in the signal
    float totalPower = 0;
    float* pf_response = (float *)malloc(sizeof(float)*5); 
    for (int i=0; i<i_signalSize; i++){
        totalPower += pf_filteredSignal[i]*pf_filteredSignal[i];
    }

    // Now to find the power around a certain frequency (to be determined when gathering actual values)
    // we need to pass a band pass filter
    // Band Pass at 2.2 kHz (for now)
    
    // Assuming 6 kHz sampling frequency (ESP ADC Frequency)
    float a0 = 0.3444719716111889;
    float a1 = 0;
    float a2 = -0.3444719716111889;
    float b1 = 0.8772677342420642;
    float b2 = 0.3110560567776222;
    
    float* pf_notchedSignal = (float*)malloc(sizeof(float)*i_signalSize);

    // Taking in initial values for now, might need to implement crossover (like filtering)
    pf_notchedSignal[0] = f_notchedSignalPrev1;
    pf_notchedSignal[1] = f_notchedSignalPrev2;

    for (int i=2; i<i_signalSize; i++){
        pf_notchedSignal[i] = a0*pf_filteredSignal[i] + a1*pf_filteredSignal[i-1] + a2*pf_filteredSignal[i-2] - b1*pf_notchedSignal[i-1] - b2*pf_notchedSignal[i-2];
    }
    pf_response[0] = pf_notchedSignal[int(i_signalSize*0.25)+0];
    pf_response[1] = pf_notchedSignal[int(i_signalSize*0.25)+1];
    
    // Find power in the filtered signal (i.e. Power at 2 kHz)
    float notchepfower = 0;

    for (int i=0; i<i_signalSize; i++){
        notchepfower += pf_notchedSignal[i]*pf_notchedSignal[i];
    }

//////////////////////////////////////////////////////////////////
    // Now to find the power around another reference frequency (again to be determined when gathering actual values)
    // as it is needed for comparison purposes
    // Banpfass at 400 Hz (for now)

    // Assuming 6 kHz sampling frequency (ESP ADC Frequency)
    a0 = 0.22336671878312517;
    a1 = 0;
    a2 = -0.22336671878312517;
    b1 = -1.4189796126194893;
    b2 = 0.5532665624337496;    
    
    // Reusing/Resetting notchedSignal array as to not waste memory
    pf_notchedSignal[0] = f_notchedReferenceSignalPrev1;
    pf_notchedSignal[1] = f_notchedReferenceSignalPrev2;

    for (int i=2; i<i_signalSize; i++){
        pf_notchedSignal[i] = a0*pf_filteredSignal[i] + a1*pf_filteredSignal[i-1] + a2*pf_filteredSignal[i-2] - b1*pf_notchedSignal[i-1] - b2*pf_notchedSignal[i-2];
    }
    pf_response[2] = pf_notchedSignal[int(i_signalSize*0.25)+0];
    pf_response[3] = pf_notchedSignal[int(i_signalSize*0.25)+1];

    // Find power in the reference notched signal (i.e. Power at 400 Hz)
    float notchedReferencePower = 0;

    for (int i=0; i<i_signalSize; i++){
        notchedReferencePower += pf_notchedSignal[i]*pf_notchedSignal[i];
    }

    float notchedToReferenceRatio = notchepfower/notchedReferencePower;
    pf_response[4] = (notchedToReferenceRatio >= f_threshhold);
    return pf_response;
}