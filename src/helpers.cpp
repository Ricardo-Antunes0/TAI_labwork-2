#include <iostream>
#include <cstdint>
#include "helpers.h"


void writeMapEntry(std::ofstream& outfile, const std::pair<std::string, std::unordered_map<char,size_t>>& entry) {
    // Write key size
    size_t keySize = entry.first.size();
    outfile.write(reinterpret_cast<char*>(&keySize), sizeof(keySize));

    // Write key data
    outfile.write(entry.first.data(), keySize * sizeof(char));

    // Write value size
    size_t valueSize = entry.second.size();
    outfile.write(reinterpret_cast<char*>(&valueSize), sizeof(valueSize));

    // Write value data
    for(const auto& e : entry.second) {
        size_t x = e.second;
        outfile.write(&e.first,sizeof(char));
        outfile.write(reinterpret_cast<char*>(&x),sizeof(size_t));
    }
}

// Function to save the Data struct to a binary file
void saveDataToFile(const std::unordered_map<std::string,std::unordered_map<char,size_t>>& data, const std::string& filename,uint64_t k,double alpha,char mostFrequent) {
    std::ofstream outfile(filename, std::ios::binary);

    if (!outfile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    uint64_t mk = k;
    outfile.write(reinterpret_cast<char*>(&mk),sizeof(uint64_t));

    double a = alpha;
    outfile.write(reinterpret_cast<char*>(&a),sizeof(double));

    outfile.write(&mostFrequent,sizeof(char));

    // Write the number of entries in counts map
    size_t numCountsEntries = data.size();
    outfile.write(reinterpret_cast<char*>(&numCountsEntries), sizeof(numCountsEntries));
    
    // Write each entry from counts map
    for (const auto& entry : data) {
        writeMapEntry(outfile, entry);
    }

    outfile.close();
}

// Function to read a single key-value pair from binary file
std::pair<std::string, std::unordered_map<char,size_t>> readMapEntry(std::ifstream& infile) {
    // Read key size
    size_t keySize;
    infile.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));

    // Read key data
    std::vector<char> keyData(keySize);
    infile.read(keyData.data(), keySize * sizeof(char));

    // Create key from data
    std::string key(keyData.begin(),keyData.end());

    // Read value size
    size_t valueSize;
    infile.read(reinterpret_cast<char*>(&valueSize), sizeof(valueSize));

    // Read value data
    std::unordered_map<char,size_t> valueData;
    for(size_t i = 0; i < valueSize; i++) {
        char key;
        size_t value;
        infile.read(&key,sizeof(char));
        infile.read(reinterpret_cast<char*>(&value),sizeof(size_t));
        valueData[key] = value;
    }

    // Return the key-value pair
    return std::make_pair(key,valueData);
}

// Function to read the Data struct from a binary file
std::unordered_map<std::string,std::unordered_map<char,size_t>> readDataFromFile(const std::string& filename,uint64_t& k,double& alpha,char& mostFrequent) {
    std::unordered_map<std::string,std::unordered_map<char,size_t>> data;
    std::ifstream infile(filename, std::ios::binary);

    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename;
        return data; // Return empty data struct on error
    }

    infile.read(reinterpret_cast<char *>(&k), sizeof(uint64_t));
    infile.read(reinterpret_cast<char *>(&alpha), sizeof(double));
    infile.read(&mostFrequent,sizeof(char));

    // Read the number of entries in counts map
    size_t numCountsEntries;
    infile.read(reinterpret_cast<char*>(&numCountsEntries), sizeof(numCountsEntries));

    // Read each entry from counts map
    for (size_t i = 0; i < numCountsEntries; i++) {
        auto entry = readMapEntry(infile);
        data.insert(entry);
    }

    infile.close();
    return data;
}
