#include <stdint.h>
#include <windows.h>

uint16_t* filter_signal(uint16_t* raw_signal, int size, uint16_t prev1, uint16_t prev2);
double* filter_signal(double* raw_signal, int size, double prev1, double prev2);
// std::vector<double> analyze_signal();
