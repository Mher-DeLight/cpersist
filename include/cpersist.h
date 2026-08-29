#pragma once

#include "aes.h"
#include "error_handler.h"
#include "serializer.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cpersist {
std::vector<uint8_t> generateKeyFromString(const std::string& s);

namespace fs = std::filesystem;
constexpr char CPERSIST_MAGIC_HEADER[] = "CPERSIST_MAGIC_HEADER";
constexpr std::string folderName = "savedata";

// ARCHIVES

class SaveManager;
class File;

class Archive {
protected:
    std::string parent;
    File& owner;

public:
    Archive(std::string parent, File& owner_) : parent(std::move(parent)), owner(owner_) {}
};
class WriteArchive : public Archive {
public:
    using Archive::Archive;

    template <typename T> void operator()(const std::string& key, T& value);
};
class ReadArchive : public Archive {
public:
    using Archive::Archive;

    template <typename T> void operator()(const std::string& key, T& value);
};

template <typename T>
concept hasWrite = requires(T& t, const std::string& parent) { t.write(parent); };

template <typename T>
concept hasRead = requires(T& t, const std::string& parent) { t.read(parent); };

template <typename T>
concept hasArchive = requires(T& t, WriteArchive& war) { t.archive(war); } &&
                     requires(T& t, ReadArchive& rar) { t.archive(rar); };

class Field {
public:
    Field(const std::string& fieldname, std::vector<uint8_t>& fieldvalue)
        : name(fieldname), value(fieldvalue) {}

    std::string name;
    std::vector<uint8_t> value;
};

class SaveManager {
private:
    std::string current_file;
    std::unordered_map<std::string, std::vector<Field>> files;

    void debugLog(const std::string& message) {
        if (!debugMode) {
            return;
        }

        std::cout << "[CPERSIST LOG] " << message << std::endl;
    }

    std::string fileExtension = ".bin";
    std::string folderName = "savedata";
    std::filesystem::path fullFilePath;

    std::vector<Field> readFile(const std::string& filename);

    std::vector<uint8_t> toBytes(uint64_t value) {
        return {static_cast<uint8_t>(value >> 56), static_cast<uint8_t>(value >> 48),
                static_cast<uint8_t>(value >> 40), static_cast<uint8_t>(value >> 32),
                static_cast<uint8_t>(value >> 24), static_cast<uint8_t>(value >> 16),
                static_cast<uint8_t>(value >> 8),  static_cast<uint8_t>(value)};
    }

    bool debugMode = true;
    bool encryption_enabled = true;
    bool commitOnDestroy = false;

public:
    // Normal object construction/destruction.
    SaveManager() {
        init();
    }

    ~SaveManager() {
        if (commitOnDestroy) {
            try {
                commit();
            } catch (...) {
            }
        }
    }

    // Prevent accidental copying of manager state.
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;

    void init();

    void reinit();

    void loadExistingFiles();

    bool filename_fits_standards(const std::string& filename);

    void make_filename_safe(std::string& filename);

    bool change_file(const std::string& new_file);

    void change_file_safe(const std::string& new_file);

    bool create_new_file(const std::string& new_file);

    bool file_exists(const std::string& filename);

    bool file_exists_on_disk(const std::string& filename);

    bool open(const std::string& filename);

    void ensure_exists(std::initializer_list<std::string> filenames);

    void ensure_exists(std::vector<std::string> filenames);

    // WRITING
    uint64_t getDataPosition(const std::string& name, const bool loose = false);

    std::vector<uint8_t> readFileAsBinary(const std::string& filename);

    bool isFileEncrypted(const std::string& filename = "");

    void erase(const std::string& fieldname);

    // READING

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

using byte = uint8_t;
class File {
private:
    bool encryptionEnabled = false;
    std::vector<byte> encryptionKey = std::vector<byte>{};
    std::vector<Field> fields;

    void init();
    void loadFile();
    std::vector<Field> parseFile();
    bool isDiskFileEncrypted();
    std::vector<byte> readFileAsBinary();

public:
    const std::string filename;
    const std::string extension = "bin";

    File(const std::string& filename_, const std::string& extension_ = "bin",
         const std::string& encryptionKey_ = "",
         const std::vector<Field>& fields_ = std::vector<Field>{})
        : filename(filename_), extension(extension_),
          encryptionKey(generateKeyFromString(encryptionKey_)), fields(fields_) {
        if (encryptionKey_.empty())
            encryptionEnabled = false;
        init();
    }

    // === GETTERS/SETTERS ===
    void enable_encryption(const std::string& key = "") {
        encryptionEnabled = true;
        encryptionKey = generateKeyFromString(key);
        init();
    }
    void disable_encryption() {
        encryptionEnabled = false;
        encryptionKey = std::vector<byte>();
        init();
    }

    // === OTHER ===
    void refresh();
    bool contains(const std::string& fieldnames, bool loose = true);
    bool contains(const std::initializer_list<std::string>& fieldname, bool loose = true);

    // === TEMPLATES ===
    template <typename T>
    void write(const std::string& fieldname, const T& fieldvalue, const std::string& parent = "") {
        std::string fullname = parent.empty() ? fieldname : parent + "." + fieldname;
        std::stringstream dataStream(std::ios::in | std::ios::out | std::ios::binary);

        if constexpr (cpersist::hasArchive<T>) {
            WriteArchive ar(fullname, *this);
            const_cast<T&>(fieldvalue).archive(ar);
            return;
        } else {
            cpersist::Serializer<T>::write(dataStream, fieldvalue);
        }

        dataStream.seekg(0, std::ios::end);
        if (dataStream.tellg() == std::streampos(-1)) {
            cpersist_internal::ErrorManager::get().throwError("Serialization failed.");
        }
        std::string dataString = dataStream.str();
        std::vector<uint8_t> serialized(dataString.begin(), dataString.end());

        for (auto& fd : fields) {
            if (fd.name == fullname) {
                fd.value = serialized;
                return;
            }
        }

        Field field(fullname, serialized);
        fields.push_back(field);
    }

    template <typename T>
    T read(const std::string& fieldname, std::optional<T> defaultValue = std::nullopt,
           const std::string& parent = "") {

        std::string fullname = parent.empty() ? fieldname : parent + "." + fieldname;

        if constexpr (cpersist::hasArchive<T>) {
            T object;

            ReadArchive ar(fullname, *this);
            object.archive(ar);
            return object;
        }

        auto fieldIt = std::find_if(fields.begin(), fields.end(),
                                    [&](const Field& field) { return field.name == fullname; });

        if (fieldIt == fields.end()) {
            if (defaultValue) {
                return *defaultValue;
            }

            cpersist_internal::ErrorManager::get().throwError("Entry \"" + fullname +
                                                              "\" not found.");
        }

        std::stringstream stream(std::string(reinterpret_cast<const char*>(fieldIt->value.data()),
                                             fieldIt->value.size()),
                                 std::ios::binary | std::ios::in);

        T object;
        cpersist::Serializer<T>::read(stream, object);
        return object;
    }

    void commit();
};

// Archive implementations.
//
// IMPORTANT:
// These now use the SaveManager instance that owns the Archive.
// There is NO global saveMgr anymore.

template <typename T> void WriteArchive::operator()(const std::string& key, T& value) {
    owner.write(key, value, parent);
}

template <typename T> void ReadArchive::operator()(const std::string& key, T& value) {
    value = owner.read<T>(key, std::nullopt, parent);
}

} // namespace cpersist