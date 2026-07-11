#include <iostream>
#include "includes/cpersist.h"
#include <algorithm>
#include <fstream>


// CHECKERS
bool SaveManager::filename_fits_standards(const std::string& filename) {
    if (filename.find(' ') != std::string::npos) {return false;} // contains space
    return true;
}
void SaveManager::make_filename_safe(std::string& filename) {
    if (filename_fits_standards(filename)) {return;}

    std::replace(filename.begin(), filename.end(), ' ', '_');
}

// CURRENT FILE MANIPULATION
bool SaveManager::change_file(const std::string& new_file) {
    if (!filename_fits_standards(new_file)) {return false;} // doesn't fit naming standards
    if (!files.count(new_file)) {return false;} // file doesn't exist

    current_file = new_file;
    return true;
}
void SaveManager::change_file_safe(const std::string& new_file) {
    current_file = new_file; // currently not present, but will be inserted later
    if (!filename_fits_standards(current_file)) {
        make_filename_safe(current_file);
    }
    
    if (!files.count(current_file)) {
        files.try_emplace(current_file);
    }
}
bool SaveManager::create_new_file(const std::string& new_file) {
    if (!filename_fits_standards(new_file)) {return false;} // doesn't fit naming standards
    
    if (files.contains(new_file)) {
        return false; // file already exists
    }
    
    files.emplace(new_file, std::stringstream{});
    return true;
}

// WRITING / READING
int SaveManager::getDataPosition(const std::string& name) {
    std::vector<uint8_t> data = readFileAsBinary(current_file);
    size_t position = 0;

    while (position < data.size()) {
        // first of all, we need to make sure there's enough room for nameSize
        if (position + sizeof(uint64_t) > data.size()) {return -1;} // data is probably invalid, not found

        // ===== NAMESIZE
        uint64_t nameSize;
        std::memcpy(&nameSize, data.data() + position, sizeof(nameSize)); // cool pointer stuff. moves data from the vector to nameSize
        position += sizeof(nameSize); // move forward

        // ===== NAME
        // check bounds again
        if (position + nameSize > data.size()) {return -1;}

        std::string currentName(
            reinterpret_cast<char*>(data.data() + position),
            nameSize
        );
        position += nameSize;

        // ===== DATASIZE
        if (position + sizeof(uint64_t) > data.size()) {return -1;}

        uint64_t dataSize;
        std::memcpy(&dataSize, data.data() + position, sizeof(dataSize));
        position += sizeof(dataSize);

        // the position now points at the data itself. return it if there's a match
        if (currentName == name) {
            return static_cast<int>(position);            
        }

        // the end
        if (position + dataSize > data.size()) {return -1;}

        // move to the next
        position += dataSize;
    }

    return -1;
}
std::vector<uint8_t> SaveManager::readFileAsBinary(const std::string& filename)
{
    std::ifstream file(filename + ".dat", std::ios::binary);

    if (!file) {
        cpersist_internal::ErrorManager::get().throwError("Failed to open file: " + filename + ".dat");
    }

    // Find the file's size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // then read it
    std::vector<uint8_t> bytes(size);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        cpersist_internal::ErrorManager::get().throwError("Cannot read data file " + filename + ".dat");
    }

    return bytes;
}

// COMMIT
void SaveManager::commit() {
    if (current_file.empty()) {
        cpersist_internal::ErrorManager::get().throwError("Can't commit changes while no file is open.");
    }
    std::ofstream file(current_file + ".dat", std::ios::app); // write into <current_file>.dat, append if already exists
    if (file.is_open()) {
        file << files[current_file].rdbuf();
    }
}

// LOGGERS
void SaveManager::log_filenames() {
    for (auto& fn: files) {
        std::cout << fn.first << " ";
    }
    std::cout << std::endl;
}
void SaveManager::log_current_filename() {
    std::cout << get_current_file() << std::endl;
}

// GETTERS
std::string& SaveManager::get_current_file() {
    return current_file;
}