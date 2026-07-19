#pragma once
#include <string>
#include <type_traits>
#include <unordered_map>
#include <sstream>
#include <ostream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <iostream>
#include <filesystem>
#include <optional>
#include <fstream>  
#include <span>
#include <initializer_list>
#include <memory>
#include "error_handler.h"
#include "serializer.h"
#include "aes.h"

class ReadArchive;
class WriteArchive;

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

class Archive {
protected: // we don't want others to access it, only it and its children
    std::string parent;

public:
    explicit Archive(std::string parent)
        : parent(std::move(parent)) {}
};

class ByteArchive : public Archive {
public:
    using Archive::Archive;

    std::vector<uint8_t> bytes;

    template<typename T>
    void operator()(const std::string& key, T& value) {
        std::stringstream dataStream(std::ios::in | std::ios::out | std::ios::binary);
        std::string fullname = parent.empty() ? key : parent + "." + key;

        if constexpr (cpersist::hasArchive<T>) {
            ByteArchive ba(fullname);
            value.archive(ba);
            for (auto& byte : ba.bytes) {
                bytes.push_back(byte);
            }
            return;
        }

        cpersist::Serializer<T>::write(dataStream, value);
        std::string data = dataStream.str();


        bytes.push_back(static_cast<uint8_t>(fullname.size()));
        for (auto& c : fullname) {
            bytes.push_back(c);
        }

        uint32_t dataSize = static_cast<uint32_t>(data.size());
        std::vector<std::uint8_t> dataSizeVector = {
            static_cast<std::uint8_t>((dataSize      ) & 0xFF),
            static_cast<std::uint8_t>((dataSize >> 8 ) & 0xFF),
            static_cast<std::uint8_t>((dataSize >> 16) & 0xFF),
            static_cast<std::uint8_t>((dataSize >> 24) & 0xFF)
        };
        for (auto byt : dataSizeVector) {
            bytes.push_back(byt);
        }
        bytes.insert(bytes.end(), data.begin(), data.end());

    }
};



namespace cpersist_internal {
    std::vector<uint8_t> hashString(const std::string& s);
}

class Field {
public:
    virtual ~Field() = default;
    virtual void commit(std::vector<uint8_t>& bytes) = 0;
    std::string name;

    Field(const std::string& fieldname):
        name(fieldname) {}
};
class ValField : public Field {
public:
    ValField(const std::string& fieldname, std::vector<uint8_t> fieldvalue):
        Field(fieldname), value(std::move(fieldvalue)) {}

    std::vector<uint8_t> value;
    
    void commit(std::vector<uint8_t>& bytes) override {
        bytes.push_back(name.size());
        for (auto& c : name) {
            bytes.push_back(c);
        }

        uint32_t valueSize = static_cast<uint32_t>(value.size());
        std::vector<std::uint8_t> valueSizeVector = {
            static_cast<std::uint8_t>((valueSize      ) & 0xFF),
            static_cast<std::uint8_t>((valueSize >> 8 ) & 0xFF),
            static_cast<std::uint8_t>((valueSize >> 16) & 0xFF),
            static_cast<std::uint8_t>((valueSize >> 24) & 0xFF)
        };
        for (auto byt : valueSizeVector) {
            bytes.push_back(byt);
        }
        
        for (auto& vl : value) {
            bytes.push_back(vl);
        }
    }
};
template<typename T>
class RefField : public Field {
public:
    RefField(const std::string& fieldname, T& obj):
        Field(fieldname), object(obj) {}

    T& object;
    void commit(std::vector<uint8_t>& bytes) override {
        ByteArchive ba("");
        ba(name, object);
        bytes.insert(bytes.end(), ba.bytes.begin(), ba.bytes.end());
        return;
    }
};


class SaveManager {
private:
    template <typename T>
    using uq = std::unique_ptr<T>;


    std::string current_file;
    std::unordered_map<std::string, std::vector<uq<Field>>> files;
    std::string fileExtension = ".bin";
    std::string folderName = "savedata";
    std::filesystem::path fullFilePath;
    std::vector<uq<Field>> readFile(const std::string& filename);
    
