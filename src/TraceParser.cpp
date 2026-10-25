#include "TraceParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::uint64_t parseAddress(std::string text, std::size_t lineNumber) {
    text = trim(text);
    if (text.empty()) {
        throw std::runtime_error("Missing address on trace line " +
                                 std::to_string(lineNumber));
    }

    try {
        std::size_t parsedChars = 0;
        const int base = (text.size() > 2 && text[0] == '0' &&
                          (text[1] == 'x' || text[1] == 'X')) ? 16 : 10;
        const std::uint64_t address = std::stoull(text, &parsedChars, base);
        if (parsedChars != text.size()) {
            throw std::runtime_error("Invalid address");
        }
        return address;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid address on trace line " +
                                 std::to_string(lineNumber) + ": " + text);
    }
}
} // namespace

std::vector<MemoryAccess> parseTrace(std::istream& input) {
    std::vector<MemoryAccess> accesses;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        // Accept either whitespace or a single comma as a separator.
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream row(line);

        std::string operationText;
        std::string addressText;
        std::string extra;
        if (!(row >> operationText >> addressText) || (row >> extra)) { // makes use of short-circuiting
            throw std::runtime_error(
                "Expected exactly an operation and address on trace line " +
                std::to_string(lineNumber));
        }

        if (operationText.size() != 1) {
            throw std::runtime_error("Invalid operation on trace line " +
                                     std::to_string(lineNumber));
        }

        // TODO : fully understand the purpose of this cast
        const char operation = static_cast<char>(
            std::toupper(static_cast<unsigned char>(operationText[0])));
        if (operation != 'R' && operation != 'W') {
            throw std::runtime_error("Operation must be R or W on trace line " +
                                     std::to_string(lineNumber));
        }

        accesses.push_back(
            MemoryAccess{operation, parseAddress(addressText, lineNumber),
                         lineNumber});
    }

    if (input.bad()) {
        throw std::runtime_error("I/O error while reading trace.");
    }
    return accesses;
}

std::vector<MemoryAccess> parseTraceFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open trace file: " + path);
    }
    return parseTrace(file);
}
