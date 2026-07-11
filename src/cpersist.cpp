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