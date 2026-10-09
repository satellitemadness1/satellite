#pragma once
// satellite/machine/satellite_log.hpp -- satellite.log: EVERY REPORT satl MAKES, KEPT (M5).
//
// WHAT GOES IN IT, AND WHOSE WORDS EACH PART IS:
//
//   a name clash        DESIGN §7: "a clash is written to satellite.log as an entry"
//   a warning           003's ruling (the author, 2026-09-13): a warning "just permanently
//                       sits in that file until you or I clear it out"
//   a conversion        the author, 2026-09-16: a number where text is expected -- "just
//                       convert the number to the string ... but record the warning in
//                       satellite.log"
//   what checking finds the author, 2026-09-16 (M29): "checking reports into satellite.log
//                       with warnings, and dies at that"
//
// SO EVERY REPORT, NOT ONLY WARNINGS: a refusal, a notice and a warning all go in, each
// once (critical_report.hpp's tally) -- which is also what makes PLAN's testing rule
// readable: "a milestone is done when that run writes no [entry]". A run that says
// nothing writes nothing. CHOSEN HERE, reversible in the three callers: 003 kept warnings
// only.
//
// THE ENTRY IS THE AUTHOR'S write_entry (DESIGN §7), with the two things §7 said to
// settle when it was built:
//
//   the path     comes from config: arguments.log_path, "~/.satl/satellite.log" -- one
//                file a person, under ~/.satl because that is where satl is installed
//                (003's choice, the author's, over the program's folder, which would
//                scatter the file). One machine may set `log_path = ...` in config.ini.
//   the mutex    one lock around the whole entry, and the whole entry handed to ONE
//                write(2) on an O_APPEND file, so two threads -- or two satls, which a
//                mutex cannot reach -- never interleave inside an entry.
//
// A LOG THAT CANNOT BE WRITTEN NEVER STOPS A PROGRAM. write_entry answers 1 and the
// caller decides: a report was already on the screen, and a warning meant only for the
// log is printed instead, so nothing is ever lost (log_only_warning, critical_report.hpp).
//
// NOTHING RAW REACHES THE FILE: a report's text has been through shown() before it gets
// here -- render(), notice_line() and report_error() do it -- and the heading's two paths
// go through it below, so a name holding ESC [ 2 J is \x1b[2J in the file and cannot
// clear the screen of whoever reads it with cat.

#include "shown.hpp"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace satellite004 {

struct SatelliteLog {
    std::mutex lock;
    std::string path;       // "" until satl has read its config, and then the resolved file
    std::string program;    // what each entry names: a program's absolute path, or "the prompt"
};

inline SatelliteLog &satellite_log()
{
    static SatelliteLog one;
    return one;
}

// "~" AND "~/..." ARE THE HOME FOLDER, as arguments.directory.default reads them; "" when
// $HOME is unset and the path needs it -- no log, rather than one in a surprising place.
inline std::string log_path_from(const std::string &written)
{
    if (written != "~" && written.rfind("~/", 0) != 0)
        return written;
    const char *home = std::getenv("HOME");
    if (home == nullptr || *home == '\0')
        return std::string();
    return std::string(home) + written.substr(1);
}

inline constexpr const char *kDefaultLogPath = "~/.satl/satellite.log";

// Set once at start-up, from arguments.log_path, before the program is read.
inline void set_log_path(const std::string &written)
{
    const std::lock_guard<std::mutex> held(satellite_log().lock);
    satellite_log().path = log_path_from(written);
}

// ABSOLUTE, because the log outlives the folder a person was standing in: "prog.satl line
// 21" names nothing a week later (003's reason, found the same way).
inline void set_log_program(const std::string &file)
{
    std::string named = file.empty() ? std::string("the prompt") : file;
    if (char *whole = ::realpath(file.c_str(), nullptr)) {
        named = whole;
        std::free(whole);
    }
    const std::lock_guard<std::mutex> held(satellite_log().lock);
    satellite_log().program = std::move(named);
}

