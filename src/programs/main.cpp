// satl -- the interpreter.
//
// THIS BINARY LINKS NO GUI. The window lives in satl-term (M11.A, built), and
// satellite.window.new() will reach a dlopen'd library (M13). The split is
// measured rather than tidy-minded: `ldd` on this satl lists 6 shared objects
// and on the first satellite's satl-term lists 79, and the dynamic linker loads
// every one of them before main() on every run. The first satellite measured
// what that costs at 25.9 ms with the link against 2.5 ms without.
//
// See make_support/040-sources.mk for this build's own startup numbers and for
// why the "119 shared objects" its predecessor's source claims is not repeated
// here.
//
// At milestone 1 there is no interpreter behind any of this. What there is: a
// binary that says what it is, says how to run a file, and refuses to pretend
// about the parts that have not been built. See PLAN_ONE.md, M1.

#include "abstract_syntax_tree/unparse.hpp"
#include "lexical_analyzer/dump.hpp"
#include "parser/parser.hpp"
#include "programs/opening.hpp"
#include "programs/source_file.hpp"
#include "programs/window_handover.hpp"
#include "satellite_words/dump.hpp"
#include "system_facts/version.hpp"

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include <unistd.h>

namespace {

// Whether a path names something this process can read.
//
// R_OK and not F_OK, because "it is there" and "I may open it" are different
// answers and only the second one is useful to somebody about to be told their
// file cannot be run. access() rather than a stat of the mode bits, because
// access() asks the kernel the question with THIS process's real ids instead of
// reconstructing the answer from permissions and getting it wrong on an ACL.
bool readable(const std::string &path)
{
    return access(path.c_str(), R_OK) == 0;
}

// What satl says when the command line was right and the milestone is not here.
//
// SEPARATE FROM A USAGE ERROR, and that separation is the whole point of the
// function. `satl hello.satl` is a correct sentence; answering it with a usage
// dump teaches the user that they typed something wrong, which is false and
// sends them to reread a specification that already agrees with them.
//
// The file is checked even though nothing will be run with it, because the two
// failures a user is about to have are different and they should not have to
// guess which one they are in. A misspelled path reported as "not built yet"
// is a bug report waiting to be filed at M8.
int not_yet(const std::string &what, const std::string &file,
            const char *milestone)
{
    fprintf(stderr, "satl: %s is not built yet -- it lands at %s.\n",
            what.c_str(), milestone);

    if (!file.empty()) {
        if (readable(file))
            fprintf(stderr, "      %s was found and nothing was done with it.\n",
                    file.c_str());
        else
            fprintf(stderr, "      %s could not be read either, so check the "
                            "path before %s arrives.\n",
                    file.c_str(), milestone);
    }
    return satellite::EXIT_NOT_YET;
}

// A FLAG THAT ONLY PRINTS AND EXITS, which is the one kind of argument that
// must never open a window.
//
// --no-window was the first of these and was special-cased at the handover call
// below. It turned out not to be special: it is one member of a class, and the
// rest of the class was found the hard way on 2026-08-28, when `./install.sh`
// with its output redirected died on
//     satl-term: unknown option --version
// The installer verifies what it installed by running `satl --version`, and
// window_handover.cpp's six refusals do not cover that case -- refusal TWO is
// a controlling terminal, which a package build, a cron job, an ssh command
// without a tty and a CI runner all lack, and refusal THREE is something
// READING our output, which a plain `> file` is not. So satl handed a
// machine-readable query to a GUI binary that has no such flag, and exited 2.
//
// The handover's own comment says it runs "BEFORE ANY ARGUMENT IS READ, AND
// THAT IS THE POINT", and that is still right for everything that RUNS
// something: those can fail, and their diagnosis has to be somewhere a person
// launching from a menu can see it. But these five produce an answer on stdout
// and stop. Nobody double-clicks an icon to be told a version number, and the
// answer has to reach a pipe, a file and a variable, which a window cannot do.
// 080-report.sh in the installer already states the rule for the other binary
// -- "window.cpp answers --version before it touches GTK, on purpose ... so
// this check runs on a headless box and in a container" -- and this is that
// same rule, arriving late on the side that needed it more.
bool only_prints_and_exits(const char *arg)
{
    const std::string flag(arg);
    return flag == "--no-window" || flag == "--version" || flag == "-V" ||
           flag == "--help" || flag == "-h" || flag == "--words" ||
           flag == "--tokens" || flag == "--unparse";
}

// A usage failure: the command line did not name something satl can do.
//
// Usage goes to STDERR here and to stdout in the --help arm, and that is not an
// inconsistency. `satl --help | less` is someone reading the list on purpose
// and it belongs on stdout; a complaint about a bad command line belongs on
// stderr, where it survives a pipe that was set up for output that will now
// never come.
int usage_error(const std::string &complaint)
{
    fprintf(stderr, "satl: %s\n", complaint.c_str());
    fputs(satellite::usage_text().c_str(), stderr);
    return satellite::EXIT_USAGE;
}

} // namespace

