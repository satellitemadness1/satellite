#pragma once

// The `.satc` file -- PLAN M4.5. [SATC.md](../../SATC.md) is the specification
// and this header is the door onto it.
//
// WHAT A `.satc` IS, IN ONE SENTENCE THAT IS EASY TO GET WRONG: a source file
// with the dictionary already applied. SATC.md §7 draws the line and it is the
// difference between a cache and a bytecode VM -- nothing in the file names a
// handler, an instruction, or an evaluation order. It holds a TREE, and a
// tree's evaluation order is decided by the evaluator on every run, so there is
// no order in the file to get wrong.
//
// IT IS A CACHE AND NOT A BUILD PRODUCT. DESIGN §12 promises "no JIT, and no
// compile step the user ever runs", and what keeps that true is that deleting
// every `.satc` on the machine costs nothing but the walk. §4's reading order
// is where that promise is actually kept: a missing, stale or unreadable
// `.satc` is never an error, only a cache miss.
//
// WHERE THE WRITER SITS IN THE PIPELINE IS WHAT DECIDES ITS CONTENTS, and
// SATC.md §3.2 is the section that makes the argument: this runs after M4's
// parser and before M6's resolve, so at the instant the file is written nothing
// has yet decided that `my_list` is a list and the identity of `sort` is
// UNAVAILABLE -- not awkward to record. That is why a path becomes a number and
// a selector does not, and satellite_cache/paths.hpp is where the distinction
// is enforced rather than remembered.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "parser/parser.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <thread>

namespace satellite::cache {

// The format version this build writes -- SATC.md §2's `satc <n>` line, whose
// mismatch behaviour is the only one of the three that is an ERROR rather than
// a miss: a file claiming a format we do not have is refused in plain words.
inline constexpr unsigned kFormatVersion = 1;

// Where a `.satc` came from -- §2's third header line, `source <name> <mtime>
// <size>`.
//
// THE NAME IS IN THE FILE EVEN THOUGH THE FILE IS NAMED AFTER IT, and that is
// load-bearing rather than redundant now that a `.satc` lives in one shared
// directory rather than beside its source: two programs called `hello_world.satl`
// in two directories are two different sources, and this line is what a reader
// checks before believing the cache it just opened is the one it wanted.
struct Source {
    std::string name;
    uint64_t mtime = 0;
    uint64_t size = 0;
};

// The whole file: three header lines, a blank line, and the program.
std::string satc_text(const Ast &ast, const words::Words &words,
                      const Source &source);

// §2's three lines alone. Separate because a reader compares them without
// having read anything else, and because a test wants to bend one of them.
std::string header_text(const Source &source);

// The program alone -- SATC.md §1.1's body, with its comment column.
//
// `words` IS THE RUN'S NUMBERING AND NOT A GLOBAL, for the reason
// words_runtime.hpp gives about M11.B: a user's PathId is valid inside one run
// only, so the object that allocated the names has to be the object asked about
// them. What gets WRITTEN is never that number -- SATC.md §3 -- but the parent
// it hangs under is language-owned and is, and asking is how that is found.
std::string body_text(const Ast &ast, const words::Words &words);

// WHY A `.satc` IS NOT READ BACK BY A SECOND PARSER. SATC.md §4 asks for a
// tree, and the file already is a satellite program -- one with its
// language-owned words spelled as numbers. So the reader turns the numbers back
// into the words they stand for and hands the text to the lexer and the parser
// that read a source, which is the same fixpoint argument
// abstract_syntax_tree/unparse.hpp makes: the strongest statement available is
// that printing and reading are inverses, and it is only available if there is
// exactly one reader. A second grammar for `.satc` would be a second place the
// language is defined, and it would drift.

// Why a `.satc` was not used. SATC.md §4's three misses and its one error.
//
// FOUR ANSWERS AND NOT TWO, because §4 draws a line inside "did not work": a
// missing or stale file "is NEVER an error ... the only cost is the walk that
// would have happened anyway", while a malformed one "says something went wrong
// that a person may want to know about". A caller that could not tell them
// apart would either be silent about a broken cache or would talk about a cache
// that had simply never been written, and the second is worse -- it is a
// message about nothing, printed on the first run of every program.
enum class Miss {
    NONE,       // it was used
    NO_FILE,    // nothing to read, which is what the first run of a program sees
    STALE,      // read, and not about this source or this numbering
    REFUSED,    // a format version this build does not have -- §2's one refusal
    MALFORMED,  // a `.satc` that cannot be believed
};

// What looking for a `.satc` came back with.
struct Reading {
    Miss why = Miss::NO_FILE;

