#pragma once
// THE SATELLITE CRITICAL ERROR REPORT -- the shape a serious failure is told in.
//
// The author, 2026-09-18, drew it: *"it'll look like it came out of some space
// kinda novel or something!"*
//
/*
//     --------------------------------------------------------------------------------
//     SATELLITE CRITICAL ERROR REPORT
//     --------------------------------------------------------------------------------
//     S010: BASIC_ERROR_NAME
//     FULL_ERROR_DESCRIPTION_HERE
//
//     directory: path
//     syntax: code here
//                          /\
//                        however you had 003
//
//     --------------------------------------------------------------------------------
*/
//
// EIGHTY COLUMNS, AND THE RULE IS EXACTLY EIGHTY DASHES. Not the terminal's
// width: a report copied into a bug entry, a file or a chat keeps its shape, and
// a report that reflowed to whoever's window would be a different report every
// time it was shown. 80 is what a terminal is when nobody has said otherwise,
// which is the same argument console width's fallback makes.
//
// EVERY FIELD BELOW THE HEADER IS OPTIONAL, AND A REPORT WITH ONLY A CODE AND A
// DESCRIPTION IS A COMPLETE ONE. A failure with no file has no `directory:` row
// rather than a row saying none; a failure with no line of code has no `syntax:`
// row and therefore no caret. Nothing prints a placeholder -- a report that says
// `path` where a path should be teaches a person to stop reading the rows.
//
// AND THERE IS ROOM BEFORE THE CLOSING RULE (the author, 2026-09-18): *"before
// the last ------- line, add anything else you want to add that we can, we could
// have super huge error reports"*. `notes` is that room. It is last so that a
// person who already knows the error can stop reading at the caret, and so that
// nothing added later moves the four rows that are always in the same place.
//
// SEE SATELLITE_ERROR.md for what an S-code is, which ones exist, and the rule
// for adding one.

#include "console_lock.hpp"
#include "satellite_log.hpp"
#include "shown.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace satellite004 {

inline constexpr std::size_t kReportWidth = 80;

struct CriticalReport {
    std::string code;          // "S010" -- SATELLITE_ERROR.md owns the number
    std::string name;          // "CONFIG_FILE_MISSING" -- short, shouted, no spaces
    std::string description;   // the sentence a person reads first; may be several lines

    std::string directory;     // the file or folder it happened to, if there is one
    std::string syntax;        // the line of code it happened on, if there is one

    // WHERE THE `/\` POINTS, as a position INSIDE `syntax` -- not inside the
    // printed row, because the caller should not have to know that the row is
    // written with an eight-character `syntax: ` in front of it. npos draws no
    // caret, which is what a report with a line but no one place to blame wants.
    std::size_t caret_at = std::string::npos;
    std::string caret_note;    // what the caret says; wrapped under itself

    std::vector<std::string> notes;   // anything else, before the closing rule
};

inline std::string report_rule()
{
    return std::string(kReportWidth, '-');
}

// Wrap `text` to `width`, every line starting at `indent`. Breaks on spaces, and
// a word longer than the width is left whole and allowed to run over -- a path
// or an identifier cut in half to fit is worse than a long row.
inline void wrapped_into(std::string &out, const std::string &text,
                         std::size_t indent, std::size_t width)
{
    const std::string pad(indent, ' ');
    std::string line;
    std::string::size_type at = 0;
    while (at <= text.size()) {
        const std::string::size_type space = text.find(' ', at);
        const std::string word = text.substr(at, space == std::string::npos ? std::string::npos : space - at);
        if (!line.empty() && indent + line.size() + 1 + word.size() > width) {
            out += pad + line + "\n";
            line = word;
        } else {
            line += line.empty() ? word : " " + word;
        }
        if (space == std::string::npos)
            break;
        at = space + 1;
    }
    if (!line.empty())
        out += pad + line + "\n";
}

// shown() A LINE AT A TIME (M5, DESIGN §9). A report quotes the user's bytes -- a
// name, a path, the line itself -- and before this the `syntax:` row wrote them raw:
// a name holding ESC [ 2 J cleared the screen of the person being told about it, and
// would have done it again to whoever read satellite.log. The newlines are satl's own
// (S101's "Add this as the first line:"), so they are kept and each line is escaped.
inline std::string shown_lines(const std::string &text)
{
    std::string out;
    std::string::size_type at = 0;
    for (;;) {
        const std::string::size_type end = text.find('\n', at);
        out += shown(std::string_view(text).substr(at, end == std::string::npos ? std::string::npos : end - at));
        if (end == std::string::npos)
            return out;
        out += '\n';
        at = end + 1;
    }
}

