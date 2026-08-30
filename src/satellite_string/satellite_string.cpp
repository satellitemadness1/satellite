// satellite_string: encode and decode between byte text and the code table.
// See satellite_string.hpp for the table layout and the reasoning.
//
// Ported at M3 from old_versions/first_satellite/src/satellite_string/
// satellite_string.cpp (155 lines). Four changes, each with its reason:
//
// 1. THE LIVE-VALUE DECODE IS STUBBED, because its readers are M14's and this
//    module arrived four milestones early for the lexer's sake. The header says
//    why nothing in the lexer can reach one.
// 2. The escape table is constexpr and its ordering rule is now a
//    static_assert rather than a comment -- see kEscapes.
// 3. PUNCT and ESCAPES are kPunct and kEscapes, per FORMAT/CXX.md §3.
// 4. encode_byte() finds a punctuation code with string_view::find over a view
//    of known length, rather than strchr() guarded by `c != '\0'`. The guard
//    was there because strchr finds the terminator and would have numbered a
//    NUL as punctuation 32; a view that stops at 32 cannot, so the special
//    case goes away instead of being carried.

#include "satellite_string/satellite_string.hpp"

#include <cstddef>
#include <string_view>

namespace satellite {

namespace {

// Codes 63..94, in the order the language defines them.
constexpr char kPunct[] = "!@#$%^&*()-_=+[{]}\\|;:'\",<.>/?`~";
static_assert(sizeof(kPunct) - 1 == 32, "punctuation table must be 32 chars");

// Keep the named codes in satellite_string.hpp honest. The casts are needed
// because the two enums are distinct unnamed types, and C++20 deprecates
// arithmetic between them.
constexpr size_t punct_index(SatChar code)
{
    return size_t(code) - size_t(SatChar(SAT_PUNCT_BASE));
}
static_assert(kPunct[punct_index(SAT_BANG)]       == '!',  "SAT_BANG");
static_assert(kPunct[punct_index(SAT_UNDERSCORE)] == '_',  "SAT_UNDERSCORE");
static_assert(kPunct[punct_index(SAT_EQUAL)]      == '=',  "SAT_EQUAL");
static_assert(kPunct[punct_index(SAT_BACKSLASH)]  == '\\', "SAT_BACKSLASH");
static_assert(kPunct[punct_index(SAT_APOSTROPHE)] == '\'', "SAT_APOSTROPHE");
static_assert(kPunct[punct_index(SAT_QUOTE)]      == '"',  "SAT_QUOTE");
static_assert(kPunct[punct_index(SAT_LESS)]       == '<',  "SAT_LESS");
static_assert(kPunct[punct_index(SAT_DOT)]        == '.',  "SAT_DOT");
static_assert(kPunct[punct_index(SAT_GREATER)]    == '>',  "SAT_GREATER");
static_assert(kPunct[punct_index(SAT_SLASH)]      == '/',  "SAT_SLASH");

struct Escape {
    std::string_view name;
    SatChar code;
};

// Longest names first: encode() takes the FIRST match, so "t" must not shadow
// "threads". backslash-backslash and backslash-quote resolve to the real table
// codes for those characters; \n \t \r have no code-table entry, so they park
// in the raw area (DESIGN §5.4).
constexpr Escape kEscapes[] = {
    {"memtotal", SAT_MEM_TOTAL_MB}, {"memused", SAT_MEM_USED_MB},
    {"threads", SAT_THREADS},       {"home", SAT_LINUX_HOME},
    {"user", SAT_LINUX_USERNAME},   {"void", SAT_VOID},
    {"cwd", SAT_CWD},
    {"n", SAT_RAW_BASE + '\n'},     {"t", SAT_RAW_BASE + '\t'},
    {"r", SAT_RAW_BASE + '\r'},     {"\\", SAT_BACKSLASH},
    {"\"", SAT_QUOTE},
    // \' is a REDUNDANT spelling of '. An apostrophe needs no escaping inside a
    // double-quoted string, and there are no single-quoted strings for it to be
    // escaping from -- so a program that writes "DOESN\'T" is being over-careful
    // rather than wrong, and used to be punished for it by having the backslash
    // kept in the output. That was silent: no error, just a stray backslash in
    // every line, 501 of them in one real program.
    //
    // Accepted rather than rejected because the alternative -- making an unknown
    // escape an error -- would break \d, \s and every other backslash somebody
    // has already written into a string on purpose, and encode() has no way to
    // tell those apart from a mistake. DESIGN §13 records the decision.
    {"'", SAT_APOSTROPHE},
};

constexpr size_t kEscapeCount = sizeof(kEscapes) / sizeof(kEscapes[0]);

// NO NAME MAY BE A PREFIX OF A LATER ONE, and this is the ordering rule above
// made checkable instead of merely stated. v1 held it by hand: "t" sits four
// rows below "threads" and nothing but care kept it there, so an alphabetised
// edit or an appended name would have silently made the longer one unreachable
// and turned "\threads" into a tab followed by "hreads". FORMAT/CXX.md §1 --
// prose may explain a number, it may never be the only place the number lives.
constexpr bool no_name_shadows_a_later_one()
{
    for (size_t i = 0; i < kEscapeCount; i++)
        for (size_t j = i + 1; j < kEscapeCount; j++)
            if (kEscapes[j].name.starts_with(kEscapes[i].name))
                return false;
    return true;
}
static_assert(no_name_shadows_a_later_one(),
              "kEscapes: an escape name is a prefix of a later one -- encode() "
              "takes the first match, so the later one can never be reached");

// What decode() prints for a live value until M7/M14 port the readers.
//
// LOOKED UP IN kEscapes RATHER THAN WRITTEN OUT, so the placeholder and the
// escape that produces the code cannot drift: "\threads" round-trips to
// "<threads>" by construction, and a table edit moves both.
std::string placeholder_for(SatChar code)
{
    for (const Escape &esc : kEscapes)
        if (esc.code == code)
            return "<" + std::string(esc.name) + ">";
    return "<?>";
}

// One byte -> one code, with no escape handling. Shared by encode() and
// encode_raw() so the two can never disagree about the code table.
SatChar encode_byte(char c)
{
    if (c >= 'a' && c <= 'z')
        return SAT_A + (c - 'a');
    if (c >= 'A' && c <= 'Z')
        return SAT_UPPER_A + (c - 'A');
    if (c >= '0' && c <= '9')
        return SAT_DIGIT_0 + (c - '0');
    const std::string_view punct(kPunct, sizeof(kPunct) - 1);
    if (const size_t at = punct.find(c); at != std::string_view::npos)
        return SAT_PUNCT_BASE + SatChar(at);
    // Unassigned by the table so far -- park it in the raw area.
    return SAT_RAW_BASE + SatChar(static_cast<unsigned char>(c));
}

} // namespace

SatString encode(const std::string &text)
{
    SatString out;
    out.reserve(text.size());

    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];

        if (c == '\\') {
            bool matched = false;
            for (const Escape &esc : kEscapes) {
                if (text.compare(i + 1, esc.name.size(), esc.name) == 0) {
                    out.push_back(esc.code);
                    i += esc.name.size();
                    matched = true;
                    break;
                }
            }
            if (matched)
                continue;
            // no escape name: fall through, backslash is a punctuation code
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
            out += kPunct[c - SAT_PUNCT_BASE];
            continue;
        }
        // THE LIVE VALUES (95-100), STUBBED UNTIL M7/M14. The readers are
        // home_dir(), username(), hardware_threads(), mem_total_mb(),
        // mem_used_mb() and cwd(), all in system_facts/, all ported by M14.
        // Replacing these six lines with those six calls is the whole of the
        // change, and this module's own test is what will notice it has not
        // happened.
        if (c >= SAT_LINUX_HOME && c <= SAT_CWD) {
            out += placeholder_for(c);
            continue;
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
