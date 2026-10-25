#pragma once

#include "CacheConfig.h"
#include "CacheStats.h"

#include <cstdint>
#include <vector>

class Cache {
public:
    explicit Cache(CacheConfig config);

    // Process one memory access. Returns true on hit, false on miss.
    bool access(std::uint64_t address, char operation);

    const CacheConfig& config() const noexcept { return config_; }
    const CacheStats& stats() const noexcept { return stats_; }
    void reset();

private:
    struct Line {
        bool valid = false;
        std::uint64_t tag = 0;
        std::uint64_t insertionOrder = 0;
        std::uint64_t lastUsedOrder = 0;
    };

    CacheConfig config_;
    CacheStats stats_;
    std::vector<std::vector<Line>> sets_;
    std::uint64_t accessClock_ = 0;

    void validateOperation(char operation) const;
};
