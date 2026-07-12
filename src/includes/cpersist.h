#pragma once
#include <string>
#include <type_traits>
#include <unordered_map>
#include <sstream>
#include <ostream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <iostream>
#include <filesystem>
#include <optional>
#include <fstream>
#include <span>
#include "error_handler.h"
#include "serializer.h"

namespace cpersist {
    template<typename T>
    concept hasSerialize = requires(T& t, const std::string& parent) {
        t.serialize(parent);
    };

    template<typename T>
    concept hasDeserialize = requires(T& t, const std::string& parent) {
        t.deserialize(parent);
    };
}
namespace cpersist_internal {
    uint64_t hashString(const std::string& s);
}

class SaveManager {
private:
    std::string current_file;
    std::unordered_map<std::string, std::stringstream> files;
    bool debugMode = true;
    void debugLog(const std::string& message) {
        if (!debugMode) {return;}
        std::cout << "[CPERSIST LOG] " << message << std::endl;
    }
    std::string fileExtension = ".bin";
    std::string folderName = "cpersist_data";

    SaveManager() = default;   // Private constructor
    ~SaveManager() = default;  // Optional
public:
    // === SINGLETON PROPERTIES
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;

    
    static SaveManager& get() {
        static SaveManager instance;
        return instance;
    }



    void init(const bool loadPresentFiles = true, std::optional<std::span<std::string>> initialFiles = std::nullopt);
    void loadExistingFiles();

    bool filename_fits_standards(const std::string& filename); // check if the filename fits the naming standards
    void make_filename_safe(std::string& filename);

    bool change_file(const std::string& new_file);             // changes the current file
    void change_file_safe(const std::string& new_file);        // creates file if it doesn't exist, then moves to it in either case
    bool create_new_file(const std::string& new_file);         // creates a new file
    bool file_exists(const std::string& filename);

    // WRITING
    template<typename T>
    void write(const std::string& name, const T& object, const uint64_t& parent=0) {
        if (current_file.empty()) {
            cpersist_internal::ErrorManager::get().throwError("Can't write data while no file is chosen.");
        }
        uint64_t namehash = cpersist_internal::hashString(parent==0? name : std::to_string(parent) + "." + name);
        
        std::stringstream dataStream(std::ios::in | std::ios::out | std::ios::binary); // those flags are to allow input, output, and make reading in binary

        if constexpr (cpersist::hasSerialize<T>) { // we need a constexpr because otherwise we would have a compiler-time error
            object.serialize(namehash); // object contains a serialize function
            return;
        } else {
            cpersist::Serializer<T>::write(dataStream, object);
        }

        dataStream.seekg(0, std::ios::end);
        if (dataStream.tellg() == std::streampos(-1)) {
            cpersist_internal::ErrorManager::get().throwError("Serialization failed.");
        }

        std::string serializedData = dataStream.str();
        uint32_t dataSize = serializedData.size();

        
        if (file_exists(current_file)) {
            uint64_t dataPosition = getDataPosition(namehash);
            if (dataPosition != -1) { // data exists. modify it.
                writeBytesIntoFile(serializedData.data(), dataSize, dataPosition);
                return; // already modified existing entry; don't append a new one
            }
        }

        std::stringstream& file = files[current_file];
        file.write(reinterpret_cast<const char*>(&namehash), sizeof(uint64_t));  // write the name hash, always 8 bytes thanks to hashing
        file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize)); // then write the data size
        file.write(serializedData.data(), serializedData.size());               // then write the data
    };
    uint64_t getDataPosition(const uint64_t& namehash);
    std::vector<uint8_t> readFileAsBinary(const std::string& filename);
    void writeBytesIntoFile(const char* bytes, const std::uint32_t size, const std::uint64_t position);

    // READING
    template<typename T>
    T read(const std::string& name, const uint64_t& parent=0, std::optional<T> defaultValue = std::nullopt) {
        if (current_file.empty()) {
            cpersist_internal::ErrorManager::get().throwError("Can't read data while no file is chosen.");
        }
        uint64_t namehash = cpersist_internal::hashString(parent==0? name : std::to_string(parent) + "." + name);

        if constexpr (cpersist::hasDeserialize<T>) {
            T object;
            object.deserialize(namehash);
            return object;
        }

        uint64_t dataPosition = getDataPosition(namehash);
        
        if (dataPosition == static_cast<uint64_t>(-1)) {
            if (defaultValue) {return *defaultValue;} // not found
            cpersist_internal::ErrorManager::get().throwError("Entry \"" + std::string(parent==0? name : std::to_string(parent) + "." + name) +"\" not found.");
        }

        std::ifstream file(std::filesystem::path(folderName) / (current_file + fileExtension), std::ios::binary);
        if (!file) {
            cpersist_internal::ErrorManager::get().throwError("Can't read file " + current_file + fileExtension + "; it might be deleted or corrupted");
        }

        file.seekg(dataPosition);
        uint32_t dataSize;
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize)); // read data size

        std::string buffer(dataSize, '\0'); // make a string of dataSize many characters, each of which is initially 00
        file.read(buffer.data(), dataSize);

        std::stringstream dataStream(buffer);

        T object;
        cpersist::Serializer<T>::read(dataStream, object);
        return object;
    }
    bool file_contains_data(const std::string& dataname);

    // COMMIT
    void commit();

    // LOGGERS
    void log_filenames();
    void log_current_filename();
    
    // GETTERS
    const std::string& get_current_file();
    const std::string& get_file_extension();

    // SETTERS
    void set_file_extension(const std::string& new_extension);
};

inline SaveManager& saveMgr = SaveManager::get();