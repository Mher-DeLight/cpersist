#pragma once
#include <istream>
#include <ostream>
#include <string>
#include <vector>
#include <type_traits>
#include <cstring>
#include <cstdint>

namespace cpersist
{
    template<typename T, typename Enable = void>
    struct Serializer;

    // ===== GENERIC =====
    template<typename T>
    struct Serializer<T, std::enable_if_t<std::is_trivially_copyable_v<T>>> {
        static void write(std::ostream& os, const T& value) {
            os.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }

        static void read(std::istream& is, T& value) {
            is.read(reinterpret_cast<char*>(&value), sizeof(T));
        }
    };

    // ===== STD::STRING =====
    template<>
    struct Serializer<std::string> {
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
    template<typename T>
    struct Serializer<std::vector<T>> {
        static void write(std::ostream& os, const std::vector<T>& value) {
            uint32_t size = static_cast<uint32_t>(value.size());
            os.write(reinterpret_cast<const char*>(&size), sizeof(size));

            if constexpr (std::is_trivially_copyable_v<T>) {
                if (!value.empty()) {
                    os.write(
                        reinterpret_cast<const char*>(value.data()),
                        size * sizeof(T)
                    );
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
                    is.read(reinterpret_cast<char*>(value.data()), size * sizeof(T)
                    );
                }
            } else {
                for (auto& element : value) {
                    Serializer<T>::read(is, element);
                }
            }
        }
    };
}