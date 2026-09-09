// `satl --resolve <file>` -- M7's consumer. See programs/resolve_command.hpp
// for why this arm has a file of its own.
//
// THE READING ORDER IS NOT REPEATED HERE. programs/cache_command.hpp's
// read_program() is SATC.md §4's three steps, factored out of `--satc` at this
// milestone precisely so that there is one of them: read.cpp's own header says
// that file is about WHICH CHECK RUNS FIRST, and a second copy of that order
// would be a second answer to it.
//
// WHAT THIS ARM DECIDES THAT NO OTHER ONE HAS TO. A `.satc` is written from a
// tree that PARSED -- cache_command.cpp refuses to cache anything else -- and
// resolve is the first pass that can find something wrong in a tree that
// parsed. So a program with an unknown name in it gets cached on its first run
// and read back on its second, and the tree that comes back has spans into the
// `.satc`'s WORDS: no comments, blank lines where the writer put them, and line
// 6 of that is not line 6 of the file the user wrote. A caret drawn from those
// spans lands on the wrong line of the right file, which is worse than none.
//
// SO A WARM RUN THAT FINDS A PROBLEM IS THROWN AWAY AND TAKEN AGAIN FROM THE
// SOURCE. The cost lands only on a program that is already broken -- and a
// broken program is one somebody is editing, which invalidates the `.satc` on
// the next save anyway. errors.def's S03xx block makes the same call about the
// same file for the same reason: "a caret pointing at byte 412 of a generated
// file tells somebody to read a file that the next sentence tells them to
// delete."

#include "programs/resolve_command.hpp"

#include "error_reporter/report.hpp"
#include "name_resolver/dump.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "programs/cache_command.hpp"
#include "programs/check_command.hpp"
#include "programs/opening.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>
#include <utility>

namespace satellite {

int resolve_command(const std::string &path)
{
    // A RUN'S NAMES END WITH THE RUN, which is why this is a local and not a
    // global: words_runtime.hpp makes the point that M22 runs many programs in
    // one process and each needs its own numbering.
    words::Words words;
    cache::Save writing;

    Reading read = read_program(path, words, writing);
    if (!read.ok)
        return EXIT_USAGE;

    // NOTHING IS RESOLVED IN A TREE THAT DID NOT PARSE. Half a program has
    // names whose declarations were never read, so every one of them would be
    // reported unknown -- twenty carets under a file whose real problem is the
    // bracket on line 4. The parser already said what is wrong; this pass adds
    // nothing by saying it again in its own words.
    if (!read.parsed.ok())
        return EXIT_MALFORMED;

    // RESOLVED ALREADY, BY read_program() -- M19.6 moved it there because the
    // `.satc` writer needs its answer. Taking it rather than running it again is
    // not an optimisation: a second resolve over the same tree with the same
    // numbering would meet every capsule a second time and report the user's own
    // program as declaring each of them twice, which is the shape M4.5's reader
    // already had to avoid for the same reason.
    // MOVED AND NOT COPIED, which is worth the word `std::move` rather than
    // being left to be noticed. `Resolved` carries one `Info` per AST node, so
    // a copy here is an allocation and a walk proportional to the program --
    // and it was measurable the moment read_program() started producing one:
    // 0.19 ms on example/frames.satl, on an arm that does exactly the same work
    // it did before this milestone.
    resolve::Resolved resolved = std::move(read.resolved);

    if (!resolved.ok() && read.from_cache) {
        // TAKEN AGAIN FROM THE SOURCE -- the header says why. `words` is left
        // as it is: the `.satc` reader already declared this program's names
        // into it in the order the file declared them (SATC.md §5.2), so a
        // second parse of the same program meets them again and would report
        // every capsule as declared twice. A fresh numbering is what a fresh
        // walk needs.
        fprintf(stderr,
                "satl: %s did not resolve, so it was read again from the "
                "source -- a caret into a `.satc` points at the wrong line.\n",
                path.c_str());

        std::string source;
        if (!open_source(path, source))
            return EXIT_USAGE;

        words::Words fresh;
        const Parse again = parse(source, fresh);
        resolved = resolve::resolve(again.ast, fresh);
        fputs(errors::render(resolved.problems, errors::Source{path, source}).c_str(),
              stderr);
        fputs(resolve::dump_text(path, again.ast, fresh, resolved).c_str(), stdout);
        return EXIT_MALFORMED;
    }

    // THE ANSWER GOES TO STDOUT AND THE COMPLAINTS TO STDERR, which is the
    // split --tokens got wrong on its first day and every arm since has kept:
    // `satl --resolve f.satl > frames.txt` must write the frames, and a person
    // watching the terminal must still see what did not resolve. Both are
    // printed for a program with a problem in it, because a frame that WAS
    // decided is an answer even when one name in the file was not.
    if (!resolved.problems.empty())
        fputs(errors::render(resolved.problems, errors::Source{path, read.text}).c_str(),
              stderr);

    fputs(resolve::dump_text(path, read.parsed.ast, words, resolved).c_str(), stdout);
    return resolved.ok() ? EXIT_FINE : EXIT_MALFORMED;
}

} // namespace satellite
