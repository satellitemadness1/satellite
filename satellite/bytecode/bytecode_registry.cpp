// satellite/bytecode/bytecode_registry.cpp -- .satl in, 16-bit tokens out.
// The header says why the rows are bitset<16> and why the threads get batches.

#include "bytecode_registry.hpp"
#include "cascade_convert.hpp"

#include "../satellite_variable_string/character_table.hpp"
#include "word_codes.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <string_view>
#include <thread>

namespace satellite004 {
namespace {

using token::Code;

bool identifier_start(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool identifier_body(char c) { return identifier_start(c) || (c >= '0' && c <= '9'); }
bool a_digit(char c) { return c >= '0' && c <= '9'; }
bool a_hex_digit(char c) { return a_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
bool blank(char c) { return c == ' ' || c == '\t'; }

// One UTF-8 character at `i`, advancing it. A byte that cannot start a
// character is handed back as itself, so no input can stop the conversion.
std::uint32_t one_character(std::string_view s, std::size_t &i)
{
    const unsigned char first = static_cast<unsigned char>(s[i]);
    std::size_t extra = 0;
    std::uint32_t value = first;
    if (first >= 0xF0) { extra = 3; value = first & 0x07u; }
    else if (first >= 0xE0) { extra = 2; value = first & 0x0Fu; }
    else if (first >= 0xC0) { extra = 1; value = first & 0x1Fu; }
    else { ++i; return first; }
    if (i + extra >= s.size()) { ++i; return first; }
    for (std::size_t k = 1; k <= extra; ++k) {
        const unsigned char next = static_cast<unsigned char>(s[i + k]);
        if ((next & 0xC0u) != 0x80u) { ++i; return first; }
        value = (value << 6) | (next & 0x3Fu);
    }
    i += extra + 1;
    return value;
}

struct Line {
    std::string_view text;
    std::vector<std::bitset<16>> &row;
    std::size_t i = 0;

    void put(Code code) { row.push_back(std::bitset<16>(code)); }

    // One character of a payload: ASCII in the author's order, anything above
    // 127 behind wide_run_token so it can never be read as a token.
    static void character_codes(std::uint32_t c, std::vector<std::bitset<16>> &out)
    {
        if (c < 128) { out.push_back(std::bitset<16>(character_table::code_of_ascii[c])); return; }
        if (c <= 0xFFFF) {
            out.push_back(std::bitset<16>(token::wide_run_token));
            out.push_back(std::bitset<16>(1));
            out.push_back(std::bitset<16>(static_cast<Code>(c)));
            return;
        }
        out.push_back(std::bitset<16>(token::wide_run_32_token));
        out.push_back(std::bitset<16>(1));
        out.push_back(std::bitset<16>(static_cast<Code>(c >> 16)));
        out.push_back(std::bitset<16>(static_cast<Code>(c & 0xFFFFu)));
    }

    // A payload token, its count, and that many codes. The count covers every
    // code that follows, wide runs included, and a reader SKIPS them all.
    //
    // WRITTEN STRAIGHT INTO THE ROW, with the count left blank and filled in
    // afterwards. The obvious version built the codes in a temporary vector
    // first, which is one allocation for every literal in the program; on
    // 100,000 lines that and the rows' own growth were most of the 35 ms this
    // took (measured 2026-09-16).
    void put_payload(Code marker, std::size_t from, std::size_t to)
    {
        put(marker);
        const std::size_t count_at = row.size();
        put(0); // filled in below, once the codes are counted
        std::size_t k = from;
        while (k < to) character_codes(one_character(text, k), row);
        const std::size_t written = row.size() - count_at - 1;
        if (written <= 0xFFFFull) { row[count_at] = std::bitset<16>(static_cast<Code>(written)); return; }

        // A count past 65535 needs extra codes, so the payload shifts along to
        // make room for them. Rare by construction: it is a literal of more
        // than 65,535 codes, and it must still have no ceiling (DESIGN §1.2).
        std::vector<std::bitset<16>> chunks;
        unsigned long long int n = written;
        while (n > 0xFFFFull) {
            chunks.push_back(std::bitset<16>(token::long_count_token));
            chunks.push_back(std::bitset<16>(static_cast<Code>(n & 0xFFFFull)));
            n >>= 16;
        }
        chunks.push_back(std::bitset<16>(static_cast<Code>(n)));
        row.erase(row.begin() + static_cast<long>(count_at));
        row.insert(row.begin() + static_cast<long>(count_at), chunks.begin(), chunks.end());
    }

    bool two_ahead(const char *pair) const
    {
        return i + 1 < text.size() && text[i] == pair[0] && text[i + 1] == pair[1];
    }
};

struct TwoCharacter { const char *spelling; Code code; };

// Longest first: // and /* are tried before /, and !! before !.
const TwoCharacter kTwoCharacter[] = {
    {"==", token::equals_token},          {"!=", token::not_equals_token},
    {"<=", token::less_or_equal_token},   {">=", token::greater_or_equal_token},
    {"&&", token::and_token},             {"||", token::or_token},
    {"!!", token::bit_run_join_token},    {"<<", token::shift_left_token},
    {">>", token::shift_right_token},     {"+=", token::plus_assign_token},
    {"-=", token::minus_assign_token},    {"*=", token::times_assign_token},
    {"/=", token::divide_assign_token},   {"%=", token::modulus_assign_token},
    {"::", token::namespace_token},       {"->", token::arrow_token},
    {"/*", token::comment_start_token},   {"*/", token::comment_end_token},
};

// The six the author's rule covers, spaced and touching. 0 for anything else.
// Kept as two functions rather than a table so the registry's own names appear
// in the source and a reader can grep either spelling.
Code arithmetic_character(char c)
{
    switch (c) {
    case '+': return token::plus_token;
    case '-': return token::minus_token;
    case '*': return token::times_token;
    case '/': return token::divide_token;
    case '%': return token::modulus_token;
    case '^': return token::power_token;
    default: return 0;
    }
}

Code tight_arithmetic_character(char c)
{
    switch (c) {
    case '+': return token::tight_plus_token;
    case '-': return token::tight_minus_token;   // -5: the unary minus
    case '*': return token::tight_times_token;
    case '/': return token::fraction_token;      // handled above; here for completeness
    case '%': return token::tight_modulus_token;
    case '^': return token::tight_power_token;
    default: return 0;
    }
}

Code one_character_token(char c)
{
    switch (c) {
    case '{': return token::left_brace_token;
    case '}': return token::right_brace_token;
    case '(': return token::left_parenthesis_token;
    case ')': return token::right_parenthesis_token;
    case '[': return token::left_square_bracket_token;
    case ']': return token::right_square_bracket_token;
    // + - * / % ^ ARE NOT HERE. They are the author's six arithmetic
    // characters and the whitespace rule above claims every one of them, spaced
    // or touching, before this is ever reached. `^` in particular no longer
    // reaches bit_exclusive_or_token: the author ruled on 2026-09-16 that `^` is
    // power, and the registry row says so.
    case '=': return token::assign_token;
    case '<': return token::less_than_token;
    case '>': return token::greater_than_token;
    case ',': return token::comma_token;
    case ';': return token::semicolon_token;
    case ':': return token::colon_token;
    case '.': return token::method_token;
    case '!': return token::not_token;
    case '&': return token::bit_and_token;
    case '|': return token::bit_or_token;
    case '~': return token::bit_not_token;
    case '?': return token::question_token;
    case '\'': return token::single_quote_token;
    case '\\': return token::escape_token;
    default: return 0;
    }
}

} // namespace

void tokenise_one_line(std::string_view text, std::vector<std::bitset<16>> &row)
{
    Line line{text, row};
    const std::size_t n = text.size();
    row.reserve(n + 8); // one code a character, plus the line's own markers

    while (line.i < n) {
        const char c = text[line.i];
        if (blank(c)) { ++line.i; continue; }

        // A comment runs to the end of the line and is a MARKER ONLY: 003's
        // DESIGN §5.6 discards the text before the parser, so the stored
        // program never carries it and M1.5's converter cannot bring it back.
        if (line.two_ahead("//")) { line.put(token::comment_token); break; }

        // A string literal: its characters travel behind string_token, counted.
        if (c == '"') {
            const std::size_t from = line.i + 1;
            std::size_t k = from;
            while (k < n && text[k] != '"') k += (text[k] == '\\' && k + 1 < n) ? 2 : 1;
            line.put_payload(token::string_token, from, std::min(k, n));
            line.i = (k < n) ? k + 1 : n;
            continue;
        }

        // 0#down -- a folded option (SATC.md §1.1: "a third kind of token").
        if (c == '0' && line.two_ahead("0#")) {
            std::size_t k = line.i + 2;
            while (k < n && identifier_body(text[k])) ++k;
            line.put_payload(token::option_token, line.i + 2, k);
            line.i = k;
            continue;
        }

        // A number, the dot joining it only when a digit follows (003's rule).
        if (a_digit(c)) {
            std::size_t k = line.i;
            while (k < n && (a_digit(text[k]) || (text[k] == '.' && k + 1 < n && a_digit(text[k + 1])))) ++k;
            line.put_payload(token::number_token, line.i, k);
            line.i = k;
            continue;
        }

        // A WORD OF THE LANGUAGE, as one code. satellite.console.display is
        // 4163, not name . name . name, and the code IS the index into the
        // function table -- which is what the 4096 range is for.
        //
        // LONGEST FIRST, and only a whole path counts. `my_list.append` is not a
        // word (the method needs the variable's declared type, which is PLAN
        // M1's job), and `satellite.console` inside `satellite.console.display`
        // must not win, so the dotted run is read once and then shortened a
        // segment at a time until one matches or none does.
        if (identifier_start(c)) {
            std::size_t run = line.i;
            while (run < n && (identifier_body(text[run]) ||
                               (text[run] == '.' && run + 1 < n && identifier_start(text[run + 1]))))
                ++run;
            const std::size_t before = line.i;
            for (std::size_t stop = run; stop > line.i; ) {
                const std::string_view path = text.substr(line.i, stop - line.i);
                const Code code = word::code_of_spelling(path);
                if (code != 0) { line.put(code); line.i = stop; break; }
                const std::size_t dot = path.rfind('.');
                if (dot == std::string_view::npos) break;
                stop = line.i + dot;
            }
            if (line.i != before) continue;
        }

        // A name -- or a bits literal, which is b then 0/1 and x then hex.
        if (identifier_start(c)) {
            std::size_t k = line.i;
            while (k < n && identifier_body(text[k])) ++k;
            const std::size_t length = k - line.i;

            // THE AUTHOR'S `.find(` TRIGGER, 2026-09-16: "we simply create the
            // syntax token period_token... then we make find_token, so when we
            // have period token and find_token together with parentheses token,
            // with all of those tokens it triggers a detailed code". method_token
            // IS the period token and always was; this is the other half. A
            // method's NAME now has its own 16-bit code, so recognising `.find(`
            // is two integer compares instead of rebuilding a std::string from
            // codes and comparing it -- the same cost the CPython race found.
            //
            // ONLY STRAIGHT AFTER A PERIOD, which is what keeps `find` usable as
            // an ordinary name: `satellite.variable.number find = 5` is a name in
            // the ordinary position, `s.find("x")` is the method. The lexer does
            // not know the receiver's TYPE -- that is the object model's job --
            // and it does not have to: it only has to know this is a member name.
            if (!row.empty() && static_cast<Code>(row.back().to_ulong()) == token::method_token) {
                const Code method = token::method_code_of(text.substr(line.i, length));
                if (method != 0) { line.put(method); line.i = k; continue; }
            }

            Code marker = token::name_token;
            std::size_t from = line.i;
            if (length > 1 && (c == 'b' || c == 'x')) {
                bool all = true;
                for (std::size_t d = line.i + 1; d < k && all; ++d)
                    all = (c == 'b') ? (text[d] == '0' || text[d] == '1') : a_hex_digit(text[d]);
                if (all) {
                    marker = (c == 'b') ? token::binary_token : token::hexadecimal_token;
                    from = line.i + 1;
                }
            }
            line.put_payload(marker, from, k);
            line.i = k;
            continue;
        }

        // THE TWO-CHARACTER OPERATORS ARE TRIED FIRST, and they have to be: the
        // arithmetic rule below looks at the character AFTER the sign, and in
        // `a += b` that character is `=`, not a blank -- so without this the
        // rule would read `+=` as a touching plus and then a lone `=`. Longest
        // match first is the lexer's existing discipline (// and /* before /),
        // and this is the same discipline applied one step earlier.
        bool matched_pair = false;
        for (const TwoCharacter &pair : kTwoCharacter) {
            if (!line.two_ahead(pair.spelling)) continue;
            line.put(pair.code);
            line.i += 2;
            matched_pair = true;
            break;
        }
        if (matched_pair) continue;

        // EVERY ARITHMETIC OPERATION NEEDS WHITESPACE ON BOTH SIDES (the author,
        // 2026-09-16): "Literally every math operation, ANY math operation has to
        // have spacebar(sign)spacebar". This used to be the rule for `/` alone;
        // generalising it is what frees the TOUCHING spelling of each character:
        //
        //     5 / 4   division          5/4   a FRACTION (the author, same day)
        //     5 - 4   subtraction       -5    the unary minus
        //     dir/file                        still a path, because neither side is a digit
        //
        // A touching `/` is a fraction only BETWEEN TWO DIGITS; anywhere else it
        // stays the path separator, so include(dir/file) is untouched.
        if (arithmetic_character(c) != 0) {
            const bool before = line.i > 0 && blank(text[line.i - 1]);
            const bool after = line.i + 1 < n && blank(text[line.i + 1]);
            if (before && after) {
                line.put(arithmetic_character(c));
                ++line.i;
                continue;
            }
            if (c == '/') {
                const bool digits_both_sides = line.i > 0 && a_digit(text[line.i - 1]) &&
                                               line.i + 1 < n && a_digit(text[line.i + 1]);
                line.put(digits_both_sides ? token::fraction_token : token::path_separator_token);
                ++line.i;
                continue;
            }
            line.put(tight_arithmetic_character(c));
            ++line.i;
            continue;
        }

        const Code single = one_character_token(c);
        if (single != 0) { line.put(single); ++line.i; continue; }

        // The lexer never throws: it marks the character and carries on. The
        // character travels as error_token's counted payload, so a reader skips
        // it like any other and can say WHICH character it was.
        std::size_t k = line.i;
        one_character(text, k);
        line.put_payload(token::error_token, line.i, k);
        line.i = k;
    }

    line.put(token::line_end_token);
}

void add_file_to_bytecode_registry(const std::string &filename,
                                   const std::string &source,
                                   StartupThreads &threads,
                                   unsigned long long int batches,
                                   BytecodeRegistry &registry,
                                   BytecodeFilenames &filenames)
{
    registry.emplace_back();
    filenames.push_back(filename);
    std::vector<std::bitset<16>> &file = registry.back();

    // THE LINES AS STRINGS (the author, 2026-09-16): "the program is returned in
    // a std::vector<std::vector<std::string>> if it isn't programmed like that --
    // make it like that". They were string_views into `source` before, and that
    // is a real cost paid on purpose: copying 100,000 lines into std::strings was
    // measured at 45 of the 47 ms this used to take, one allocation a line. His
    // design hands each thread a string of its own, so each thread owns what it
    // reads and nothing it reads can move under it.
    std::vector<std::string> lines;
    const std::string_view whole(source);
    std::size_t from = 0;
    while (from <= whole.size()) {
        const std::size_t stop = whole.find('\n', from);
        const std::string_view line = whole.substr(from, (stop == std::string_view::npos ? whole.size() : stop) - from);
        lines.emplace_back(line);
        if (stop == std::string_view::npos) break;
        from = stop + 1;
    }
    if (lines.empty()) {
        file.push_back(std::bitset<16>(token::end_of_file_token));
        return;
    }

    // A THREAD FOR EACH LINE (the author). cascade_convert.hpp holds the design
    // and what 200,000 threads cost.
    (void)batches;
    convert_a_thread_for_each_line(lines, file, threads);
    file.push_back(std::bitset<16>(token::end_of_file_token));
}

signed long long int build_bytecode_registry(const std::string &filename,
                                             const std::string &source,
                                             StartupThreads &threads,
                                             unsigned long long int batches,
                                             BytecodeRegistry &registry,
                                             BytecodeFilenames &filenames,
                                             MachineState &state)
{
    registry.clear();
    filenames.clear();
    add_file_to_bytecode_registry(filename, source, threads, batches, registry, filenames);

    unsigned long long int errors = 0;
    for (const std::vector<std::bitset<16>> &row : registry)
        for (const std::bitset<16> &code : row)
            if (static_cast<Code>(code.to_ulong()) == token::error_token) ++errors;

    state.set("bytecode_registry(built): " + std::to_string(registry.size()) + " file" +
                  (registry.size() == 1 ? "" : "s") + ", " + std::to_string(codes_in(registry)) + " codes",
              success);
    // REPORTED, NEVER FATAL. The lexer never throws (DESIGN §5.6): a character
    // with no code is marked in the stream with error_token and the conversion
    // finishes, so whatever reads the registry next can see the whole program
    // and report the fault with a position rather than a bare stop. Answering
    // satl_line_not_understood here would end the run instead, because
    // stops_the_program() treats it as fatal.
    if (errors != 0)
        report_error("bytecode_registry(characters with no code): " + std::to_string(errors) +
                         ", each marked with error_token",
                     success);
    return success;
}

std::string row_as_bits(const std::vector<std::bitset<16>> &row)
{
    std::string out;
    for (const std::bitset<16> &code : row) {
        if (!out.empty()) out += ' ';
        out += code.to_string();
    }
    return out;
}

unsigned long long int count_at(const std::vector<std::bitset<16>> &row, std::size_t &at)
{
    if (at >= row.size()) return 0;
    const Code marker = static_cast<Code>(row[at].to_ulong());
    if (!token::carries_a_count(marker)) return 0;

    // Least significant 16 bits first, each extra chunk announced by
    // long_count_token -- the other half of put_payload's writing.
    std::size_t i = at + 1;
    unsigned long long int value = 0;
    int shift = 0;
    while (i < row.size() && static_cast<Code>(row[i].to_ulong()) == token::long_count_token) {
        if (++i >= row.size()) break;
        value |= static_cast<unsigned long long int>(row[i].to_ulong()) << shift;
        shift += 16;
        ++i;
    }
    if (i < row.size()) {
        value |= static_cast<unsigned long long int>(row[i].to_ulong()) << shift;
        ++i;
    }
    at = i;
    return value;
}

std::string text_at(const std::vector<std::bitset<16>> &row, std::size_t &at)
{
    std::size_t i = at;
    const unsigned long long int count = count_at(row, i);
    const std::size_t stop = std::min(row.size(), i + static_cast<std::size_t>(count));

    std::string out;
    out.reserve(static_cast<std::size_t>(count));
    while (i < stop) {
        const Code code = static_cast<Code>(row[i].to_ulong());
        if (code == token::wide_run_token || code == token::wide_run_32_token) {
            const bool wide32 = code == token::wide_run_32_token;
            std::size_t k = i;
            const unsigned long long int run = count_at(row, k);
            for (unsigned long long int w = 0; w < run && k < stop; ++w) {
                std::uint32_t value = static_cast<std::uint32_t>(row[k].to_ulong());
                ++k;
                if (wide32 && k < stop) { value = (value << 16) | static_cast<std::uint32_t>(row[k].to_ulong()); ++k; }
                // Back to UTF-8, which is what a library's std::string holds.
                if (value < 0x80) { out += static_cast<char>(value); }
                else if (value < 0x800) {
                    out += static_cast<char>(0xC0 | (value >> 6));
                    out += static_cast<char>(0x80 | (value & 0x3F));
                } else if (value < 0x10000) {
                    out += static_cast<char>(0xE0 | (value >> 12));
                    out += static_cast<char>(0x80 | ((value >> 6) & 0x3F));
                    out += static_cast<char>(0x80 | (value & 0x3F));
                } else {
                    out += static_cast<char>(0xF0 | (value >> 18));
                    out += static_cast<char>(0x80 | ((value >> 12) & 0x3F));
                    out += static_cast<char>(0x80 | ((value >> 6) & 0x3F));
                    out += static_cast<char>(0x80 | (value & 0x3F));
                }
            }
            i = k;
            continue;
        }
        if (code < 128) out += static_cast<char>(character_table::ascii_of_code[code]);
        ++i;
    }
    at = stop;
    return out;
}

unsigned long long int codes_in(const BytecodeRegistry &registry)
{
    unsigned long long int total = 0;
    for (const std::vector<std::bitset<16>> &row : registry) total += row.size();
    return total;
}

} // namespace satellite004
