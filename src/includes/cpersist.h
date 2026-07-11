#pragma once
#include <string>
#include <type_traits>
#include <unordered_map>
#include <sstream>
#include <ostream>
#include "error_handler.h"

namespace cpersist {
    template<typename T>
    concept hasSerialize = requires(T t) {
        t.hasSerialize();
    };
}

class SaveManager {
private:
    std::string current_file;
    std::unordered_map<std::string, std::stringstream> files;
public:
    bool filename_fits_standards(const std::string& filename); // check if the filename fits the naming standards
    void make_filename_safe(std::string& filename);

    bool change_file(const std::string& new_file);            // changes the current file
    void change_file_safe(const std::string& new_file);       // creates file if it doesn't exist, then moves to it in either case
    bool create_new_file(const std::string& new_file);        // creates a new file

    // WRITING
    template<typename T>
    void write(const T& object, const bool& custom_serialize = false) {
        if (current_file.empty()) {
            cpersist_internal::ErrorManager::get().throwError("Can't write data while no file is chosen.");
        }

        if constexpr (cpersist::hasSerialize<T>) { // we need a constexpr because otherwise we would have a compiler-time error
            object.serialize(files[current_file]); // object contains a serialize function
        } else {
            static_assert(std::is_trivially_copyable_v<T>,
                  "Type must be trivially copyable or implement serialize(). Types such as std::vector or std::map cannot be encoded as they contain \
                  pointers and dynamically allocated memory.");
            files[current_file].write(reinterpret_cast<const char*>(&object), sizeof(object)); // reinterpreting as char* turns it into raw bytes
        }
    };
    
    void write(const std::string& str);

    // COMMIT
    void commit();


    void log_filenames();
    void log_current_filename();
    

    std::string& get_current_file();
};