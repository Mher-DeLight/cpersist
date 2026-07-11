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
#include "error_handler.h"

namespace cpersist {
    template<typename T>
    concept hasSerialize = requires(T t) {
        t.serialize();
    };

    template<typename T>
    concept hasDeserialize = requires(T t) {
        t.deserialize();
    };
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
public:
    bool filename_fits_standards(const std::string& filename); // check if the filename fits the naming standards
    void make_filename_safe(std::string& filename);

    bool change_file(const std::string& new_file);             // changes the current file
    void change_file_safe(const std::string& new_file);        // creates file if it doesn't exist, then moves to it in either case
    bool create_new_file(const std::string& new_file);         // creates a new file

    // WRITING
    template<typename T>
    void write(const std::string& name, const T& object, const std::string& parent="") {
        if (current_file.empty()) {
            cpersist_internal::ErrorManager::get().throwError("Can't write data while no file is chosen.");
        }
        std::string full_name = parent.empty()? name : parent + "." + name;
        
        std::stringstream dataStream(std::ios::in | std::ios::out | std::ios::binary); // those flags are to allow input, output, and make reading in binary

        if constexpr (cpersist::hasSerialize<T>) { // we need a constexpr because otherwise we would have a compiler-time error
            object.serialize(full_name); // object contains a serialize function
            return;
        } else {
            static_assert(std::is_trivially_copyable_v<T>,
                  "Type must be trivially copyable or implement serialize(). Types such as std::vector or std::map cannot be encoded as they contain \
                  pointers and dynamically allocated memory.");
                  const char* data = reinterpret_cast<const char*>(&object);
                  dataStream.write(data, sizeof(object)); // reinterpreting as char* turns it into raw bytes
        }

        dataStream.seekg(0, std::ios::end);
        if (dataStream.tellg() == std::streampos(-1)) {
            cpersist_internal::ErrorManager::get().throwError("Serialization failed.");
        }

        std::string serializedData = dataStream.str();
        uint32_t dataSize = serializedData.size();

        
        if (std::filesystem::is_regular_file(std::string(current_file + fileExtension))) {
            uint64_t dataPosition = getDataPosition(full_name);
            if (dataPosition != -1) { // data exists. modify it.
                writeBytesIntoFile(serializedData.data(), dataSize, dataPosition);
                return; // already modified existing entry; don't append a new one
            }
        }

        uint8_t nameSize = std::strlen(full_name.data());

        std::stringstream& file = files[current_file];
        file.write(reinterpret_cast<const char*>(&nameSize), sizeof(nameSize)); // write name size first
        file.write(full_name.data(), nameSize);                                             // then the name
        file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize)); // then write the data size
        file.write(serializedData.data(), serializedData.size());               // then write the data
    };
    uint64_t getDataPosition(const std::string& full_name, bool skipDataSize = false);
    std::vector<uint8_t> readFileAsBinary(const std::string& filename);
    void writeBytesIntoFile(const char* bytes, const std::uint32_t size, const std::uint64_t position);

    // READING
    template<typename T>
    T read(const std::string& name, const std::string& parent="", std::optional<T> defaultValue = std::nullopt) {
        if (current_file.empty()) {
            cpersist_internal::ErrorManager::get().throwError("Can't read data while no file is chosen.");
        }
        std::string full_name = parent.empty() ? name : parent + "." + name;

        uint64_t dataPosition = getDataPosition(full_name);
        
        if (dataPosition == static_cast<uint64_t>(-1)) {
            if (defaultValue) {return *defaultValue;} // not found
            cpersist_internal::ErrorManager::get().throwError("Entry \"" + std::string(full_name) +"\" not found.");
        }

        std::ifstream file(std::string(current_file + fileExtension), std::ios::binary);
        if (!file) {
            cpersist_internal::ErrorManager::get().throwError("Can't read file " + current_file + fileExtension + "; it might be deleted or corrupted");
        }

        file.seekg(dataPosition);
        uint32_t dataSize;
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize)); // read data size

        std::string buffer(dataSize, '\0'); // make a string of dataSize many characters, each of which is initially 00
        file.read(buffer.data(), dataSize);

        std::stringstream dataStream(buffer);

        if constexpr (cpersist::hasDeserialize<T>) {
            T object;
            object.deserialize(full_name);
            return object;
        } else {
            static_assert(std::is_trivially_copyable_v<T>,
                "Type must be trivially copyable or implement deserialize().");

            if (dataSize != sizeof(T)) {
                cpersist_internal::ErrorManager::get().throwError(
                    "Stored object has incorrect size.");
            }

            T object;
            std::memcpy(&object, buffer.data(), sizeof(T));
            return object;
        }
    }
    bool file_contains_data(const std::string& dataname) {
        return getDataPosition(dataname) != static_cast<uint64_t>(-1);
    }

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