// THE FIRST LINE OF AN ENTRY: when, which process, which program, and the folder it ran
// in -- the report's own `directory:` row is the file as it was typed, often relative.
inline std::string entry_heading()
{
    char when[32] = "";
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    if (localtime_r(&now, &local) != nullptr)
        std::strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", &local);
    std::string heading = std::string(when) + "  pid " + std::to_string(::getpid());
    // THE PATHS ARE THE USER'S BYTES, so they go through shown() like everything else: a
    // program saved as `ESC[2J.satl` put a raw ESC here, found by the suite's hostile names.
    const std::string &program = satellite_log().program;
    if (!program.empty())
        heading += "  " + shown(program);
    char here[PATH_MAX];
    if (::getcwd(here, sizeof here) != nullptr)
        heading += "  (run in " + shown(here) + ")";
    return heading;
}

// A MESSAGE THAT IS SEVERAL LINES, as one element a line -- an entry is one line an element.
inline std::vector<std::string> lines_of(const std::string &text)
{
    std::vector<std::string> lines;
    std::string::size_type at = 0;
    while (at < text.size()) {
        const std::string::size_type end = text.find('\n', at);
        lines.push_back(text.substr(at, end == std::string::npos ? std::string::npos : end - at));
        if (end == std::string::npos)
            break;
        at = end + 1;
    }
    return lines;
}

// THE AUTHOR'S write_entry, as DESIGN §7 has it -- the same name, the same argument, the
// same [entry] and [/entry] around one line a vector element, and the same 0 or 1 -- with
// its path from config, its mutex, the heading line first, and ONE write for the whole
// entry instead of a stream that may flush it in pieces.
inline signed long long int write_entry(const std::vector<std::string> &text_input_vector)
{
    const std::lock_guard<std::mutex> held(satellite_log().lock);
    std::string path = satellite_log().path;
    if (path.empty())
        path = log_path_from(kDefaultLogPath);    // a report before the config was read
    if (path.empty())
        return(1);
    // ~/.satl ITSELF IS MADE WHEN IT IS MISSING -- one folder, never a tree, and only the
    // default's: a path a person set names a folder they already have.
    if (satellite_log().path.empty() || satellite_log().path == log_path_from(kDefaultLogPath)) {
        const std::string folder = path.substr(0, path.rfind('/'));
        if (!folder.empty() && ::mkdir(folder.c_str(), 0755) != 0 && errno != EEXIST)
            return(1);
    }

    // ONLY write_entry WRITES A MARKER (the review, 2026-09-25): a report quotes a program's
    // own strings, and one holding "\n[/entry]\n...\n[entry]\n" made one report two entries,
    // the second with a heading of its choosing -- and the markers are what a person, and
    // check.sh, count. So a line of the text that IS a marker is written with a \ in front.
    const auto defused = [](const std::string &text) {
        std::string out;
        for (const std::string &line : lines_of(text))
            out += (line == "[entry]" || line == "[/entry]" ? "\\" + line : line) + "\n";
        return text.empty() ? std::string("\n") : out;
    };
    std::string entry = "\n\n[entry]\n\n" + entry_heading() + "\n";
    for (unsigned long long int vector_index = 0; vector_index < text_input_vector.size(); vector_index++)
        entry += defused(text_input_vector[vector_index]);
    entry += "\n\n[/entry]\n\n";

    const int fd = ::open(path.c_str(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
    if (fd < 0)
        return(1);
    std::size_t sent = 0;
    bool whole = true;
    while (sent < entry.size()) {
        const ssize_t put = ::write(fd, entry.data() + sent, entry.size() - sent);
        if (put < 0 && errno == EINTR)
            continue;
        if (put <= 0) {
            whole = false;
            break;
        }
        sent += static_cast<std::size_t>(put);
    }
    ::close(fd);
    return whole ? 0 : 1;
}

} // namespace satellite004
