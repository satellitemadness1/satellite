// The sixteen `satellite.variable.string` methods, `1 6 1 1` through
// `1 6 1 16` -- PLAN M11. See satellite_scalars/handlers.hpp for what
// installs and methods_internal.hpp for the checks every row makes first.
//
// THE SIXTEEN CAME FROM QUAD AND NOT FROM v1, which decides how they are
// specified. v1's string surface is six methods and only three of these are
// among them; the rest were numbered on 2026-08-27 when QUAD.md §3 settled
// what `<string>` and `<sstream>` become, with signatures in WORD_NUMBERS
// §2.2 and no semantics anywhere. So the semantics are decided HERE, at the
// milestone that builds them, on two rules, and MILESTONES/M11.md carries
// every decision for the author to overrule:
//
//   1. POSITIONS COUNT FROM 0, end-exclusive where a range is asked --
//      methods_internal.hpp's position_at carries the argument.
//   2. WHERE ANOTHER LANGUAGE HANDS BACK A SENTINEL, THESE REFUSE -- find on
//      a missing needle, at past the end, to_number on a word. A refusal can
//      loosen into an answer when M12 makes "nothing" askable; a -1 that
//      programs test for is forever. `contains` and `size` exist precisely so
//      the questions can be asked first.
//
// A METHOD SEES CODES, NOT CHARACTERS-ON-A-SCREEN. A string is SatChars over
// DESIGN §5's table, and a live code (`\threads`) is ONE code that render
// answers at display time -- so `size` counts it as 1 and `upper` passes it
// through, the same way the lexer and the `.satc` writer treat it. Resolving
// live codes here would make `size` disagree with what is stored, which is
// the render/store boundary satellite_string's header draws.

#include "satellite_scalars/methods_internal.hpp"

#include "error_reporter/report.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::scalars {

