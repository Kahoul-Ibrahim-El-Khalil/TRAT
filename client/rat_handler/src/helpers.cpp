#include "rat/Handler.hpp"

#include <boost/algorithm/string.hpp>

namespace rat::handler {
// Helper functions
void __normalizeWhiteSpaces(std::string &String_Input) {
    boost::trim(String_Input);
    boost::replace_all(String_Input, "  ", " ");
}

size_t __countWhiteSpaces(const std::string &String_Input) {
    return std::count(String_Input.begin(), String_Input.end(), ' ');
}

size_t __findFirstOccurenceOfChar(const std::string &Input_String, char arg_C) {
    auto pos = Input_String.find(arg_C);
    return pos != std::string::npos ? pos : Input_String.size();
}

uint16_t _stringToUint16(const std::string &str) {
    try {
        unsigned long value = std::stoul(str);
        if(value > static_cast<unsigned long>(std::numeric_limits<uint16_t>::max())) {
            throw std::out_of_range("Value too large for uint16_t");
        }
        return static_cast<uint16_t>(value);
    } catch(const std::exception &) {
        throw;
    }
}

std::vector<std::string> splitArguments(const std::string &Input_String) {
    std::vector<std::string> args;
    std::string current;
    bool in_quotes = false;

    for(size_t i = 0; i < Input_String.size(); ++i) {
        char c = Input_String[i];

        if(c == '"') {
            in_quotes = !in_quotes; // toggle quote state
        } else if(c == ' ' && !in_quotes) {
            if(!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if(!current.empty())
        args.push_back(current);
    return args;
}

std::string stripQuotes(const std::string &Input_String) {
    std::string string = Input_String;

    // Remove outer double or single quotes
    if(string.size() >= 2 && ((string.front() == '"' && string.back() == '"') ||
                              (string.front() == '\'' && string.back() == '\''))) {
        string = string.substr(1, string.size() - 2);
    }

    // Remove remaining single quotes at start/end
    if(!string.empty() && string.front() == '\'')
        string.erase(0, 1);
    if(!string.empty() && string.back() == '\'')
        string.pop_back();

    return string;
}

} // namespace rat::handler

#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
#undef DEBUG_RAT_HANDLER
#endif
