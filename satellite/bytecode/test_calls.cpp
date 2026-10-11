// satellite/bytecode/test_calls.cpp -- satellite.test's words. test_calls.hpp says what each one
// runs and prints, and why they have no library.

#include "test_calls.hpp"

#include "word_codes.hpp"
#include "../machine/console_lock.hpp"
#include "../satellite_test/test_programs.hpp"
#include "../satellite_test/test_run.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <set>

namespace satellite004 {

namespace {

using token::Code;

// THE NINE ROWS ARE ONE RUN OF CODES: words_004.tsv appends `1 33` and then each word and its `()`,
// so a word's code is its row and the run is a range -- full, full(), speed, speed(), loss, loss(),
// all, all() after satellite.test itself.
constexpr Code kTest = word::fixed_code<1, 33>;
constexpr Code kAllCalled = word::fixed_code<1, 33, 4, 0>;
static_assert(kAllCalled - kTest == 8, "satellite.test's nine rows are one run of codes");
static_assert(word::fixed_code<1, 33, 1> == kTest + 1 && word::fixed_code<1, 33, 2, 0> == kTest + 4 &&
                  word::fixed_code<1, 33, 3> == kTest + 5,
              "1 33 n is kTest + 2n - 1, and its () the row after");

enum class Which { test, full, speed, loss, all };

Which which_of(Code code)
{
    return static_cast<Which>((static_cast<unsigned>(code - kTest) + 1) / 2);
}

const char *kRecommended = "it is recommended that you run satellite.test.all() to run every test";
const char *kAboutHere = " -- about 60 on the author's machine";

// full() RUNS ITS SET THIS MANY TIMES, so it lasts about a minute on the author's machine as the
// other two do: one round of its 149 programs took 10.56 s there (build 0042, 2026-10-06, about
// half of it satellite.random's spin, which is by design), so six are ~63 s and five ~53. A quick
// run (SATL_TEST_PASSES) runs it once.
constexpr long long kFullRounds = 6;

// HOW LONG ONE PROGRAM MAY RUN before it is stopped and said to have hung -- many times what each
// takes here (the slowest full() program ~6 s, a speed section ~2.5 s, the loss chain ~60 s and up
// to ~140 s on a busy disk), so only a hang reaches it, and a hang is said rather than waited on.
constexpr double kFullLimit = 60.0;
constexpr double kSpeedLimit = 120.0;
constexpr double kLossLimit = 900.0;

void say(const std::string &line)
{
    ConsoleHold hold;
    std::cout << line << '\n' << std::flush;
}

// A POINT, NEVER A COMMA: to_chars reads no locale, and satl's own console sets one from the
// person's environment (GTK's init) -- printf would print 61,42 under de_DE.
std::string seconds_text(double seconds, int places = 2)
{
    char text[64];
    const std::to_chars_result made = std::to_chars(text, text + sizeof text, seconds, std::chars_format::fixed, places);
    return made.ec == std::errc() ? std::string(text, made.ptr) : std::string("?");
}

// A QUICK RUN SAYS SO, whichever test it is: its programs ran at SATL_TEST_PASSES passes, so its
// seconds and its answers are not a full test's.
void say_if_quick(long long passes)
{
    if (passes > 0)
        say("a quick run: every program at " + std::to_string(passes) + (passes == 1 ? " pass" : " passes") +
            " (SATL_TEST_PASSES)");
}

// THE FIRST REPORT A RUN WROTE TO satellite.log, in one line: its S-code -- a report's own line
// (S110: ...) or a notice's ([satellite] S011 ...) -- or else the entry's first line of words.
std::string first_logged(const std::string &logged)
{
    const auto code_at = [](const std::string &line, std::size_t at) {
        return at + 4 < line.size() && line[at] == 'S' && std::isdigit(static_cast<unsigned char>(line[at + 1])) &&
               std::isdigit(static_cast<unsigned char>(line[at + 2])) &&
               std::isdigit(static_cast<unsigned char>(line[at + 3]));
    };
    std::size_t at = 0;
    std::string first;
    while (at < logged.size()) {
        const std::size_t end = std::min(logged.find('\n', at), logged.size());
        const std::string line = logged.substr(at, end - at);
        at = end + 1;
        if (code_at(line, 0))
            return line;
        const std::size_t notice = line.find("] S");
        if (notice != std::string::npos && code_at(line, notice + 2))
            return line.substr(notice + 2);
        // THE HEADING (a date, a pid, the program) and the markers say nothing of what went wrong.
        if (first.empty() && !line.empty() && line != "[entry]" && line != "[/entry]" &&
            !std::isdigit(static_cast<unsigned char>(line[0])))
            first = line;
    }
    return first.empty() ? std::string("an entry") : first;
}

// A REPORT AS ONE LINE OF WORDS: satl wraps a report at 80 columns (critical_report.hpp), so a
// phrase can fall across a line's end -- its newlines become spaces, and runs of spaces one.
std::string flattened(const std::string &text)
{
    std::string flat;
    for (const char c : text) {
        const char put = c == '\n' ? ' ' : c;
        if (put == ' ' && (flat.empty() || flat.back() == ' '))
            continue;
        flat += put;
    }
    return flat;
}

// WHAT A PROGRAM'S REPORT SAYS, in one line: its S-code line and the sentence under it, up to the
// report's first empty line -- or the last line of its errors when it has no report.
std::string report_line(const std::string &err)
{
    std::size_t at = 0;
    while (at < err.size()) {
        const std::size_t end = std::min(err.find('\n', at), err.size());
        const std::string line = err.substr(at, end - at);
        if (line.size() > 5 && line[0] == 'S' && line[4] == ':' && std::isdigit(static_cast<unsigned char>(line[1]))) {
            std::size_t next = end + 1, sentence_end = next;
            while (sentence_end < err.size()) {
                const std::size_t stop = std::min(err.find('\n', sentence_end), err.size());
                if (stop == sentence_end)
                    break;
                sentence_end = stop + 1;
            }
            return next < err.size() ? line + " -- " + flattened(err.substr(next, sentence_end - next)) : line;
        }
        at = end + 1;
    }
    std::string last;
    at = 0;
    while (at < err.size()) {
        const std::size_t end = std::min(err.find('\n', at), err.size());
        if (end > at)
            last = err.substr(at, end - at);
        at = end + 1;
    }
    return last.empty() ? std::string("no report") : last;
}

std::vector<std::string> lines_of(const std::string &text)
{
    std::vector<std::string> lines;
    std::size_t at = 0;
    while (at < text.size()) {
        const std::size_t end = std::min(text.find('\n', at), text.size());
        lines.push_back(text.substr(at, end - at));
        at = end + 1;
    }
    return lines;
}

// THE PASSES A PROGRAM IS WRITTEN WITH: its twin is given the same.
long long passes_written(const char *text)
{
    static const std::string line = "\n    satellite.variable.number passes = ";
    const std::string program(text);
    const std::size_t at = program.find(line);
    return at == std::string::npos ? 1 : std::atoll(program.c_str() + at + line.size());
}

struct Outcome {
    bool right = true;
    bool stopped = false;
    double seconds = 0.0;
    std::string summary;   // all()'s last line: a few words for this test
};

// THE LINES AN ANSWERS PROGRAM PRINTS WHEN EVERYTHING IS RIGHT: one a check.
long long checks_in(const TestProgram &program)
{
    if (program.kind == TestKind::refused)
        return 1;
    if (program.kind != TestKind::answers || program.expected == nullptr)
        return 0;
    return static_cast<long long>(lines_of(program.expected).size());
}

// ONE WRONG THING, said once however many rounds found it.
struct Wrongs {
    std::vector<std::string> lines;
    std::set<std::string> seen;
    void add(const std::string &line)
    {
        if (seen.insert(line).second)
            lines.push_back(line);
    }
};

// AN ANSWERS PROGRAM IS RIGHT when it ran to the end and printed its lines exactly -- each one it
// should print is a check, and a line it should not is said as it came (a WRONG line, or an `ok`
// line gone astray). A REFUSED ONE when satl stopped it with its code, its report holds the phrase,
// and it printed nothing.
long long judge(const TestProgram &program, const TestRun &run, Wrongs &wrongs)
{
    const std::string name(program.name);
    if (!run.started) {
        wrongs.add(name + ": " + run.why);
        return 0;
    }
    if (run.timed_out) {
        wrongs.add(name + " was still running after " + seconds_text(kFullLimit, 0) + " seconds, so it was stopped");
        return 0;
    }
    if (program.kind == TestKind::refused) {
        if (run.code == program.exit_code && flattened(run.err).find(program.phrase) != std::string::npos &&
            run.out.empty())
            return 1;
        if (run.code == program.exit_code && flattened(run.err).find(program.phrase) != std::string::npos)
            wrongs.add(name + " was refused as it should be, but printed first: " + lines_of(run.out).front());
        else
            wrongs.add(name + " should be refused with " + std::to_string(program.exit_code) + " (\"" +
                       program.phrase + "\"), and " +
                       (run.code == 0 ? std::string("it ran")
                                      : "it stopped with " + std::to_string(run.code) + ": " + report_line(run.err)));
        return 0;
    }
    // WHAT A RIGHT RUN WRITES TO satellite.log: nothing -- a warning goes there and nowhere else.
    if (!run.logged.empty())
        wrongs.add(name + " wrote to satellite.log: " + first_logged(run.logged));
    // EXACTLY, IN ORDER: the right lines in another order are not the right run. Then each line it
    // should not have printed is said, and each it did not print.
    const std::vector<std::string> printed = lines_of(run.out), wanted = lines_of(program.expected);
    if (run.code == 0 && run.logged.empty() && printed == wanted)
        return static_cast<long long>(wanted.size());
    std::multiset<std::string> owed(wanted.begin(), wanted.end());
    if (run.code == 0 && std::multiset<std::string>(printed.begin(), printed.end()) == owed) {
        long long in_place = 0;
        for (std::size_t n = 0; n < wanted.size() && n < printed.size(); ++n)
            in_place += printed[n] == wanted[n];
        wrongs.add(name + " printed every line it should, in another order");
        return run.logged.empty() ? in_place : 0;
    }
    long long right = 0;
    for (const std::string &line : printed) {
        const auto found = owed.find(line);
        if (found != owed.end()) {
            owed.erase(found);
            ++right;
        } else {
            wrongs.add(name + ": " + line);
        }
    }
    // A PROGRAM THAT STOPPED is one line, and what it never reached one more -- not a line each.
    if (run.code != 0) {
        wrongs.add(name + " stopped with " + std::to_string(run.code) + ": " + report_line(run.err));
        if (!owed.empty())
            wrongs.add(name + " never printed " + std::to_string(owed.size()) + " of its lines");
        return right;
    }
    for (const std::string &line : owed)
        wrongs.add(name + " did not print: " + line);
    return run.logged.empty() ? right : 0;
}

// WHAT A TEST SAYS WHEN IT WAS STOPPED: a Ctrl-C, its thread's stop(), or its program's end.
const char *kStopped = "stopped before it finished";

Outcome run_full(bool recommend)
{
    Outcome outcome;
    const long long passes = test_passes_override();
    const long long rounds = passes > 0 ? 1 : kFullRounds;
    long long checks = 0;
    for (std::size_t n = 0; n < kTestProgramCount; ++n)
        checks += checks_in(kTestPrograms[n]);
    say("satellite.test.full(): every capability of satl -- " + std::to_string(checks) + " checks, run " +
        std::to_string(rounds) + (rounds == 1 ? " time" : " times"));
    say_if_quick(passes);
    Wrongs wrongs;
    long long right = 0;
    const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
    {
        TestPlace place;
        for (long long round = 0; round < rounds && !outcome.stopped; ++round) {
            for (std::size_t n = 0; n < kTestProgramCount; ++n) {
                const TestProgram &program = kTestPrograms[n];
                if (program.kind != TestKind::answers && program.kind != TestKind::refused)
                    continue;
                if (test_stop_wanted()) {
                    outcome.stopped = true;
                    break;
                }
                const TestRun run = place.run(program, passes, kFullLimit);
                if (run.interrupted) {
                    outcome.stopped = true;
                    break;
                }
                right += judge(program, run, wrongs);
            }
        }
    }
    outcome.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
    if (outcome.stopped) {
        say(kStopped);
        outcome.right = false;
        outcome.summary = "stopped";
    } else if (wrongs.lines.empty()) {
        say("all " + std::to_string(checks * rounds) + " answered right");
        outcome.summary = "all " + std::to_string(checks * rounds) + " right";
    } else {
        for (const std::string &line : wrongs.lines)
            say("    " + line);
        say(std::to_string(right) + " of " + std::to_string(checks * rounds) + " answered right");
        outcome.right = false;
        outcome.summary = std::to_string(wrongs.lines.size()) + " WRONG";
    }
    say(seconds_text(outcome.seconds) + " seconds" + kAboutHere);
    if (recommend)
        say(kRecommended);
    return outcome;
}

Outcome run_speed(bool recommend)
{
    Outcome outcome;
    const long long passes = test_passes_override();
    say("satellite.test.speed(): the same work in satl and in C++ built into satl");
    say_if_quick(passes);
    std::size_t width = 0;
    for (std::size_t n = 0; n < kTestProgramCount; ++n)
        if (kTestPrograms[n].kind == TestKind::speed)
            width = std::max(width, std::string(kTestPrograms[n].what).size());
    double satl_seconds = 0.0, cpp_seconds = 0.0;
    long long timed = 0;
    const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
    {
        TestPlace place;
        for (std::size_t n = 0; n < kTestProgramCount; ++n) {
            const TestProgram &program = kTestPrograms[n];
            if (program.kind != TestKind::speed || program.twin == nullptr)
                continue;
            if (test_stop_wanted()) {
                outcome.stopped = true;
                break;
            }
            const TestRun run = place.run(program, passes, kSpeedLimit);
            if (run.interrupted) {
                outcome.stopped = true;
                break;
            }
            std::string what(program.what);
            what.resize(width, ' ');
            if (!run.started || run.timed_out) {
                say(what + "   -- " +
                    (run.timed_out ? "still running after " + seconds_text(kSpeedLimit, 0) + " seconds, so it was stopped"
                                   : run.why));
                outcome.right = false;
                continue;
            }
            const std::chrono::steady_clock::time_point twin_began = std::chrono::steady_clock::now();
            const std::string twin_line = program.twin(passes > 0 ? passes : passes_written(program.text));
            const double twin_seconds =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - twin_began).count();
            satl_seconds += run.seconds;
            cpp_seconds += twin_seconds;
            ++timed;
            std::string line =
                what + "   satl " + seconds_text(run.seconds) + " s   C++ " + seconds_text(twin_seconds, 3) + " s";
            const std::vector<std::string> out = lines_of(run.out);
            const std::string satl_line = out.empty() ? std::string() : out.front();
            if (run.code != 0) {
                line += "   -- satl stopped with " + std::to_string(run.code) + ": " + report_line(run.err);
                outcome.right = false;
            } else if (out.size() != 1 || satl_line != twin_line) {
                line += "   -- NOT THE SAME ANSWER: satl " + (satl_line.empty() ? std::string("nothing") : satl_line) +
                        ", C++ " + twin_line;
                outcome.right = false;
            } else if (!run.logged.empty()) {
                line += "   -- it wrote to satellite.log: " + first_logged(run.logged);
                outcome.right = false;
            }
            say(line);
        }
    }
    outcome.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
    if (outcome.stopped) {
        say(kStopped);
        outcome.right = false;
        outcome.summary = "stopped";
    } else if (timed == 0) {
        say("no section ran, so there is nothing to compare");
        outcome.right = false;
        outcome.summary = "no section ran";
    } else {
        const double share = satl_seconds > 0.0 ? 100.0 * cpp_seconds / satl_seconds : 0.0;
        const std::string percent = seconds_text(share, share > 0.0 && share < 0.01 ? 4 : 2);
        say("satl " + seconds_text(satl_seconds) + " seconds, C++ " + seconds_text(cpp_seconds, 3) +
            " seconds -- satl runs at " + percent + "% of C++ speed");
        say("satl's " + seconds_text(satl_seconds) + " seconds" + kAboutHere);
        outcome.summary = "satl at " + percent + "% of C++" + (outcome.right ? "" : ", an answer WRONG");
    }
    if (recommend)
        say(kRecommended);
    return outcome;
}

Outcome run_loss(bool recommend)
{
    Outcome outcome;
    const long long passes = test_passes_override();
    say_if_quick(passes);
    say("these two numbers should match:");
    say("547311173");
    long long ran = 0;
    const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
    {
        TestPlace place;
        for (std::size_t n = 0; n < kTestProgramCount; ++n) {
            const TestProgram &program = kTestPrograms[n];
            if (program.kind != TestKind::loss)
                continue;
            if (test_stop_wanted()) {
                outcome.stopped = true;
                break;
            }
            const TestRun run = place.run(program, passes, kLossLimit);
            if (run.interrupted) {
                outcome.stopped = true;
                break;
            }
            ++ran;
            if (!run.started) {
                say(run.why);
                outcome.right = false;
            } else if (run.timed_out) {
                say("the loss test was still running after " + seconds_text(kLossLimit, 0) +
                    " seconds, so it was stopped");
                outcome.right = false;
            } else if (run.code != 0) {
                say("the loss test stopped with " + std::to_string(run.code) + ": " + report_line(run.err));
                outcome.right = false;
            } else {
                const std::vector<std::string> out = lines_of(run.out);
                for (const std::string &line : out)
                    say(line);
                if (out.size() != 1 || out.front() != "547311173")
                    outcome.right = false;
                if (!run.logged.empty()) {
                    say("and it wrote to satellite.log: " + first_logged(run.logged));
                    outcome.right = false;
                }
            }
        }
    }
    if (ran == 0 && !outcome.stopped) {
        say("no loss test ran");
        outcome.right = false;
    }
    outcome.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
    if (outcome.stopped) {
        say(kStopped);
        outcome.right = false;
        outcome.summary = "stopped";
    } else {
        outcome.summary = outcome.right ? "the two numbers match" : "the two numbers do NOT match";
    }
    say(seconds_text(outcome.seconds) + " seconds" + kAboutHere);
    if (recommend)
        say(kRecommended);
    return outcome;
}

Outcome run_all()
{
    Outcome outcome;
    const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
    const Outcome full = run_full(false);
    Outcome speed, loss;
    if (!full.stopped) {
        say("");
        speed = run_speed(false);
    }
    if (!full.stopped && !speed.stopped) {
        say("");
        loss = run_loss(false);
    }
    outcome.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
    outcome.stopped = full.stopped || speed.stopped || loss.stopped;
    outcome.right = full.right && speed.right && loss.right && !outcome.stopped;
    say("");
    if (outcome.stopped)
        say("satellite.test.all() was stopped after " + seconds_text(outcome.seconds) + " seconds");
    else
        say("satellite.test.all(): " + seconds_text(outcome.seconds) + " seconds -- full: " + full.summary +
            "; speed: " + speed.summary + "; loss: " + loss.summary);
    return outcome;
}

} // namespace

