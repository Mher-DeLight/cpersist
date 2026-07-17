#include <iostream>
#include "includes/cpersist.h"
#include <algorithm>

void SaveManager::init(const bool loadPresentFiles, std::optional<std::span<std::string>> initialFiles) {
    std::filesystem::create_directory(folderName); // creates the folder if it doesn't exist
    
    if (loadPresentFiles) {
        loadExistingFiles();
    }

    if (initialFiles) {
        for (const std::string& inf : *initialFiles) {
            if (file_exists(inf)) {continue;}
            else {create_new_file(inf);}
        }
    }
}
void SaveManager::loadExistingFiles() {
    for (const auto& entry : std::filesystem::directory_iterator(folderName)) {
        if (entry.is_regular_file() && entry.path().extension() == fileExtension) {
            create_new_file(entry.path().stem().string());
        }
    }
}

// CHECKERS
bool SaveManager::filename_fits_standards(const std::string& filename) {
    if (filename.find(' ') != std::string::npos) {return false;} // contains space
    return true;
}
void SaveManager::make_filename_safe(std::string& filename) {
    if (filename_fits_standards(filename)) {return;}

    std::replace(filename.begin(), filename.end(), ' ', '_');
}
bool SaveManager::file_exists(const std::string& filename) {
    if (files.contains(filename)) {
        return true;
    } else {
        return false;
    }
}

// CURRENT FILE MANIPULATION
bool SaveManager::change_file(const std::string& new_file) {
    if (!filename_fits_standards(new_file)) {return false;} // doesn't fit naming standards
    if (!files.count(new_file)) {return false;} // file doesn't exist

    current_file = new_file;
    fullFilePath = std::filesystem::path(folderName) / (current_file + fileExtension);
    return true;
}
void SaveManager::change_file_safe(const std::string& new_file) {
    current_file = new_file; // currently not present, but will be inserted later
    fullFilePath = std::filesystem::path(folderName) / (current_file + fileExtension);
    if (!filename_fits_standards(current_file)) {
        make_filename_safe(current_file);
    }
    
    if (!files.count(current_file)) {
        files.try_emplace(current_file);
    }
}
bool SaveManager::create_new_file(const std::string& new_file) {
    if (!filename_fits_standards(new_file)) {return false;} // doesn't fit naming standards
    
    if (file_exists(new_file)) {
        return false; // file already exists
    }
    
    files.emplace(new_file, std::stringstream{});
    return true;
}
bool SaveManager::open(const std::string& filename) {
    if (!filename_fits_standards(filename)) {return false;} // doesn't fit naming standards
    if (!files.count(current_file)) { // file doesn't exist, create it
        files.try_emplace(current_file);
    }

    current_file = filename;
    fullFilePath = std::filesystem::path(folderName) / (current_file + fileExtension);
    return true;
}
void SaveManager::ensure_exists(std::initializer_list<std::string> filenames) {
    for (auto fn : filenames) {
        make_filename_safe(fn);

        if (!file_exists(fn)) {
            create_new_file(fn);
        }
    }
}
void SaveManager::ensure_exists(std::vector<std::string> filenames) {
    for (auto fn : filenames) {
        make_filename_safe(fn);

        if (!file_exists(fn)) {
            create_new_file(fn);
        }
    }
}

// WRITING / READING
uint64_t SaveManager::getDataPosition(const std::string& name, const bool loose) {
    std::vector<uint8_t> data;
    try {
        data = readFileAsBinary(current_file);
    } catch (std::exception& e) {
        return -1; 
    }
    uint64_t position = 0;

    while (position < data.size()) {
        // ===== NAME
        // check bounds
        if (position + sizeof(uint8_t) > data.size()) {return -1;}

        uint8_t nameSize;
        std::memcpy(&nameSize, data.data() + position, sizeof(nameSize));
        position += sizeof(nameSize);


        // ===== NAME
        // check bounds
        if (position + nameSize > data.size()) {return -1;}

        std::string currentName(
            reinterpret_cast<const char*>(data.data() + position),
            nameSize
        );
        position += nameSize;

        // ===== DATASIZE
        if (position + sizeof(uint32_t) > data.size()) {return -1;}

        uint32_t dataSize;
        std::memcpy(&dataSize, data.data() + position, sizeof(dataSize));
        position += sizeof(dataSize);

        // the position now points at the data itself. move back to dataSize then return it if there's a match.
        if (currentName == name || (currentName.starts_with(name + ".") && loose)) {
            return static_cast<uint64_t>(position - sizeof(dataSize));
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
    std::ifstream file(fullFilePath, std::ios::binary);

    if (!file) {
        cpersist_internal::ErrorManager::get().throwError("Failed to open file: " + filename + fileExtension);
    }

    // Find the file's size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // then read it
    std::vector<uint8_t> bytes(size);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        cpersist_internal::ErrorManager::get().throwError("Cannot read data file " + filename + fileExtension);
    }

    return bytes;
}
void SaveManager::writeBytesIntoFile(const char* bytes, const std::uint32_t size, const std::uint64_t position, const bool encrypt) {
    std::fstream file(fullFilePath, std::ios::in | std::ios::out | std::ios::binary);

    if (!file) {
        cpersist_internal::ErrorManager::get().throwError("Failed to modify file: " + current_file + fileExtension + " at position " + std::to_string(position));
    }

    std::vector<uint8_t> data = encrypt? encrMgr.encrypt(std::vector<uint8_t>(bytes, bytes + size)) : std::vector<uint8_t>(bytes, bytes + size);

    std::streamoff offset(position);
    file.seekp(offset);
    
    // write data size
    std::uint32_t totalSize = static_cast<std::uint32_t>(data.size());
    file.write(reinterpret_cast<const char*>(&totalSize), sizeof(totalSize));

    // write data
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    

    if (!file) {
        cpersist_internal::ErrorManager::get().throwError("Failed to write to file " + current_file + fileExtension);
    }
}
bool SaveManager::contains(const std::string& dataname) {
    return getDataPosition(dataname, true) != -1;
}

// COMMIT
void SaveManager::commit() {
    if (current_file.empty()) {
        cpersist_internal::ErrorManager::get().throwError("Can't commit changes while no file is open.");
    }
    std::ofstream file(fullFilePath, std::ios::app); // write into <current_file>.<ext>, append if already exists
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
const std::string& SaveManager::get_current_file() {
    return current_file;
}
const std::string& SaveManager::get_file_extension() {
    return fileExtension;
}

// SETTERS
void SaveManager::set_file_extension(const std::string& new_extension) {
    fileExtension = "." + new_extension;
}

uint64_t cpersist_internal::hashString(const std::string& s)  {
    uint64_t hash = 14695981039346656037ull;

    for (unsigned char c : s) {
        hash ^= c;
        hash *= 1099511628211ull;
    }

    return hash;
}