inline std::string render(const CriticalReport &report)
{
    const std::string rule = report_rule();
    std::string out;
    out += rule + "\n";
    out += "SATELLITE CRITICAL ERROR REPORT\n";
    out += rule + "\n";

    out += report.code;
    if (!report.name.empty())
        out += ": " + report.name;
    out += "\n";

    if (!report.description.empty())
        wrapped_into(out, shown_lines(report.description), 0, kReportWidth);

    if (!report.directory.empty() || !report.syntax.empty())
        out += "\n";
    if (!report.directory.empty())
        out += "directory: " + shown(report.directory) + "\n";

    if (!report.syntax.empty()) {
        const std::string prefix = "syntax: ";
        // A TAB IS ONE SPACE HERE, not \x09 (the review, 2026-09-25): tab-aligned comments
        // are common, and one column a tab is also what keeps the caret under its character
        // -- a raw tab never did, since the terminal chose how far it went.
        std::string syntax = report.syntax;
        for (char &c : syntax)
            if (c == '\t')
                c = ' ';
        out += prefix + shown(syntax) + "\n";
        if (report.caret_at != std::string::npos && report.caret_at <= syntax.size()) {
            // THE CARET COUNTS WHAT IS SHOWN, NOT WHAT WAS WRITTEN: every byte before it
            // that became \x1b is four columns now, and the /\ has to move with them.
            const std::size_t shown_before = shown(std::string_view(syntax).substr(0, report.caret_at)).size();
            const std::size_t column = prefix.size() + shown_before;
            out += std::string(column, ' ') + "/\\\n";
            if (!report.caret_note.empty()) {
                // TWO LEFT OF THE CARET, WHICH IS WHAT THE AUTHOR DREW. The note
                // sits under the `/\` rather than beside it, so a long note runs
                // down the page instead of pushing the caret off the right.
                const std::size_t note_indent = column >= 2 ? column - 2 : 0;
                wrapped_into(out, shown_lines(report.caret_note), note_indent, kReportWidth);
            }
        }
    }

    if (!report.notes.empty()) {
        out += "\n";
        for (const std::string &note : report.notes)
            wrapped_into(out, shown_lines(note), 0, kReportWidth);
    }

    out += "\n";
    out += rule + "\n";
    return out;
}

// THE SAME THING IS NOT SAID TWICE.
//
// The author, 2026-09-18: *"this is going to require some tracking so we are not
// reporting the exact same thing twice"*. A refusal inside a loop is one mistake
// that happens forty thousand times, and forty thousand copies of one report
// scroll the useful part off the screen -- which is the console queue's lesson
// with a terminal instead of memory.
//
// THE KEY IS THE THREE TOGETHER: the S-code, the file and the line. The same code
// at a DIFFERENT line is a different mistake and is said; the same code at the
// same line is the same mistake and is counted. That is why the key is not just
// the code -- one `types_do_not_meet` silencing every later one would hide real
// faults.
//
// LOCKED BY WHOEVER PRINTS (2026-09-23): a program's threads can raise reports, so
// print_critical and print_notice hold the console lock (console_lock.hpp) across the
// tally and the write -- the tally is only ever touched inside them. This comment used
// to say the day would come; threads are the day.
struct ReportTally {
    std::vector<std::string> keys;
    std::vector<std::uint64_t> counts;

    // Answers whether this is the FIRST time -- the caller prints only then.
    bool first_time(const CriticalReport &report)
    {
        const std::string key = report.code + "\n" + report.directory + "\n" + report.syntax;
        for (std::size_t i = 0; i < keys.size(); ++i)
            if (keys[i] == key) {
                ++counts[i];
                return false;
            }
        keys.push_back(key);
        counts.push_back(1);
        return true;
    }

    // What was held back, said once at the end. Empty when nothing repeated.
    //
    // `logged` IS satellite.log's COUNT (M5): the place goes in with the code, because a
    // log holds the runs of weeks and "S020 happened 3 times" there names no line of any
    // of them. The screen's count stays as it was -- the report it counts is just above.
    std::string repeats(bool logged = false) const
    {
        std::string out;
        for (std::size_t i = 0; i < keys.size(); ++i) {
            if (counts[i] < 2)
                continue;
            const std::string::size_type first_break = keys[i].find('\n');
            std::string what = keys[i].substr(0, first_break);
            if (logged) {
                const std::string::size_type second_break = keys[i].find('\n', first_break + 1);
                const std::string place = keys[i].substr(first_break + 1, second_break - first_break - 1);
                if (!place.empty())
                    what += " at " + shown(place);
            }
            out += "[satellite] " + what + " happened " + std::to_string(counts[i]) + " times in all; it was " +
                   (logged ? "logged" : "reported") + " once\n";
        }
        return out;
    }
};