namespace {

Value as_value(SatString text)
{
    return Value::string(std::move(text));
}

// DESIGN §5's table keeps letters in two runs 26 apart -- a..z at 1, A..Z at
// 27 -- so case is one add on the letter runs and identity everywhere else,
// raw bytes and live codes included.
SatChar lowered(SatChar c)
{
    return c >= SAT_UPPER_A && c < SAT_DIGIT_0 ? static_cast<SatChar>(c - 26) : c;
}

SatChar uppered(SatChar c)
{
    return c >= SAT_A && c < SAT_UPPER_A ? static_cast<SatChar>(c + 26) : c;
}

// What `trim` strips: the whitespace bytes, which live in the raw area --
// space, tab, carriage return and newline have no codes of their own, so they
// round-trip as SAT_RAW_BASE + byte and that is where trim has to look.
bool is_space(SatChar c)
{
    return c == SAT_RAW_BASE + ' ' || c == SAT_RAW_BASE + '\t' ||
           c == SAT_RAW_BASE + '\r' || c == SAT_RAW_BASE + '\n';
}

// --- the sixteen, in numbering order ---------------------------------------

bool string_size(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    if (!string_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number(static_cast<long long>(self->size())));
    return true;
}

bool string_empty(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    if (!string_at(m, a, 0, &self))
        return false;
    *answer = Value::boolean(self->empty());
    return true;
}

bool string_find(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    const SatString *needle = nullptr;
    if (!string_at(m, a, 0, &self) || !string_at(m, a, 1, &needle))
        return false;
    const size_t at = self->find(*needle);
    if (at == SatString::npos) {
        // S0716 AND NOT -1 -- the file-note's second rule at its sharpest
        // site. The sentence names `contains` because that is the question
        // the program should have asked first.
        m.refuse(errors::make<errors::Code::EVAL_NOT_FOUND>(
            m.span_of(m.here()), decode(*needle)));
        return false;
    }
    *answer = Value::number(Number(static_cast<long long>(at)));
    return true;
}

bool string_contains(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    const SatString *needle = nullptr;
    if (!string_at(m, a, 0, &self) || !string_at(m, a, 1, &needle))
        return false;
    *answer = Value::boolean(self->find(*needle) != SatString::npos);
    return true;
}

bool string_substring(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    unsigned long long start = 0;
    unsigned long long end = 0;
    if (!string_at(m, a, 0, &self) || !position_at(m, a, 1, &start) ||
        !position_at(m, a, 2, &end))
        return false;
    if (start > end) {
        m.refuse(errors::make<errors::Code::EVAL_BACKWARDS>(
            m.span_of(m.here()), std::to_string(start), std::to_string(end)));
        return false;
    }
    if (end > self->size()) {
        // `end` IS EXCLUSIVE, so end == size() is the whole tail and legal;
        // one past it is the first number with nothing under it.
        m.refuse(errors::make<errors::Code::EVAL_PAST_THE_END>(
            m.span_of(m.here()), std::string(m.text_of(m.here())),
            std::to_string(end), std::to_string(self->size())));
        return false;
    }
    *answer = as_value(self->substr(start, end - start));
    return true;
}

bool string_starts_with(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    const SatString *head = nullptr;
    if (!string_at(m, a, 0, &self) || !string_at(m, a, 1, &head))
        return false;
    *answer = Value::boolean(self->size() >= head->size() &&
                             self->compare(0, head->size(), *head) == 0);
    return true;
}

bool string_ends_with(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    const SatString *tail = nullptr;
    if (!string_at(m, a, 0, &self) || !string_at(m, a, 1, &tail))
        return false;
    *answer = Value::boolean(
        tail->size() <= self->size() &&
        self->compare(self->size() - tail->size(), tail->size(), *tail) == 0);
    return true;
}

bool string_lower(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    if (!string_at(m, a, 0, &self))
        return false;
    SatString out = *self;
    for (SatChar &c : out)
        c = lowered(c);
    *answer = as_value(std::move(out));
    return true;
}

bool string_upper(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    if (!string_at(m, a, 0, &self))
        return false;
    SatString out = *self;
    for (SatChar &c : out)
        c = uppered(c);
    *answer = as_value(std::move(out));
    return true;
}

bool string_split(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    // THE ROW THAT WAS WAITING FOR M16, ANSWERED. Until 2026-09-05 this
    // refused by name because "it answers a `satellite.container.list`, and
    // PLAN.md §8 builds the containers at M16" -- the containers exist now,
    // so the sentence has become the implementation.
    const SatString *self = nullptr;
    const SatString *separator = nullptr;
    if (!string_at(m, a, 0, &self) || !string_at(m, a, 1, &separator))
        return false;

    List out;
    // AN EMPTY SEPARATOR SPLITS INTO CHARACTERS rather than looping forever,
    // which is the one edge this needs a rule for. It is also the useful
    // answer: `s.split("")` is how a program asks for the characters, and
    // the alternative -- a refusal -- would send it to `at(n)` and a `for`.
    if (separator->empty()) {
        for (SatChar c : *self)
            out.push_back(Value::string(SatString(1, c)));
        *answer = Value::list(std::move(out));
        return true;
    }

    // EVERY PIECE, INCLUDING THE EMPTY ONES AT THE ENDS. "a,,b" split on ","
    // is three pieces and ",a" is two, because a split that quietly dropped
    // them would lose a column in every CSV line that has a blank field --
    // and the count of pieces is what a program checks.
    size_t at = 0;
    for (;;) {
        const size_t next = self->find(*separator, at);
        if (next == SatString::npos)
            break;
        out.push_back(Value::string(self->substr(at, next - at)));
        at = next + separator->size();
    }
    out.push_back(Value::string(self->substr(at)));
    *answer = Value::list(std::move(out));
    return true;
}

bool string_trim(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    if (!string_at(m, a, 0, &self))
        return false;
    size_t first = 0;
    size_t last = self->size();
    while (first < last && is_space((*self)[first]))
        first++;
    while (last > first && is_space((*self)[last - 1]))
        last--;
    *answer = as_value(self->substr(first, last - first));
    return true;
}

bool string_replace(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    const SatString *from = nullptr;
    const SatString *to = nullptr;
    if (!string_at(m, a, 0, &self) || !string_at(m, a, 1, &from) ||
        !string_at(m, a, 2, &to))
        return false;
    if (from->empty()) {
        // AN EMPTY NEEDLE "APPEARS" EVERYWHERE AND NOWHERE, and every
        // language that answers something here answers something surprising.
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), "replace", "a non-empty string to replace",
            "empty"));
        return false;
    }
    // EVERY OCCURRENCE, LEFT TO RIGHT, NOT RESCANNING WHAT WAS WRITTEN IN --
    // so replace("a", "aa") terminates and means what it reads as.
    SatString out;
    size_t from_at = 0;
    for (;;) {
        const size_t at = self->find(*from, from_at);
        if (at == SatString::npos)
            break;
        out.append(*self, from_at, at - from_at);
        out.append(*to);
        from_at = at + from->size();
    }
    out.append(*self, from_at, self->size() - from_at);
    *answer = as_value(std::move(out));
    return true;
}

