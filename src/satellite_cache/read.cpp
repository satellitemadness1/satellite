// Reading a `.satc` back -- SATC.md §4. See satellite_cache/cache.hpp for what
// a `.satc` is and why the numbers are turned back into words rather than given
// a grammar of their own.
//
// READING COMES BEFORE WRITING AND THE ORDER IS EASY TO GET BACKWARDS, which is
// the sentence §4 opens with. The three steps are: look for the file, use it
// when it is well-formed and all three header lines match, otherwise walk the
// source and write a fresh one. Nothing here writes; save.cpp is step 3 and
// main.cpp is where the two are put in that order.
//
// A MISS IS NOT A FAILURE AND THIS FILE IS WHERE THAT IS ENFORCED. §4: "a
// missing, stale or unreadable `.satc` is NEVER an error. It is a cache miss,
// the program runs exactly as it would have, and the only cost is the walk that
// would have happened anyway." So every branch below that cannot produce a tree
// produces a Miss instead, and only two of them produce a sentence -- a file
// this build cannot read at all, and a file that is a `.satc` and is wrong.
// Both are facts about the machine rather than about the user's program, which
// is the line §5 draws between them and a permission the user declined to give.
//
// THE SUBSTITUTION ITSELF IS NEXT DOOR, in satellite_cache/unnumber.cpp, for
// the reason write_internal.hpp gives about its own three files: this one is
// about the ORDER things are checked in and that one is about what a number
// stands for, and neither wants to be read while looking for the other.
//
// WHICH IS THE MILESTONE'S REASON TO EXIST AT THIS POINT IN THE PLAN. PLAN's
// M4.5 paragraph puts the cache before M5 "because a malformed `.satc` is the
// first thing in the language that has to say something to a user in plain
// words", and Reading::note is that thing.
//
// M5 TOOK THE SENTENCES AND LEFT THE ORDER, which is exactly the split that
// paragraph predicted. This file used to compose four string literals and an
// `ignored()` helper that appended a fifth to each of them; they are now
// errors.def's S03xx block, one row each, and the fifth is a NOTE that all four
// attach rather than a suffix each of them remembers to add. What is still
// here, and is what this file was always about, is WHICH CHECK RUNS FIRST and
// which failures are allowed to say anything at all.

#include "satellite_cache/cache.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "parser/parser.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>
#include <utility>

