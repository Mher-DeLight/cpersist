#include <iostream>
#include "includes/cpersist.h"
#include <algorithm>

void SaveManager::init() {
    std::filesystem::create_directory(folderName); // creates the folder if it doesn't exist
    
    loadExistingFiles();

    for (auto& pair : files) {
        files[pair.first] = readFile(pair.first); // load already written data so truncate doesn't overwrite it
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
    return files.contains(filename);
}
bool SaveManager::file_exists_on_disk(const std::string& filename) {
    std::filesystem::path fp = std::filesystem::path(folderName) / (filename + fileExtension); 
    
    if (std::filesystem::exists(fp) && std::filesystem::is_regular_file(fp)) {
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
    if (!filename_fits_standards(current_file)) {
        make_filename_safe(current_file);
    }
    fullFilePath = std::filesystem::path(folderName) / (current_file + fileExtension);
    
    if (!files.count(current_file)) {
        files.try_emplace(current_file);
    }
}
bool SaveManager::create_new_file(const std::string& new_file) {
    if (!filename_fits_standards(new_file)) {return false;} // doesn't fit naming standards
    
    if (file_exists(new_file)) {
        return false; // file already exists
    }
    
    files.try_emplace(new_file);
    return true;
}
bool SaveManager::open(const std::string& filename) {
    if (!filename_fits_standards(filename)) {return false;} // doesn't fit naming standards
    if (!files.count(filename)) { // file doesn't exist, create it
        files.try_emplace(filename);
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
    uint64_t position = 0; // encryption flag is already skipped

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
std::vector<uint8_t> SaveManager::readFileAsBinary(const std::string& filename) {
    bool fileEncr = isFileEncrypted(filename);
    std::filesystem::path customFilePath = std::filesystem::path(folderName) / (filename + fileExtension);
    std::ifstream file(customFilePath, std::ios::binary);

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
    bytes.erase(bytes.begin());
    
    if (fileEncr) {
        bytes = encrMgr.decrypt(bytes);
    }
    
    return bytes;
}
bool SaveManager::contains(const std::string& dataname, const bool loose) {
    auto fileIt = files.find(current_file);
    if (fileIt == files.end()) {
        cpersist_internal::ErrorManager::get().throwError("Current file is not loaded.");
    }

    const auto& fields = fileIt->second;

    auto fieldIt = std::find_if(fields.begin(), fields.end(),
        [&](const Field& field) {
            return (field.name == dataname) || (field.name.starts_with(dataname + ".") && loose);
        });

    return fieldIt != fields.end();
}
bool SaveManager::contains(const std::initializer_list<std::string>& datanames, const bool loose) {
    auto fileIt = files.find(current_file);
    if (fileIt == files.end()) {
        cpersist_internal::ErrorManager::get().throwError("Current file is not loaded.");
    }

    const auto& fields = fileIt->second;

    for (const auto& dataname : datanames) {
        auto fieldIt = std::find_if(fields.begin(), fields.end(),
            [&](const Field& field) {
                return (field.name == dataname) || (field.name.starts_with(dataname + ".") && loose);
            });

        if (fieldIt == fields.end()) {
            return false;
        }
    }

    return true;
}
bool SaveManager::isFileEncrypted(const std::string& filename) {
    std::string curFp = filename.empty()? fullFilePath : (std::filesystem::path(folderName) / (filename + fileExtension));
    std::ifstream file(curFp, std::ios::binary);

    if (!file) {
        cpersist_internal::ErrorManager::get().throwError("Failed to open file: " + current_file);
    }

    uint8_t encryptionMagicByte;
    if (!file.read(reinterpret_cast<char*>(&encryptionMagicByte), 1)) {
        cpersist_internal::ErrorManager::get().throwError("Cannot read data file " + current_file);
    }

    return encryptionMagicByte != 0x00;
}
std::vector<Field> SaveManager::readFile(const std::string& filename) {
    if (!file_exists(filename)) {
        cpersist_internal::ErrorManager::get().throwError("Cannot parse file \"" + filename + fileExtension + "\"; it is either \
            deleted, corrupted, or not loaded into the buffer.");
    }

    if (isFileEncrypted(filename) && encrMgr.encryKeyEmpty()) {
        return std::vector<Field>();
    }

    std::vector<uint8_t> data = readFileAsBinary(filename);
    
    std::vector<Field> fields;
    uint64_t position = 0;
    while (position < data.size()) {
        // ===== NAME
        // check bounds
        if (position + sizeof(uint8_t) > data.size()) {
            cpersist_internal::ErrorManager::get().throwError("file " + filename + fileExtension + " is incorrectly formatted");
        }

        uint8_t nameSize;
        std::memcpy(&nameSize, data.data() + position, sizeof(nameSize));
        position += sizeof(nameSize);


        // ===== NAME
        // check bounds
        if (position + nameSize > data.size()) {
            cpersist_internal::ErrorManager::get().throwError("file " + filename + fileExtension + " is incorrectly formatted");
        }

        std::string currentName(
            reinterpret_cast<const char*>(data.data() + position),
            nameSize
        );
        position += nameSize;

        // ===== DATASIZE
        if (position + sizeof(uint32_t) > data.size()) {
            cpersist_internal::ErrorManager::get().throwError("file " + filename + fileExtension + " is incorrectly formatted");
        }

        uint32_t dataSize;
        std::memcpy(&dataSize, data.data() + position, sizeof(dataSize));
        position += sizeof(dataSize);

        // the position now points at the data itself
        if (position + dataSize > data.size()) {
            cpersist_internal::ErrorManager::get().throwError("file " + filename + fileExtension + " is incorrectly formatted");
        }

        // copy the data into a new vector
        std::vector<uint8_t> fieldData(data.begin() + position,data.begin() + position + dataSize);

        // construct and store the field. we'll use emplace to avoid making a temporary Field object
        fields.emplace_back(currentName, fieldData);

        // Move to the next field
        position += dataSize;
    }
    return fields;
}

// COMMIT
void SaveManager::commit() {
    if (current_file.empty()) {
        cpersist_internal::ErrorManager::get().throwError("Can't commit changes while no file is open.");
    }
    std::ofstream file(fullFilePath, std::ios::binary | std::ios::trunc); // write into <current_file>.<ext>, append if already exists
    
    if (!file) {
        cpersist_internal::ErrorManager::get().throwError("Unable to commit to file \"" + fullFilePath.string() + ".\"");
        return;
    }

    file.write(reinterpret_cast<const char*>(&encryption_enabled), sizeof(encryption_enabled));

    std::vector<uint8_t> bytes;
    std::vector<Field> fields = files[current_file];
    for (auto& field : fields) {
        bytes.push_back(field.name.size());
        for (auto& c : field.name) {
            bytes.push_back(c);
        }

        uint32_t valueSize = static_cast<uint32_t>(field.value.size());
        std::vector<std::uint8_t> valueSizeVector = {
            static_cast<std::uint8_t>((valueSize      ) & 0xFF),
            static_cast<std::uint8_t>((valueSize >> 8 ) & 0xFF),
            static_cast<std::uint8_t>((valueSize >> 16) & 0xFF),
            static_cast<std::uint8_t>((valueSize >> 24) & 0xFF)
        };
        for (auto byt : valueSizeVector) {
            bytes.push_back(byt);
        }
        
        for (auto& vl : field.value) {
            bytes.push_back(vl);
        }
    }

    if (encryption_enabled) {
        bytes = encrMgr.encrypt(bytes);
    }

    file.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );
}
void SaveManager::erase(const std::string& fieldname) {
    auto fileIt = files.find(current_file);
    if (fileIt == files.end()) {
        cpersist_internal::ErrorManager::get().throwError("Current file is not loaded.");
    }

    auto& fields = fileIt->second;

    auto fieldIt = std::find_if(fields.begin(), fields.end(),
        [&](const Field& field) {
            return (field.name == fieldname) || field.name.starts_with(fieldname + ".");
        });
    
    if (fieldIt == fields.end()) {
        cpersist_internal::ErrorManager::get().throwError("Cannot delete field \"" + fieldname + "\" as it is nonexistent.");
        return;
    }

    fields.erase(fieldIt);
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
void SaveManager::set_encryption_key(const std::string& key) {
    encrMgr.setEncryptionKey(cpersist_internal::hashString(key));
    init(); // reinit to parse existing files into the buffer
}
void SaveManager::enable_encryption(const bool enable) {
    encryption_enabled = enable;
}
void SaveManager::enable_autocommit_on_exit(const bool enable) {
    commitOnDestroy = enable;
}

std::vector<uint8_t> cpersist_internal::hashString(const std::string& str) {
    constexpr uint64_t seeds[4] = {
        0x243F6A8885A308D3ULL,
        0x13198A2E03707344ULL,
        0xA4093822299F31D0ULL,
        0x082EFA98EC4E6C89ULL
    };

    std::vector<uint8_t> key;
    key.reserve(32);

    for (uint64_t seed : seeds)
    {
        uint64_t h = seed;

        for (unsigned char c : str)
        {
            h ^= c;
            h *= 0x100000001B3ULL;      // FNV prime
            h ^= h >> 32;
            h *= 0x9E3779B185EBCA87ULL; // extra mixing
        }

        for (int i = 0; i < 8; ++i)
            key.push_back(static_cast<uint8_t>(h >> (i * 8)));
    }

    return key;
}