#include "Cache.h"
#include "TraceParser.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void printUsage(const char* executable) {
    std::cerr
        << "Usage: " << executable
        << " --capacity BYTES --block-size BYTES --associativity WAYS\n"
        << "       --replacement LRU|FIFO --trace PATH [--csv PATH]\n";
}

std::size_t parsePositiveSize(const std::string& text, const std::string& name) {
    try {
        std::size_t parsed = 0;
        const unsigned long long value = std::stoull(text, &parsed, 10);
        if (parsed != text.size() || value == 0 ||
            value > static_cast<unsigned long long>(
                        static_cast<std::size_t>(-1))) {
            throw std::invalid_argument("invalid");
        }
        return static_cast<std::size_t>(value);
    } catch (const std::exception&) {
        throw std::invalid_argument(name + " must be a positive integer.");
    }
}

struct Arguments {
    CacheConfig config;
    std::string tracePath;
    std::string csvPath;
};

Arguments parseArguments(int argc, char* argv[]) {
    Arguments args;
    bool haveCapacity = false, haveBlock = false, haveAssoc = false;
    bool haveReplacement = false, haveTrace = false;

    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help" || key == "-h") {
            printUsage(argv[0]);
            std::exit(0);
        }
        if (i + 1 >= argc) {
            throw std::invalid_argument("Missing value for " + key);
        }
        const std::string value = argv[++i];

        if (key == "--capacity") {
            args.config.capacityBytes = parsePositiveSize(value, "Capacity");
            haveCapacity = true;
        } else if (key == "--block-size") {
            args.config.blockSizeBytes = parsePositiveSize(value, "Block size");
            haveBlock = true;
        } else if (key == "--associativity") {
            args.config.associativity = parsePositiveSize(value, "Associativity");
            haveAssoc = true;
        } else if (key == "--replacement") {
            args.config.replacementPolicy = parseReplacementPolicy(value);
            haveReplacement = true;
        } else if (key == "--trace") {
            args.tracePath = value;
            haveTrace = true;
        } else if (key == "--csv") {
            args.csvPath = value;
        } else {
            throw std::invalid_argument("Unknown argument: " + key);
        }
    }

    if (!haveCapacity || !haveBlock || !haveAssoc ||
        !haveReplacement || !haveTrace) {
        throw std::invalid_argument("Missing one or more required arguments.");
    }
    args.config.validate();
    return args;
}

void writeCsv(const std::string& path, const CacheConfig& config,
              const CacheStats& stats) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Could not open CSV output: " + path);
    }
    output << "capacity_bytes,block_size_bytes,associativity,replacement,"
              "accesses,reads,writes,hits,misses,hit_rate_percent,miss_rate_percent\n";
    output << config.capacityBytes << ',' << config.blockSizeBytes << ','
           << config.associativity << ',' << toString(config.replacementPolicy)
           << ',' << stats.accesses << ',' << stats.reads << ',' << stats.writes
           << ',' << stats.hits << ',' << stats.misses << ','
           << std::fixed << std::setprecision(4) << stats.hitRate() << ','
           << stats.missRate() << '\n';
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        const Arguments args = parseArguments(argc, argv);
        const auto trace = parseTraceFile(args.tracePath);

        Cache cache(args.config);
        for (const auto& access : trace) {
            cache.access(access.address, access.operation);
        }

        const CacheStats& stats = cache.stats();
        std::cout << "Cache Configuration\n"
                  << "-------------------\n"
                  << "Capacity:        " << args.config.capacityBytes << " bytes\n"
                  << "Block Size:      " << args.config.blockSizeBytes << " bytes\n"
                  << "Associativity:   " << args.config.associativity << "-way\n"
                  << "Number of Sets:  " << args.config.numberOfSets() << '\n'
                  << "Replacement:     " << toString(args.config.replacementPolicy)
                  << "\n\nSimulation Results\n"
                  << "------------------\n"
                  << "Memory Accesses: " << stats.accesses << '\n'
                  << "Reads:           " << stats.reads << '\n'
                  << "Writes:          " << stats.writes << '\n'
                  << "Cache Hits:      " << stats.hits << '\n'
                  << "Cache Misses:    " << stats.misses << '\n'
                  << std::fixed << std::setprecision(2)
                  << "Hit Rate:        " << stats.hitRate() << "%\n"
                  << "Miss Rate:       " << stats.missRate() << "%\n";

        if (!args.csvPath.empty()) {
            writeCsv(args.csvPath, args.config, stats);
            std::cout << "\nCSV written to: " << args.csvPath << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
