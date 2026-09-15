// satl -- the interpreter.
//
// THIS BINARY LINKS NO GUI. The window lives in satl-term (M1.5, built), and
// satellite.window.new() will reach a dlopen'd library (M24). The split is
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

#include "machine_limits/dump.hpp"
#include "machine_limits/watchdog.hpp"
#include "programs/cache_command.hpp"
#include "programs/check_command.hpp"
#include "programs/dump_commands.hpp"
#include "programs/evaluate_commands.hpp"
#include "programs/file_commands.hpp"
#include "programs/arms.hpp"
#include "programs/limits_command.hpp"
#include "programs/number_command.hpp"
#include "programs/opening.hpp"
#include "programs/resolve_command.hpp"
#include "programs/run_command.hpp"
#include "programs/window_handover.hpp"
#include "satellite_prompt/prompt.hpp"
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
// is a bug report waiting to be filed.
//
// ONE CALLER LEFT AFTER M10, AND IT PASSES NO FILE. `--repl` was M22's and named
// no path, so the block below was unreached -- kept because the next arm to
// arrive with a file behind it will want it, and because deleting the half of a
// function that states a rule is how the rule gets rediscovered. Running a file
// was this function's reason for existing from M1 to M10 and is now
// programs/run_command.cpp's.
//
// AND M22 TOOK THE LAST CALLER, ON 2026-09-07. The prompt landed, `--repl`
// dispatches to it, and this function now has none at all -- which -Wall says
// out loud, so the attribute is here to keep the build clean rather than to
// hide anything. THE AUTHOR'S CALL IS WHETHER IT GOES: the sentence above is an
// argument for keeping it and it is still true (M24's windows and M25's second
// file are both arms with a file behind them), and the counter-argument is this
// tree's own rule about code nothing reaches -- satellite_console/console.cpp
// removed a guard "against an impossible state" for exactly that reason.
// MILESTONES/M22.md §6 carries it as open rather than settling it here, because
// deleting an argument somebody wrote down is not a warning fix.
[[maybe_unused]] int not_yet(const std::string &what, const std::string &file,
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
// launching from a menu can see it. But the ones below produce an answer on
// stdout and stop. Nobody double-clicks an icon to be told a version number,
// and the answer has to reach a pipe, a file and a variable, which a window
// cannot do.
// 080-report.sh in the installer already states the rule for the other binary
// -- "window.cpp answers --version before it touches GTK, on purpose ... so
// this check runs on a headless box and in a container" -- and this is that
// same rule, arriving late on the side that needed it more.
//
// --watchdog IS THE ONE MEMBER THAT DOES NOT ONLY PRINT, AND IT BELONGS HERE
// ANYWAY. What the class is really about is where an answer has to ARRIVE: every
// flag below produces something for a shell, and a window that opens and closes
// with the process delivers none of it. `satl --watchdog cfg.ini > run.log` in a
// CI job is the installer failure above exactly -- no controlling terminal,
// output redirected to a file nothing is reading, refusal THREE silent.
bool only_prints_and_exits(const char *arg)
{
    const std::string flag(arg);
    return flag == "--no-window" || flag == "--version" || flag == "-V" ||
           flag == "--help" || flag == "-h" || flag == "--words" ||
           flag == "--tokens" || flag == "--unparse" || flag == "--satc" ||
           flag == "--check" || flag == "--errors" || flag == "--limits" ||
           flag == "--arms" ||
           flag == "--resolve" || flag == "--number" ||
           flag == "--compile" || flag == "--call" ||
           flag == "--watchdog";
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

    // THE LIMITS, THE POOL AND THE WATCHDOG, BEFORE ANY ARM RUNS AND AFTER THE
    // ARGUMENTS ARE ASSEMBLED -- because a config file can be named on the
    // command line and the pool is a property of the process rather than of
    // what the process was asked to do. programs/limits_command.hpp carries
    // both halves of that, including why a malformed config stops even
    // `satl --version`.
    if (const int status = satellite::start_limits(args);
        status != satellite::EXIT_FINE)
        return status;

    // NOTHING TO DO IS NOT AN ERROR. satl started with no arguments shows the
    // opening information, which is what says how to run a file.
    //
    // At M22 this arm gains the prompt, and the banner it prints first is this
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

    // THE ARMS ARE IN TWO FILES AND THIS IS THE SWITCH, which is the seam
    // MILESTONES/M6.md §6.1 named and declined and M7 had to take: "the arms
    // that DUMP a registry and the arms that read a FILE are two subjects", and
    // this file was 427 lines with a fourteenth arm about to be added to it.
    // programs/dump_commands.hpp says what the two subjects actually are.
    //
    // EVERY ONE OF THEM IS A CONSUMER OF THE MILESTONE THAT BUILT IT, which is
    // PLAN M2's rule and the reason this switch keeps growing: --words is M2's,
    // --tokens M3's, --unparse M4's, --satc M4.5's, --check M5's, --limits and
    // --watchdog M6's, and --resolve is M7's. The first satellite shipped three
    // commits where its registry had no reader at all and four defects
    // accumulated in that window.
    if (first == "--words")
        return satellite::words_command(args.size() < 3 ? satellite::kNoKey : args[2]);

    if (first == "--errors")
        return satellite::errors_command(args.size() < 3 ? satellite::kNoKey : args[2]);

    // AND EVERY FILE ARM NEEDS AN OPERAND, so a missing one is a real usage
    // error rather than a milestone that has not landed. `satl --unparse` with
    // nothing after it is wrong at M10 too.
    if (first == "--tokens") {
        if (args.size() < 3)
            return usage_error("--tokens needs a file after it");
        return satellite::tokens_command(args[2]);
    }

    if (first == "--unparse") {
        if (args.size() < 3)
            return usage_error("--unparse needs a file after it");
        return satellite::unparse_command(args[2]);
    }

    if (first == "--satc") {
        if (args.size() < 3)
            return usage_error("--satc needs a file after it");
        return satellite::satc_command(args[2]);
    }

    if (first == "--check") {
        if (args.size() < 3)
            return usage_error("--check needs a file after it");
        return satellite::check_command(args[2]);
    }

    if (first == "--resolve") {
        if (args.size() < 3)
            return usage_error("--resolve needs a file after it");
        return satellite::resolve_command(args[2]);
    }

    // M6's CONSUMER, AND THE ONLY WAY TO SEE THAT MILESTONE AT ALL -- nothing
    // in the language reads a limit until M8 and nothing runs until M10, so
    // without this §4.5 is a pool nobody can observe and a watchdog that has
    // not fired. The file it was given, if it was given one, was already read
    // by start_limits() above.
    if (first == "--limits") {
        fputs(satellite::limits::limits_text().c_str(), stdout);
        return satellite::EXIT_FINE;
    }

    // WHICH ARM INSTALLS WHAT -- ERROR_HANDLING.md §4.3, and it answers the
    // question M23's omission made somebody ask for three weeks: "the language
    // has this, so why does THIS process say it does not?"
    if (first == "--arms") {
        fputs(satellite::arms::report().c_str(), stdout);

        std::string complaints;
        if (!satellite::arms::audit(&complaints)) {
            fputs("\nthe audit is unhappy:\n", stderr);
            fputs(complaints.c_str(), stderr);
            return satellite::EXIT_USAGE;
        }
        return satellite::EXIT_FINE;
    }

    // THE ONE ARM THAT DOES NOT FINISH. machine_limits/watchdog.hpp argues why
    // a flag exists for this: M6's done-when asks for a demonstration in which
    // the watchdog kills the process, nothing satl does today lasts a second,
    // and a watchdog that never fires is indistinguishable from no watchdog.
    if (first == "--watchdog")
        return satellite::limits::hold_for_the_watchdog();

    // M9's TWO CONSUMERS. `--compile` prints the closure tree the way
    // `--resolve` prints frames; `--call` runs one capsule and prints its
    // answer. Neither runs a PROGRAM: `--call` has no console behind it and no
    // `satellite.main` in front of it, which is still the difference between
    // them and the two arms at the bottom of this switch.
    if (first == "--compile") {
        if (args.size() < 3)
            return usage_error("--compile needs a file after it");
        return satellite::compile_command(args[2]);
    }

    if (first == "--call") {
        if (args.size() < 4)
            return usage_error("--call needs a file and the name of a capsule "
                               "in it, and then one number per parameter");
        return satellite::call_command(args);
    }

    // M8's CONSUMER. Three operands and not an expression -- number_command.hpp
    // says why that is the design rather than a shortcut, and the short version
    // is that DESIGN §6 is already the grammar and a second one here would be a
    // second place expression syntax is decided.
    if (first == "--number") {
        if (args.size() < 5)
            return usage_error("--number needs three things after it: a number, "
                               "one of + - * / %, and another number");
        return satellite::number_command(args);
    }

    // M22's CONSUMER. This arm answered `not_yet` from M1 until the prompt
    // landed, which is what let M1.5 be demonstrated before there was anything
    // to demonstrate it with -- PLAN M22: "`satl --repl` answers 'not built
    // yet' and exits EXIT_NOT_YET, so the window stays up with the explanation
    // on it."
    if (first == "--repl")
        return satellite::prompt::run_prompt();

    // M10's CONSUMER, AND THE ONE THIS BINARY HAS BEEN POINTING AT SINCE M1.
    // `--run` takes an operand, so a missing one is a usage error rather than a
    // milestone that has not landed.
    //
    // TWO SPELLINGS AND ONE ARM. `satl file.satl` is what a person types and
    // `satl --run file.satl` is what a script types when the filename might
    // begin with a dash; they differ only in where the path sits, which is why
    // run_command takes an index rather than a path and a vector.
    if (first == "--run") {
        if (args.size() < 3)
            return usage_error("--run needs a file after it");
        return satellite::run_command(args, 2);
    }

    // A bare word that is not a flag is a filename. Checked LAST of the arms
    // that can match a word, which is what the ordering above is for.
    if (!first.empty() && first[0] != '-')
        return satellite::run_command(args, 1);

    return usage_error("unknown option " + first);
}