bool is_test_word(Code code)
{
    return code >= kTest && code <= kAllCalled;
}

signed long long int test_word_refused(Code code, std::size_t given, std::string &why)
{
    if (!is_test_word(code))
        return success;
    if (which_of(code) == Which::test) {
        why = "satellite.test is not a call on its own -- pick a test and call it: satellite.test.full(), "
              "satellite.test.speed(), satellite.test.loss() or satellite.test.all()";
        return satl_line_not_understood;
    }
    if (given != 0) {
        std::string spelled(word::spelling_of(code));
        spelled = spelled.substr(0, spelled.find('('));
        why = spelled + " takes nothing, and was given " + std::to_string(given) +
              (given == 1 ? " argument" : " arguments");
        return satl_line_not_understood;
    }
    return success;
}

Value call_test_word(Code code, const std::vector<Value> &arguments, ExpressionContext &context)
{
    std::string why;
    if (const signed long long int refused = test_word_refused(code, arguments.size(), why); refused != success) {
        context.refuse(refused, why);
        return Value();
    }
    // A CTRL-C FROM A FILE RUN ASKS THE TEST TO STOP, and ends satl once its place is gone (test_run.hpp).
    const CtrlCDuringATest ctrl_c;
    switch (which_of(code)) {
    case Which::full:
        return Value::of_bool(run_full(true).right);
    case Which::speed:
        return Value::of_bool(run_speed(true).right);
    case Which::loss:
        return Value::of_bool(run_loss(true).right);
    case Which::all:
        return Value::of_bool(run_all().right);
    default:
        return Value();
    }
}

} // namespace satellite004
