#pragma once

// The error reporter -- PLAN M5, and the one door over this module.
//
// DESIGN §9 IS THE SPECIFICATION AND IT IS ONE LINE OF IT:
//
//     { ErrorCode, Span, vector<Note>, vector<FrameRef> }
//
// "with RENDERING IN EXACTLY ONE PLACE. Spans on every node. A source excerpt
// with a caret. Notes carrying their own spans. And "did you mean" over the
// trie level that failed." Everything below is that sentence with the corners
// filled in, and the shape is built as written rather than as the parts M5
// happens to have producers for -- see FrameRef.
//
// BUILT BEFORE THE EVALUATOR, DELIBERATELY, which is PLAN's whole argument for
// putting this milestone where it is: "every milestone after this reports
// properly from its first commit. Retrofitting this is exactly how the first
// satellite ended up with 200 bespoke message sites." The count is DESIGN §9's
// and it is worse than the round number suggests -- 199 sites composing 224
// distinct string literals, with no codes, no stack and no suggestions.
//
// NOT BY THROWING, and DESIGN §9.1 measured it: 8.5 ns returned as an enum
// against 1537 ns thrown, 181x. It rules out "record the error and return
// nullptr" just as firmly, because 199 sites where a missed null check is a
// segfault is the wrong shape. What this module hands back is a VALUE that
// carries everything a reader needs, and the two producers so far -- the lexer
// and the parser -- both return their diagnostics in a vector beside the thing
// they built, so nothing has to unwind and a second error is still reachable.
//
// THIS MODULE KNOWS NOTHING ABOUT A TREE OR A TOKEN. It takes a Span, which is
// three integers, and the text those integers index. That is what lets the
// lexer, the parser, the `.satc` reader and M7's resolve all report through it
// without this file learning what any of them is -- and what lets a test render
// a diagnostic that no pass produced.

#include "error_reporter/codes.hpp"
#include "error_reporter/suggest.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace satellite::errors {

// Where in a source something is. Half-open [start, end) BYTE offsets, and a
// 1-based line.
//
// THE SAME THREE FIELDS A Token ALREADY HAS, and that is not a coincidence to
// be tidied away into a typedef. lexer.hpp keeps them because DESIGN §6.2's
// same-line rule needs the line and unparse needs the text; this keeps them
// because a caret needs to be drawn. A Token is a lexical fact and a Span is a
// place, and M7's resolve will make spans for things that are not one token.
//
// LINE 0 MEANS NOWHERE. A file's first line is 1, so 0 cannot be a real place
// and needs no separate flag -- which matters because a Diagnostic about a
// whole file (a `.satc` that is not one) has to be renderable without inventing
// a byte offset for it.
// AND WHICH FILE, SINCE M25's INCLUDE -- 2026-09-13. `file` is 0 for the file
// satl was given and n for the nth spaceship a run loaded, which is the id
// Source::others below is indexed by. Every span written before there was a
// second file says 0 by default initialisation, and that is the right answer
// for all of them.
struct Span {
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t line = 0;
    uint32_t file = 0;

    bool somewhere() const { return line != 0; }
};

inline constexpr Span kNowhere = {};

// A second place worth looking at. DESIGN §9: "notes carrying their own spans."
//
// A NOTE HAS A CODE FOR THE SAME REASON AN ERROR DOES. It is the half of a
// message registry easiest to exempt, and exempting it would put the sentences
// back at the call sites at half the volume -- errors.def carries that
// argument. `NOTE_OPENED_HERE` is attached by ten sites and belongs to none of
// them.
struct Note {
    Code code = Code::NONE;
    Span at = kNowhere;
    std::vector<std::string> arguments;
};

