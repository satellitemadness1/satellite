// `satl --satc <file>` -- SATC.md §4's reading order, run over the real cache
// directory. See programs/cache_command.hpp for why this arm has a file.
//
// IT IS THE WHOLE LOOP AND NOT ONLY THE PRINTING, which is a change from this
// command's first day, when it printed a `.satc` and deliberately wrote none.
// The milestone is a loop -- look for a `.satc`, use it when its three header
// lines match, walk the source and write a fresh one when they do not -- and a
// consumer that only ever did the middle third of it left the reader and the
// writer with no consumer at all. §4's malformed note also had nowhere to be
// seen, and that note is the reason PLAN schedules this milestone before M5.
//
// THE FILE GOES TO STDOUT AND WHAT HAPPENED GOES TO STDERR, which is the split
// --tokens got wrong on its first day and had to be corrected. `satl --satc
// f.satl > f.satc` still writes the program, and a person watching the terminal
// still sees whether it was read from the cache or built from the source.

#include "programs/cache_command.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "programs/check_command.hpp"
#include "programs/opening.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>

namespace satellite {

Reading read_program(const std::string &path, words::Words &words,
                     cache::Save &writing)
{
    Reading out;

    // THE HEADER NEEDS THE FILE AND NOT ITS TEXT, which is why the source is
    // stat'd before anything is read. §2's third line asks "which file was it
    // and when", and stat is the only thing that knows. It is also half of what
    // a cache LOOKUP compares against, so it has to happen before the lookup
    // rather than beside the write.
    if (!cache::stamp(path, out.stamp)) {
        std::string unused;
        open_source(path, unused);
        return out;
    }

    // §4 STEPS 1 AND 2.
    const cache::Reading found = cache::read(path, out.stamp, words);
    out.cache_file = found.file;

    // §4's PLAIN-WORDS NOTE, AND IT IS Code::NONE ON EVERY ORDINARY MISS. A
    // first run has nothing to say and a stale file is the cache doing its job;
    // what reaches here is a file that is a `.satc` and cannot be believed,
    // which "says something went wrong that a person may want to know about".
    // read.cpp chooses the code, because the module that found the fact is the
    // one that can say which fact it was.
    //
    // AND THE FILE IT IS ABOUT IS THE `.satc`, NOT THE SOURCE, which makes this
    // the one place in the tree that renders against a path the user never
    // typed. A `.satc` diagnostic carries no span -- errors.def's S03xx block
    // argues that -- so what the header line has to name is the file itself,
    // and that is also the file the note says it is safe to delete.
    if (found.note.code != errors::Code::NONE)
        fputs(errors::render(found.note, errors::Source{found.file, {}}).c_str(),
              stderr);

    if (found.hit()) {
        fprintf(stderr, "satl: read %s\n", found.file.c_str());
        out.parsed = found.program;
        out.marks = found.marks;
        out.folded = found.folded;
        out.text = found.text;
        out.from_cache = true;
        out.ok = true;

        // RESOLVED ON THE WARM SIDE TOO, AND THE FIXPOINT IS WHY. `--satc`
        // prints what the WRITER makes of the tree the READER built, so a warm
        // print that skipped resolve would have no folds in it and would differ
        // from the file on disk by exactly the tokens this milestone added --
        // tests/satc_test/reading.cpp compares those two and would say so.
        out.resolved =
            resolve::resolve(out.parsed.ast, words, out.marks, out.folded);
        out.satc = cache::satc_text(out.parsed.ast, words, out.stamp,
                                    resolve::folds_of(out.resolved));
        return out;
    }

    // §4 STEP 3: walk the source as normal, and write a fresh `.satc`
    // afterwards.
    std::string source;
    if (!open_source(path, source))
        return out;

    out.parsed = parse(source, words);
    out.text = source;
    report(path, source, out.parsed.errors);

    // NOTHING IS RESOLVED IN A TREE THAT DID NOT PARSE, which is the rule
    // programs/resolve_command.cpp already states in full: half a program has
    // names whose declarations were never read, so every one of them would be
    // reported unknown -- twenty carets under a file whose real problem is the
    // bracket on line 4.
    //
    // THE PROBLEMS ARE NOT PRINTED HERE. This function's job is the reading
    // order; `--resolve` renders them against the right text and `--satc` is
    // about the format. A pass that both resolved and complained would print
    // them twice for the one caller that wants them.
    if (out.parsed.ok())
        out.resolved = resolve::resolve(out.parsed.ast, words);

    out.satc = cache::satc_text(out.parsed.ast, words, out.stamp,
                                resolve::folds_of(out.resolved));
    out.ok = true;

    // §5: THE RUN DOES NOT WAIT FOR THE WRITE. The thread starts here and is
    // joined when the caller's `writing` goes out of scope, which at M10 will
    // be after the program has RUN rather than after one fputs -- so the write
    // happens beside the work instead of in front of it, which is the whole of
    // §5's "the first run of a program is never slower for having produced
    // one". The thread is handed its own copy of the text rather than a
    // reference to this one, because a writer that outlived its caller's
    // locals would be a cache that corrupts a machine in a second way.
    //
    // A PROGRAM THAT DID NOT PARSE IS NOT CACHED. What the writer made of a
    // partial tree is worth printing -- what was understood is an answer, which
    // is the rule --unparse already keeps -- but a `.satc` is read back INSTEAD
    // of its source, so caching half a program would hide the errors above on
    // every later run.
    out.cache_file = cache::cache_path(path);

    // AND A PROGRAM THAT DID NOT RESOLVE IS NOT CACHED EITHER -- M19.6, and
    // PLAN named it as the open question this milestone closes. A `.satc` now
    // records a decision taken about the program's TYPES, so writing one for a
    // program whose types did not check would be caching a fold nothing
    // verified. The parse test above already had the same shape and the same
    // reason: a file that is read back INSTEAD of its source must not carry
    // anything the source would have been refused for.
    if (out.parsed.ok() && out.resolved.ok()) {
        writing.start(out.cache_file, out.satc);
        // "writing" AND NOT "wrote", because it has not happened yet and may
        // not: §5 says a failed write is silent, so the only honest thing to
        // report here is that one was started. The proof that it finished is
        // the NEXT run saying "read".
        fprintf(stderr, "satl: writing %s\n",
                out.cache_file.empty() ? "nothing -- this process has no HOME"
                                       : out.cache_file.c_str());
    }
    return out;
}

int satc_command(const std::string &path)
{
    // A RUN'S NAMES END WITH THE RUN, which is why this is a local and not a
    // global: words_runtime.hpp makes the point that M22 runs many programs
    // in one process and each needs its own numbering.
    words::Words words;
    cache::Save writing;

    const Reading read = read_program(path, words, writing);
    if (!read.ok)
        return EXIT_USAGE;

    // PRINTED FROM THE TREE AND NOT COPIED FROM THE FILE, which makes this
    // command a check rather than a `cat`. What comes back out is what the
    // writer makes of the tree the READER built, so a reader that lost
    // something prints a file that differs from the one on disk -- and two
    // runs of --satc are then two different answers. Copying the bytes
    // through would have shown nothing at all.
    fputs(read.satc.c_str(), stdout);

    // THE EXIT STATUS M3 LEFT OPEN AND THIS WAS THE THIRD ARM TO WANT. It is
    // EXIT_MALFORMED now; programs/opening.hpp carries why it is 1 and why no
    // arm was allowed to invent it.
    return read.parsed.ok() ? EXIT_FINE : EXIT_MALFORMED;
}

} // namespace satellite