namespace satellite::cache {

namespace {

// The whole file, or false.
//
// NOT programs/source_file.hpp, WHICH DOES THE SAME THING ONE LAYER UP. That
// one belongs to the binary and reports what it could not read, because a
// source the user named and satl cannot open is a sentence somebody has to
// write. This one belongs to a module that must stay silent: a `.satc` that is
// not there is the ordinary first run of every program, and a reader that
// borrowed the caller's error path would say so out loud once per program.
bool slurp(const std::string &path, std::string &into)
{
    FILE *handle = fopen(path.c_str(), "rb");
    if (handle == nullptr)
        return false;
    char buffer[8192];
    size_t got = 0;
    while ((got = fread(buffer, 1, sizeof buffer, handle)) > 0)
        into.append(buffer, got);
    const bool whole = ferror(handle) == 0;
    fclose(handle);
    return whole;
}

// The line starting at `at`, and `at` moved past its newline. A last line with
// no newline is still a line.
std::string next_line(const std::string &text, size_t &at)
{
    const size_t end = text.find('\n', at);
    const std::string line =
        text.substr(at, end == std::string::npos ? end : end - at);
    at = end == std::string::npos ? text.size() : end + 1;
    return line;
}

// The note every one of these ends with.
//
// SAYING WHAT HAPPENS NEXT IS THE WHOLE POINT OF SAYING ANYTHING. A person told
// only that a file is damaged has been given a job; a person told that it was
// ignored, that the program ran anyway, and that deleting it is safe has been
// given a fact. DESIGN §9's model is a full sentence that names the fix, and
// for a cache the fix is almost always "nothing".
//
// A NOTE AND NOT A SUFFIX, AS OF M5. It was a string this file appended to each
// of the four sentences, which meant four places had to remember to append it;
// it is one row in errors.def now, attached the way DESIGN §9's notes are, and
// the renderer is what decides how it sits under the error.
errors::Note ignored()
{
    return errors::note<errors::Code::NOTE_SATC_IGNORED>(errors::kNowhere);
}

} // namespace

Reading read_text(const std::string &text, const Source &source,
                  words::Words &words)
{
    Reading out;
    size_t at = 0;

    // §2's FIRST LINE IS THE ONLY ONE THAT IS AN ERROR, and it is read before
    // anything else for the reason the table gives: the other two lines mean
    // what this one says they mean. A file from a format satl does not have is
    // refused rather than guessed at, because guessing is how a reader
    // misreads a field that moved and runs a program that was never written.
    const std::string format = next_line(text, at);
    if (format.compare(0, 5, "satc ") != 0) {
        out.why = Miss::MALFORMED;
        out.note = errors::make<errors::Code::SATC_NOT_A_SATC>(errors::kNowhere);
        out.note.notes.push_back(ignored());
        return out;
    }
    const std::string version = format.substr(5);
    if (version != std::to_string(kFormatVersion)) {
        out.why = Miss::REFUSED;
        out.note = errors::make<errors::Code::SATC_WRONG_VERSION>(
            errors::kNowhere, version, kFormatVersion);
        out.note.notes.push_back(ignored());
        return out;
    }

    // THE OTHER TWO ARE COMPARED AS WHOLE LINES AGAINST THE ONES THIS BUILD
    // WOULD WRITE, which is what header_text() is a separate function for. Every
    // field in them -- the numbering's version, its digest, the source's name,
    // mtime and size -- has to match, and comparing the text is the one form of
    // that check which cannot forget a field somebody adds later.
    const std::string expected = header_text(source);
    size_t want = expected.find('\n') + 1;
    for (int i = 0; i < 2; i++)
        if (next_line(text, at) != next_line(expected, want)) {
            // NOT A SENTENCE, AND THAT IS THE POINT. §2 says a `words` or
            // `source` mismatch means "ignore the file, walk the source" -- it
            // is the cache doing its job, and it happens every time a program is
            // edited. A note here would be a message printed on the second run
            // of every program somebody is working on.
            out.why = Miss::STALE;
            return out;
        }

    if (next_line(text, at) != std::string()) {
        out.why = Miss::MALFORMED;
        out.note = errors::make<errors::Code::SATC_NO_BLANK_LINE>(errors::kNowhere);
        out.note.notes.push_back(ignored());
        return out;
    }

    std::string program;
    if (!unnumber(text.substr(at), program, out.note, &out.marks,
                  &out.folded)) {
        // THE CODE COMES FROM unnumber() AND THE NOTE COMES FROM HERE, which is
        // the split that keeps a text pass out of the business of talking to a
        // person: it knows that `#1.99.1` names nothing, and this file knows
        // what happens next and that the user has nothing to do about it.
        out.note.notes.push_back(ignored());
        out.why = Miss::MALFORMED;
        return out;
    }

    // THE CALLER'S NUMBERING IS TOUCHED ONLY ON A HIT, and the local below is
    // the whole of that guarantee. A user's capsule takes the next number free
    // under its parent when the name is first met (WORD_NUMBERS §3), so parsing
    // a `.satc` that then turns out to be malformed would leave `fact` and
    // `helper` already allocated -- and the caller's fall back to the SOURCE
    // would meet them a second time, find them defined, and report the user's
    // own program as declaring a name twice. A miss has to cost exactly one
    // walk and nothing else, which is §4's whole promise.
    words::Words fresh;
    out.program = parse(program, fresh);
    if (!out.program.ok()) {
        // A `.satc` THAT DOES NOT PARSE IS MALFORMED AND A SOURCE THAT DOES NOT
        // IS THE USER'S. Nobody typed this file: the writer produced it from a
        // tree that had already parsed, so a parse error in it is evidence
        // about the machine -- a truncated write, a damaged disk, an edit by
        // hand -- and never about the program the user wrote.
        out.why = Miss::MALFORMED;
        // THE PARSER'S SENTENCE QUOTED AND NOT RENDERED, which is the one
        // caller errors::sentence() exists for. A caret drawn into a `.satc`
        // would point at a line of a file the next sentence says to delete.
        out.note = errors::make<errors::Code::SATC_DID_NOT_PARSE>(
            errors::kNowhere, errors::sentence(out.program.errors.front()));
        out.note.notes.push_back(ignored());
        return out;
    }

    words = std::move(fresh);
    out.text = std::move(program);
    out.why = Miss::NONE;
    return out;
}

Reading read(const std::string &source_path, const Source &source,
             words::Words &words)
{
    Reading out;
    out.file = cache_path(source_path);
    if (out.file.empty())
        return out;

    std::string text;
    if (!slurp(out.file, text))
        return out;

    Reading found = read_text(text, source, words);
    found.file = out.file;
    return found;
}

} // namespace satellite::cache
