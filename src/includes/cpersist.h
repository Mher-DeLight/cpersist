#pragma once
#include <string>
#include <type_traits>
#include <unordered_map>
#include <sstream>
#include <ostream>
#include "error_handler.h"
#include <cstring>
#include <cstdint>
#include <vector>
#include <iostream>

namespace cpersist {
    template<typename T>
    concept hasSerialize = requires(T t) {
        t.serialize();
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
public:
    bool filename_fits_standards(const std::string& filename); // check if the filename fits the naming standards
    void make_filename_safe(std::string& filename);

    bool change_file(const std::string& new_file);             // changes the current file
    void change_file_safe(const std::string& new_file);        // creates file if it doesn't exist, then moves to it in either case
    bool create_new_file(const std::string& new_file);         // creates a new file

    // WRITING
    template<typename T>
    void write(const char* name, const T& object) {
        if (current_file.empty()) {
            cpersist_internal::ErrorManager::get().throwError("Can't write data while no file is chosen.");
        }

        std::stringstream dataStream(std::ios::in | std::ios::out | std::ios::binary); // those flags are to allow input, output, and make reading in binary

        if constexpr (cpersist::hasSerialize<T>) { // we need a constexpr because otherwise we would have a compiler-time error
            object.serialize(dataStream); // object contains a serialize function
        } else {
            static_assert(std::is_trivially_copyable_v<T>,
                  "Type must be trivially copyable or implement serialize(). Types such as std::vector or std::map cannot be encoded as they contain \
                  pointers and dynamically allocated memory.");
                  const char* data = reinterpret_cast<const char*>(&object);
                  dataStream.write(data, sizeof(data)); // reinterpreting as char* turns it into raw bytes
        }

        dataStream.seekg(0, std::ios::end);
        if (dataStream.tellg() == std::streampos(-1)) {
            cpersist_internal::ErrorManager::get().throwError("Serialization failed.");
        }

        std::uint64_t dataSize = dataStream.tellg();
        dataStream.seekg(0, std::ios::beg);

        std::uint64_t nameSize = std::strlen(name);

        std::stringstream& file = files[current_file];
        file.write(reinterpret_cast<const char*>(&nameSize), sizeof(nameSize)); // write name size first
        file.write(name, nameSize);                                             // then the name
        file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize)); // then write the data size
        file << dataStream.rdbuf();                                             // then write the data
    };
    int getDataPosition(const std::string& name);
    std::vector<uint8_t> readFileAsBinary(const std::string& filename);

    // COMMIT
    void commit();

    void log_filenames();
    void log_current_filename();
    

    std::string& get_current_file();
};