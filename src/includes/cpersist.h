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
#include "aes.h"


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
    std::vector<uint8_t> hashString(const std::string& s);
}

class SaveManager {
private:
    class Field {
    public:
        Field(const std::string& fieldname, std::vector<uint8_t>& fieldvalue):
            name(fieldname), value(fieldvalue) {}

        std::string name;
        std::vector<uint8_t> value;
    };

    std::string current_file;
    std::unordered_map<std::string, std::vector<Field>> files;
    bool debugMode = true;
    void debugLog(const std::string& message) {
        if (!debugMode) {return;}
        std::cout << "[CPERSIST LOG] " << message << std::endl;
    }
    std::string fileExtension = ".bin";
    std::string folderName = "savedata";
    std::filesystem::path fullFilePath;
    bool encryption_enabled = true;

    SaveManager() {
        init(true);
    };
    ~SaveManager() = default;
    void init(const bool loadPresentFiles = true, std::optional<std::span<std::string>> initialFiles = std::nullopt);

    std::vector<uint8_t> toBytes(uint64_t value) {
        return {
            static_cast<uint8_t>(value >> 56),
            static_cast<uint8_t>(value >> 48),
            static_cast<uint8_t>(value >> 40),
            static_cast<uint8_t>(value >> 32),
            static_cast<uint8_t>(value >> 24),
            static_cast<uint8_t>(value >> 16),
            static_cast<uint8_t>(value >> 8),
            static_cast<uint8_t>(value)
        };
    }
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

        std::string dataString = dataStream.str();
        std::vector<uint8_t> serialized(dataString.begin(),dataString.end());
        uint32_t dataSize = serialized.size();
        uint8_t nameSize = fullname.size();

        
        if (file_exists(current_file)) {
            uint64_t dataPosition = getDataPosition(fullname);
            if (dataPosition != -1) { // data exists. modify it.
                writeBytesIntoFile(reinterpret_cast<const char*>(serialized.data()), dataSize, dataPosition);
                return; // already modified existing entry; don't append a new one
            }
        }

        Field field(fullname.data(), serialized);
        files[current_file].push_back(field);
    };
    uint64_t getDataPosition(const std::string& name, const bool loose = false);
    std::vector<uint8_t> readFileAsBinary(const std::string& filename);
    void writeBytesIntoFile(const char* bytes, const std::uint32_t size, const std::uint64_t position, const bool encrypt = true);
    bool isFileEncrypted();

    // READING
    template<typename T>
    T read(const std::string& name, std::optional<T> defaultValue = std::nullopt, const std::string& parent = "") {
        if (current_file.empty()) {
            cpersist_internal::ErrorManager::get().throwError("Can't read data while no file is chosen.");
        }

        std::string fullname = parent.empty() ? name : parent + "." + name;

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
            if (defaultValue)
                return *defaultValue;

            cpersist_internal::ErrorManager::get().throwError("Entry \"" + fullname + "\" not found.");
        }

        // data is already decrypted
        std::vector<uint8_t> data = readFileAsBinary(current_file);

        if (dataPosition + sizeof(uint32_t) > data.size()) {
            cpersist_internal::ErrorManager::get().throwError(
                "Corrupted save file.");
        }

        uint32_t dataSize;
        std::memcpy(
            &dataSize,
            data.data() + dataPosition,
            sizeof(dataSize)); // copy the data into memory so we can modify it freely
        dataPosition += sizeof(uint32_t);

        if (dataPosition + dataSize > data.size()) {
            cpersist_internal::ErrorManager::get().throwError("Corrupted save file.");
        }

        std::stringstream stream(std::string(reinterpret_cast<char*>(data.data() + dataPosition), dataSize));

        T object;
        cpersist::Serializer<T>::read(stream, object);

        return object;
    }
    template<typename T>
    void read_into(const std::string& name, T& result_into, std::optional<T> defaultValue = std::nullopt, const std::string& parent = "") {
        result_into = read<T>(name, defaultValue, parent);
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
    void enable_encryption(const bool enable);
    void set_encryption_key(const std::string& key);
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