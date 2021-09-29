// #include <vector>
#include <stdint.h>
#include <windows.h>

// std::vector<float> filter_signal(std::vector<float> raw_signal);
uint16_t* filter_signal(uint16_t* raw_signal, int size);
float* filter_signal(float* raw_signal, int size);
// std::vector<float> classify_signal();
