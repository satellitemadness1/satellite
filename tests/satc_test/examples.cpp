// Every acceptance program, written as a `.satc`. See
// tests/satc_test/satc_test.hpp.
//
// LAYOUT.md CALLS THE FILES IN example/ "not samples -- each of these is what a
// milestone means by done", and 065-tests.mk makes all six a prerequisite of
// this binary for the reason it gives about WORD_NUMBERS.md: editing an
// acceptance program must re-run the test that reads it.
//
// IT FAILS LOUDLY RATHER THAN SKIPPING when it cannot read a file. words_test
// established that rule for WORD_NUMBERS.md and parser_test carried it into a
// third place; this is the fourth. A suite that quietly checks nothing is the
// green line 065-tests.mk's own header calls the worst failure a suite has.
//
// WHAT IS CHECKED HERE THAT THE FIXTURES CANNOT REACH. §5.2 requires the writer
// to emit in SOURCE ORDER, because a user's capsule takes "the next number free
// when the name is first met" and a program read back from a `.satc` has to
// arrive at the same allocation. The consequence §6 draws is that the output
// becomes byte-identical and therefore comparable -- so writing the same
// program twice is a check, and it is one no single-statement fixture can make.

#include "satc_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "abstract_syntax_tree/unparse.hpp"
#include "parser/parser.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace satc_test {

namespace {

// The four programs LAYOUT.md names as acceptance, which are the four that
// parse. class_test.satl and gui_example.satl are the two M4 pinned as files
// that must NOT parse, and they are checked in parser_test rather than here --
// a writer has nothing to say about a tree that was never built.
const char *const kPrograms[] = {"hello_world.satl", "advanced.satl",
                                 "thread_test.satl", "super_advanced.satl"};

// Every MARKED path in a `.satc` line, in the order it was written.
//
// THE MARK IS WHAT MAKES THIS SCAN POSSIBLE AT ALL. Written as "every run of
// digits and dots" it also picked up float literals -- `1.5` the number is
// spelled exactly as `1 5` the path -- and a program with one in it would have
// been reported as a line whose comment column had lost a path. paths.hpp's
// kPathMark is the character that settles which is which, and reading it here
// is the same question a reader of the file asks.
std::vector<std::string> numbers_in(const std::string &line)
{
    std::vector<std::string> found;
    for (size_t at = 0; at < line.size(); at++) {
        if (line[at] != satellite::cache::kPathMark)
            continue;
        const size_t start = ++at;
        while (at < line.size() &&
               ((line[at] >= '0' && line[at] <= '9') || line[at] == '.'))
            at++;
        found.push_back(line.substr(start, at - start));
    }
    return found;
}

// The comment's paths, split on the spaces BETWEEN them.
//
// NOT EVERY SPACE, because a path may carry an argument list and an argument
// list may carry a comma and a space -- `satellite.console.input(prompt,
// target)` is one path with a space in the middle of it. Depth is what tells
// the two apart, and a split that did not track it would report `target)` as a
// path the language does not have.
std::vector<std::string> split_comment(const std::string &comment)
{
    std::vector<std::string> paths;
    std::string current;
    int depth = 0;
    for (const char c : comment) {
        if (c == '(')
            depth++;
        else if (c == ')')
            depth--;
        if (c == ' ' && depth == 0) {
            if (!current.empty())
                paths.push_back(current);
            current.clear();
            continue;
        }
        current += c;
    }
    if (!current.empty())
        paths.push_back(current);
    return paths;
}

// §5.2: THE WRITER EMITS IN SOURCE ORDER, and this is the check the four
// acceptance programs cannot make.
//
// WRITING THE SAME PROGRAM TWICE PROVES STABILITY AND NOT ORDER. Both runs go
// through the same writer, so a writer that sorted its declarations would sort
// them identically twice and the comparison above would pass. What order
// actually costs is in WORD_NUMBERS §3: a user's capsule takes "the next number
// free when the name is FIRST MET", so a program read back from a `.satc` whose
// declarations had been reordered would allocate `zebra` and `alpha` the other
// way round and mean something else. The names below are chosen so that any
// sort -- alphabetical, by length, by number -- moves them.
void source_order()
{
    const std::string written = body(
        "satellite.capsule zebra()\n{\nsatellite.return()\n}\n\n"
        "satellite.capsule alpha()\n{\nsatellite.return()\n}\n");
    const size_t first = written.find("zebra");
    const size_t second = written.find("alpha");
    check(first != std::string::npos && second != std::string::npos,
          "both capsules were written");
    check(first < second,
          "declarations are written in source order and not sorted");
}

void one_program(const std::string &name)
{
    std::string source;
    if (!read_example(name, source)) {
        check(false, "could not read " + example_directory + "/" + name);
        return;
    }

    satellite::words::Words words;
    const satellite::Parse parsed = satellite::parse(source, words);
    if (!parsed.ok()) {
        check(false, name + " did not parse: " + parsed.errors.front().reason);
        return;
    }

    const std::string written = satellite::cache::body_text(parsed.ast, words);
    check(!written.empty(), name + " produced an empty body");

    // §5.2, AND THE ONLY WAY TO CHECK IT WITHOUT A READER. A second parse of
    // the same text is a second run: `words` is fresh, so every user name is
    // met and numbered again from nothing. Source order is what makes the two
    // agree, and a writer that sorted, grouped or hoisted anything would differ here.
    satellite::words::Words again_words;
    const satellite::Parse again = satellite::parse(source, again_words);
    check(satellite::cache::body_text(again.ast, again_words) == written,
          name + " is not written the same way twice");

    // THE COMMENT COLUMN NAMES EXACTLY THE PATHS ON ITS LINE, which is the
    // property §1.1 asks the column to have -- "how a person confirms that
    // 1.5.1 still says what they think it says". A comment that has drifted
    // from its line is worse than no comment, because it is the thing a reader
    // trusts instead of looking the number up.
    size_t at = 0;
    while (at < written.size()) {
        const size_t end = written.find('\n', at);
        const std::string line =
            written.substr(at, end == std::string::npos ? end : end - at);
        at = end == std::string::npos ? written.size() : end + 1;

        const std::string comment = comment_of(line);
        const std::string code = code_of(line);
        if (comment.empty()) {
            check(numbers_in(code).empty(),
                  name + ": a numbered line carries no comment: " + code);
            continue;
        }

        // EVERY PATH THE COMMENT NAMES IS ON THE LINE AS ITS NUMBER, checked
        // through the trie rather than by counting. This is the substitution
        // run backwards: words::walk() turns the comment's text into the id it
        // names and number_text() turns that id into the digits the writer
        // should have emitted, so a comment that has drifted from its line
        // fails here instead of misleading the person who trusts it.
        for (const std::string &path : split_comment(comment)) {
            const satellite::words::Walk found = satellite::words::walk(path);
            check(found.error == satellite::words::WalkError::NONE,
                  name + ": the comment names a path the language does not "
                         "have: " + path);
            if (found.error != satellite::words::WalkError::NONE)
                continue;
            check(code.find(satellite::cache::number_text(found.id)) !=
                      std::string::npos,
                  name + ": " + path + " is named but " +
                      satellite::cache::number_text(found.id) +
                      " is not on the line: " + code);
        }
    }
}

} // namespace

void section_examples()
{
    for (const char *const name : kPrograms)
        one_program(name);
    source_order();
}

} // namespace satc_test
