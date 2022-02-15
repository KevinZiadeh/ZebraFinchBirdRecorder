// #include <vector>
#include "Arduino.h"

/**
 * @brief filter the noise out of the signal 
 * 
 * @param dp_bufferSignal pointer to the raw signal
 * @param i_singleBufferSize size of the raw signal
 * @param d_filteredPrev1 before last element of previously filtered buffer > initiliazed to 0
 * @param d_filteredPrev2 before element of previously filtered buffer > initiliazed to 0
 * @return double* filtered signal
 */
double* filter_signal(double* &dp_bufferSignal, int i_singleBufferSize, double &d_filteredPrev1, double &d_filteredPrev2);

/**
 * @brief 
 * 
 * @param dp_filteredSignal pointer to the filtered signal 
 * @param i_signalSize size of the filtered signal
 * @param d_threshold threshold to compare with for vocalisation detection
 * @return an array containing [    before last notched signal value, 
 *                                  last notched signal value,
 *                                  before last reference notched signal value, 
 *                                  last reference notched signal value,
 *                                  1 if vocalisation is detected else 0
 *                             ]
 */
double* analyze_signal(double* &dp_filteredSignal, int i_signalSize, double &d_notchedSignalPrev1, double &d_notchedSignalPrev2, double &d_notchedReferenceSignalPrev1, double &d_notchedReferenceSignalPrev2, double d_threshhold);