#include "../include/cpersist.h"
#include <algorithm>
#include <iostream>

namespace cpersist {
using byte = uint8_t;
std::vector<uint8_t> generateKeyFromString(const std::string& str) {

    constexpr uint64_t seeds[4] = {0x243F6A8885A308D3ULL, 0x13198A2E03707344ULL,
                                   0xA4093822299F31D0ULL, 0x082EFA98EC4E6C89ULL};

    std::vector<uint8_t> key;
    key.reserve(32);

    for (uint64_t seed : seeds) {
        uint64_t h = seed;

        for (unsigned char c : str) {
            h ^= c;
            h *= 0x100000001B3ULL; // FNV prime
            h ^= h >> 32;
            h *= 0x9E3779B185EBCA87ULL; // extra mixing
        }

        for (int i = 0; i < 8; ++i)
            key.push_back(static_cast<uint8_t>(h >> (i * 8)));
    }

    return key;
}

#pragma region file
// == PUBLIC ==
void File::commit() {
    fs::path fullFilePath = (fs::path(folderName) / fs::path(filename + "." + extension));

    std::ofstream file(
        fullFilePath,
        std::ios::binary |
            std::ios::trunc); // write into <current_file>.<ext>, append if already exists

    if (!file) {
        cpersist_internal::ErrorManager::get().throwError("Unable to commit to file \"" +
                                                          fullFilePath.string() + ".\"");
        return;
    }
    file.write(CPERSIST_MAGIC_HEADER, sizeof(CPERSIST_MAGIC_HEADER) - 1);
    file.write(reinterpret_cast<const char*>(&encryptionEnabled), sizeof(encryptionEnabled));

    std::vector<uint8_t> bytes;
    for (auto& field : fields) {
        bytes.push_back(field.name.size());
        for (auto& c : field.name) {
            bytes.push_back(c);
        }

        uint32_t valueSize = static_cast<uint32_t>(field.value.size());
        std::vector<std::uint8_t> valueSizeVector = {
            static_cast<std::uint8_t>((valueSize) & 0xFF),
            static_cast<std::uint8_t>((valueSize >> 8) & 0xFF),
            static_cast<std::uint8_t>((valueSize >> 16) & 0xFF),
            static_cast<std::uint8_t>((valueSize >> 24) & 0xFF)};
        for (auto byt : valueSizeVector) {
            bytes.push_back(byt);
        }

        for (auto& vl : field.value) {
            bytes.push_back(vl);
        }
    }

    if (encryptionEnabled) {
        encrMgr.setEncryptionKey(encryptionKey);
        bytes = encrMgr.encrypt(bytes);
    }

    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
}
void File::refresh() {
    init();
}
bool File::contains(const std::string& fieldname, bool loose) {
    auto fieldIt = std::find_if(fields.begin(), fields.end(), [&](const Field& field) {
        return (field.name == fieldname) || (field.name.starts_with(fieldname + ".") && loose);
    });

    return fieldIt != fields.end();
}
bool File::contains(const std::initializer_list<std::string>& fieldnames, const bool loose) {
    for (const auto& dataname : fieldnames) {
        if (!contains(dataname))
            return false;
    }

    return true;
}
void File::erase(const std::string& fieldname) {
    auto fieldIt = std::find_if(fields.begin(), fields.end(), [&](const Field& field) {
        return (field.name == fieldname) || field.name.starts_with(fieldname + ".");
    });

    if (fieldIt == fields.end()) {
        cpersist_internal::ErrorManager::get().throwError("Cannot delete field \"" + fieldname +
                                                          "\" as it is nonexistent.");
        return;
    }

    fields.erase(fieldIt);
}
void File::erase(const std::initializer_list<std::string>& fieldnames) {
    for (auto& fieldname : fieldnames) {
        erase(fieldname);
    }
}
void File::merge(File& other) {
    for (auto& field : other.fields) {
        write_bytes(field.name, field.value);
    }
}

// == PRIVATE ==
void File::init() {
    std::filesystem::create_directory(folderName); // creates the folder if it doesn't exist

    fields.clear();
    loadFile();
}
void File::loadFile() {
    for (const auto& entry : std::filesystem::directory_iterator(folderName)) {
        bool regularFile = entry.is_regular_file();
        auto ext = entry.path().extension().string();
        auto stem = entry.path().stem().string();

        if (regularFile && ext == ("." + extension) && stem == filename) {
            fields = parseFile();
        }
    }
}
std::vector<Field> File::parseFile() {
    if (isDiskFileEncrypted() && encryptionKey.empty()) {
        return std::vector<Field>();
    }

    std::vector<uint8_t> data = readFileAsBinary();

    std::vector<Field> fields;
    uint64_t position = 0;
    while (position < data.size()) {
        // ===== NAME
        // check bounds
        if (position + sizeof(uint8_t) > data.size()) {
            cpersist_internal::ErrorManager::get().throwError("file " + filename + extension +
                                                              " is incorrectly formatted");
        }

        uint8_t nameSize;
        std::memcpy(&nameSize, data.data() + position, sizeof(nameSize));
        position += sizeof(nameSize);

        // ===== NAME
        // check bounds
        if (position + nameSize > data.size()) {
            cpersist_internal::ErrorManager::get().throwError("file " + filename + extension +
                                                              " is incorrectly formatted");
        }

        std::string currentName(reinterpret_cast<const char*>(data.data() + position), nameSize);
        position += nameSize;

        // ===== DATASIZE
        if (position + sizeof(uint32_t) > data.size()) {
            cpersist_internal::ErrorManager::get().throwError("file " + filename + extension +
                                                              " is incorrectly formatted");
        }

        uint32_t dataSize;
        std::memcpy(&dataSize, data.data() + position, sizeof(dataSize));
        position += sizeof(dataSize);

        // the position now points at the data itself
        if (position + dataSize > data.size()) {
            cpersist_internal::ErrorManager::get().throwError("file " + filename + extension +
                                                              " is incorrectly formatted");
        }

        // copy the data into a new vector
        std::vector<uint8_t> fieldData(data.begin() + position, data.begin() + position + dataSize);

        // construct and store the field. we'll use emplace to avoid making a temporary Field object
        fields.emplace_back(currentName, fieldData);

        // Move to the next field
        position += dataSize;
    }
    return fields;
}
bool File::isDiskFileEncrypted() {
    fs::path curFp = (fs::path(folderName) / fs::path(filename + "." + extension));
    std::ifstream file(curFp, std::ios::binary);
    if (!file) {
        cpersist_internal::ErrorManager::get().throwError("Failed to open file: " + filename);
    }

    std::string magicHeader(sizeof(CPERSIST_MAGIC_HEADER) - 1, '\0');
    if (!file.read(magicHeader.data(), magicHeader.size())) {
        cpersist_internal::ErrorManager::get().throwError("Cannot read data file " + filename);
    }

    if (magicHeader != CPERSIST_MAGIC_HEADER) {
        cpersist_internal::ErrorManager::get().throwError("Invalid magic header in data file " +
                                                          filename);
    }

    uint8_t encryptionMagicByte;
    if (!file.read(reinterpret_cast<char*>(&encryptionMagicByte), 1)) {
        cpersist_internal::ErrorManager::get().throwError("Cannot read data file " + filename);
    }

    return encryptionMagicByte != 0x00;
}
std::vector<byte> File::readFileAsBinary() {
    bool fileEncr = isDiskFileEncrypted();

    std::filesystem::path customFilePath =
        std::filesystem::path(folderName) / (filename + "." + extension);
    std::ifstream file(customFilePath, std::ios::binary);

    if (!file) {
        cpersist_internal::ErrorManager::get().throwError("Failed to open file: " + filename +
                                                          extension);
    }

    // Find the file's size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // then read it
    std::vector<uint8_t> bytes(size);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        cpersist_internal::ErrorManager::get().throwError("Cannot read data file " + filename +
                                                          extension);
    }
    bytes.erase(bytes.begin(), bytes.begin() + sizeof(CPERSIST_MAGIC_HEADER));

    if (fileEncr) {
        encrMgr.setEncryptionKey(encryptionKey);
        bytes = encrMgr.decrypt(bytes);
    }

    return bytes;
}
void File::write_bytes(const std::string& fieldname, const std::vector<byte>& fieldvalue,
                       const std::string& parent) {
    std::string fullname = parent.empty() ? fieldname : parent + "." + fieldname;
    std::stringstream dataStream(std::ios::in | std::ios::out | std::ios::binary);

    for (auto& bt : fieldvalue) {
        Serializer<byte>::write(dataStream, bt); // emit directly
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
#pragma endregion
#pragma region global_functions
File CopyFile(File& file) {
    return file; // automatically copies. thanks, c++.
}

#pragma endregion

} // namespace cpersist