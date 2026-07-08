#include <vector>
#include "Record.h"
#include "Serializer.h"
#include "StorageEngine.h"
#include <iostream>
#include <fstream>
#include <string>

StorageEngine::StorageEngine(const std::string& filename)
    : filename(filename)
{}

// read code from file
std::vector <Record> StorageEngine::load(){
    Serializer serializer;
    std::vector<Record> loaded;
    std::ifstream file(filename);

    // check if the file opened successfully
    if (!file.is_open()) {
        return loaded; // empty vector to avoid crashing
    }

    // read the file line by line
    std::string line;
    while (std::getline(file, line)) {
        Record record = serializer.deserialize(line);
        loaded.push_back(record);
    }
    return loaded;
}

// save info in database
void StorageEngine::save(const std::vector<Record>& records) {
    Serializer serializer;
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "Error: Could not open the file!" << std::endl;
        return;
    }

    for (const Record& record : records) {
        file << serializer.serialize(record) << '\n';
    }
}
