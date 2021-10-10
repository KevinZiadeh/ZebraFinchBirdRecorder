#include <stdint.h>
#include <windows.h>

/**
 * @brief filter the noise out of the signal 
 * 
 * @param u16_bufferSignal pointer to the raw signal
 * @param i_singleBufferSize size of the raw signal
 * @param d_filteredPrev1 before last element of previously filtered buffer > initiliazed to 0
 * @param d_filteredPrev2 before element of previously filtered buffer > initiliazed to 0
 * @return double* filtered signal
 */
double* filter_signal(uint16_t* u16_bufferSignal, int i_singleBufferSize, double d_filteredPrev1, double d_filteredPrev2);

/**
 * @brief filter the noise out of the signal 
 * 
 * @param dp_bufferSignal pointer to the raw signal
 * @param i_singleBufferSize size of the raw signal
 * @param d_filteredPrev1 before last element of previously filtered buffer > initiliazed to 0
 * @param d_filteredPrev2 before element of previously filtered buffer > initiliazed to 0
 * @return double* filtered signal
 */
double* filter_signal(double* dp_bufferSignal, int i_singleBufferSize, double d_filteredPrev1, double d_filteredPrev2);

/**
 * @brief 
 * 
 * @param dp_filteredSignal pointer to the filtered signal 
 * @param i_signalSize size of the filtered signal
 * @return true when the signal corresponds to a vocalisation 
 * @return false when the signal does not correspond to a vocalisation
 */
bool analyze_signal(double* dp_filteredSignal, int i_signalSize)