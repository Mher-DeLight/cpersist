#include "aes.h"
#include <iostream>

const EVP_CIPHER* AES_GCM_Manager::getCipher() {
    switch (encrKey.size())
    {
        case 16:
            return EVP_aes_128_gcm();
            break;
        case 24:
            return EVP_aes_192_gcm();
            break;
        case 32:
            return EVP_aes_256_gcm();
            break;
        default:
            panic<std::runtime_error>("invalid key size");
            break;
    }
    panic<std::runtime_error>("something has gone horribly wrong");
    return EVP_aes_256_gcm(); // just to satsify the compiler
}

void AES_GCM_Manager::setEncryptionKey(const std::vector<uint8_t>& key) {
    std::size_t keyLen = key.size();
    if (keyLen != 16 && keyLen != 24 && keyLen != 32) {
        panic<std::runtime_error>("invalid AES key size");
    }

    encrKey = key;
}

std::vector<uint8_t> AES_GCM_Manager::encrypt(const std::vector<uint8_t>& bytes) {
    if (encrKey.empty()) {
        panic<std::runtime_error>("key not initialized");
    }
    if (bytes.size() > INT_MAX) {
        // we keep converting back and forth between size_t and int, which might implicitly trim some content for huge inputs.
        // instead of fixing it like a normal person, throw on large inputs.
        panic<std::runtime_error>("input too large");
    }
    
    int inputLen = static_cast<int>(bytes.size());

    const EVP_CIPHER* cipher = getCipher();

    constexpr std::size_t nonceLen = 12;
    constexpr std::size_t tagLen = 16;
    // (not magic numbers, they're the standard)

    uint8_t nonce[nonceLen];
    if (RAND_bytes(nonce, nonceLen) != 1) {
        panic<std::runtime_error>("nonce generation failed");   
    }

    CtxPtr ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);

    if (!ctx) {
        panic<std::runtime_error>("ctx allocation failed");
    }

    std::vector<uint8_t> output(nonceLen + inputLen + tagLen);
    std::copy(nonce, nonce + nonceLen, output.begin());

    int len = 0;
    int cipherLen = 0;

    
    if (EVP_EncryptInit_ex(ctx.get(), cipher, nullptr, nullptr, nullptr) != 1) {
        panic<std::runtime_error>("init failed");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, nonceLen, nullptr) != 1) {
        panic<std::runtime_error>("iv length failed");
    }

    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, encrKey.data(), nonce) != 1) {
        panic<std::runtime_error>("key setup failed");
    }

    if (EVP_EncryptUpdate(ctx.get(), output.data() + nonceLen, &len, bytes.data(), inputLen) != 1) {
        panic<std::runtime_error>("encrypt failed");
    }

    cipherLen = len;
    

    if (EVP_EncryptFinal_ex(ctx.get(), output.data() + nonceLen + cipherLen, &len) != 1) {
        panic<std::runtime_error>("final failed");
    }

    cipherLen += len;

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, tagLen, output.data() + nonceLen + cipherLen) != 1) {
        panic<std::runtime_error>("tag failed");
    }

    output.resize(nonceLen + cipherLen + tagLen);
    return output;
}


std::vector<uint8_t> AES_GCM_Manager::decrypt(const std::vector<uint8_t>& bytes) {
    if (encrKey.size() != 16 && encrKey.size() != 24 && encrKey.size() != 32) {
        panic<std::invalid_argument>("invalid AES key size");
    }
    
    constexpr int nonceLen = 12;
    constexpr int tagLen = 16;
    
    if (bytes.size() < nonceLen + tagLen) {
        panic<std::runtime_error>("ciphertext too short");
    }
    
    const EVP_CIPHER* cipher = getCipher();
    CtxPtr ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) {
        panic<std::runtime_error>("ctx allocation failed");
    }
    
    size_t cipherLen = bytes.size() - nonceLen - tagLen;
    
    if (cipherLen > static_cast<size_t>(INT_MAX)) {
        panic<std::runtime_error>("input too large");
    }
    
    std::vector<uint8_t> out(cipherLen);
    
    int len = 0;
    int plainLen = 0;
    
    if (EVP_DecryptInit_ex(ctx.get(), cipher, nullptr, nullptr, nullptr) != 1 ||
    EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, nonceLen, nullptr) != 1 ||
        EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, encrKey.data(), bytes.data()) != 1) {
            panic<std::runtime_error>("init failed");
        }
        
        if (EVP_DecryptUpdate(ctx.get(), out.data(), &len, bytes.data() + nonceLen, cipherLen) != 1) {
            panic<std::runtime_error>("decrypt failed");
        }
        
        plainLen = len;
        
        if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, tagLen,
        const_cast<uint8_t*>(bytes.data() + nonceLen + cipherLen)) != 1) {
            panic<std::runtime_error>("tag failed");
        }
        
        
        if (EVP_DecryptFinal_ex(ctx.get(), out.data() + plainLen, &len) != 1) {
            panic<std::runtime_error>("authentication failed");
        }
        
    plainLen += len;
    out.resize(plainLen);

    return out;
}