// One frame of the call stack an error happened in -- DESIGN §9's `FrameRef`.
//
// NOTHING PRODUCES ONE AT M5 AND THE TYPE IS HERE ANYWAY, which is a decision
// rather than an oversight and is the opposite of the call M4 made about a
// node's extent. The difference is that this is IN §9's shape and that was not:
// §9 writes four fields and this is the fourth, so building three of them and
// adding the fourth when the evaluator arrives is exactly the retrofit this
// milestone exists to prevent. Rendering lives in one place; a field that
// arrives later arrives as an edit to that place, by somebody who is thinking
// about a stack trace rather than about every other message satl prints.
//
// So the field is carried, the renderer draws it, and tests/reporter_test
// renders a synthetic one -- because a branch nothing exercises is a branch
// that does not work. Its first real producer is M9's evaluator, and M25's
// `satellite.include` is what makes the capsule's own path worth printing.
struct FrameRef {
    words::PathId capsule = words::kNoPath;
    Span at = kNowhere;
};

// One thing wrong, everything known about it, and nothing about how it looks.
//
// `arguments` FILL THE SENTENCE'S HOLES IN ORDER and the count is checked at
// the call site -- see make() below. `suggestion` is the bare word "did you
// mean" offers, not the sentence around it, because the sentence is the
// renderer's and the word is the trie's.
struct Diagnostic {
    Code code = Code::NONE;
    Span at = kNowhere;
    std::vector<std::string> arguments;
    std::vector<Note> notes;
    std::vector<FrameRef> frames;
    std::string suggestion;

    bool is_error() const { return severity_of(code) == Severity::ERROR; }
};

namespace detail {

inline std::string as_text(std::string value) { return value; }
inline std::string as_text(std::string_view value) { return std::string(value); }
inline std::string as_text(const char *value) { return std::string(value); }
inline std::string as_text(char value) { return std::string(1, value); }
inline std::string as_text(unsigned long value) { return std::to_string(value); }
inline std::string as_text(long value) { return std::to_string(value); }
inline std::string as_text(unsigned value) { return std::to_string(value); }
inline std::string as_text(int value) { return std::to_string(value); }

// THE LONG LONGS ARE A DISTINCT TYPE FROM THE LONGS EVEN WHERE THEY ARE THE SAME
// WIDTH, and leaving them out was a hole M6 walked into on its first build:
// `unsigned long long` is what a byte count is here (system_facts/facts.hpp),
// and with no overload for it every conversion is ambiguous against the five
// above rather than picking the obvious one. Added 2026-08-30, with the pair
// completed rather than only the half that was needed, because the next site to
// hand this a signed 64-bit value should not have to come back here.
inline std::string as_text(unsigned long long value) { return std::to_string(value); }
inline std::string as_text(long long value) { return std::to_string(value); }

} // namespace detail

// A diagnostic, with its holes filled.
//
// THE CODE IS A TEMPLATE PARAMETER SO THE ARITY IS A COMPILE ERROR. That is the
// one property this arrangement has that the first satellite's 199 sites could
// not have had at any price: there, the sentence WAS the argument, so there was
// nothing to check it against. Here a site that hands two arguments to
// PARSE_EXPECTED_PUNCT -- three holes -- names the code in the build failure.
//
// A note is built the same way and is a different type, so the two cannot be
// swapped: note<Code::NOTE_OPENED_HERE>(...) goes in `notes` and nowhere else,
// and a code declared SAT_ERROR cannot be one.
template <Code C, typename... Args>
Diagnostic make(Span at, Args &&...arguments)
{
    static_assert(sizeof...(Args) == arity_of(C),
                  "errors::make -- this code's sentence in errors.def has a "
                  "different number of {n} holes than the arguments given here");
    Diagnostic out;
    out.code = C;
    out.at = at;
    out.arguments = {detail::as_text(std::forward<Args>(arguments))...};
    return out;
}

