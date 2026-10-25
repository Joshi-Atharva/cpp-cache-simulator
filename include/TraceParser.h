#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

struct MemoryAccess {
    char operation;             // 'R' or 'W'
    std::uint64_t address;
    std::size_t lineNumber;
};

std::vector<MemoryAccess> parseTrace(std::istream& input);
std::vector<MemoryAccess> parseTraceFile(const std::string& path);
