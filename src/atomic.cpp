#include "../include/cpersist.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <cstdio>
#endif

namespace cpersist {

namespace fs = std::filesystem;

bool File::atomicReplace() {
    const fs::path target = fs::path(folderName) / (filename + "." + extension);
    const fs::path targetDir = target.parent_path();
    std::error_code ec;
    fs::create_directories(targetDir, ec);

    if (ec)
        return false;

    const fs::path temp = targetDir / (target.filename().string() + ".temp");
    std::vector<uint8_t> bytes;

    for (const auto& field : fields) {
        // == NAME LENGTH ==
        if (field.name.size() > UINT8_MAX)
            return false;

        bytes.push_back(static_cast<uint8_t>(field.name.size()));

        // == NAME ==
        for (char c : field.name) {
            bytes.push_back(static_cast<uint8_t>(c));
        }

        // == VALUE LENGTH ==
        if (field.value.size() > UINT32_MAX)
            return false;

        const uint32_t valueSize = static_cast<uint32_t>(field.value.size());

        bytes.push_back(static_cast<uint8_t>(valueSize & 0xFF));
        bytes.push_back(static_cast<uint8_t>((valueSize >> 8) & 0xFF));
        bytes.push_back(static_cast<uint8_t>((valueSize >> 16) & 0xFF));
        bytes.push_back(static_cast<uint8_t>((valueSize >> 24) & 0xFF));

        // == VALUE ==
        bytes.insert(bytes.end(), field.value.begin(), field.value.end());
    }

    // == ENCRYPTION ==
    if (encryptionEnabled) {
        encrMgr.setEncryptionKey(encryptionKey);
        bytes = encrMgr.encrypt(bytes);
    }

    /*
     * Write to temorary first. If we succeeded, atomically replace it with the normal file.
     *
     * == FILE FORMAT ==
     * [magic header]
     * [encryption byte]
     * [version]
     * [data]
     *
     */
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);

        if (!out)
            return false;

        const uint8_t encryptionByte = encryptionEnabled ? 0x01 : 0x00;

        out.write(CPERSIST_MAGIC_HEADER, sizeof(CPERSIST_MAGIC_HEADER) - 1);
        out.write(reinterpret_cast<const char*>(&encryptionByte), sizeof(encryptionByte));
        out.write(reinterpret_cast<const char*>(&write_version), sizeof(write_version));
        if (!bytes.empty()) {
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }

        if (!out.good()) {
            out.close();
            fs::remove(temp, ec);
            return false;
        }

        out.flush();

        if (!out.good()) {
            out.close();
            fs::remove(temp, ec);
            return false;
        }
    }

    // temporary file should be finished. now atomically replace.
    // this is platform-dependent so we'll have to write some ugly code :(
#ifdef _WIN32
    if (fs::exists(target, ec)) {
        if (ReplaceFileW(target.wstring().c_str(), temp.wstring().c_str(), nullptr,
                         REPLACEFILE_IGNORE_ACL_ERRORS, nullptr, nullptr)) {

            read_version = write_version;
            return true;
        }

        fs::remove(temp, ec);
        return false;
    }
    if (MoveFileExW(temp.wstring().c_str(), target.wstring().c_str(), MOVEFILE_WRITE_THROUGH)) {
        read_version = write_version;
        return true;
    }

    fs::remove(temp, ec);
    return false;

#else

    // POSIX rename() already replaces atomically
    if (std::rename(temp.c_str(), target.c_str()) == 0) {
        read_version = write_version;
        return true;
    }

    fs::remove(temp, ec);
    return false;

#endif
}

} // namespace cpersist