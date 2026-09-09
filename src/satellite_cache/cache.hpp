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
// parser and before M7's resolve, so at the instant the file is written nothing
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
#include <vector>

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

// WHICH SELECTORS RESOLVE FOLDED AN OPTION INTO -- what the writer needs from
// the pass that now runs before it, and it is a side table for the same reason
// `resolve::Info` is one: ast.hpp forbids a mutable field on a node.
//
// A `std::vector<bool>` AND NOT `const resolve::Resolved &`, WHICH IS A
// DEPENDENCY AND NOT A TASTE. `name_resolver/resolve.hpp` includes THIS file --
// `resolve()` takes `cache::Marks` -- so a writer that named `resolve::Resolved`
// would close the cycle and neither module would compile. The seam is the one
// `eval::Policy::interrupted` already keeps: the caller reads the pass it has
// and hands over the one fact, rather than the two modules learning each other.
// programs/cache_command.cpp is the caller that fills it in.
//
// INDEXED BY THE MEMBER NODE, WHICH IS WHERE THE RESOLVER PUTS IT.
// `fold_option(call, target, under)` sets the flag on `target` -- the Member
// that carries the word `sort` -- and never on the Call around it, so a writer
// asking about the Call would ask about the wrong node and always get false.
struct Folds {
    std::vector<bool> selector;

    bool at(NodeIndex node) const
    {
        return node < selector.size() && selector[node];
    }
};

// NOTHING FOLDED -- what a caller that has not resolved hands over, and what
// every arm did before M19.6. A default rather than an assert, which is the
// choice ast.hpp makes for node 0: a program with no options in it resolves to
// exactly this and writing one is the ordinary case rather than an error.
inline const Folds &nothing_folded()
{
    static const Folds none;
    return none;
}

// The whole file: three header lines, a blank line, and the program.
//
// `folds` IS WHAT MOVING THE WRITE AFTER RESOLVE BOUGHT -- M19.6, and SATC.md
// §5.1 is the section that used to forbid it. Defaulted, because the two
// callers that write a file without resolving one first are the tests that
// check the FORMAT rather than the fold.
std::string satc_text(const Ast &ast, const words::Words &words,
                      const Source &source,
                      const Folds &folds = nothing_folded());

// §2's three lines alone. Separate because a reader compares them without
// having read anything else, and because a test wants to bend one of them.
std::string header_text(const Source &source);

// The program alone -- SATC.md §1.1's body, with its comment column.
//
// `words` IS THE RUN'S NUMBERING AND NOT A GLOBAL, for the reason
// words_runtime.hpp gives about M22: a user's PathId is valid inside one run
// only, so the object that allocated the names has to be the object asked about
// them. What gets WRITTEN is never that number -- SATC.md §3 -- but the parent
// it hangs under is language-owned and is, and asking is how that is found.
std::string body_text(const Ast &ast, const words::Words &words,
                      const Folds &folds = nothing_folded());

// WHY A `.satc` IS NOT READ BACK BY A SECOND PARSER. SATC.md §4 asks for a
// tree, and the file already is a satellite program -- one with its
// language-owned words spelled as numbers. So the reader turns the numbers back
// into the words they stand for and hands the text to the lexer and the parser
// that read a source, which is the same fixpoint argument
// abstract_syntax_tree/unparse.hpp makes: the strongest statement available is
// that printing and reading are inverses, and it is only available if there is
// exactly one reader. A second grammar for `.satc` would be a second place the
// language is defined, and it would drift.

