#include "Cache.h"
#include "TraceParser.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
int testsRun = 0;
int testsFailed = 0;

void check(bool condition, const std::string& message) {
    ++testsRun;
    if (!condition) {
        ++testsFailed;
        std::cerr << "[FAIL] " << message << '\n';
    } else {
        std::cout << "[PASS] " << message << '\n';
    }
}

template <typename Function>
void checkThrows(Function function, const std::string& message) {
    bool threw = false;
    try {
        function();
    } catch (const std::exception&) {
        threw = true;
    }
    check(threw, message);
}

void testColdAndRepeatedAccess() {
    Cache cache(CacheConfig{1024, 64, 1, ReplacementPolicy::LRU});
    check(!cache.access(0x1000, 'R'), "first access is a cold miss");
    check(cache.access(0x1000, 'R'), "repeated access is a hit");
    check(cache.access(0x103F, 'W'), "same block address is a hit");
    check(cache.stats().accesses == 3, "access count is tracked");
    check(cache.stats().reads == 2 && cache.stats().writes == 1,
          "read/write counts are tracked");
    check(cache.stats().hits == 2 && cache.stats().misses == 1,
          "hit/miss counts are tracked");
}

void testDirectMappedConflict() {
    // Capacity 128 B, 64 B blocks => two direct-mapped sets.
    Cache cache(CacheConfig{128, 64, 1, ReplacementPolicy::LRU});
    check(!cache.access(0, 'R'), "direct-mapped initial miss");
    check(!cache.access(128, 'R'), "same-index address causes conflict miss");
    check(!cache.access(0, 'R'), "evicted block misses when accessed again");
}

void testAssociativityAvoidsConflict() {
    // One 128 B set with two ways; blocks 0 and 128 map to different tags,
    // but to the same set. Both should fit in this 2-way set.
    Cache cache(CacheConfig{128, 64, 2, ReplacementPolicy::LRU});
    check(!cache.access(0, 'R'), "2-way first block misses");
    check(!cache.access(128, 'R'), "2-way second block misses");
    check(cache.access(0, 'R'), "2-way cache retains both blocks");
}

void testLruVersusFifo() {
    // Two lines, block size 64. Access A, B, A, C:
    // LRU evicts B; FIFO also evicts A because FIFO ignores the hit to A.
    Cache lru(CacheConfig{128, 64, 2, ReplacementPolicy::LRU});
    Cache fifo(CacheConfig{128, 64, 2, ReplacementPolicy::FIFO});

    lru.access(0, 'R');    // A
    lru.access(64, 'R');   // B
    lru.access(0, 'R');    // refresh A only for LRU
    lru.access(128, 'R');  // C
    check(lru.access(0, 'R'), "LRU retains recently used A");

    fifo.access(0, 'R');
    fifo.access(64, 'R');
    fifo.access(0, 'R');
    fifo.access(128, 'R');
    check(!fifo.access(0, 'R'), "FIFO evicts oldest inserted A");
}

void testResetAndEmptyRates() {
    Cache cache(CacheConfig{128, 64, 1, ReplacementPolicy::LRU});
    check(cache.stats().hitRate() == 0.0, "empty cache hit rate is zero");
    cache.access(0, 'R');
    cache.reset();
    check(cache.stats().accesses == 0, "reset clears statistics");
    check(!cache.access(0, 'R'), "reset invalidates cache lines");
}

void testInvalidInputs() {
    checkThrows([] { Cache(CacheConfig{100, 64, 1, ReplacementPolicy::LRU}); },
                "invalid capacity divisibility is rejected");
    Cache cache(CacheConfig{128, 64, 1, ReplacementPolicy::LRU});
    checkThrows([&] { cache.access(0, 'X'); }, "invalid operation is rejected");
}

void testTraceParser() {
    std::istringstream trace(
        "# sample trace\n"
        "R 0x1000\n"
        "W,0x1040\n"
        "r 4160\n");
    const auto accesses = parseTrace(trace);
    check(accesses.size() == 3, "trace parser reads three accesses");
    check(accesses[0].operation == 'R' && accesses[0].address == 0x1000,
          "trace parser reads hexadecimal address");
    check(accesses[1].operation == 'W' && accesses[1].address == 0x1040,
          "trace parser supports comma separator");
    check(accesses[2].operation == 'R' && accesses[2].address == 4160,
          "trace parser accepts lowercase operation and decimal address");

    std::istringstream badTrace("X 123\n");
    checkThrows([&] { parseTrace(badTrace); },
                "trace parser rejects invalid operation");
}
} // namespace

int main() {
    testColdAndRepeatedAccess();
    testDirectMappedConflict();
    testAssociativityAvoidsConflict();
    testLruVersusFifo();
    testResetAndEmptyRates();
    testInvalidInputs();
    testTraceParser();

    std::cout << "\nTests run: " << testsRun
              << ", failures: " << testsFailed << '\n';
    return testsFailed == 0 ? 0 : 1;
}
