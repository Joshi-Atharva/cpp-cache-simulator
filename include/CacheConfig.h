#pragma once

#include <cstddef>
#include <string>

enum class ReplacementPolicy {
    LRU,
    FIFO
};

struct CacheConfig {
    std::size_t capacityBytes = 4096;
    std::size_t blockSizeBytes = 64;
    std::size_t associativity = 1;
    ReplacementPolicy replacementPolicy = ReplacementPolicy::LRU;

    // Throws std::invalid_argument if the configuration is not valid.
    void validate() const;

    std::size_t numberOfSets() const;
    std::size_t linesPerSet() const;
};

std::string toString(ReplacementPolicy policy);
ReplacementPolicy parseReplacementPolicy(const std::string& text);
