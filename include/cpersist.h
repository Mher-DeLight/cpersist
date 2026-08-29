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
using byte = uint8_t;
namespace fs = std::filesystem;

constexpr char CPERSIST_MAGIC_HEADER[] = "CPERSIST_MAGIC_HEADER";
constexpr std::string folderName = "savedata";
std::vector<uint8_t> generateKeyFromString(const std::string& s);

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

#pragma region concepts
template <typename T>
concept hasWrite = requires(T& t, const std::string& parent) { t.write(parent); };

template <typename T>
concept hasRead = requires(T& t, const std::string& parent) { t.read(parent); };

template <typename T>
concept hasArchive = requires(T& t, WriteArchive& war) { t.archive(war); } &&
                     requires(T& t, ReadArchive& rar) { t.archive(rar); };
#pragma endregion

class Field {
public:
    Field(const std::string& fieldname, std::vector<uint8_t>& fieldvalue)
        : name(fieldname), value(fieldvalue) {}

    std::string name;
    std::vector<uint8_t> value;
};
class File {
private:
    bool encryptionEnabled = false;
    bool autocommit_on_destroy = false;
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
    ~File() {
        if (autocommit_on_destroy)
            commit();
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
    void enable_autocommit_on_destroy(bool enable) {
        autocommit_on_destroy = enable;
    }

    // === OTHER ===
    void refresh();
    bool contains(const std::string& fieldnames, bool loose = true);
    bool contains(const std::initializer_list<std::string>& fieldnames, bool loose = true);
    void erase(const std::string& filedname);
    void erase(const std::initializer_list<std::string>& fieldnames);

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

template <typename T> void WriteArchive::operator()(const std::string& key, T& value) {
    owner.write(key, value, parent);
}
template <typename T> void ReadArchive::operator()(const std::string& key, T& value) {
    value = owner.read<T>(key, std::nullopt, parent);
}

} // namespace cpersist