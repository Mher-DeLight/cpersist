#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <istream>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <filesystem>
#include<unordered_set>

namespace cpersist {
template <typename T, typename Enable = void> struct Serializer;

namespace detail {
template <typename T>
struct RawCopyEligible : std::bool_constant<std::is_trivially_copyable_v<T>> {};

template <typename T> struct RawCopyEligible<std::optional<T>> : std::false_type {};

template <typename T, size_t Size>
struct RawCopyEligible<std::array<T, Size>> : RawCopyEligible<std::remove_cv_t<T>> {};

template <typename T>
inline constexpr bool isRawCopyEligible = RawCopyEligible<std::remove_cv_t<T>>::value;
} // namespace detail

// ===== GENERIC =====
template <typename T> struct Serializer<T, std::enable_if_t<detail::isRawCopyEligible<T>>> {
    static void write(std::ostream& os, const T& value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    static void read(std::istream& is, T& value) {
        is.read(reinterpret_cast<char*>(&value), sizeof(T));
    }
};

// ===== STD::OPTIONAL =====
template <typename T> struct Serializer<std::optional<T>> {
    static void write(std::ostream& os, const std::optional<T>& value) {
        const uint8_t discriminator = value.has_value() ? 1 : 0;
        Serializer<uint8_t>::write(os, discriminator);
        if (discriminator == 1) {
            Serializer<T>::write(os, *value);
        }
    }

    static void read(std::istream& is, std::optional<T>& value) {
        uint8_t discriminator = 0;
        Serializer<uint8_t>::read(is, discriminator);
        if (!is) {
            throw std::runtime_error("Failed to read std::optional presence discriminator.");
        }
        if (discriminator > 1) {
            is.setstate(std::ios::failbit);
            throw std::runtime_error("Invalid std::optional presence discriminator.");
        }
        if (discriminator == 0) {
            value.reset();
            return;
        }

        if (!value) {
            value.emplace();
        }
        Serializer<T>::read(is, *value);
    }
};

// ===== STD::PAIR =====
template <typename First, typename Second> struct Serializer<std::pair<First, Second>> {
    static void write(std::ostream& os, const std::pair<First, Second>& value) {
        Serializer<First>::write(os, value.first);
        Serializer<Second>::write(os, value.second);
    }

    static void read(std::istream& is, std::pair<First, Second>& value) {
        Serializer<First>::read(is, value.first);
        Serializer<Second>::read(is, value.second);
    }
};

// ===== STD::ARRAY =====
// Elements with semantic serializers are handled individually.
// Raw-copy-eligible arrays use the generic memcpy specialization.
template <typename T, size_t Size>
struct Serializer<std::array<T, Size>, std::enable_if_t<!detail::isRawCopyEligible<T>>> {
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

// ===== STD::SET =====
template <typename Value, typename Compare, typename Allocator>
struct Serializer<std::set<Value, Compare, Allocator>> {
    static void write(std::ostream& os, const std::set<Value, Compare, Allocator>& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        Serializer<uint32_t>::write(os, size);

        for (const auto& element : value) {
            Serializer<Value>::write(os, element);
        }
    }

    static void read(std::istream& is, std::set<Value, Compare, Allocator>& value) {
        uint32_t size;
        Serializer<uint32_t>::read(is, size);

        value.clear();
        for (uint32_t i = 0; i < size; ++i) {
            Value element;
            Serializer<Value>::read(is, element);
            value.emplace(std::move(element));
        }
    }
};


// ===== STD::UNORDERED_SET =====

template < typename Value, typename Hash, typename KeyEqual, typename Allocator>
struct Serializer<std::unordered_set< Value, Hash,KeyEqual, Allocator>> {
    static void write(std::ostream& os,
                      const std::unordered_set< Value, Hash,KeyEqual, Allocator>& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        Serializer<uint32_t>::write(os, size);

        for (const auto& element : value) {
            Serializer<Value>::write(os, element);
        }
    }

    static void read(std::istream& is,
                     std::unordered_set< Value, Hash,KeyEqual, Allocator>& value) {
        uint32_t size;
        Serializer<uint32_t>::read(is, size);

        value.clear();
        for (uint32_t i = 0; i < size; ++i) {
            Value element;
            Serializer<Value>::read(is, element);
            value.emplace(std::move(element));
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

        if constexpr (detail::isRawCopyEligible<T>) {
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

        if constexpr (detail::isRawCopyEligible<T>) {
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

// ===== STD::FILESYSTEM::PATH =====
template <> struct Serializer<std::filesystem::path> {
    static void write(std::ostream& os, const std::filesystem::path& value) {
        std::string str = value.string();
        uint32_t size = static_cast<uint32_t>(str.size());
        os.write(reinterpret_cast<const char*>(&size), sizeof(size));
        os.write(str.data(), size);
    }

    static void read(std::istream& is, std::filesystem::path& value) {
        uint32_t size;
        is.read(reinterpret_cast<char*>(&size), sizeof(size));
        std::string str;
        str.resize(size);
        is.read(&str[0], size);
        value = str;
    }
};
} // namespace cpersist