// WHERE THE FILE ALREADY SAID A NUMBER -- one entry per substitution, in the
// order they were made, so the list ascends and a reader may bisect it.
//
// THIS IS THE HALF MILESTONES/M4.5.md §5 SAYS THE CACHE WAS MISSING. That note
// is written in three places on purpose, and what it says is that "the tree a
// reader hands back has NOWHERE TO PUT the `PathId`s the file already carries",
// so M7's resolve numbers every path again and a warm hit does strictly more
// work than reading the source. This is the somewhere. It is not on the tree --
// ast.hpp forbids that and PLAN §2.2 says why -- and it is not a second tree
// either: it is where the substituted WORDS end in the text the parser is about
// to read, which is a fact about the text and dies with it.
//
// `ends` AND NOT `starts`, and the difference is the whole reason this works.
// A chain's root is the same token for `satellite.time.now()` and for the
// `.some_function()` wrapped around it, so keying on where the substitution
// BEGAN would hand the outer node the inner one's number. Every node that names
// a path is anchored at the path's LAST segment -- ast.hpp: "the token that
// NAMES the node" -- so the end of the substituted words belongs to exactly one
// node, whichever node that turns out to be.
//
// THE ARGUMENT LIST IS NOT COUNTED IN IT. A row may keep its parentheses when
// the words are written back -- `input()` is `1 5 2` and the number says its
// own brackets -- and those characters are not part of any node's anchor, so
// `ends` stops at the last word either way.
struct Mark {
    uint32_t ends = 0;
    words::PathId id = words::kNoPath;
};

using Marks = std::vector<Mark>;

// WHERE A FOLDED SELECTOR'S WORD ENDS -- M19.6, and it is a SECOND list rather
// than a flag on Mark above.
//
// A Mark CARRIES A NUMBER AND THIS CANNOT. `0#down` names the option and not
// the row, which is the author's call and SATC.md §3's line kept: the file says
// what the program WROTE and the fold stays a thing that is worked out. The row
// `1 4 2 5` depends on the receiver's TYPE, and a reader that turns text back
// into text has not resolved anything and does not have one. So an entry here
// is a bare offset -- "the selector ending at this byte had an option folded
// into it" -- and putting one in `Marks` with `kNoPath` for its id would hand
// `mark_ending_at()` an entry meaning "no number" in a list whose whole purpose
// is to answer with one.
//
// THE WORD ITSELF IS NOT CARRIED, BECAUSE THE TREE ALREADY HAS IT. unnumber()
// puts `"down"` back into the text as an ordinary string literal, so the
// argument is a String node the resolver can read with `ast_.text_of()`. What
// the file adds is not the word; it is that the fold HAPPENED, which is the one
// thing a walk over the source has to work out and a warm read does not.
//
// WHAT IT SAVES IS THE DECISION AND NOT THE LOOKUP, and MILESTONES/M19.6.md is
// careful about the difference: `takes_options()`, the sibling scan that
// collects `down, up`, and the bare-word retry are all skipped, and the one
// `shape_path()` walk for `sort_down` remains. A number in the file would have
// removed that too, and it is the thing `0#down` deliberately does not say.
using Folded = std::vector<uint32_t>;

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

    // What the file had already numbered, for M7's resolve. Empty on a miss,
    // for the reason `program` is not filled on one: the text those offsets are
    // into was never handed to a parser.
    Marks marks;

    // Which selectors the file says were folded -- M19.6, and `Folded` above
    // says why it is not a column of `marks`.
    Folded folded;

    // THE TEXT THE TREE'S SPANS INDEX INTO, WHICH IS NOT THE SOURCE FILE. It is
    // the `.satc` body with its numbers turned back into words -- no comments,
    // and blank lines where the writer put them -- so line 6 of this is not
    // line 6 of the program the user wrote. Carried because a caller that wants
    // to RENDER anything about this tree has to render against it, and M7 is
    // the first pass that can find something wrong in a tree that parsed.
    // programs/resolve_command.cpp is what does something about that.
    std::string text;

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
//
// `marks` IS OPTIONAL AND IS NULL FOR EVERY CALLER BUT ONE. A test that wants
// the text does not want the record, and a substitution pass that always built
// one would be paying for M7 in the two places that only need M4.5.
// `folded` IS THE SAME BARGAIN ONE MILESTONE LATER -- M19.6. Null for every
// caller that only wants the text, filled for the one that is about to resolve.
bool unnumber(const std::string &body, std::string &into,
              errors::Diagnostic &why, Marks *marks = nullptr,
              Folded *folded = nullptr);

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
