#pragma once

// The codes -- part of report.hpp, which includes this file. errors.def is the
// data and this is what reads it.
//
// A CODE IS A HANDLE AND NOT A MESSAGE, which is the distinction DESIGN §9 is
// built on: "what is missing is everything that makes an error ADDRESSABLE -- a
// code to look up, a stack to place it in, a suggestion to act on." So the
// number is what a person searches for, `satl --errors` is where they find it,
// and the sentence beside it here is the one and only wording it has.
//
// THE SENTENCE IS A TEMPLATE AND THE HOLES ARE CHECKED AT COMPILE TIME. That is
// the part worth reading the file for. `text_of` is a switch, so two rows
// sharing a number is `error: duplicate case value` naming both; `arity_of`
// counts the highest `{n}` in the text at compile time, so a site that hands
// three arguments to a two-hole sentence is a static_assert in report.hpp
// naming the code -- and `holes_are_dense` catches a text that skips `{2}` and
// would therefore silently drop an argument the site did supply.

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace satellite::errors {

// Every message satl can say, in the order errors.def declares them.
//
// THE VALUE IS THE PRINTED NUMBER, so `static_cast<unsigned>(code)` is the 231
// in S0231 and there is no second table mapping one to the other. NONE is 0 and
// is not a message; it is what an uninitialised Diagnostic holds, and render()
// says so rather than printing an empty sentence.
enum class Code : uint16_t {
    NONE = 0,
#define SAT_CODE(number, ident, severity, text) ident = number,
#include "error_reporter/errors.def"
};

// Which of two things a row is. The values come from errors.def so they live
// there and only there, exactly as words::kNumbered does.
enum class Severity : uint8_t {
    NOTE = SAT_NOTE,
    ERROR = SAT_ERROR,
};

// The sentence, with `{1}`-style holes still in it. Empty for NONE.
constexpr std::string_view text_of(Code code)
{
    switch (code) {
#define SAT_CODE(number, ident, severity, text) case Code::ident: return text;
#include "error_reporter/errors.def"
    case Code::NONE: break;
    }
    return {};
}

// Whether a row is an error or a note.
//
// A CODE THIS FUNCTION DOES NOT KNOW ANSWERS `ERROR`, deliberately. The only
// way to reach that arm is a cast from an integer -- `satl --errors S9999` is
// the one caller that can -- and a message satl cannot classify is not a note.
constexpr Severity severity_of(Code code)
{
    switch (code) {
#define SAT_CODE(number, ident, severity, text) \
    case Code::ident: return static_cast<Severity>(severity);
#include "error_reporter/errors.def"
    case Code::NONE: break;
    }
    return Severity::ERROR;
}

// The identifier as written in errors.def, for `satl --errors`. Metadata:
// nothing a message says depends on it.
constexpr std::string_view ident_of(Code code)
{
    switch (code) {
#define SAT_CODE(number, ident, severity, text) case Code::ident: return #ident;
#include "error_reporter/errors.def"
    case Code::NONE: break;
    }
    return "NONE";
}

// How many holes a sentence has: the highest `{n}` in it.
//
// COUNTED FROM THE TEXT RATHER THAN DECLARED IN A COLUMN. A column would be a
// second place the same fact lives and it is the column that goes stale -- a
// hole added to a sentence with the number left alone is an argument silently
// dropped, which is the failure this whole registry exists to make impossible.
constexpr unsigned arity_of(Code code)
{
    const std::string_view text = text_of(code);
    unsigned highest = 0;
    for (size_t i = 0; i + 2 < text.size(); i++) {
        if (text[i] != '{' || text[i + 2] != '}')
            continue;
        const char digit = text[i + 1];
        if (digit < '1' || digit > '9')
            continue;
        const unsigned hole = static_cast<unsigned>(digit - '0');
        if (hole > highest)
            highest = hole;
    }
    return highest;
}

// Every code, in file order, so the invariants and `satl --errors` can walk
// them. NONE is not in it -- it is not a message.
inline constexpr Code kCodes[] = {
#define SAT_CODE(number, ident, severity, text) Code::ident,
#include "error_reporter/errors.def"
};

inline constexpr size_t kCodeCount = sizeof(kCodes) / sizeof(kCodes[0]);

// S0231. Four digits because the first two are the block (errors.def says which
// is whose) and a fixed width means a column of them reads straight down.
//
// NOT std::to_string AND A PAD, because this is constexpr and its callers are a
// switch in a test and a dump that runs before anything is allocated.
constexpr char kCodeTextSize = 6;

struct CodeText {
    char text[kCodeTextSize] = {'S', '0', '0', '0', '0', '\0'};

    constexpr std::string_view view() const { return {text, 5}; }
};

constexpr CodeText code_text(Code code)
{
    CodeText out;
    unsigned value = static_cast<unsigned>(code);
    for (int i = 4; i >= 1; i--) {
        out.text[i] = static_cast<char>('0' + value % 10);
        value /= 10;
    }
    return out;
}

// S0231 back into a code, or NONE. What `satl --errors S0231` reads, and the
// reason a code is a fixed five characters rather than however many digits it
// happens to need.
constexpr Code code_of(std::string_view text)
{
    if (text.size() != 5 || (text[0] != 'S' && text[0] != 's'))
        return Code::NONE;
    unsigned value = 0;
    for (size_t i = 1; i < text.size(); i++) {
        if (text[i] < '0' || text[i] > '9')
            return Code::NONE;
        value = value * 10 + static_cast<unsigned>(text[i] - '0');
    }
    for (const Code code : kCodes)
        if (static_cast<unsigned>(code) == value)
            return code;
    return Code::NONE;
}

namespace detail {

// The numbers ascend. THIS IS THE HALF words.def GETS FOR FREE and this file
// has to buy, and errors.def's header is where the trade is argued: a word's
// number is a position, so a duplicate cannot be written down; a code's number
// is a column, so it can. Ascending catches an insertion into the middle of a
// block, which is the edit somebody makes when they want a code next to a
// related one -- and it catches a duplicate too, though `text_of`'s switch gets
// that one first and names both rows.
constexpr bool numbers_ascend()
{
    for (size_t i = 1; i < kCodeCount; i++)
        if (static_cast<unsigned>(kCodes[i]) <= static_cast<unsigned>(kCodes[i - 1]))
            return false;
    return true;
}

// No code is 0, because 0 is NONE and a row numbered 0 would be a message that
// an uninitialised Diagnostic already claims to be.
constexpr bool none_is_reserved()
{
    for (const Code code : kCodes)
        if (static_cast<unsigned>(code) == 0)
            return false;
    return true;
}

// Every sentence has a sentence in it. An empty text is a row somebody added
// meaning to come back to.
constexpr bool every_code_says_something()
{
    for (const Code code : kCodes)
        if (text_of(code).size() < 10)
            return false;
    return true;
}

// The holes run 1..arity with none missing.
//
// THE ONE THAT CATCHES A REAL EDIT. A sentence rewritten from "{1} under {2}"
// to "{2}" keeps arity 2 and silently ignores whatever the site passes as the
// first argument -- the text renders, the build is quiet, and one of the two
// facts the message was about is gone. There is no way to see that by reading
// either half alone.
constexpr bool holes_are_dense()
{
    for (const Code code : kCodes) {
        const std::string_view text = text_of(code);
        for (unsigned hole = 1; hole <= arity_of(code); hole++) {
            const char wanted[3] = {'{', static_cast<char>('0' + hole), '}'};
            if (text.find(std::string_view(wanted, 3)) == std::string_view::npos)
                return false;
        }
    }
    return true;
}

// A sentence is a fragment: it opens lower case and does not end in a stop.
//
// THIS IS THE ONLY THING ABOUT THE WORDING AN ASSERT CAN HONESTLY SEE, and it
// is here instead of the length floor an earlier draft had. That one demanded
// forty characters on the grounds that DESIGN §9's model sentence is
// fifty-two -- and it failed on `{1} was declared here`, which is a perfectly
// good note, and on `this did not parse: {1}`, whose fix is in the note beside
// it. A length is arithmetic wearing a judgement's clothes, which is exactly
// what FORMAT/CXX.md §8 warns about. What IS checkable is the FORMAT: every
// message is rendered after `satl: <where>: error S0231: `, so a capital or a
// full stop is a message that will read wrong in the one place it appears.
constexpr bool every_sentence_is_a_fragment()
{
    for (const Code code : kCodes) {
        const std::string_view text = text_of(code);
        if (text.front() >= 'A' && text.front() <= 'Z')
            return false;
        if (text.back() == '.')
            return false;
    }
    return true;
}

} // namespace detail

static_assert(detail::numbers_ascend(),
              "errors.def: the numbers must ascend -- append inside a block, "
              "never insert, and take a fresh block rather than interleaving");
static_assert(detail::none_is_reserved(),
              "errors.def: 0 is Code::NONE and is not a message");
static_assert(detail::every_code_says_something(),
              "errors.def: a row with no sentence is a code nobody can act on");
static_assert(detail::holes_are_dense(),
              "errors.def: a sentence's holes run {1}..{n} with none skipped -- "
              "a skipped hole silently drops the argument the site passed");
static_assert(detail::every_sentence_is_a_fragment(),
              "errors.def: a sentence is rendered after `satl: ... error "
              "S0231: `, so it opens lower case and ends without a full stop");

// WHAT THESE DO NOT COVER, said out loud because the list above reads like a
// guarantee. They see the TABLE. They cannot see whether a call site passes the
// arguments in the right order, whether a sentence is true of the code that
// raises it, whether two rows say the same thing in two ways, or -- the one
// DESIGN §9 actually asks for -- whether a sentence NAMES THE FIX rather than
// stopping at a label. tests/reporter_test is where the first is checked by
// rendering, and the rest are what reading errors.def as one page is for.

} // namespace satellite::errors
