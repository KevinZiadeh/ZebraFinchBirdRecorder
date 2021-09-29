#include <vector>
#include <iostream>
#include "noise_filter.h"

using namespace std;

#define BUFFER_SIZE 4096

#include <string>
#include <fstream>
#include <utility> // std::pair
#include <stdexcept> // std::runtime_error
#include <sstream> // std::stringstream

vector<float> read_csv(std::string filename){
    // Reads a CSV file into a vector of <string, vector<float>> pairs where
    // each pair represents <column name, column values>

    // Create an input filestream
    ifstream myFile(filename);

    // Make sure the file is open
    if(!myFile.is_open()) throw std::runtime_error("Could not open file");

    // Helper vars
    string line, colname;
    float val;

    // Read data, line by line
    bool col = false;
    vector<float> result;
    while(getline(myFile, line, ','))
    {
        if (col){
            result.push_back(stof(line));
        }
        col = !col;
    }
    // Close file
    myFile.close();

    return result;
}

int main(){
    float* audioBuffer1 = (float *)malloc(sizeof(float)*BUFFER_SIZE);
    float* audioBuffer2 = (float *)malloc(sizeof(float)*BUFFER_SIZE);

    int buffer1_head = 0;
    int buffer2_head = 0;
    int buffer1_tail = BUFFER_SIZE;
    int buffer2_tail = BUFFER_SIZE;

    vector<float> data = read_csv("data.csv");


    return 0;
}