bool string_to_number(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    if (!string_at(m, a, 0, &self))
        return false;
    // EXACTLY WHAT A LITERAL WOULD PARSE AS, refusals included -- S0610 is the
    // literal's sentence and this is the same sentence about the same rule, so
    // `"  42".to_number()` refuses rather than quietly trimming. `trim` is one
    // dot away and asking for it is the program's decision to make.
    const std::string text = decode(*self);
    Number value;
    if (!Number::parse(text, value)) {
        m.refuse(errors::make<errors::Code::NUMBER_NOT_A_NUMBER>(
            m.span_of(m.here()), text));
        return false;
    }
    *answer = Value::number(std::move(value));
    return true;
}

bool string_append(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    const SatString *tail = nullptr;
    if (!string_at(m, a, 0, &self) || !string_at(m, a, 1, &tail))
        return false;
    // THE ANSWER IS THE RECEIVER'S NEW VALUE -- the mutating contract,
    // operations_dispatch.cpp's file note. The machine writes it back to the
    // slot the method was called on; this row only says what the slot becomes.
    *answer = as_value(*self + *tail);
    return true;
}

bool string_clear(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    if (!string_at(m, a, 0, &self))
        return false;
    *answer = as_value(SatString());
    return true;
}

bool string_char_at(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const SatString *self = nullptr;
    unsigned long long at = 0;
    if (!string_at(m, a, 0, &self) || !position_at(m, a, 1, &at))
        return false;
    if (at >= self->size()) {
        m.refuse(errors::make<errors::Code::EVAL_PAST_THE_END>(
            m.span_of(m.here()), std::string(m.text_of(m.here())),
            std::to_string(at), std::to_string(self->size())));
        return false;
    }
    // ONE CHARACTER IS A STRING OF SIZE 1. The language has no character type
    // and DESIGN §8's table shows no row where one would go, so the honest
    // answer is the smallest value of the type the question was asked of.
    *answer = as_value(SatString(1, (*self)[at]));
    return true;
}

} // namespace

void install_string_methods()
{
    using words::NodeId;
    eval::Handlers &table = eval::Handlers::table();

    // EVERY ROW BINDS ITS RECEIVER AND COUNTS IT IN THE ARITY -- argument 0 is
    // the string the method was asked of, per DESIGN §6.4's written-out form,
    // and operations_dispatch.cpp subtracts it back out of any sentence about
    // counts. The two mutating rows are the last column doing its job.
    struct Row {
        NodeId path;
        eval::HandlerFn fn;
        uint32_t arity;
        bool mutates;
    };
    static constexpr Row rows[] = {
        {NodeId::VARIABLE_STRING_SIZE,        string_size,        1, false},
        {NodeId::VARIABLE_STRING_EMPTY,       string_empty,       1, false},
        {NodeId::VARIABLE_STRING_FIND,        string_find,        2, false},
        {NodeId::VARIABLE_STRING_CONTAINS,    string_contains,    2, false},
        {NodeId::VARIABLE_STRING_SUBSTRING,   string_substring,   3, false},
        {NodeId::VARIABLE_STRING_STARTS_WITH, string_starts_with, 2, false},
        {NodeId::VARIABLE_STRING_ENDS_WITH,   string_ends_with,   2, false},
        {NodeId::VARIABLE_STRING_LOWER,       string_lower,       1, false},
        {NodeId::VARIABLE_STRING_UPPER,       string_upper,       1, false},
        {NodeId::VARIABLE_STRING_SPLIT,       string_split,       2, false},
        {NodeId::VARIABLE_STRING_TRIM,        string_trim,        1, false},
        {NodeId::VARIABLE_STRING_REPLACE,     string_replace,     3, false},
        {NodeId::VARIABLE_STRING_TO_NUMBER,   string_to_number,   1, false},
        {NodeId::VARIABLE_STRING_APPEND,      string_append,      2, true},
        {NodeId::VARIABLE_STRING_CLEAR,       string_clear,       1, true},
        {NodeId::VARIABLE_STRING_AT,          string_char_at,     2, false},
    };
    for (const Row &row : rows)
        table.install(static_cast<words::PathId>(row.path),
                      {row.fn, true, row.arity, "M11", row.mutates});
}

} // namespace satellite::scalars