int main(int argc, char **argv)
{
    // BEFORE ANY ARGUMENT IS READ, AND THAT IS THE POINT. If this process was
    // started with no console -- a file manager, a .desktop entry, a desktop
    // menu -- then every line below prints into nothing, INCLUDING the usage
    // text and the error messages. Handing over first means the person sees
    // whatever satl was going to say, rather than seeing the window only in the
    // cases somebody remembered to route through it.
    //
    // It returns here on a terminal, in a pipeline, with output redirected to
    // something that is READING it, with no display, without a satl-term to
    // hand to, or when SATL_NO_WINDOW is set -- six refusals, all named in
    // window_handover.cpp. The seventh is here rather than there, because it is
    // about flags and flags are this function's business:
    // only_prints_and_exits() names the arguments that answer a question and
    // stop, and those are answered where they were asked. See its comment for
    // the installer failure that found the other four.
    if (argc < 2 || !only_prints_and_exits(argv[1]))
        satellite::hand_over_to_the_window(argv);

    // --no-window IS CONSUMED HERE AND EXISTS NOWHERE BELOW. Filtered out of the
    // arguments rather than skipped over at each use: the arms further down
    // index args[2] for their operands, so carrying the flag would make every
    // one of them off by one in exactly the case nobody tests. `satl
    // --no-window` alone is therefore `satl` alone, which is what it reads as.
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        if (i == 1 && std::string(argv[i]) == "--no-window")
            continue;
        args.emplace_back(argv[i]);
    }

    // NOTHING TO DO IS NOT AN ERROR. satl started with no arguments shows the
    // opening information, which is what says how to run a file.
    //
    // At M11.B this arm gains the prompt, and the banner it prints first is this
    // same opening_text() -- which is why that function returns a string rather
    // than printing one.
    if (args.size() == 1) {
        fputs(satellite::opening_text().c_str(), stdout);
        return satellite::EXIT_FINE;
    }

    const std::string &first = args[1];

    // BEFORE the bare-filename arm, so that a file which happens to be called
    // --version cannot shadow the flag. Exit 0: asking a program what it is, is
    // not an error, and a packaging script that greps this is entitled to a
    // zero.
    if (first == "--version" || first == "-V") {
        fputs(satellite::version_text("satl").c_str(), stdout);
        return satellite::EXIT_FINE;
    }

    // Also before the bare-filename arm, and also exit 0. The first satellite
    // exited 2 here; see the note on ExitStatus for why that changed.
    if (first == "-h" || first == "--help") {
        fputs(satellite::usage_text().c_str(), stdout);
        return satellite::EXIT_FINE;
    }

    // THE REGISTRY'S CONSUMER, AND THE REASON M2 HAS ONE. PLAN M2 asks for
    // this by name because the first satellite shipped three commits where its
    // word registry had no reader at all, and four defects accumulated behind a
    // guarantee nothing was checking.
    //
    // NOT AN ARM THAT SAYS "not built yet", which every other unfinished thing
    // here does. The numbering IS built, so this answers -- and what it prints
    // ends by saying that almost nothing it lists runs yet, because a dump of
    // 254 paths with no such line would read as a feature list.
    if (first == "--words") {
        if (args.size() < 3) {
            fputs(satellite::words::dump_text().c_str(), stdout);
            return satellite::EXIT_FINE;
        }
        // A path that resolves is an answer and goes to stdout; a path the
        // language does not have is a command line that named something satl
        // cannot do, so it goes to stderr with the same status a bad option
        // gets. That is the split the --help arm already makes, applied to an
        // operand instead of to a flag.
        bool resolved = false;
        const std::string report = satellite::words::walk_text(args[2], resolved);
        fputs(report.c_str(), resolved ? stdout : stderr);
        return resolved ? satellite::EXIT_FINE : satellite::EXIT_USAGE;
    }

    // THE LEXER'S CONSUMER, AND THE REASON M3 HAS ONE -- the same rule the
    // --words arm above records, applied one milestone on. M3 produces a token
    // stream that no parser reads until M4, so this is the only way to see what
    // it decided.
    //
    // NOT AN ARM THAT SAYS "not built yet". The lexer IS built, so it answers.
    if (first == "--tokens") {
        if (args.size() < 3)
            return usage_error("--tokens needs a file after it");

        // A file that cannot be READ is a different failure from a file that
        // cannot be LEXED, and they get different words and different streams.
        // The first is the user's command line; the second is their program.
        std::string source;
        if (!satellite::read_file(args[2], source)) {
            fprintf(stderr, "satl: %s could not be read.\n", args[2].c_str());
            return satellite::EXIT_USAGE;
        }

        // THE DUMP GOES TO STDOUT EVEN WHEN THE PROGRAM IS MALFORMED, and the
        // --words arm above is why that has to be said. There the operand IS
        // the command line, so a path the language does not have is a usage
        // failure and belongs on stderr. Here the operand is a FILE, and a
        // program with a bad token in it is not a bad command line -- the
        // question asked was "what does the lexer make of this", the answer is
        // the stream including its Error token, and that answer is what the
        // person asked for.
        //
        // Copying the split from --words was the first version of this arm and
        // it was wrong in a way a person would not notice and a script would:
        // `satl --tokens bad.satl > tokens.txt` produced an EMPTY tokens.txt
        // with the whole dump on stderr. Fixed 2026-08-30.
        //
        // THE EXIT STATUS IS STILL WRONG AND IS M5'S TO FIX. Non-zero is right
        // -- a script has to be able to tell -- but EXIT_USAGE is the only
        // non-zero code that exists here and its own definition says "the
        // command line did not name something satl can do", which this is not.
        // What is missing is a code for "the program you gave me is malformed",
        // and inventing one is the error reporter's decision to make with all
        // its codes in view rather than this arm's to guess at. MILESTONES/M3.md
        // §6 carries it as open.
        bool clean = false;
        const std::string report = satellite::tokens_text(source, clean);
        fputs(report.c_str(), stdout);
        return clean ? satellite::EXIT_FINE : satellite::EXIT_USAGE;
    }

    // THE PARSER'S CONSUMER, AND THE REASON M4 HAS ONE -- the same rule the two
    // arms above record, one milestone on. And it is the strongest of the
    // three: --words prints a table and --tokens prints a list, while this
    // prints a SATELLITE PROGRAM, which satl can read back. PLAN M4 states the
    // milestone in this command -- "satl --unparse file.satl round-trips, which
    // is how we know the parser is right before anything can run" -- because
    // nothing runs until M8 and a tree is otherwise only visible to whoever
    // wrote the code that built it.
    //
    // WHAT COMES BACK IS NOT THE FILE. Comments are gone (DESIGN §5.6 discards
    // them), blank lines were never tokens, and brackets a program wrote around
    // a single value are gone too. What is guaranteed is that printing this
    // output and parsing it again gives the same text -- a fixpoint, which
    // abstract_syntax_tree/unparse.hpp argues is the strongest statement
    // available and a real one.
    if (first == "--unparse") {
        if (args.size() < 3)
            return usage_error("--unparse needs a file after it");

        std::string source;
        if (!satellite::read_file(args[2], source)) {
            fprintf(stderr, "satl: %s could not be read.\n", args[2].c_str());
            return satellite::EXIT_USAGE;
        }

        // A RUN'S NAMES END WITH THE RUN, which is why this is a local and not
        // a global: words_runtime.hpp makes the point that M11.B runs many
        // programs in one process and each needs its own numbering.
        satellite::words::Words words;
        const satellite::Parse parsed = satellite::parse(source, words);

        // THE ANSWER GOES TO STDOUT AND THE COMPLAINTS TO STDERR, which is the
        // split --tokens got wrong on its first day and had to be corrected:
        // `satl --unparse f.satl > out.satl` must write the program, and a
        // person watching the terminal must still see what did not parse. Both
        // are printed for a partly-parsed file, because what was understood is
        // an answer even when the whole file was not.
        for (const satellite::ParseError &problem : parsed.errors) {
            const satellite::Token &at = parsed.ast.token(problem.token);
            fprintf(stderr, "satl: %s:%u: %s\n", args[2].c_str(), at.line,
                    problem.reason.c_str());
        }
        fputs(satellite::unparse(parsed.ast).c_str(), stdout);

        // THE EXIT STATUS IS THE ONE M3 LEFT OPEN AND M5 STILL OWNS. EXIT_USAGE
        // is the only non-zero code that exists and its own definition is "the
        // command line did not name something satl can do", which a malformed
        // program is not. MILESTONES/M3.md §6 carries it; this arm is the
        // second one to need the code that is missing, which is worth more to
        // M5 than one arm was.
        return parsed.ok() ? satellite::EXIT_FINE : satellite::EXIT_USAGE;
    }

    if (first == "--repl")
        return not_yet("the prompt", std::string(), "M11.B");

    // --run takes an operand, so a missing one is a real usage error rather
    // than a milestone that has not landed: `satl --run` with nothing after it
    // is wrong at M8 too.
    if (first == "--run") {
        if (args.size() < 3)
            return usage_error("--run needs a file after it");
        return not_yet("running a file", args[2], "M8");
    }

    // A bare word that is not a flag is a filename. Checked LAST of the arms
    // that can match a word, which is what the ordering above is for.
    if (!first.empty() && first[0] != '-')
        return not_yet("running a file", first, "M8");

    return usage_error("unknown option " + first);
}