    bool debugMode = true;
    bool encryption_enabled = true;
    bool commitOnDestroy = false;
    
       
    void debugLog(const std::string& message) {
        if (!debugMode) {return;}
        std::cout << "[CPERSIST LOG] " << message << std::endl;
    }
    
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


    SaveManager() {
        init();
    };
    ~SaveManager() {
        if (commitOnDestroy) {
            try {
                commit();
            } catch (...) {}
        }
    };
public:
    void init();
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
    bool file_exists_on_disk(const std::string& filename);
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
            for (auto& fd : files[current_file]) {
                if (fd->name == fullname) {
                    if (auto* vF = dynamic_cast<ValField*>(fd.get())) {
                        vF->value = serialized;
                    } else {
                        cpersist_internal::ErrorManager::get().throwError("cannot overwrite Reference Field object");
                    }
                    return;
                }
            }
        }

        auto field = std::make_unique<ValField>(fullname.data(), serialized);
        files[current_file].push_back(std::move(field));
    };
    uint64_t getDataPosition(const std::string& name, const bool loose = false);
    std::vector<uint8_t> readFileAsBinary(const std::string& filename);
    bool isFileEncrypted(const std::string& filename = "");
    
    template<typename T>
    void sync(const std::string& name, T& value) {
        if (contains(name)) {
            read_into(name, value);
        } else {
            write(name, value);
        }
    }

    template<typename T>
    void link(const std::string& name, T& object, const std::string& parent = "") {
        auto fd = std::make_unique<RefField<T>>(parent.empty()? name : parent + "." + name, object);
        files[current_file].push_back(std::move(fd));
    }

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

        auto fileIt = files.find(current_file);
        if (fileIt == files.end()) {
            cpersist_internal::ErrorManager::get().throwError("Current file is not loaded.");
        }

        const auto& fields = fileIt->second;

        auto fieldIt = std::find_if(fields.begin(), fields.end(),
            [&](const uq<Field>& field) {
                return field->name == fullname;
            });

        if (fieldIt == fields.end()) {
            if (defaultValue)
                return *defaultValue;

            cpersist_internal::ErrorManager::get().throwError("Entry \"" + fullname + "\" not found.");
        }

        std::stringstream stream(std::ios::binary | std::ios::in);
        if (auto* valueField = dynamic_cast<ValField*>(fieldIt->get())) {
            stream.write(reinterpret_cast<const char*>(valueField->value.data()), static_cast<std::streamsize>(valueField->value.size()));
        } else if (auto* refField = dynamic_cast<RefField<T>*>(fieldIt->get())) {
            cpersist::Serializer<T>::write(stream, refField->object);
        } else {
            cpersist_internal::ErrorManager::get().throwError("unidentified field derivative class, cannot read");
        }
        stream.seekg(0);

        
        T object;
        cpersist::Serializer<T>::read(stream, object);
        return object;
    }
    template<typename T>
    void read_into(const std::string& name, T& result_into, std::optional<T> defaultValue = std::nullopt, const std::string& parent = "") {
        result_into = read<T>(name, defaultValue, parent);
    }
    template<typename T, typename S>
    void read_into_stream(const std::string& name, S& stream, std::optional<T> defaultValue = std::nullopt, const std::string& parent = "") {
        stream << read<T>(name, defaultValue, parent);
    }

    bool contains(const std::string& dataname, const bool loose = true);
    bool contains(const std::initializer_list<std::string>& datanames, const bool loose = true);

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
    void set_encryption_key(const std::string& key);
    void enable_encryption(const bool enable);
    void enable_autocommit_on_exit(const bool enable);
};

inline SaveManager& saveMgr = SaveManager::get();

class WriteArchive : public Archive {
public:
    using Archive::Archive; // inherit the constructors too

    template<typename T>
    void operator()(const std::string& key, T& value) {
        saveMgr.write(key, value, parent);
    }
};

class ReadArchive : public Archive {
public:
    using Archive::Archive;

    template<typename T>
    void operator()(const std::string& key, T& value) {
        value = saveMgr.read<T>(key, std::nullopt, parent);
    }
};