template <Code C, typename... Args>
Note note(Span at, Args &&...arguments)
{
    static_assert(severity_of(C) == Severity::NOTE,
                  "errors::note -- this code is declared SAT_ERROR in "
                  "errors.def, and an error is not a note");
    static_assert(sizeof...(Args) == arity_of(C),
                  "errors::note -- this code's sentence in errors.def has a "
                  "different number of {n} holes than the arguments given here");
    Note out;
    out.code = C;
    out.at = at;
    out.arguments = {detail::as_text(std::forward<Args>(arguments))...};
    return out;
}

// What a diagnostic is rendered AGAINST: the file's name as the user typed it,
// and its bytes.
//
// THE BYTES MAY BE EMPTY AND THE PATH MAY BE EMPTY, and both cases are real
// rather than defensive. A `.satc` that is not a `.satc` has a path and no text
// worth quoting; a diagnostic rendered by a test has text and no path. Each
// drops the part of the header it cannot fill instead of printing a placeholder.
//
// ONE SOURCE AND NOT A MAP UNTIL M25 CHANGED IT, and this paragraph predicted
// exactly where. `satellite.include` makes a program more than one file, so a
// Span says WHICH file (`Span::file`) and a Source carries the others: file 0
// is this one, and file n is `others[n - 1]`. A renderer handed a span from a
// file it has no Source for falls back to this one, which is wrong but bounded
// -- the caret block's clamp says the same about a stale span.
struct Source {
    std::string_view path;
    std::string_view text;

    // THE RUN'S NUMBERING, FOR THE ONE FIELD THAT NEEDS IT, AND M9 IS THE
    // MILESTONE THAT FOUND OUT. FrameRef below is DESIGN §9's fourth field and
    // has been carried since M5 with no producer; its first real producer is
    // the evaluator's call stack, and the renderer could print every frame of a
    // recursion as "a capsule this program declared" -- which is useless
    // exactly when a stack trace is wanted, because every line reads the same.
    //
    // A USER'S CAPSULE HAS A NUMBER AND NOT A SPELLING ANYWHERE ELSE. A
    // language path can be named from constexpr data (words::path_text), which
    // is why the other half of that branch needs nothing; a name the parser
    // interned lives only in the `words::Words` that interned it, and
    // words_runtime.hpp is emphatic that it is "valid inside one run only".
    // So the renderer is handed the run alongside the file, which is what a
    // diagnostic is rendered against anyway.
    //
    // NULL IS A REAL CASE AND NOT A DEFENCE. tests/reporter_test renders
    // synthetic diagnostics with no run behind them at all, and every producer
    // before M9 has no capsule to name. The renderer falls back to the sentence
    // it used to always print.
    const words::Words *words = nullptr;

    // EVERY SPACESHIP THE RUN LOADED, by file id less one -- M25. Null for a
    // program of one file, which is every program written before 2026-09-13.
    const std::vector<Source> *others = nullptr;

    // The Source a span of `file` is rendered against.
    const Source &of(uint32_t file) const
    {
        if (file == 0 || others == nullptr || file > others->size())
            return *this;
        return (*others)[file - 1];
    }
};

// The one place a diagnostic becomes characters. DESIGN §9.
std::string render(const Diagnostic &problem, const Source &source);

// Every diagnostic, in the order they were found, each ending in a newline.
std::string render(const std::vector<Diagnostic> &problems, const Source &source);

// The sentence alone -- the holes filled, no code, no location, no excerpt.
//
// FOR A CALLER THAT IS QUOTING RATHER THAN REPORTING, and there is exactly one:
// satellite_cache/read.cpp says "this did not parse: <the parser's sentence>",
// where the parse error is a fact about the cache file rather than a thing to
// report on its own. Anything that has a place to point at should render()
// instead -- a sentence with no caret is what the first satellite printed.
std::string sentence(const Diagnostic &problem);
std::string sentence(const Note &remark);

// Whether any of them is an error rather than a note. What decides EXIT_MALFORMED.
bool any_error(const std::vector<Diagnostic> &problems);

} // namespace satellite::errors
