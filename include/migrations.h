#pragma once
#include "cpersist.h"
namespace cpersist::migrations {

template <typename T> struct Migrator {
    int migrate(int from, File& file, const std::string& parent) {
        internal::ErrorManager::get().throwError("Cannot migrate without migration function.");
    }
};

} // namespace cpersist::migrations