#pragma once

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <string>
#include <memory>


class AES_GCM_Manager {
private:
    template<typename erType>
    void panic(const std::string& msg) {
        // i personally have my own error manager, so i refactored the error messages to this panic function.
        throw erType("ENCRYPTION ERROR: \"" + msg + "\"");
    }
    AES_GCM_Manager() = default;
    ~AES_GCM_Manager() {
        if (!encrKey.empty()) {
            OPENSSL_cleanse(encrKey.data(), encrKey.size());
        }
    }
public:
    // === SINGLETON PROPERTIES
    AES_GCM_Manager(const AES_GCM_Manager&) = delete;
    AES_GCM_Manager& operator=(const AES_GCM_Manager&) = delete;

    
    static AES_GCM_Manager& get() {
        static AES_GCM_Manager instance;
        return instance;
    };

    const EVP_CIPHER* getCipher();

    using CtxPtr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;
    void setEncryptionKey(const std::vector<uint8_t>& key);

    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& bytes);

private:
    std::vector<uint8_t> encrKey;
};

inline AES_GCM_Manager& encrMgr = AES_GCM_Manager::get();