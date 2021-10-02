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

#define print_vec(vec) for(auto e: vec) {cout << e << " ";} cout << endl;

vector<double> read_csv(string filename, int col){
    // Create an input filestream
    ifstream myFile(filename);

    // Make sure the file is open
    if(!myFile.is_open()) throw std::runtime_error("Could not open file");

    // Helper vars
    string line;

    // Read data, line by line
    vector<double> result;
    int index;
        while(getline(myFile, line))
        {
            index = 0;
            stringstream ss(line);
            while(getline(ss, line, ',')){
                if (index == col){
                    result.push_back(stof(line));
                }   
                index++; 
            }
        }
    // Close file
    myFile.close();
    return result;
}