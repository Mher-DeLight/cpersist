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
#include <initializer_list>
#include "error_handler.h"
#include "serializer.h"

class WriteArchive;
class ReadArchive; // forward declaration so we can use them in hasArchive

namespace cpersist {
    template<typename T>
    concept hasWrite = requires(T& t, const std::string& parent) {
        t.write(parent);
    };

    template<typename T>
    concept hasRead = requires(T& t, const std::string& parent) {
        t.read(parent);
    };

    template<typename T>
    concept hasArchive =
        requires(T& t, WriteArchive& war) {
            t.archive(war);
        } &&
        requires(T& t, ReadArchive& rar) {
            t.archive(rar);
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
    std::string folderName = "savedata";
    std::filesystem::path fullFilePath;

    SaveManager() {
        init(true);
    };   // Private constructor
    ~SaveManager() = default;
    void init(const bool loadPresentFiles = true, std::optional<std::span<std::string>> initialFiles = std::nullopt);
public:
    // === SINGLETON PROPERTIES
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;

    
    static SaveManager& get() {
        static SaveManager instance;
        return instance;
    };



    void loadExistingFiles();

    bool filename_fits_standards(const std::string& filename); // check if the filename fits the naming standards
    void make_filename_safe(std::string& filename);

    bool change_file(const std::string& new_file);             // changes the current file
    void change_file_safe(const std::string& new_file);        // creates file if it doesn't exist, then moves to it in either case
    bool create_new_file(const std::string& new_file);         // creates a new file
    bool file_exists(const std::string& filename);
    bool open(const std::string& filename);
    void ensure_exists(std::initializer_list<std::string> filenames);
    void ensure_exists(std::vector<std::string> filenames);

    // WRITING
    template<typename T>
    void write(const std::string& name, const T& object, const std::string& parent="") {
        if (current_file.empty()) {
            cpersist_internal::ErrorManager::get().throwError("Can't write data while no file is chosen.");
        }
        std::string fullname = parent.empty()? name : parent + "." + name;
        
        std::stringstream dataStream(std::ios::in | std::ios::out | std::ios::binary); // those flags are to allow input, output, and make reading in binary

        if constexpr (cpersist::hasArchive<T>) {
            WriteArchive ar(fullname);
            const_cast<T&>(object).archive(ar);
            return;
        }
        if constexpr (cpersist::hasWrite<T>) { // we need a constexpr because otherwise we would have a compiler-time error
            const_cast<T&>(object).write(fullname); // object contains a serialize function
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
        uint8_t nameSize = fullname.size();

        
        if (file_exists(current_file)) {
            uint64_t dataPosition = getDataPosition(fullname);
            if (dataPosition != -1) { // data exists. modify it.
                writeBytesIntoFile(serializedData.data(), dataSize, dataPosition);
                return; // already modified existing entry; don't append a new one
            }
        }

        std::stringstream& file = files[current_file];
        file.write(reinterpret_cast<const char*>(&nameSize), sizeof(uint8_t));
        file.write(fullname.data(), nameSize);  // write the name hash, always 8 bytes thanks to hashing
        file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize)); // then write the data size
        file.write(serializedData.data(), serializedData.size());               // then write the data
    };
    uint64_t getDataPosition(const std::string& name, const bool loose = false);
    std::vector<uint8_t> readFileAsBinary(const std::string& filename);
    void writeBytesIntoFile(const char* bytes, const std::uint32_t size, const std::uint64_t position);

    // READING
    template<typename T>
    T read(const std::string& name, std::optional<T> defaultValue = std::nullopt, const std::string& parent="") {
        if (current_file.empty()) {
            cpersist_internal::ErrorManager::get().throwError("Can't read data while no file is chosen.");
        }
        std::string fullname = parent.empty()? name : parent + "." + name;

        if constexpr (cpersist::hasArchive<T>) {
            T object;
            ReadArchive ar(fullname);
            object.archive(ar);
            return object;
        }

        if constexpr (cpersist::hasRead<T>) {
            T object;
            object.read(fullname);
            return object;
        }
        
        
        uint64_t dataPosition = getDataPosition(fullname);
        
        if (dataPosition == static_cast<uint64_t>(-1)) {
            if (defaultValue) {return *defaultValue;} // not found
            cpersist_internal::ErrorManager::get().throwError("Entry \"" + std::string(parent.empty()? name : parent + "." + name) +"\" not found.");
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
    template<typename T>
    void read_into(const std::string& name, T& result_into, std::optional<T> defaultValue = std::nullopt) {
        result_into = read<T>(name, defaultValue);
    }
    
    bool contains(const std::string& dataname);

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

// ARCHIVES

class Archive {
protected: // we don't want others to access it, only it and its children
    std::string parent;

public:
    explicit Archive(std::string parent)
        : parent(std::move(parent)) {}
};

class WriteArchive : public Archive {
public:
    using Archive::Archive; // inherit the constructors too

    template<typename T>
    void operator()(const std::string& key, T& value)
    {
        saveMgr.write(key, value, parent);
    }
};

class ReadArchive : public Archive {
public:
    using Archive::Archive;

    template<typename T>
    void operator()(const std::string& key, T& value)
    {
        value = saveMgr.read<T>(key, std::nullopt, parent);
    }
};