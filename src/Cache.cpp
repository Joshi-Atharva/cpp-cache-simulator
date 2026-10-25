#include "Cache.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

void CacheConfig::validate() const {
    if (capacityBytes == 0 || blockSizeBytes == 0 || associativity == 0) {
        throw std::invalid_argument(
            "Capacity, block size, and associativity must be positive.");
    }

    if (blockSizeBytes > capacityBytes) {
        throw std::invalid_argument("Block size cannot exceed cache capacity.");
    }

    if (associativity > capacityBytes / blockSizeBytes) {
        throw std::invalid_argument(
            "Associativity cannot exceed the number of cache blocks.");
    }

    const std::size_t bytesPerSet = blockSizeBytes * associativity;
    if (capacityBytes % bytesPerSet != 0) {
        throw std::invalid_argument(
            "Capacity must be divisible by block size * associativity.");
    }
}

std::size_t CacheConfig::numberOfSets() const {
    validate();
    return capacityBytes / (blockSizeBytes * associativity);
}

std::size_t CacheConfig::linesPerSet() const {
    validate();
    return associativity;
}

std::string toString(ReplacementPolicy policy) {
    switch (policy) {
        case ReplacementPolicy::LRU: return "LRU";
        case ReplacementPolicy::FIFO: return "FIFO";
    }
    throw std::invalid_argument("Unknown replacement policy.");
}

ReplacementPolicy parseReplacementPolicy(const std::string& text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (unsigned char c : text) {
        normalized.push_back(static_cast<char>(std::toupper(c)));
    }

    if (normalized == "LRU") return ReplacementPolicy::LRU;
    if (normalized == "FIFO") return ReplacementPolicy::FIFO;
    throw std::invalid_argument("Replacement policy must be LRU or FIFO.");
}

Cache::Cache(CacheConfig config) : config_(config) {
    config_.validate();
    sets_.resize(config_.numberOfSets(),
                 std::vector<Line>(config_.linesPerSet()));
}

void Cache::validateOperation(char operation) const {
    const char normalized = static_cast<char>(
        std::toupper(static_cast<unsigned char>(operation)));
    if (normalized != 'R' && normalized != 'W') {
        throw std::invalid_argument("Operation must be R (read) or W (write).");
    }
}

bool Cache::access(std::uint64_t address, char operation) {
    validateOperation(operation);
    operation = static_cast<char>(
        std::toupper(static_cast<unsigned char>(operation)));

    ++stats_.accesses;
    if (operation == 'R') ++stats_.reads;
    else ++stats_.writes;

    ++accessClock_;

    const std::uint64_t blockNumber =
        address / static_cast<std::uint64_t>(config_.blockSizeBytes);
    const std::size_t setIndex = static_cast<std::size_t>(
        blockNumber % static_cast<std::uint64_t>(sets_.size()));
    const std::uint64_t tag =
        blockNumber / static_cast<std::uint64_t>(sets_.size());

    auto& set = sets_[setIndex];

    for (auto& line : set) {
        if (line.valid && line.tag == tag) {
            ++stats_.hits;
            line.lastUsedOrder = accessClock_;
            return true;
        }
    }

    ++stats_.misses;

    // Prefer an invalid line before evicting a valid one.
    auto victim = std::find_if(set.begin(), set.end(),
                               [](const Line& line) { return !line.valid; });

    if (victim == set.end()) {
        victim = std::min_element(
            set.begin(), set.end(),
            [this](const Line& a, const Line& b) {
                if (config_.replacementPolicy == ReplacementPolicy::LRU) {
                    return a.lastUsedOrder < b.lastUsedOrder;
                }
                return a.insertionOrder < b.insertionOrder;
            });
    }

    victim->valid = true;
    victim->tag = tag;
    victim->insertionOrder = accessClock_;
    victim->lastUsedOrder = accessClock_;
    return false;
}

void Cache::reset() {
    stats_ = CacheStats{};
    accessClock_ = 0;
    for (auto& set : sets_) {
        for (auto& line : set) {
            line = Line{};
        }
    }
}