inline ReportTally &report_tally()
{
    static ReportTally one;
    return one;
}

// TO stderr, BECAUSE A REPORT IS NOT THE PROGRAM'S OUTPUT. A satellite program
// piped into another program must not have this land in the pipe: the reader on
// the far side is expecting what `display` wrote, and a report in that stream is
// a report that corrupts the thing it was trying to explain.
// AND INTO satellite.log (M5, satellite_log.hpp): the report without its three rules and
// its title, which are the frame and not the news. Once, as the screen gets it.
inline void print_critical(const CriticalReport &report)
{
    const ConsoleHold one_report;
    if (!report_tally().first_time(report))
        return;                     // said once; the tally counts the rest
    const std::string rendered = render(report);
    std::cerr << rendered;
    std::cerr.flush();
    std::vector<std::string> kept;
    for (const std::string &line : lines_of(rendered))
        if (line != report_rule() && line != "SATELLITE CRITICAL ERROR REPORT" && !(kept.empty() && line.empty()))
            kept.push_back(line);
    while (!kept.empty() && kept.back().empty())
        kept.pop_back();
    write_entry(kept);
}

// ONE LINE, FOR THE THINGS THAT ARE NOT CATASTROPHES.
//
// The author's severity ruling, 2026-09-18: *"let's not make more things fatal,
// let's make less things fatal and only use fatal when we absolutely have to use
// it"*. Two rules of eighty dashes is an alarm, and an alarm spent on a missing
// config file is an alarm nobody reads the next time.
//
// SO THE FRAME IS EARNED RATHER THAN DEFAULT. A notice says its code, its name
// and its sentence on one line and gets out of the way; `print_critical` is for
// a failure a person needs everything about. Both carry the S-code, so the
// quieter one is still searchable and still leads to the same entry.
//
// **THIS IS ABOUT THE FRAME AND NOT ABOUT STOPPING.** The same ruling says a
// program whose meaning is unclear must stop -- *"we don't want it to operate
// incorrectly ... in case someone uses it inside of a data center"*. Stop
// readily, alarm rarely: a refusal can end the run and still be one line.
inline std::string notice_line(const CriticalReport &report)
{
    std::string line = "[satellite] " + report.code;
    if (!report.name.empty())
        line += " " + report.name;
    if (!report.description.empty())
        line += ": " + shown_lines(report.description);
    if (!report.directory.empty())
        line += " (" + shown(report.directory) + ")";
    return line;
}

inline void print_notice(const CriticalReport &report)
{
    const ConsoleHold one_notice;
    if (!report_tally().first_time(report))
        return;                     // said once; the tally counts the rest
    const std::string line = notice_line(report);
    std::cerr << line << "\n";
    std::cerr.flush();
    write_entry(lines_of(line));
}

// A WARNING FOR satellite.log ALONE -- the author's own case, 2026-09-16: a number where
// text is expected is converted, "obviously the programmer meant convert to string, but
// record the warning in satellite.log". Not on the screen: the program did what was meant.
//
// ONCE A PLACE, ON ITS OWN TALLY: a conversion inside a loop is one line of the program,
// and a hundred thousand entries for it would bury everything else in the file. Its own
// tally and not the screen's, so the end of the run never says "happened 5 times; it was
// reported once" about something the screen was never shown.
//
// AND WHEN THE LOG CANNOT BE WRITTEN IT IS PRINTED, as a notice: a warning never
// disappears (003's rule for the same file).
inline ReportTally &logged_tally()
{
    static ReportTally one;
    return one;
}

// THE COUNTS GO WITH THE ENTRIES THEY COUNT (M5): the log kept the first of each, and a
// person reading it should know when the first was one of forty thousand. Called once, by
// main() after the run and its windows are over -- NOT at the end of run_satl, which
// returns from two dozen places: a run that stopped on a refusal lost its counts there
// (the review, 2026-09-25).
inline void log_the_counts()
{
    const ConsoleHold one_count;
    const std::string said = report_tally().repeats(true);
    if (!said.empty())
        write_entry(lines_of(said));
    const std::string logged = logged_tally().repeats(true);
    if (!logged.empty())
        write_entry(lines_of(logged));
}

// `once` false writes it however many times it comes -- the prompt's, where a warning has
// no file to be placed in, so the tally's key could not tell two typed lines apart.
inline void log_only_warning(const CriticalReport &report, bool once = true)
{
    const ConsoleHold one_warning;
    if (once && !logged_tally().first_time(report))
        return;
    const std::string line = notice_line(report);
    if (write_entry(lines_of(line)) != 0) {
        std::cerr << line << " -- and satellite.log could not be written, so it is said here\n";
        std::cerr.flush();
    }
}

} // namespace satellite004
