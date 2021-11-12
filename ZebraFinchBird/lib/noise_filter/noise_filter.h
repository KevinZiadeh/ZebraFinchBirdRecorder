// #include <vector>
#include "Arduino.h"

float* filter_signal(uint16_t* &u16_bufferSignal, int &i_singleBufferSize, float &f_filterepfrev1, float &f_filterepfrev2);
float* filter_signal(float* &pf_bufferSignal, int &i_singleBufferSize, float &f_filterepfrev1, float &f_filterepfrev2);
float* analyze_signal(float* &pf_filteredSignal, int &i_signalSize, float &f_notchedSignalPrev1, float &f_notchedSignalPrev2, float &f_notchedReferenceSignalPrev1, float &f_notchedReferenceSignalPrev2, float &f_threshhold);
