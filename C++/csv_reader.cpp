#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <utility> // std::pair
#include <stdexcept> // std::runtime_error
#include <sstream> // std::stringstream
#include <stdint.h>
#include <windows.h>
using namespace std;

vector<float> read_csv(string filename){
    // Create an input filestream
    ifstream myFile(filename);

    // Make sure the file is open
    if(!myFile.is_open()) throw std::runtime_error("Could not open file");

    // Helper vars
    string line;

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