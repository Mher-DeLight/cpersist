#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cpersist {
template <typename T, typename Enable = void> struct Serializer;

// ===== GENERIC =====
template <typename T> struct Serializer<T, std::enable_if_t<std::is_trivially_copyable_v<T>>> {
    static void write(std::ostream& os, const T& value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    static void read(std::istream& is, T& value) {
        is.read(reinterpret_cast<char*>(&value), sizeof(T));
    }
};

// ===== STD::ARRAY =====
// Non-trivial T only
// Trivially-copyable arrays use the generic memcpy specialization
template <typename T, size_t Size>
struct Serializer<std::array<T, Size>, std::enable_if_t<!std::is_trivially_copyable_v<T>>> {
    static void write(std::ostream& os, const std::array<T, Size>& value) {
        for (const auto& element : value) {
            Serializer<T>::write(os, element);
        }
    }
    static void read(std::istream& is, std::array<T, Size>& value) {
        for (auto& element : value) {
            Serializer<T>::read(is, element);
        }
    }
};

// ===== STD::MAP =====
template <typename Key, typename Value, typename Compare, typename Allocator>
struct Serializer<std::map<Key, Value, Compare, Allocator>> {
    static void write(std::ostream& os, const std::map<Key, Value, Compare, Allocator>& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        Serializer<uint32_t>::write(os, size);

        for (const auto& [key, mappedValue] : value) {
            Serializer<Key>::write(os, key);
            Serializer<Value>::write(os, mappedValue);
        }
    }

    static void read(std::istream& is, std::map<Key, Value, Compare, Allocator>& value) {
        uint32_t size;
        Serializer<uint32_t>::read(is, size);

        value.clear();
        for (uint32_t i = 0; i < size; ++i) {
            Key key;
            Value mappedValue;
            Serializer<Key>::read(is, key);
            Serializer<Value>::read(is, mappedValue);
            value.emplace(std::move(key), std::move(mappedValue));
        }
    }
};

// ===== STD::UNORDERED_MAP =====
template <typename Key, typename Value, typename Hash, typename KeyEqual, typename Allocator>
struct Serializer<std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>> {
    static void write(std::ostream& os,
                      const std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        Serializer<uint32_t>::write(os, size);

        for (const auto& [key, mappedValue] : value) {
            Serializer<Key>::write(os, key);
            Serializer<Value>::write(os, mappedValue);
        }
    }

    static void read(std::istream& is,
                     std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>& value) {
        uint32_t size;
        Serializer<uint32_t>::read(is, size);

        value.clear();
        for (uint32_t i = 0; i < size; ++i) {
            Key key;
            Value mappedValue;
            Serializer<Key>::read(is, key);
            Serializer<Value>::read(is, mappedValue);
            value.emplace(std::move(key), std::move(mappedValue));
        }
    }
};

// ===== STD::STRING =====
template <> struct Serializer<std::string> {
    static void write(std::ostream& os, const std::string& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        os.write(reinterpret_cast<const char*>(&size), sizeof(size));
        os.write(value.data(), size);
    }

    static void read(std::istream& is, std::string& value) {
        uint32_t size;
        is.read(reinterpret_cast<char*>(&size), sizeof(size));
        value.resize(size);
        is.read(value.data(), size);
    }
};

// ==== STD::VECTOR ==== (trivial + supported types only)
template <typename T> struct Serializer<std::vector<T>> {
    static void write(std::ostream& os, const std::vector<T>& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        os.write(reinterpret_cast<const char*>(&size), sizeof(size));

        if constexpr (std::is_trivially_copyable_v<T>) {
            if (!value.empty()) {
                os.write(reinterpret_cast<const char*>(value.data()), size * sizeof(T));
            }
        } else {
            for (const auto& element : value) {
                Serializer<T>::write(os, element);
            }
        }
    }

    static void read(std::istream& is, std::vector<T>& value) {
        uint32_t size;
        is.read(reinterpret_cast<char*>(&size), sizeof(size));

        value.resize(size);

        if constexpr (std::is_trivially_copyable_v<T>) {
            if (!value.empty()) {
                is.read(reinterpret_cast<char*>(value.data()), size * sizeof(T));
            }
        } else {
            for (auto& element : value) {
                Serializer<T>::read(is, element);
            }
        }
    }
};
} // namespace cpersist
