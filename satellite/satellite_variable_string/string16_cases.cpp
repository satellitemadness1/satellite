// satellite/satellite_variable_string/string16_cases.cpp -- the test harness for
// satellite_string. check_strings16.py feeds it one operation per line and holds
// every answer against Python's strict UTF-8 codec and the header's table. Not
// part of the interpreter.
//
// Text is written as hex bytes of UTF-8 ("-" for none); lists of codes, Unicode
// numbers and positions are hex or decimal as each line says. Every answer that
// shows a string shows it as  codes=<hex codes, comma separated, or -> fast=<0|1>.
//
//   U <utf8>                    from_utf8 into a new string
//                               -> ok codes=.. fast=.. utf8=<to_utf8 as hex>  |  bad <byte offset> codes=.. fast=..
//   UP <utf8 before> <utf8>     the same, into a string that already holds <before>
//   C <unicode hex,..>          append_code(code_of(u)) one by one, then to_utf8
//                               -> ok fast=.. utf8=..  |  bad <index refused> codes=.. fast=..
//   B <bits>                    .sati bit text, 32 bits a Unicode number: each value
//                               handed to append_code(code_of(value)), so a value
//                               that is no character is refused by satellite_string
//                               -> ok <unicode hex,..>  |  bad <offset in the bits>
//   A <utf8 left> <utf8 right>  left.append(right) -> codes=.. fast=..
//   AA <utf8>                   s.append(s)        -> codes=.. fast=..
//   S <utf8> <start> <end> <utf8 out before>
//                               substring into a string holding <out before>
//                               -> ok codes=.. fast=..  |  bad <machine code> codes=<out after> fast=..
//   SS <utf8> <start> <end>     s.substring(start, end, s) -> the same shape
//   K <utf8>                    clear, then append_code(1) (a)
//                               -> cleared size=.. empty=.. fast=.. then codes=.. fast=..
//   P <utf8> <index>            code_at -> ok <code hex>  |  bad <machine code>
//   M <utf8 left> <utf8 right>  compare and the operators -> compare=.. equal=.. not_equal=.. less=..

#include "satellite_string.hpp"

#include "../machine/machine_codes.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace satellite004;

namespace {

std::string bytes_of(const std::string &hex)
{
    std::string bytes;
    if (hex == "-")
        return bytes;
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2)
        bytes += static_cast<char>(std::strtoul(hex.substr(i, 2).c_str(), nullptr, 16));
    return bytes;
}

std::string hex_of(const std::string &bytes)
{
    static const char digits[] = "0123456789abcdef";
    std::string hex;
    for (unsigned char b : bytes) {
        hex += digits[b >> 4];
        hex += digits[b & 15];
    }
    return hex.empty() ? "-" : hex;
}

std::string hex_list(const std::vector<char32_t> &values)
{
    std::string text;
    char buffer[16];
    for (char32_t value : values) {
        std::snprintf(buffer, sizeof buffer, "%s%x", text.empty() ? "" : ",", static_cast<unsigned int>(value));
        text += buffer;
    }
    return text.empty() ? "-" : text;
}

// codes=.. fast=.. -- read through code_at, the checked way, which must agree
// with code_at_unchecked and with size().
std::string shown(const satellite_string &s)
{
    std::vector<char32_t> codes;
    for (std::size_t k = 0;; k++) {
        char32_t code = 0;
        if (s.code_at(k, code) != success)
            break;
        if (code != s.code_at_unchecked(k))
            return "code_at and code_at_unchecked DISAGREE";
        codes.push_back(code);
    }
    if (codes.size() != s.size() || s.empty() != codes.empty())
        return "size() DISAGREES with code_at";
    return "codes=" + hex_list(codes) + " fast=" + (s.fast() ? "1" : "0");
}

// A string made from UTF-8 the test knows is valid.
satellite_string made(const std::string &hex)
{
    satellite_string s;
    std::size_t bad = 0;
    if (satellite_string::from_utf8(bytes_of(hex), s, bad) != success)
        std::cout << "(test text " << hex << " is not valid UTF-8) ";
    return s;
}

std::size_t position(const std::string &text)
{
    return static_cast<std::size_t>(std::strtoull(text.c_str(), nullptr, 10));
}

