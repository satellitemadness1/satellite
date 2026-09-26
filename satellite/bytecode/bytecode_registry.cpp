// satellite/bytecode/bytecode_registry.cpp -- .satl in, 16-bit tokens out.
// The header says why the rows are bitset<16> and why the threads get batches.

#include "bytecode_registry.hpp"
#include "cascade_convert.hpp"

#include "../satellite_variable_string/character_table.hpp"
#include "word_codes.hpp"

#include <algorithm>
#include <cctype>
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

    // THE LAST TOKEN PUT, NOT THE LAST CODE. After a literal, row.back() is its
    // payload's last code, and a character's own number can be any 16 bits: "܄"str
    // ended in 0x0704, which IS method_token, so `str` became a method code the check
    // never looks at (the payload sweep, 2026-09-17). put_payload sets it to its marker.
    Code last_token = 0;

    // THE FIRST THREE TOKENS OF THE LINE, for the one rule that needs them: a colour
    // written without its x on the line that declares it (below, at "WITHOUT ITS
    // x"). Counted here and nowhere else, so a payload's own codes are never
    // counted -- put_payload writes its count straight into the row.
    unsigned int tokens = 0;
    Code first_three[3] = {0, 0, 0};

    void put(Code code)
    {
        row.push_back(std::bitset<16>(code));
        last_token = code;
        if (tokens < 3) first_three[tokens] = code;
        ++tokens;
    }

    // `<type word> <name> =` AND NOTHING YET AFTER IT: the value's first character
    // is next. Answers the type word, or 0.
    Code declaring_a_value() const
    {
        return tokens == 3 && first_three[1] == token::name_token && first_three[2] == token::assign_token
                   ? first_three[0]
                   : 0;
    }

    // One character of a payload: ASCII in the author's order, anything above
    // 127 behind wide_run_token so it can never be read as a token.
    //
    // ABOVE U+FFFF, 40000 AND THEN ONE 32-BIT INTEGER IN TWO CODES (the author's
    // D3.1, 2026-09-16: "32-bits only when we use the number 40000 as a 16-bit
    // code"). wide_token is the only way 32 bits appear in the program; the
    // wide_run_32_token this used to write is retired and nothing writes it.
    // A character whose OWN value is 40000 (U+9C40) still travels behind
    // wide_run_token as any other, so inside a payload the marker cannot be
    // mistaken for it.
    static void character_codes(std::uint32_t c, std::vector<std::bitset<16>> &out)
    {
        if (c < 128) { out.push_back(std::bitset<16>(character_table::code_of_ascii[c])); return; }
        if (c <= 0xFFFF) {
            out.push_back(std::bitset<16>(token::wide_run_token));
            out.push_back(std::bitset<16>(1));
            out.push_back(std::bitset<16>(static_cast<Code>(c)));
            return;
        }
        out.push_back(std::bitset<16>(token::wide_token));
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
        row.push_back(std::bitset<16>(0)); // filled in below, once the codes are counted
        std::size_t k = from;
        while (k < to) character_codes(one_character(text, k), row);
        put_count(row, count_at, row.size() - count_at - 1);
        last_token = marker;
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

// THE CODE FOR A WORD WRITTEN `path(...)`, whose row carries the shape: 0 when no
// row does, or when more than one could be meant. Empty brackets ask for the
// `path()` row; anything else asks for the row with ONE parameter, whatever that
// parameter is called -- `(d)`, `(x)`, `(value)` are all one argument. A path with
// several one-parameter rows is left alone rather than guessed at.
// DOES AN ARGUMENT STARTING AT `k` BEGIN `name =`, a named option (console_calls.hpp)?
// A name, spaces, and an `=` that is not the first half of `==`.
static bool an_option_starts(std::string_view text, std::size_t k)
{
    while (k < text.size() && (text[k] == ' ' || text[k] == '\t')) ++k;
    if (k >= text.size() || !(std::isalpha(static_cast<unsigned char>(text[k])) || text[k] == '_'))
        return false;
    while (k < text.size() && (std::isalnum(static_cast<unsigned char>(text[k])) || text[k] == '_')) ++k;
    while (k < text.size() && (text[k] == ' ' || text[k] == '\t')) ++k;
    return k + 1 < text.size() && text[k] == '=' && text[k + 1] != '=';
}

// HOW MANY PLAIN ARGUMENTS the brackets opened just before `inside` hold: the
// commas at their own depth, plus one, less every argument that is a named option
// -- `input("name? ", foreground=xFF8800)` is input(prompt), one argument and an
// option, and counting the option chose input(prompt, target) (2026-09-23). A comma
// inside a string, nested brackets or a braced list is not one of theirs: `{1, 2}`
// is ONE argument. The brackets are known not to be empty.
static std::size_t arguments_in(std::string_view text, std::size_t inside)
{
    std::size_t depth = 1, commas = 0, options = an_option_starts(text, inside) ? 1 : 0;
    for (std::size_t k = inside; k < text.size(); ++k) {
        const char c = text[k];
        if (c == '"') {
            ++k;
            while (k < text.size() && text[k] != '"') k += (text[k] == '\\' && k + 1 < text.size()) ? 2 : 1;
            continue;
        }
        if (c == '/' && k + 1 < text.size() && text[k + 1] == '/') break;   // the rest is a comment
        if (c == '(' || c == '[' || c == '{') ++depth;
        else if ((c == ')' || c == ']' || c == '}') && --depth == 0) break;
        else if (c == ',' && depth == 1) {
            ++commas;
            if (an_option_starts(text, k + 1)) ++options;
        }
    }
    return commas + 1 - options;
}

token::Code shaped_word_code(std::string_view text, std::size_t from, std::size_t run)
{
    std::size_t at = run;
    while (at < text.size() && (text[at] == ' ' || text[at] == '\t')) ++at;
    if (at >= text.size() || text[at] != '(')
        return 0;
    std::size_t inside = at + 1;
    while (inside < text.size() && (text[inside] == ' ' || text[inside] == '\t')) ++inside;

    const std::string path(text.substr(from, run - from));
    // EMPTY BRACKETS ARE THE `path()` ROW WHEN THERE IS ONE. When there is NOT
    // -- the word takes something -- the call falls through with 0 arguments,
    // so the scan below answers the word it names and the CHECKER refuses the
    // count by name, before a line runs (GTK-12, 2026-09-21). This used to
    // answer nothing here, and the shortening then read `satellite.window` and
    // `.menu` as a METHOD -- menu is the first word whose last segment is also
    // a method's name -- so `satellite.window.menu()` printed the lines above
    // it and was refused at run time by a value that was not there. `frame()`
    // had the milder half of the same hole: it became a NAME and was refused
    // as "no capsule named frame", a word that exists told it does not.
    const bool empty = inside < text.size() && text[inside] == ')';
    // NOTHING BUT OPTIONS IS EMPTY TOO: `input(foreground=xFF8800)` is input() with an
    // option, so it takes the `path()` row as a call with nothing in it does.
    const std::size_t given = empty ? 0 : arguments_in(text, inside);
    if (given == 0) {
        const token::Code bare = word::code_of_spelling(path + "()");
        if (bare != 0)
            return bare;
    }

    // THE ROW WITH AS MANY PARAMETERS AS THE CALL HAS ARGUMENTS WINS (2026-09-18).
    // Until then every non-empty call took the ONE-parameter row, so
    // `satellite.file.new("x", "text")` lexed as `new(path)` and its second argument
    // was refused as something that could not be read. Counting the call's own
    // commas chooses `new(path, mode)`. When no row has that many parameters, the
    // one-parameter rule below still answers, so a call with too many arguments is
    // refused by the word and not turned into a name.
    const std::string opened = path + "(";
    std::size_t low = 0, high = word::kSpelledWordCount;
    while (low < high) {
        const std::size_t middle = low + (high - low) / 2;
        if (std::string_view(word::kSpelledWords[middle].path) < std::string_view(opened)) low = middle + 1;
        else high = middle;
    }
    // THREE ANSWERS, IN THIS ORDER, and the third was added at GTK-4 (2026-09-21).
    // `exact` is a row with as many parameters as the call has arguments. `only`
    // is the one-parameter row, which is what a call with TOO MANY arguments
    // falls back to. `any` is the single row under this name whatever its shape,
    // and it exists because the fallback above only works for words that HAVE a
    // one-parameter row: `satellite.window.slider(100)` has only
    // `slider(least, most)` above it, matched nothing at all, and was refused as
    // **"no capsule named slider"** -- a word that exists, told it does not.
    // `satellite.window.new("a title", 800)` had the same hole and nobody had
    // noticed, because the check.sh row for it only ever asserted the exit code.
    //
    // IT CANNOT TAKE A CALL AWAY FROM A REAL WORD, because it answers only when
    // `exact` and `only` both found nothing and exactly one row carries the name.
    token::Code only = 0, exact = 0, any = 0;
    bool only_twice = false, exact_twice = false, any_twice = false;
    for (std::size_t row = low; row < word::kSpelledWordCount; ++row) {
        const std::string_view spelling(word::kSpelledWords[row].path);
        if (spelling.compare(0, opened.size(), opened) != 0)
            break;
        if (spelling.size() == opened.size() + 1)           // the `path()` row: these brackets are not empty
            continue;
        std::size_t parameters = 1;
        for (const char c : spelling) parameters += (c == ',') ? 1 : 0;
        if (parameters == given) {
            exact_twice = exact_twice || exact != 0;
            exact = word::kSpelledWords[row].code;
        }
        if (parameters == 1) {
            only_twice = only_twice || only != 0;
            only = word::kSpelledWords[row].code;
        }
        any_twice = any_twice || any != 0;
        any = word::kSpelledWords[row].code;
    }
    if (exact != 0)
        return exact_twice ? 0 : exact;                     // two rows could be meant: say nothing
    if (only != 0)
        return only_twice ? 0 : only;
    if (any != 0)
        return any_twice ? 0 : any;
    // A WORD WHOSE ONLY ROW IS `path()`, GIVEN SOMETHING (2026-09-23): that row, so the
    // checker can say it takes nothing. Without it `satellite.console.home(5)` was
    // shortened to `satellite.console` and `.home` and told "satellite.console is not a
    // call" -- a word that exists, described as something else.
    return given == 0 ? 0 : word::code_of_spelling(path + "()");
}

void tokenise_one_line(std::string_view text, std::vector<std::bitset<16>> &row,
                       std::vector<std::size_t> *offsets)
{
    Line line{text, row};
    const std::size_t n = text.size();
    row.reserve(n + 8); // one code a character, plus the line's own markers

    // WHERE EACH CODE'S TOKEN BEGAN, kept only when somebody asks for it.
    //
    // THIS IS WHAT PUTS A CARET UNDER THE RIGHT CHARACTER, and it is nullptr on
    // every path that runs a program -- the reporter passes a vector, once, for
    // the one line it re-read after something already went wrong. SATELLITE_ERROR
    // E1 offered "record a byte offset per token" or "re-tokenise one line at
    // report time" and recommended the second; this is the second, done with the
    // lexer's OWN offsets rather than a second copy of its splitting rules, which
    // is the part that would have drifted.
    //
    // PADDED AT THE TOP OF EACH TURN rather than the bottom, because the body
    // below is all `continue` and `break` -- one place, and no way to miss a push.
    // A payload's codes carry their MARKER's offset, so the vector stays index
    // for index with `row` and a caret under a string points at its opening quote.
    std::size_t token_began = 0;

    while (line.i < n) {
        if (offsets != nullptr)
            while (offsets->size() < row.size()) offsets->push_back(token_began);
        token_began = line.i;
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

        // A COLOUR WRITTEN WITHOUT ITS x, ON THE LINE THAT DECLARES IT (the author,
        // 2026-09-22): "satellite.variable.color my_color = x000000 or just 000000
        // without the x". THE TYPE WORD DECIDES THE READING, because the characters
        // alone cannot: `000000` is the number 0 with its width gone, `ff00aa` is a
        // name, and `00ff00` is the number 0 and then a name. Here the line has said
        // a colour comes next, so the whole run is read as hex digits, b included.
        //
        // ONLY THE COLOUR, AND ONLY ITS DECLARING LINE. A binary keeps its b and a
        // hex its x (the author, the same evening: "just make the prefix b mandatory
        // and move on"). A later `name = ...` has no type word in front of it, and
        // the lexer does not know what a name was declared as; the checker does.
        // A run with anything else in it -- `cafe_1` -- is left to be a name.
        if (line.declaring_a_value() == word::code_of(1, 6, 19) && identifier_body(c)) {
            std::size_t k = line.i;
            bool all = true;
            while (k < n && identifier_body(text[k])) {
                all = all && a_hex_digit(text[k]);
                ++k;
            }
            if (all) {
                line.put_payload(token::hexadecimal_token, line.i, k);
                line.i = k;
                continue;
            }
        }

        // A number, the dot joining it only when a digit follows (003's rule).
        if (a_digit(c)) {
            std::size_t k = line.i;
            while (k < n && (a_digit(text[k]) || (text[k] == '.' && k + 1 < n && a_digit(text[k + 1])))) ++k;

            // A PERCENTAGE IS A NUMBER WITH A % TOUCHING IT (the author,
            // 2026-09-17: "take 1% and 100% as meaning a percentage"). The
            // whitespace rule is what frees the spelling: `5 % 3` is modulus
            // because it is spaced, so a % pressed against a number's digits is
            // not an operation -- unless something name-like follows it, where
            // `5%3` stays the touching modulus the rule refuses by name.
            if (k < n && text[k] == '%' && (k + 1 >= n || !identifier_body(text[k + 1]))) {
                line.put_payload(token::percentage_token, line.i, k);
                line.i = k + 1;
                continue;
            }
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
            // A WORD WHOSE ROW SPELLS ITS ARGUMENT SHAPE is matched here, BEFORE
            // the shortening below: `satellite.directory.list()` and `list(d)`
            // are two rows over one path (WORD_NUMBERS §1.3), so the plain path
            // matches neither, and the shortening would take
            // `satellite.directory` and leave `.list()` to be read as a name.
            // Which of the two is meant is decided by the brackets that follow,
            // and the arguments inside them are lexed exactly as they always are.
            // THE WHOLE PATH FIRST, exactly as it is written: `satellite.main`
            // is a row of its own and must not become `satellite.main()`, or a
            // program's own `satellite.capsule satellite.main()` line stops
            // being the shape that declares main.
            const Code whole = word::code_of_spelling(text.substr(line.i, run - line.i));
            if (whole != 0) { line.put(whole); line.i = run; continue; }
            const Code shaped = shaped_word_code(text, line.i, run);
            if (shaped != 0) { line.put(shaped); line.i = run; continue; }
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
            if (line.last_token == token::method_token) {
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

        // `**` IS THE SECOND SPELLING OF `^` (SATELLITE_INFINITY.md, INF-1): the
        // author writes it twice -- "power ** infinity", `my_number ** my_number **
        // my_number` -- and the 2026-09-16 ruling that power is `^` stands. Spaced on
        // both sides it lexes to power_token itself, so nothing after the lexer ever
        // learns there were two spellings, and it groups right to left as `^` does.
        // A TOUCHING `**` is left to the rule below: two tight stars, refused.
        if (c == '*' && line.two_ahead("**") && line.i > 0 && blank(text[line.i - 1]) &&
            line.i + 2 < n && blank(text[line.i + 2])) {
            line.put(token::power_token);
            line.i += 2;
            continue;
        }

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

    if (offsets != nullptr)
        while (offsets->size() < row.size()) offsets->push_back(token_began);
    line.put(token::line_end_token);
    if (offsets != nullptr)
        offsets->push_back(n);   // the line end sits past the last character
}

std::vector<NeverClosed> join_statements_across_lines(std::vector<std::string> &lines)
{
    struct Open {
        char what;           // ( [ or L, a list's {
        std::size_t line;
    };
    const auto never_closed_because = [](char what) {
        return what == '(' ? std::string("this ( is never closed -- the file ends inside it, so its ) is missing")
             : what == '[' ? std::string("this [ is never closed -- the file ends inside it, so its ] is missing")
                           : std::string("this list was opened with { and never closed with } -- items are "
                                         "separated by commas, as in {\"one\", \"two\"}");
    };
    std::vector<NeverClosed> never;
    std::vector<Open> open;
    bool in_string = false;
    std::size_t string_line = 0;
    char last = '\0';                  // the last character that is not a space, outside strings and comments
    bool spaced_before_last = false;   // whether a space stood just before it
    std::size_t into = std::string::npos;
    std::string joined, joiner;

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string &physical = lines[i];
        std::size_t code_end = physical.size();
        for (std::size_t k = 0; k < physical.size(); ++k) {
            const char c = physical[k];
            if (in_string) {
                if (c == '\\' && k + 1 < physical.size()) { ++k; continue; }
                if (c == '"') { in_string = false; last = '"'; spaced_before_last = false; }
                continue;
            }
            if (c == '/' && k + 1 < physical.size() && physical[k + 1] == '/') { code_end = k; break; }
            if (c == ' ' || c == '\t') continue;
            if (c == '"') {
                in_string = true;
                string_line = i;
                continue;
            }
            if (c == '(' || c == '[') {
                open.push_back({c, i});
            } else if (c == '{') {
                // A LIST'S { stands where a value goes; a BLOCK'S follows a ), a name or a word
                // and is the statements' business, never this pass's.
                if (!open.empty() || last == '(' || last == ',' || last == '=' || last == '[')
                    open.push_back({'L', i});
            } else if (c == ')' || c == ']' || c == '}') {
                const char wants = c == ')' ? '(' : c == ']' ? '[' : 'L';
                bool held = false;
                for (const Open &each : open) held = held || each.what == wants;
                if (held) {
                    // AN OPENER CLOSED WITH THE WRONG BRACKET was never closed: `display({1, 2)`.
                    while (open.back().what != wants) {
                        never.push_back({open.back().line, never_closed_because(open.back().what)});
                        open.pop_back();
                    }
                    open.pop_back();
                }
            }
            spaced_before_last = k > 0 && (physical[k - 1] == ' ' || physical[k - 1] == '\t');
            last = c;
        }

        // DOES THE STATEMENT GO ON INTO THE NEXT LINE?
        bool goes_on = in_string || !open.empty() || last == ',' || last == '=' || last == '&' || last == '|' ||
                       (spaced_before_last && (last == '+' || last == '-' || last == '*' || last == '/' ||
                                               last == '%' || last == '^'));
        if (!goes_on && i + 1 < lines.size()) {
            const std::size_t first = lines[i + 1].find_first_not_of(" \t");
            goes_on = first != std::string::npos && lines[i + 1][first] == '.';
        }
        const std::string part = in_string ? physical : physical.substr(0, code_end);
        if (into == std::string::npos) {
            if (!goes_on || i + 1 >= lines.size())
                continue;                        // a statement of one line keeps its line exactly
            into = i;
            joined = part;
        } else {
            joined += joiner + part;
            lines[i].clear();
        }
        if (goes_on && i + 1 < lines.size()) {
            joiner = in_string ? "\n" : " ";
            // after an operator or a comma the statement is still one expression: the next
            // line's { is a list's, and last is kept as it is for that
        } else {
            lines[into] = joined;
            into = std::string::npos;
        }
    }
    if (into != std::string::npos)
        lines[into] = joined;

    // A BLOCK'S { AT THE END OF ITS HEADER'S LINE -- `satellite.statement.if(x) {`, `} satellite.
    // statement.else {` -- moves to the front of the next line (2026-09-25, the author's "accept
    // anything that is valid satellite regardless of how many spaces or lines are in it"): the
    // statements find a body's { after their line's end, and a capsule's header already took it
    // either way. The line count stays exactly what it was. A list's { (after = ( , [ or inside
    // brackets) is a value and stays where it is.
    if (!in_string && open.empty()) {
        for (std::size_t i = 0; i + 1 < lines.size(); ++i) {
            const std::string &line = lines[i];
            char before_brace = '\0', last = '\0', earlier = '\0';
            std::size_t brace_at = std::string::npos;
            int depth = 0;
            bool quoted = false;
            for (std::size_t k = 0; k < line.size(); ++k) {
                const char c = line[k];
                if (quoted) {
                    if (c == '\\' && k + 1 < line.size()) ++k;
                    else if (c == '"') { quoted = false; earlier = last; last = '"'; }
                    continue;
                }
                if (c == '/' && k + 1 < line.size() && line[k + 1] == '/') break;
                if (c == '"') { quoted = true; continue; }
                if (c == ' ' || c == '\t') continue;
                if (c == '(' || c == '[') ++depth;
                else if ((c == ')' || c == ']') && depth > 0) --depth;
                if (c == '{') { brace_at = k; before_brace = last; }
                earlier = last;
                last = c;
            }
            (void)earlier;
            const bool a_blocks_brace = last == '{' && brace_at != std::string::npos && depth == 0 &&
                                        before_brace != '\0' && before_brace != '(' && before_brace != ',' &&
                                        before_brace != '=' && before_brace != '[' && before_brace != '{';
            if (!a_blocks_brace)
                continue;
            lines[i].erase(brace_at, 1);
            lines[i + 1].insert(0, "{ ");
        }
    }
    if (in_string)
        never.push_back({string_line, "a string that begins on this line is never closed -- the file ends inside "
                                      "it, so its closing \" is missing"});
    for (const Open &each : open)
        never.push_back({each.line, never_closed_because(each.what)});
    return never;
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
    // A STATEMENT OVER SEVERAL LINES BECOMES ONE, on its first line, the rest left empty
    // (join_statements_across_lines). What the file ends inside is refused by the scan.
    join_statements_across_lines(lines);

    // EVERY LINE ON THE 1024 WARM THREADS, IN ANY ORDER, BEFORE ANYTHING RUNS
    // (the author). cascade_convert.hpp holds the design. `batches` is the number
    // of warm threads, which is arguments.threads_startup.
    convert_every_line(lines, file, threads, batches);
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

    const unsigned long long int errors = characters_with_no_code(registry);

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
        if (shift < 64) value |= static_cast<unsigned long long int>(row[i].to_ulong()) << shift;
        shift += 16;
        ++i;
    }
    if (i < row.size()) {
        // put_count ends a 64-bit count whose top chunk is 0x090A with a 0 at shift
        // 64: nothing to add, and a shift that far is undefined.
        if (shift < 64) value |= static_cast<unsigned long long int>(row[i].to_ulong()) << shift;
        ++i;
    }
    at = i;
    return value;
}

void put_count(std::vector<std::bitset<16>> &row, std::size_t at, unsigned long long int written)
{
    // A COUNT EQUAL TO long_count_token IS WRITTEN LONG. 2314 is 0x090A, and count_at
    // reads any code equal to it as "more follows" -- so a literal of exactly 2314
    // codes swallowed the rest of its row, and the program ended in a refusal that
    // named the wrong thing (found by the wide-strings review, 2026-09-17). The same
    // is true of a long count's LAST chunk. Either way it goes out as
    // long_count_token, 0x090A, and then a 0 that ends it.
    if (written <= 0xFFFFull && written != token::long_count_token) {
        row[at] = std::bitset<16>(static_cast<Code>(written));
        return;
    }

    // A count past 65535 needs extra codes, so the payload shifts along to
    // make room for them. Rare by construction: it is a literal of more
    // than 65,535 codes, and it must still have no ceiling (DESIGN §1.2).
    std::vector<std::bitset<16>> chunks;
    unsigned long long int n = written;
    while (n > 0xFFFFull || n == token::long_count_token) {
        chunks.push_back(std::bitset<16>(token::long_count_token));
        chunks.push_back(std::bitset<16>(static_cast<Code>(n & 0xFFFFull)));
        n >>= 16;
    }
    chunks.push_back(std::bitset<16>(static_cast<Code>(n)));
    row.erase(row.begin() + static_cast<long>(at));
    row.insert(row.begin() + static_cast<long>(at), chunks.begin(), chunks.end());
}

namespace {

// One Unicode number back to UTF-8, which is what a library's std::string holds.
void append_utf8(std::string &out, std::uint32_t value)
{
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

} // namespace

// PAST A PAYLOAD, BUILDING NOTHING. text_at()'s own arithmetic for `at` and not
// one byte of its string.
//
// SEVEN CALLERS ONLY EVER WANTED THIS. They were written `text_at(row, at);` with
// the answer dropped on the floor -- a whole std::string built, filled a
// character at a time and destroyed, to move a cursor. A profile of 200,000
// statements put text_at at 25% of the run with 560,134 calls, and the skips are
// a large share of them.
//
// IT MUST MOVE `at` EXACTLY AS text_at DOES, which is why it is here, next to it,
// rather than open-coded at each site: the two share the count and the same
// std::min against the row's end, so a payload that runs past the end of the row
// stops both of them in the same place.
void skip_payload(const std::vector<std::bitset<16>> &row, std::size_t &at)
{
    std::size_t i = at;
    const unsigned long long int count = count_at(row, i);
    at = std::min(row.size(), i + static_cast<std::size_t>(count));
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
        // 40000: the two codes after it are one 32-bit character (D3.1).
        if (code == token::wide_token) {
            if (i + 2 < stop)
                append_utf8(out, (static_cast<std::uint32_t>(row[i + 1].to_ulong()) << 16) |
                                     static_cast<std::uint32_t>(row[i + 2].to_ulong()));
            i += 3;
            continue;
        }
        if (code == token::wide_run_token) {
            std::size_t k = i;
            const unsigned long long int run = count_at(row, k);
            for (unsigned long long int w = 0; w < run && k < stop; ++w, ++k)
                append_utf8(out, static_cast<std::uint32_t>(row[k].to_ulong()));
            i = k;
            continue;
        }
        if (code < 128) out += static_cast<char>(character_table::ascii_of_code[code]);
        ++i;
    }
    at = stop;
    return out;
}

// A STRING LITERAL'S ESCAPES ARE WORKED OUT HERE, AND ONLY HERE (the author,
// 2026-09-24: "let's build an escape code into the string"). The lexer keeps a
// literal exactly as it was written -- 003's DESIGN 5.3, lex raw and expand only
// inside a string's body -- so the stored program still reads back as its source,
// and a newline never has to live inside a payload.
//
// 003'S SIX, and no more: \" \\ \n \t \r and \'. The last is redundant -- an
// apostrophe needs no escape inside double quotes -- and taken anyway, because 003
// printed the backslash of "DOESN\'T" 501 times in one real program before it was.
// 003's \home \user \memtotal and the rest were VALUES, not characters, and are not
// here -- they keep their backslash -- EXCEPT \threads, which starts with \t: it is
// now a tab and "hreads", as in every language with a \t, where 003 answered the
// thread count. Whether the values come back is the author's. AN ESCAPE THIS DOES
// NOT KNOW KEEPS ITS BACKSLASH, as every escape did before this: "a\qb" is a\qb.
// Whether it should be refused is the author's too (ERROR #16).
//
// A LITERAL WITH NO BACKSLASH COMES BACK AS text_at BUILT IT, one find() and no
// second string: a literal is read every time its line runs.
std::string string_at(const std::vector<std::bitset<16>> &row, std::size_t &at)
{
    std::string written = text_at(row, at);
    std::size_t k = written.find('\\');
    if (k == std::string::npos)
        return written;
    std::string meant(written, 0, k);
    meant.reserve(written.size());
    while (k < written.size()) {
        const char c = written[k];
        const char next = k + 1 < written.size() ? written[k + 1] : '\0';
        const char escaped = c != '\\' ? '\0'
                             : next == '"'  ? '"'
                             : next == '\\' ? '\\'
                             : next == 'n'  ? '\n'
                             : next == 't'  ? '\t'
                             : next == 'r'  ? '\r'
                             : next == '\'' ? '\''
                                            : '\0';
        if (escaped == '\0') {
            meant += c;
            ++k;
        } else {
            meant += escaped;
            k += 2;
        }
    }
    return meant;
}

unsigned long long int codes_in(const BytecodeRegistry &registry)
{
    unsigned long long int total = 0;
    for (const std::vector<std::bitset<16>> &row : registry) total += row.size();
    return total;
}

unsigned long long int characters_with_no_code(const BytecodeRegistry &registry)
{
    // AT A TOKEN'S POSITION ONLY. Comparing every code counted counts: a literal of
    // exactly 258 codes has count 0x0102, which is error_token, and was reported as
    // a character with no code (found by the count review, 2026-09-17).
    unsigned long long int errors = 0;
    for (const std::vector<std::bitset<16>> &row : registry)
        for (std::size_t i = 0; i < row.size(); ) {
            const Code code = static_cast<Code>(row[i].to_ulong());
            if (code == token::error_token) ++errors;
            if (token::carries_a_count(code)) { text_at(row, i); continue; }
            ++i;
        }
    return errors;
}

} // namespace satellite004
