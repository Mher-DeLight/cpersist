#pragma once
#include <string>

namespace cpersist {
namespace internal {
class ErrorManager {
public:
    static ErrorManager& get() {
        static ErrorManager instance; // only created once thanks to 'static', also thread-safe
        return instance;
    }

    void throwError(const std::string& error_message);
    void assert(const bool& condition, const char* error_message);
    void throwWarning(const std::string& warning_message);

    ErrorManager(const ErrorManager&) = delete;
    ErrorManager& operator=(const ErrorManager&) = delete;

private:
    ErrorManager() {}
    ~ErrorManager() {}
};
} // namespace internal
} // namespace cpersist