void decode(const std::string &hex, satellite_string &out)
{
    std::size_t bad = 0;
    if (satellite_string::from_utf8(bytes_of(hex), out, bad) == success)
        std::cout << "ok " << shown(out) << " utf8=" << hex_of(out.to_utf8()) << "\n";
    else
        std::cout << "bad " << bad << " " << shown(out) << "\n";
}

void bits(const std::string &text)
{
    const std::string digits = text == "-" ? "" : text;
    if (digits.size() % 32 != 0) {
        std::cout << "bad " << digits.size() - digits.size() % 32 << "\n";
        return;
    }
    satellite_string s;
    for (std::size_t i = 0; i < digits.size(); i += 32) {
        char32_t value = 0;
        for (unsigned int bit = 0; bit < 32; bit++) {
            const char c = digits[i + bit];
            if (c != '0' && c != '1') {
                std::cout << "bad " << i + bit << "\n";
                return;
            }
            value = (value << 1) | static_cast<char32_t>(c == '1');
        }
        if (s.append_code(satellite_string::code_of(value)) != success) {
            std::cout << "bad " << i << "\n";
            return;
        }
    }
    std::vector<char32_t> unicode;
    for (std::size_t k = 0; k < s.size(); k++)
        unicode.push_back(satellite_string::unicode_of(s.code_at_unchecked(k)));
    std::cout << "ok " << hex_list(unicode) << "\n";
}

} // namespace

int main()
{
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream words(line);
        std::string kind, first, second, third, fourth;
        words >> kind >> first >> second >> third >> fourth;
        if (kind == "U") {
            satellite_string out;
            decode(first, out);
        } else if (kind == "UP") {
            satellite_string out = made(first);
            decode(second, out);
        } else if (kind == "C") {
            satellite_string s;
            std::size_t index = 0, start = 0;
            bool refused = false;
            while (first != "-" && start < first.size()) {
                std::size_t comma = first.find(',', start);
                if (comma == std::string::npos)
                    comma = first.size();
                const char32_t unicode = static_cast<char32_t>(std::strtoull(first.substr(start, comma - start).c_str(), nullptr, 16));
                if (s.append_code(satellite_string::code_of(unicode)) != success) {
                    std::cout << "bad " << index << " " << shown(s) << "\n";
                    refused = true;
                    break;
                }
                index++;
                start = comma + 1;
            }
            if (!refused)
                std::cout << "ok fast=" << (s.fast() ? "1" : "0") << " utf8=" << hex_of(s.to_utf8()) << "\n";
        } else if (kind == "B") {
            bits(first);
        } else if (kind == "A") {
            satellite_string left = made(first);
            left.append(made(second));
            std::cout << shown(left) << "\n";
        } else if (kind == "AA") {
            satellite_string s = made(first);
            s.append(s);
            std::cout << shown(s) << "\n";
        } else if (kind == "S" || kind == "SS") {
            const satellite_string s = made(first);
            satellite_string out = kind == "S" ? made(fourth) : s;
            const signed long long int code = kind == "S" ? s.substring(position(second), position(third), out)
                                                          : out.substring(position(second), position(third), out);
            if (code == success)
                std::cout << "ok " << shown(out) << "\n";
            else
                std::cout << "bad " << code << " " << shown(out) << "\n";
        } else if (kind == "K") {
            satellite_string s = made(first);
            s.clear();
            std::cout << "cleared size=" << s.size() << " empty=" << s.empty() << " fast=" << s.fast();
            s.append_code(1);
            std::cout << " then " << shown(s) << "\n";
        } else if (kind == "P") {
            char32_t code = 0;
            const signed long long int answer = made(first).code_at(position(second), code);
            if (answer == success)
                std::cout << "ok " << std::hex << static_cast<unsigned int>(code) << std::dec << "\n";
            else
                std::cout << "bad " << answer << "\n";
        } else if (kind == "M") {
            const satellite_string left = made(first), right = made(second);
            std::cout << "compare=" << satellite_string::compare(left, right) << " equal=" << (left == right)
                      << " not_equal=" << (left != right) << " less=" << (left < right) << "\n";
        } else {
            std::cout << "unknown operation\n";
        }
    }
}
