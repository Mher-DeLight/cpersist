#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <istream>
#include <list>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cpersist {
template <typename T, typename Enable = void> struct Serializer;

namespace detail {
template <typename T>
struct RawCopyEligible : std::bool_constant<std::is_trivially_copyable_v<T>> {};

template <typename T> struct RawCopyEligible<std::optional<T>> : std::false_type {};

template <typename... Types> struct RawCopyEligible<std::tuple<Types...>> : std::false_type {};

template <typename Rep, typename Period>
struct RawCopyEligible<std::chrono::duration<Rep, Period>> : std::false_type {};

template <typename Clock, typename Duration>
struct RawCopyEligible<std::chrono::time_point<Clock, Duration>> : std::false_type {};

template <typename T, size_t Size>
struct RawCopyEligible<std::array<T, Size>> : RawCopyEligible<std::remove_cv_t<T>> {};

template <typename T>
inline constexpr bool isRawCopyEligible = RawCopyEligible<std::remove_cv_t<T>>::value;
} // namespace detail

// ===== GENERIC =====
template <typename T>
    requires std::is_trivially_copyable_v<T>
struct Serializer<T> {
    static void write(std::ostream& os, const T& value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    static void read(std::istream& is, T& value) {
        is.read(reinterpret_cast<char*>(&value), sizeof(T));
    }
};

// ===== STD::CHRONO =====
template <typename Rep, typename Period> struct Serializer<std::chrono::duration<Rep, Period>> {
    static void write(std::ostream& os, const std::chrono::duration<Rep, Period>& value) {
        Serializer<Rep>::write(os, value.count());
    }

    static void read(std::istream& is, std::chrono::duration<Rep, Period>& value) {
        Rep count{};
        Serializer<Rep>::read(is, count);
        if (is) {
            value = std::chrono::duration<Rep, Period>{count};
        }
    }
};

template <typename Clock, typename Duration>
struct Serializer<std::chrono::time_point<Clock, Duration>> {
    static void write(std::ostream& os, const std::chrono::time_point<Clock, Duration>& value) {
        Serializer<Duration>::write(os, value.time_since_epoch());
    }

    static void read(std::istream& is, std::chrono::time_point<Clock, Duration>& value) {
        Duration elapsed{};
        Serializer<Duration>::read(is, elapsed);
        if (is) {
            value = std::chrono::time_point<Clock, Duration>{elapsed};
        }
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

// ===== STD::TUPLE =====
template <typename... Types> struct Serializer<std::tuple<Types...>> {
    static void write(std::ostream& os, const std::tuple<Types...>& value) {
        std::apply(
            [&os](const auto&... elements) { (Serializer<Types>::write(os, elements), ...); },
            value);
    }

    static void read(std::istream& is, std::tuple<Types...>& value) {
        std::apply([&is](auto&... elements) { (Serializer<Types>::read(is, elements), ...); },
                   value);
    }
};

// ===== STD::ARRAY =====
// Elements with semantic serializers are handled individually.
// Raw-copy-eligible arrays use the generic memcpy specialization.
template <typename T, size_t Size>
    requires(!detail::isRawCopyEligible<T>)
struct Serializer<std::array<T, Size>> {
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

// ==== STD::LIST ==== (trivial + supported types only)
template <typename T> struct Serializer<std::list<T>> {
    static void write(std::ostream& os, const std::list<T>& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        os.write(reinterpret_cast<const char*>(&size), sizeof(size));
        for (const auto& element : value) {
            Serializer<T>::write(os, element);
        }
    }

    static void read(std::istream& is, std::list<T>& value) {
        uint32_t size;
        is.read(reinterpret_cast<char*>(&size), sizeof(size));

        value.resize(size);

        for (auto& element : value) {
            Serializer<T>::read(is, element);
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
