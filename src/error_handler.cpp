#include "error_handler.h"
#include <iostream>
#include <string>
#include <exception>

void cpersist_internal::ErrorManager::throwError(const std::string& error_message) {
    throw std::runtime_error("[CPERSIST ERROR] " + error_message);
}
void cpersist_internal::ErrorManager::assert(const bool& condition, const char* error_message) {
    if (condition) {return;}

    
    std::cerr << "[CPERSIST ASSERT] " << error_message << std::endl;
    abort();
}
void cpersist_internal::ErrorManager::throwWarning(const char* warning_message) {
    std::cerr << "[CPERSIST WARNING] " << warning_message << std::endl;
}