    // The file that was looked at, so a message can name it. Set even on a
    // miss, because "there was nothing at this path" is the useful half of it.
    std::string file;

    // Plain words, and Code::NONE unless `why` is REFUSED or MALFORMED -- §4
    // asks for a note rather than silence, and nothing else here ever produces
    // one: a first run has nothing to say and a stale file is the cache
    // working.
    //
    // A DIAGNOSTIC AS OF M5, WHERE IT WAS A SENTENCE AT M4.5. read.cpp's own
    // header said at the time that it wrote "a full sentence naming the fix,
    // which is DESIGN §9's model, and it is NOT a code -- codes are M5's to
    // design with all of them in view." They are designed; errors.def's S03xx
    // block is where these four sentences live now, and rendering them is the
    // reporter's.
    //
    // IT CARRIES NO SPAN, and errors.def says why beside the block: a `.satc`
    // is not a file the user wrote, so what they can act on is the file and not
    // a byte in it -- which is what the location line already names.
    errors::Diagnostic note;

    // The program, valid only on a hit. It carries its own errors like any
    // other parse, but a `.satc` that does not parse never gets here -- it is
    // MALFORMED, because the writer produced it and the source did not.
    Parse program;

    bool hit() const { return why == Miss::NONE; }
};

// SATC.md §4 steps 1 and 2: look for this source's `.satc`, and use it when all
// three header lines match. `words` is the run's numbering and comes back
// carrying the user names the file declared, in the order it declared them --
// §5.2 is the writer's half of that bargain.
Reading read(const std::string &source_path, const Source &source,
             words::Words &words);

// The same over text that is already in hand, which is what a test has and what
// read() calls once it has opened the file.
Reading read_text(const std::string &text, const Source &source,
                  words::Words &words);

// number_text() run backwards over a whole body: every `#1.5.1` becomes
// `satellite.console.display` and everything else is left exactly as it is.
// False when the body holds a mark the numbering cannot account for, with `why`
// set to the S03xx code for which of the two it was -- read.cpp is what adds
// the note about what happens next, because that is the reading order's fact
// and not this pass's.
bool unnumber(const std::string &body, std::string &into,
              errors::Diagnostic &why);

// SATC.md §5: write `<name>.<pid>.tmp`, `fsync`, `rename`. False when it could
// not be done, which is not an error and is not reported -- "a read-only
// directory, a full disk, a source tree owned by somebody else -- none of these
// are the program's problem".
bool save(const std::string &cache_file, const std::string &text);

// The same write, happening while the program runs -- §5's "the run does not
// wait for the write. The walk finishes, the program starts, and the `.satc` is
// written behind it".
//
// IT JOINS AND IS NOT DETACHED, and that is the difference between §5's promise
// and a cache that is never there. `satl hello_world.satl` finishes in under a
// millisecond, so a detached writer is a thread the process exits out from
// under: the first run pays for the walk, writes nothing, and the second run
// pays for it again, forever. Joining in the destructor puts the wait AFTER the
// program instead of before it, which is what §5 actually asks for -- the run
// does not wait for the write, the PROCESS does, and only for whatever is left
// of a write that started when the walk finished.
class Save {
public:
    Save() = default;
    Save(const Save &) = delete;
    Save &operator=(const Save &) = delete;
    ~Save();

    // Starts the write. Does nothing when `cache_file` is empty, which is what
    // a machine with no HOME gives back.
    void start(std::string cache_file, std::string text);

private:
    std::thread thread_;
};

// $HOME/.satl/cache, or empty when this process has no HOME.
std::string cache_directory();

// The `.satc` that belongs to a source, or empty when there is no cache
// directory to put one in. See file.cpp for why every `.satc` lives in one
// directory and what that costs.
std::string cache_path(const std::string &source_path);

// A source's identity for §2's third header line, or false when it cannot be
// stat'd -- which is not this module's failure to report, the same way
// programs/source_file.hpp says a file it cannot read is the caller's sentence
// to write.
bool stamp(const std::string &source_path, Source &into);

} // namespace satellite::cache
