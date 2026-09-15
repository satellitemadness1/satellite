// satellite-004/strings/test_string_methods.cpp -- calls the satellite_string
// method libraries through the real number index, one case per input line, and
// prints each answer the way 003's `display` prints it, so check_string_methods.py
// can compare 004 with 003 line for line. Not part of the interpreter.
//
// A case line, fields separated by \x1f (so a tab can be text):
//   <word path> <receiver> <count of text args>|<arg>|<arg>... <positions, , between>
// An answer line:              the answer as 003 displays it, "REFUSED <code name>",
//                              or for append/clear the receiver afterwards.
// Written 2026-09-14.

#include "machine_codes.hpp"
#include "machine_state.hpp"
#include "satellite-numbers/call_number.hpp"
#include "strings/satellite_string.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace satellite004;

static satellite_string from_utf8(const std::string &text)
{
    satellite_string s;
    size_t bad = 0;
    utf8_to_char32(text, s.text, bad);
    return s;
}

static std::string to_utf8(const satellite_string &s)
{
    std::string out;
    size_t bad = 0;
    char32_to_utf8(s.text, out, bad);
    return out;
}

static std::vector<std::string> split(const std::string &text, char separator)
{
    std::vector<std::string> parts;
    size_t start = 0;
    for (;;) {
        const size_t at = text.find(separator, start);
        parts.push_back(text.substr(start, at == std::string::npos ? std::string::npos : at - start));
        if (at == std::string::npos)
            return parts;
        start = at + 1;
    }
}

int main()
{
    MachineState state;
    NumberIndex index;
    if (index.load("build/satellite-numbers", state) != number_vector_defined)
        return 5;

    std::string line;
    while (std::getline(std::cin, line)) {
        const std::vector<std::string> field = split(line, '\x1f');
        const NumberRow *row = field.size() == 4 ? index.find(field[0]) : nullptr;
        if (row == nullptr || row->scenarios.string_method == nullptr) {
            std::cout << "NO SUCH METHOD " << field[0] << "\n";
            continue;
        }
        satellite_string self = from_utf8(field[1]);
        StringArguments arguments;
        const std::vector<std::string> texts = split(field[2], '|');  // first part is the count
        const long count = std::atol(texts[0].c_str());
        for (long k = 1; k <= count && k < (long)texts.size(); k++)
            arguments.strings.push_back(from_utf8(texts[k]));
        if (!field[3].empty())
            for (const std::string &position : split(field[3], ','))
                arguments.positions.push_back(std::atoll(position.c_str()));

        StringAnswer answer;
        const signed long long int code = row->scenarios.string_method(self, arguments, answer);
        if (code != success) {
            std::cout << "REFUSED " << machine_code_name(code) << "\n";
            continue;
        }
        switch (answer.kind) {
        case StringAnswer::Kind::nothing: std::cout << to_utf8(self) << "\n"; break;
        case StringAnswer::Kind::string: std::cout << to_utf8(answer.text) << "\n"; break;
        case StringAnswer::Kind::count: std::cout << answer.count << "\n"; break;
        case StringAnswer::Kind::flag: std::cout << (answer.flag ? "true" : "false") << "\n"; break;
        case StringAnswer::Kind::strings: {
            std::cout << "[";
            for (size_t i = 0; i < answer.list.size(); i++)
                std::cout << (i ? ", " : "") << to_utf8(answer.list[i]);
            std::cout << "]\n";
            break;
        }
        }
    }
}
