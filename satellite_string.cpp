#include "satellite_string.hpp"
#include "system.hpp"

#include <cstddef>
#include <cstring>

namespace satellite {

// Codes 63..94, in the order the language defines them.
static constexpr char PUNCT[] = "!@#$%^&*()-_=+[{]}\\|;:'\",<.>/?`~";
static_assert(sizeof(PUNCT) - 1 == 32, "punctuation table must be 32 chars");

// Keep the named codes in satellite_string.hpp honest. The casts are needed
// because the two enums are distinct unnamed types, and C++20 deprecates
// arithmetic between them.
static constexpr size_t punct_index(SatChar code)
{
    return size_t(code) - size_t(SatChar(SAT_PUNCT_BASE));
}
static_assert(PUNCT[punct_index(SAT_BANG)]       == '!',  "SAT_BANG");
static_assert(PUNCT[punct_index(SAT_UNDERSCORE)] == '_',  "SAT_UNDERSCORE");
static_assert(PUNCT[punct_index(SAT_EQUAL)]      == '=',  "SAT_EQUAL");
static_assert(PUNCT[punct_index(SAT_BACKSLASH)]  == '\\', "SAT_BACKSLASH");
static_assert(PUNCT[punct_index(SAT_QUOTE)]      == '"',  "SAT_QUOTE");
static_assert(PUNCT[punct_index(SAT_LESS)]       == '<',  "SAT_LESS");
static_assert(PUNCT[punct_index(SAT_DOT)]        == '.',  "SAT_DOT");
static_assert(PUNCT[punct_index(SAT_GREATER)]    == '>',  "SAT_GREATER");
static_assert(PUNCT[punct_index(SAT_SLASH)]      == '/',  "SAT_SLASH");

// Longest names first: the loop takes the first match, so "t" must not shadow
// "threads". \\ and \" resolve to the real table codes for those characters;
// \n \t \r have no code-table entry, so they park in the raw area.
static const struct {
    const char *name;
    SatChar code;
} ESCAPES[] = {
    {"memtotal", SAT_MEM_TOTAL_MB}, {"memused", SAT_MEM_USED_MB},
    {"threads", SAT_THREADS},       {"home", SAT_LINUX_HOME},
    {"user", SAT_LINUX_USERNAME},   {"void", SAT_VOID},
    {"cwd", SAT_CWD},
    {"n", SAT_RAW_BASE + '\n'},     {"t", SAT_RAW_BASE + '\t'},
    {"r", SAT_RAW_BASE + '\r'},     {"\\", SAT_BACKSLASH},
    {"\"", SAT_QUOTE},
};

// One byte -> one code, with no escape handling. Shared by encode() and
// encode_raw() so the two can never disagree about the code table.
static SatChar encode_byte(char c)
{
    if (c >= 'a' && c <= 'z')
        return SAT_A + (c - 'a');
    if (c >= 'A' && c <= 'Z')
        return SAT_UPPER_A + (c - 'A');
    if (c >= '0' && c <= '9')
        return SAT_DIGIT_0 + (c - '0');
    if (const char *p = strchr(PUNCT, c); p && c != '\0')
        return SAT_PUNCT_BASE + SatChar(p - PUNCT);
    // Unassigned by the table so far — park it in the raw area.
    return SAT_RAW_BASE + SatChar(static_cast<unsigned char>(c));
}

SatString encode(const std::string &text)
{
    SatString out;
    out.reserve(text.size());

    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];

        if (c == '\\') {
            bool matched = false;
            for (const auto &esc : ESCAPES) {
                size_t len = strlen(esc.name);
                if (text.compare(i + 1, len, esc.name) == 0) {
                    out.push_back(esc.code);
                    i += len;
                    matched = true;
                    break;
                }
            }
            if (matched)
                continue;
            // no escape name: fall through, '\' is a punctuation code
        }

        out.push_back(encode_byte(c));
    }
    return out;
}

SatString encode_raw(const std::string &text)
{
    SatString out;
    out.reserve(text.size());
    for (char c : text)
        out.push_back(encode_byte(c));
    return out;
}

std::string decode(const SatString &s)
{
    std::string out;
    out.reserve(s.size());

    for (SatChar c : s) {
        if (c == SAT_VOID)
            continue;
        if (c >= SAT_A && c < SAT_UPPER_A) {
            out += char('a' + (c - SAT_A));
            continue;
        }
        if (c >= SAT_UPPER_A && c < SAT_DIGIT_0) {
            out += char('A' + (c - SAT_UPPER_A));
            continue;
        }
        if (c >= SAT_DIGIT_0 && c < SAT_PUNCT_BASE) {
            out += char('0' + (c - SAT_DIGIT_0));
            continue;
        }
        if (c >= SAT_PUNCT_BASE && c < SAT_PUNCT_BASE + 32) {
            out += PUNCT[c - SAT_PUNCT_BASE];
            continue;
        }
        switch (c) {
        case SAT_LINUX_HOME:     out += home_dir(); continue;
        case SAT_LINUX_USERNAME: out += username(); continue;
        case SAT_THREADS:        out += std::to_string(hardware_threads()); continue;
        case SAT_MEM_TOTAL_MB:   out += std::to_string(mem_total_mb()); continue;
        case SAT_MEM_USED_MB:    out += std::to_string(mem_used_mb()); continue;
        case SAT_CWD:            out += cwd(); continue;
        default: break;
        }
        if (c >= SAT_RAW_BASE && c < SAT_RAW_BASE + 256) {
            out += char(c - SAT_RAW_BASE);
            continue;
        }
        out += '?'; // not a satellite char (yet)
    }
    return out;
}

} // namespace satellite
