#pragma once
// THE SATELLITE CRITICAL ERROR REPORT -- the shape a serious failure is told in.
//
// The author, 2026-09-18, drew it: *"it'll look like it came out of some space
// kinda novel or something!"*
//
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
        wrapped_into(out, report.description, 0, kReportWidth);

    if (!report.directory.empty() || !report.syntax.empty())
        out += "\n";
    if (!report.directory.empty())
        out += "directory: " + report.directory + "\n";

    if (!report.syntax.empty()) {
        const std::string prefix = "syntax: ";
        out += prefix + report.syntax + "\n";
        if (report.caret_at != std::string::npos && report.caret_at <= report.syntax.size()) {
            const std::size_t column = prefix.size() + report.caret_at;
            out += std::string(column, ' ') + "/\\\n";
            if (!report.caret_note.empty()) {
                // TWO LEFT OF THE CARET, WHICH IS WHAT THE AUTHOR DREW. The note
                // sits under the `/\` rather than beside it, so a long note runs
                // down the page instead of pushing the caret off the right.
                const std::size_t note_indent = column >= 2 ? column - 2 : 0;
                wrapped_into(out, report.caret_note, note_indent, kReportWidth);
            }
        }
    }

    if (!report.notes.empty()) {
        out += "\n";
        for (const std::string &note : report.notes)
            wrapped_into(out, note, 0, kReportWidth);
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
// NOT LOCKED, AND THAT IS TRUE TODAY AND WILL NOT ALWAYS BE. Nothing reports from
// a worker thread yet; the walker is one thread and every raise below comes from
// it. When a thread can raise one, this needs a mutex -- said here rather than
// found when two reports interleave into one unreadable line.
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
    std::string repeats() const
    {
        std::string out;
        for (std::size_t i = 0; i < keys.size(); ++i) {
            if (counts[i] < 2)
                continue;
            const std::string::size_type first_break = keys[i].find('\n');
            out += "[satellite] " + keys[i].substr(0, first_break) + " happened " +
                   std::to_string(counts[i]) + " times in all; it was reported once\n";
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
inline void print_critical(const CriticalReport &report)
{
    if (!report_tally().first_time(report))
        return;                     // said once; the tally counts the rest
    std::cerr << render(report);
    std::cerr.flush();
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
inline void print_notice(const CriticalReport &report)
{
    if (!report_tally().first_time(report))
        return;                     // said once; the tally counts the rest
    std::cerr << "[satellite] " << report.code;
    if (!report.name.empty())
        std::cerr << " " << report.name;
    if (!report.description.empty())
        std::cerr << ": " << report.description;
    if (!report.directory.empty())
        std::cerr << " (" << report.directory << ")";
    std::cerr << "\n";
    std::cerr.flush();
}

} // namespace satellite004
