#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <vector>
#include <stdint.h>

std::vector<float> filter_signal(uint16_t* raw_signal);
std::vector<float> filter_signal(std::vector<float> raw_signal);
std::vector<float> classify_signal();

#endif
