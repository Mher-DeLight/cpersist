#pragma once
#include "aes.h"
#include "error_handler.h"
#include "serializer.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cpersist {
using byte = uint8_t;
namespace fs = std::filesystem;

constexpr char CPERSIST_MAGIC_HEADER[] = "CPERSIST_MAGIC_HEADER";
constexpr std::string folderName = "savedata";
std::vector<uint8_t> generateKeyFromString(const std::string& s);

namespace internal {

struct StashedObject {
    void* object;
    std::type_index type;
};
inline std::map<std::string, StashedObject> stashMap;

} // namespace internal

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

// CONCEPTS, FIELDS, FILES, AND IMPLEMENTATIONS & OTHERS744
template <typename T> T* LoadStash(const std::string& name);

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
    int write_version = 0;
    int read_version = 0;

    bool encryptionEnabled = false;
    bool autocommit_on_destroy = false;
    std::vector<byte> encryptionKey = std::vector<byte>{};
    std::vector<Field> fields;

    std::function<void()> debatedWrite;

    void init();
    void loadFile();
    std::vector<Field> parseFile();
    bool isDiskFileEncrypted();
    int getDiskFileVersion();
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
        else
            encryptionEnabled = true;
        init();
    }
    ~File() {
        if (debatedWrite)
            internal::ErrorManager::get().throwError("cannot destroy file \"" + filename +
                                                     "\" while a write is still being debated");
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

    void set_schema_version(int version) {
        write_version = version;
    }
    int get_schema_file_version() {
        return read_version;
    }
    int get_schema_buffer_version() {
        return write_version;
    }
    void schema_standard(int standard) {
        if (read_version < standard) {
            internal::ErrorManager::get().throwError(
                "file with schema standard " + std::to_string(standard) +
                " cannot read file with verison " + std::to_string(read_version));
        }
        if (write_version < standard) {
            internal::ErrorManager::get().throwError(
                "file with schema standard " + std::to_string(standard) +
                " cannot write file with verison " + std::to_string(read_version));
        }
    }

    // === OTHER ===
    void refresh();
    void discard();
    bool contains(const std::string& fieldnames, bool loose = true);
    bool contains(const std::initializer_list<std::string>& fieldnames, bool loose = true);
    bool erase(const std::string& filedname);
    bool erase(const std::initializer_list<std::string>& fieldnames);
    void merge(File& other);

    // === WRITING ===
    template <typename T>
    void write(const std::string& fieldname, const T& fieldvalue, const std::string& parent = "") {
        std::string fullname = parent.empty() ? fieldname : parent + "." + fieldname;
        std::stringstream dataStream(std::ios::in | std::ios::out | std::ios::binary);

        if constexpr (cpersist::hasArchive<T>) {
            WriteArchive ar(fullname, *this);
            const_cast<T&>(fieldvalue).archive(ar);
            return;
        } else {
            cpersist::Serializer<T>::write(
                dataStream,
                fieldvalue); // TODO: why does templatestruct in writestash take this route :(
        }

        dataStream.seekg(0, std::ios::end);
        if (dataStream.tellg() == std::streampos(-1)) {
            cpersist::internal::ErrorManager::get().throwError("Serialization failed.");
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
    void writeStash(const std::string& stashname, const std::string& fieldname = "") {
        std::string appliedfieldname = fieldname;
        if (fieldname.empty()) {
            if (contains(stashname)) {
                internal::ErrorManager::get().throwError(
                    "Implicit stash name cannot overwrite existing Fieldname in writeStash()");
            }
            appliedfieldname = stashname;
        }

        auto data = *LoadStash<T>(stashname);
        write(appliedfieldname, data);
    }

    void write_bytes(const std::string& fieldname, const std::vector<byte>& fieldvalue,
                     const std::string& parent = "");

    template <typename T>
    void debateWrite(const std::string& fieldname, const T& fieldvalue,
                     const std::string& parent = "") {
        if (debatedWrite) {
            internal::ErrorManager::get().throwError("a write is already being debated");
        }

        std::string fullname = parent.empty() ? fieldname : parent + "." + fieldname;
        debatedWrite = [this, fullname, fieldvalue]() { write(fullname, fieldvalue); };
    }
    void acceptWrite() {
        if (!debatedWrite) {
            internal::ErrorManager::get().throwError("no write is currently being debated");
        }

        auto pendingWrite = std::move(debatedWrite);
        debatedWrite = nullptr;

        pendingWrite();
    }
    void rejectWrite() {
        if (!debatedWrite) {
            internal::ErrorManager::get().throwError("no write is currently being debated");
        }

        debatedWrite = nullptr;
    }

    // READING
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

            cpersist::internal::ErrorManager::get().throwError("Entry \"" + fullname +
                                                               "\" not found.");
        }

        std::stringstream stream(std::string(reinterpret_cast<const char*>(fieldIt->value.data()),
                                             fieldIt->value.size()),
                                 std::ios::binary | std::ios::in);

        T object;
        cpersist::Serializer<T>::read(stream, object);
        return object;
    }

    template <typename T>
    void sync(const std::string& fieldname, T& fieldvalue, const std::string& parent = "") {
        if (contains(fieldname)) {
            fieldvalue = read<T>(fieldname);
        } else {
            write(fieldname, fieldvalue, parent);
        }
    }
    void commit();

    template <typename T>
    void read_into(const std::string& fieldname, T& result,
                   std::optional<T> defaultValue = std::nullopt, const std::string& parent = "") {
        result = read<T>(fieldname);
    }

    friend File CopyFile(File& file, const std::string& newname, const std::string& newextension,
                         const std::string& newencrpytionkey);
};
template <typename T> class Stash {
public:
    template <typename... Args> explicit Stash(const std::string& name, Args&&... args) {
        if (internal::stashMap.contains(name)) {
            throw std::runtime_error("A stash already exists with the name \"" + name + "\"");
        }

        T* object = new T(std::forward<Args>(args)...);

        internal::stashMap.emplace(
            name, internal::StashedObject{static_cast<void*>(object), std::type_index(typeid(T))});
    }

    ~Stash() = default;
};
template <typename T> T* LoadStash(const std::string& name) {
    auto it = internal::stashMap.find(name);

    if (it == internal::stashMap.end()) {
        throw std::runtime_error("No stash exists with the name \"" + name + "\"");
    }

    if (it->second.type != std::type_index(typeid(T))) {
        throw std::runtime_error("Stash \"" + name + "\" has the wrong type");
    }

    return static_cast<T*>(it->second.object);
}
template <typename T> bool FreeStash(const std::string& name) {
    auto it = internal::stashMap.find(name);

    if (it == internal::stashMap.end()) {
        return false;
    }

    auto ti = std::type_index(typeid(T));
    if (it->second.type != ti) {
        throw std::runtime_error("Stash \"" + name + "\" has the wrong type");
    }

    T* castedObject = static_cast<T*>(it->second.object);
    castedObject->~T();
    internal::stashMap.erase(it);
    return true;
}

File CopyFile(File& file, const std::string& newname, const std::string& newextension = "bin",
              const std::string& newencrpytionkey = "");

template <typename T> void WriteArchive::operator()(const std::string& key, T& value) {
    owner.write(key, value, parent);
}
template <typename T> void ReadArchive::operator()(const std::string& key, T& value) {
    value = owner.read<T>(key, std::nullopt, parent);
}

} // namespace cpersist