#pragma once

#include <cstddef>

struct CacheStats {
    std::size_t accesses = 0;
    std::size_t reads = 0;
    std::size_t writes = 0;
    std::size_t hits = 0;
    std::size_t misses = 0;

    double hitRate() const {
        return accesses == 0 ? 0.0
                             : 100.0 * static_cast<double>(hits) /
                                   static_cast<double>(accesses);
    }

    double missRate() const {
        return accesses == 0 ? 0.0
                             : 100.0 * static_cast<double>(misses) /
                                   static_cast<double>(accesses);
    }
};
