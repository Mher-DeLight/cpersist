#include "../include/cpersist.h"
#include <exception>
#include <iostream>
#include <string>

void cpersist::internal::ErrorManager::throwError(const std::string& error_message) {
    throw std::runtime_error("[CPERSIST ERROR] " + error_message);
}
void cpersist::internal::ErrorManager::assert(const bool& condition, const char* error_message) {
    if (condition) {
        return;
    }

    std::cerr << "[CPERSIST ASSERT] " << error_message << std::endl;
    abort();
}
void cpersist::internal::ErrorManager::throwWarning(const char* warning_message) {
    std::cerr << "[CPERSIST WARNING] " << warning_message << std::endl;
}