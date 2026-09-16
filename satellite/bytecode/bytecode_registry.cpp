// satellite/bytecode/bytecode_registry.cpp -- .satl in, 16-bit tokens out.
// The header says why the rows are bitset<16> and why the threads get batches.

#include "bytecode_registry.hpp"

#include "../satellite_variable_string/character_table.hpp"

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

Code one_character_token(char c)
{
    switch (c) {
    case '{': return token::left_brace_token;
    case '}': return token::right_brace_token;
    case '(': return token::left_parenthesis_token;
    case ')': return token::right_parenthesis_token;
    case '[': return token::left_square_bracket_token;
    case ']': return token::right_square_bracket_token;
    case '+': return token::plus_token;
    case '-': return token::minus_token;
    case '*': return token::times_token;
    case '%': return token::modulus_token;
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
    case '^': return token::bit_exclusive_or_token;
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

        // A comment runs to the end of the line and carries its own text.
        if (line.two_ahead("//")) { line.put_payload(token::comment_token, line.i + 2, n); break; }

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

        // A name -- or a bits literal, which is b then 0/1 and x then hex.
        if (identifier_start(c)) {
            std::size_t k = line.i;
            while (k < n && identifier_body(text[k])) ++k;
            const std::size_t length = k - line.i;
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

        // A slash is division ONLY with whitespace on both sides (the author,
        // 2026-09-16); a slash touching anything is a path separator.
        if (c == '/') {
            const bool before = line.i > 0 && blank(text[line.i - 1]);
            const bool after = line.i + 1 < n && blank(text[line.i + 1]);
            line.put((before && after) ? token::divide_token : token::path_separator_token);
            ++line.i;
            continue;
        }

        bool matched = false;
        for (const TwoCharacter &pair : kTwoCharacter) {
            if (!line.two_ahead(pair.spelling)) continue;
            line.put(pair.code);
            line.i += 2;
            matched = true;
            break;
        }
        if (matched) continue;

        const Code single = one_character_token(c);
        if (single != 0) { line.put(single); ++line.i; continue; }

        // The lexer never throws: it marks the character and carries on.
        line.put(token::error_token);
        std::size_t k = line.i;
        Line::character_codes(one_character(text, k), row);
        line.i = k;
    }

    line.put(token::line_end_token);
}

signed long long int build_bytecode_registry(const std::string &source,
                                             StartupThreads &threads,
                                             unsigned long long int batches,
                                             BytecodeRegistry &registry,
                                             MachineState &state)
{
    // WHERE the lines are, never copies of them. Copying 100,000 lines into
    // std::strings cost 45 of the 47 ms this used to take (measured
    // 2026-09-16): one allocation a line, on one thread, before any batch
    // starts. The jobs read straight out of `source`, which outlives them.
    std::vector<std::string_view> lines;
    const std::string_view whole(source);
    std::size_t from = 0;
    while (from <= whole.size()) {
        const std::size_t stop = whole.find('\n', from);
        lines.push_back(whole.substr(from, (stop == std::string_view::npos ? whole.size() : stop) - from));
        if (stop == std::string_view::npos) break;
        from = stop + 1;
    }

    registry.assign(lines.size(), {});
    if (lines.empty()) {
        registry.push_back({std::bitset<16>(token::end_of_file_token)});
        state.set("bytecode_registry(built): 1 row, an empty program", success);
        return success;
    }

    // BATCHES OF LINES, NEVER ONE LINE EACH -- the header says why, with the
    // measurement. A batch is never smaller than one line.
    const unsigned long long int jobs = std::max<unsigned long long int>(1, std::min<unsigned long long int>(batches, lines.size()));
    const std::size_t per = (lines.size() + jobs - 1) / jobs;
    std::atomic<unsigned long long int> finished{0};

    for (unsigned long long int job = 0; job < jobs; ++job) {
        threads.submit([&, job] {
            const std::size_t start = static_cast<std::size_t>(job) * per;
            const std::size_t stop = std::min(lines.size(), start + per);
            for (std::size_t l = start; l < stop; ++l) tokenise_one_line(lines[l], registry[l]);
            finished.fetch_add(1, std::memory_order_release);
        });
    }
    while (finished.load(std::memory_order_acquire) < jobs) std::this_thread::yield();

    // The last row ends the file, so a reader that has a row has a statement.
    registry.back().push_back(std::bitset<16>(token::end_of_file_token));

    unsigned long long int errors = 0;
    for (const std::vector<std::bitset<16>> &row : registry)
        for (const std::bitset<16> &code : row)
            if (static_cast<Code>(code.to_ulong()) == token::error_token) ++errors;

    state.set("bytecode_registry(built): " + std::to_string(registry.size()) + " rows, " +
                  std::to_string(codes_in(registry)) + " codes, " + std::to_string(jobs) + " batches",
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

unsigned long long int codes_in(const BytecodeRegistry &registry)
{
    unsigned long long int total = 0;
    for (const std::vector<std::bitset<16>> &row : registry) total += row.size();
    return total;
}

} // namespace satellite004
