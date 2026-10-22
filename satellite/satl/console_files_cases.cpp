// satellite/satl/console_files_cases.cpp -- the console's store with no console around it (KEY_PLAN.md
// KEY-1 to KEY-3): the day from a clock of its own, the day's folder made 0700 at the first Enter and
// not before, the console's own file claimed in it at the same Enter -- console-001.c4, one more than
// the highest there, 0600, held open above 0, 1 and 2 -- every Enter kept, a satellite.variable.string
// among the console's lines and a line of that file, less what what_is_kept drops, and S017
// CONSOLE_NOT_SAVED where the folder or the file cannot be made, or a line cannot be written. And the
// walk when the store is made (KEY-5): the consoles before it, newest first, each read back only while
// it is the file the walk listed; and Left's list, the newest of them up to the cache (KEY-6).
//
//     make build/console_files_cases && build/console_files_cases "${TMPDIR:-/tmp}"    (check.sh runs it)
//
// IT WRITES ONLY INSIDE ONE FRESH FOLDER, made in the folder it is given and removed at the end, and HOME
// POINTS INTO IT before anything else runs, so nothing the store calls can reach a person's ~/.satl.
// Each case gives the store a home of its own in there, and a clock of the harness's own -- never the
// machine's. S017 IS COUNTED WHERE THE STORE SAYS IT (ConsoleFiles::Say), never read off stderr:
// print_notice's tally says a notice once a place, its code and directory (critical_report.hpp), so the
// cases with no HOME to use, whose S017s have no directory, would print as one -- and print_notice
// writes satellite.log as well.
//
// `ok <name>` for each case that passes, `FAIL <name>: <what happened>` for each that does not; the exit
// status is 0 only when none failed. The two cases that need a folder they cannot write are skipped as root.

#include "console_files.hpp"

#include "../machine/shown.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <csignal>
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace satellite004;

namespace {

int failed = 0;
int passed = 0;
std::string room;   // the fresh folder every case works in

// HOW MUCH LEFT REACHES (KEY-6), the harness's own: arguments.input.cache's 32 mb as satl gives it, in bytes.
// The harness never includes satellite_config.hpp, whose build row a bare make rewrites while it links.
constexpr unsigned long long int kMb = 1024ull * 1024ull;
constexpr unsigned long long int kCache = 32 * kMb;

void check(bool right, const std::string &name, const std::string &detail = "")
{
    if (right) {
        ++passed;
        std::printf("ok %s\n", name.c_str());
    } else {
        ++failed;
        std::printf("FAIL %s: %s\n", name.c_str(), detail.empty() ? "(no detail)" : detail.c_str());
    }
}

void skipped(const std::string &name, const char *why)
{
    ++passed;
    std::printf("ok %s (skipped: %s)\n", name.c_str(), why);
}

// WHAT THE STORE SAID, in place of print_notice: every S017 counted, the last one kept. Locked, because
// one case Enters from sixteen threads at once.
std::mutex said_lock;
int said = 0;
CriticalReport said_last;

void count_it(const CriticalReport &report)
{
    const std::lock_guard<std::mutex> held(said_lock);
    ++said;
    said_last = report;
}

void forget_what_was_said()
{
    said = 0;
    said_last = CriticalReport();
}

std::string what_was_said()
{
    return std::to_string(said) + " said" +
           (said == 0 ? std::string() : ", the last \"" + notice_line(said_last) + "\"");
}

// A HOME OF ITS OWN FOR EACH CASE, inside the room, with no .satl in it yet.
std::string home_for(const std::string &name)
{
    const std::string home = room + "/" + name;
    ::mkdir(home.c_str(), 0755);
    return home;
}

// CLOCKS OF THE HARNESS'S OWN. Each moment is made with mktime from local time, so localtime_r gives the
// same date back whatever TZ the harness runs under.
std::time_t local_moment(int year, int month, int day, int hour, int minute)
{
    std::tm moment{};
    moment.tm_year = year - 1900;
    moment.tm_mon = month - 1;
    moment.tm_mday = day;
    moment.tm_hour = hour;
    moment.tm_min = minute;
    moment.tm_isdst = -1;
    return std::mktime(&moment);
}

int clock_asked = 0;

std::time_t on_2026_10_20()
{
    ++clock_asked;
    return local_moment(2026, 10, 20, 12, 0);
}

std::time_t on_2027_01_05()
{
    ++clock_asked;
    return local_moment(2027, 1, 5, 12, 0);
}

// 23:59 ON 2026-10-19 THE FIRST TIME IT IS ASKED, AND 00:01 ON 2026-10-20 EVERY TIME AFTER: a console
// started a minute before midnight and Entered a minute after. A store that read the day at its first
// Enter would make 2026-10-20.
std::time_t across_midnight()
{
    return ++clock_asked == 1 ? local_moment(2026, 10, 19, 23, 59) : local_moment(2026, 10, 20, 0, 1);
}

// What is at a path, a link never followed.
bool there(const std::string &path)
{
    struct stat about {};
    return ::lstat(path.c_str(), &about) == 0;
}

bool is_folder(const std::string &path)
{
    struct stat about {};
    return ::lstat(path.c_str(), &about) == 0 && S_ISDIR(about.st_mode);
}

bool is_link(const std::string &path)
{
    struct stat about {};
    return ::lstat(path.c_str(), &about) == 0 && S_ISLNK(about.st_mode);
}

mode_t mode_of(const std::string &path)
{
    struct stat about {};
    return ::lstat(path.c_str(), &about) == 0 ? (about.st_mode & 07777) : 0;
}

std::string octal(mode_t mode)
{
    char written[16];
    std::snprintf(written, sizeof written, "%04o", static_cast<unsigned>(mode));
    return written;
}

// The names in a folder, sorted; none for a folder that is not there.
std::vector<std::string> names_in(const std::string &folder)
{
    std::vector<std::string> names;
    DIR *listing = ::opendir(folder.c_str());
    if (listing == nullptr)
        return names;
    while (const dirent *entry = ::readdir(listing))
        if (std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0)
            names.push_back(entry->d_name);
    ::closedir(listing);
    std::sort(names.begin(), names.end());
    return names;
}

std::string listed(const std::vector<std::string> &names)
{
    std::string out = "{";
    for (const std::string &name : names)
        out += (out.size() > 1 ? ", " : "") + name;
    return out + "}";
}

// The descriptors this process holds, as /proc says -- less the one the listing itself holds.
int open_descriptors()
{
    const std::vector<std::string> names = names_in("/proc/self/fd");
    return static_cast<int>(names.size()) - 1;
}

bool put(const std::string &path, const std::string &bytes)
{
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0)
        return false;
    const bool wrote = ::write(fd, bytes.data(), bytes.size()) == static_cast<ssize_t>(bytes.size());
    return ::close(fd) == 0 && wrote;
}

std::string bytes_of(const std::string &path)
{
    std::string out;
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return "<unreadable>";
    char piece[4096];
    for (ssize_t got; (got = ::read(fd, piece, sizeof piece)) > 0;)
        out.append(piece, static_cast<std::size_t>(got));
    ::close(fd);
    return out;
}

// THE CONSOLE'S LINES AS UTF-8, oldest first (KEY-3).
std::vector<std::string> kept_lines(const prompt::ConsoleLines &lines)
{
    std::vector<std::string> out;
    for (std::size_t index = 0; index < lines.size(); ++index)
        out.push_back(lines.at(index).to_utf8());
    return out;
}

// WHAT THOSE LINES ARE AS A FILE: each with its newline.
std::string as_a_file(const std::vector<std::string> &lines)
{
    std::string out;
    for (const std::string &line : lines)
        out += line + "\n";
    return out;
}

// LINES FOR A FAILURE'S DETAIL, every byte that could upset a terminal shown as \xNN.
std::string listed_lines(const std::vector<std::string> &lines)
{
    std::string out = "{";
    for (std::size_t index = 0; index < lines.size() && index < 20; ++index)
        out += (index == 0 ? "\"" : ", \"") + shown(lines[index]) + "\"";
    return out + (lines.size() > 20 ? ", ... " + std::to_string(lines.size()) + " in all}" : "}");
}

// THE DESCRIPTORS THIS PROCESS HOLDS ON NAMES IN `folder`, as /proc says -- each name with its
// descriptor's number, sorted by name. How a case sees the file a store holds open, and where.
struct HeldName {
    int fd;
    std::string name;
};

std::vector<HeldName> held_in(const std::string &folder)
{
    std::vector<HeldName> held;
    DIR *listing = ::opendir("/proc/self/fd");
    if (listing == nullptr)
        return held;
    const std::string inside = folder + "/";
    while (const dirent *entry = ::readdir(listing)) {
        if (entry->d_name[0] == '.' || std::atoi(entry->d_name) == ::dirfd(listing))
            continue;
        char target[PATH_MAX];
        const ssize_t length =
            ::readlink((std::string("/proc/self/fd/") + entry->d_name).c_str(), target, sizeof target - 1);
        if (length > 0 && std::string(target, static_cast<std::size_t>(length)).compare(0, inside.size(), inside) == 0)
            held.push_back({std::atoi(entry->d_name), std::string(target + inside.size(), target + length)});
    }
    ::closedir(listing);
    std::sort(held.begin(), held.end(), [](const HeldName &a, const HeldName &b) { return a.name < b.name; });
    return held;
}

std::string listed(const std::vector<HeldName> &held)
{
    std::string out = "{";
    for (const HeldName &one : held)
        out += (out.size() > 1 ? ", " : "") + one.name + " on " + std::to_string(one.fd);
    return out + "}";
}

// A DAY'S FOLDER AS EARLIER CONSOLES OF THE DAY LEFT IT: ~/.satl/console/2026-10-20 in a home of its
// own, holding `files`, each with a line in it. Answers the day's folder; the home is the folder three
// above it.
std::string a_day_holding(const std::string &name, const std::vector<std::string> &files)
{
    const std::string home = home_for(name), day = home + "/.satl/console/2026-10-20";
    ::mkdir((home + "/.satl").c_str(), 0755);
    ::mkdir((home + "/.satl/console").c_str(), 0700);
    ::mkdir(day.c_str(), 0700);
    for (const std::string &file : files)
        if (!put(day + "/" + file, "a line an earlier console kept\n"))
            check(false, "setup: " + day + "/" + file, std::strerror(errno));
    return day;
}

std::string home_of(const std::string &day)
{
    return day.substr(0, day.size() - std::string("/.satl/console/2026-10-20").size());
}

std::vector<std::string> with(std::vector<std::string> names, const std::string &more)
{
    names.push_back(more);
    std::sort(names.begin(), names.end());
    return names;
}

// ---------------------------------------------------------------------------
// 1. THE DAY IS THE CLOCK'S, written %Y-%m-%d, and the folder is named by it
// ---------------------------------------------------------------------------
void the_day_is_the_clocks()
{
    struct Day {
        ConsoleFiles::Clock clock;
        const char *want;
    };
    for (const Day &one : {Day{on_2026_10_20, "2026-10-20"}, Day{on_2027_01_05, "2027-01-05"}}) {
        const std::string home = home_for(std::string("day_") + one.want);
        forget_what_was_said();
        ConsoleFiles store(one.clock, home.c_str(), kCache, count_it);
        store.entered("satellite.console.display(1)");
        const std::vector<std::string> days = names_in(home + "/.satl/console");
        check(store.day() == one.want && days == std::vector<std::string>{one.want} && said == 0,
              std::string("a clock at ") + one.want + " gives " + one.want,
              "day() is \"" + store.day() + "\", ~/.satl/console holds " + listed(days) + ", " + what_was_said());
    }
}

// ---------------------------------------------------------------------------
// 2. THE DAY IS READ ONCE, when the store is made
// ---------------------------------------------------------------------------
void the_day_is_read_once()
{
    const std::string home = home_for("midnight");
    forget_what_was_said();
    clock_asked = 0;
    ConsoleFiles store(across_midnight, home.c_str(), kCache, count_it);
    const int asked_when_made = clock_asked;
    store.entered("typed at 00:01");
    const std::vector<std::string> days = names_in(home + "/.satl/console");
    check(asked_when_made == 1 && clock_asked == 1 && days == std::vector<std::string>{"2026-10-19"} && said == 0,
          "a clock that answers 2026-10-19 23:59 when the console starts, and 2026-10-20 00:01 at its first "
          "Enter, makes 2026-10-19",
          "the clock was asked " + std::to_string(asked_when_made) + " time(s) when the store was made and " +
              std::to_string(clock_asked) + " in all; ~/.satl/console holds " + listed(days));
}

// ---------------------------------------------------------------------------
// 3. NOTHING BEFORE THE FIRST ENTER, AND THE FOLDERS' MODES
// ---------------------------------------------------------------------------
void the_folders()
{
    const std::string home = home_for("modes");
    const std::string satl = home + "/.satl", console = satl + "/console", day = console + "/2026-10-20";
    forget_what_was_said();
    ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
    check(names_in(home).empty(), "nothing is made before the first Enter", "the home holds " + listed(names_in(home)));
    check(store.root() == console, "the root is $HOME/.satl/console, absolute",
          "root() is \"" + store.root() + "\"");
    store.entered("");
    check(is_folder(console) && is_folder(day) && mode_of(console) == 0700 && mode_of(day) == 0700 && said == 0,
          "both folders are 0700",
          "console is " + octal(mode_of(console)) + " and the day " + octal(mode_of(day)) + ", " + what_was_said());
    check(is_folder(satl) && mode_of(satl) == 0755, "a missing ~/.satl is made, 0755, as satellite.log's write makes it",
          "~/.satl is " + octal(mode_of(satl)));
    // THE SAME ENTER CLAIMED THE CONSOLE'S FILE IN THE FOLDER IT MADE (KEY-2): an empty folder gives 001.
    check(names_in(day) == std::vector<std::string>{"console-001.c4"},
          "the first Enter claims console-001.c4 in the day's folder it made",
          "the day holds " + listed(names_in(day)));
    store.entered("a second line");
    check(names_in(console) == std::vector<std::string>{"2026-10-20"} &&
              names_in(day) == std::vector<std::string>{"console-001.c4"} &&
              bytes_of(day + "/console-001.c4") == "\na second line\n" && held_in(day).size() == 1 && said == 0,
          "an Enter after the first claims nothing more, and each Enter is a line of the file (KEY-3)",
          "console holds " + listed(names_in(console)) + ", the day " + listed(names_in(day)) + ", the file \"" +
              shown(bytes_of(day + "/console-001.c4")) + "\", held " + listed(held_in(day)));
}

// ---------------------------------------------------------------------------
// 4. WHAT WAS THERE ALREADY
// ---------------------------------------------------------------------------
void what_was_there()
{
    const std::string home = home_for("kept");
    const std::string satl = home + "/.satl", console = satl + "/console";
    ::mkdir(satl.c_str(), 0750);
    ::chmod(satl.c_str(), 0750);
    put(satl + "/satellite.log", "an entry of the person's\n");
    forget_what_was_said();
    ConsoleFiles first(on_2026_10_20, home.c_str(), kCache, count_it);
    first.entered("x");
    check(mode_of(satl) == 0750 && bytes_of(satl + "/satellite.log") == "an entry of the person's\n" &&
              names_in(satl) == std::vector<std::string>{"console", "satellite.log"} && said == 0,
          "a ~/.satl already there keeps its mode and everything in it",
          "~/.satl is " + octal(mode_of(satl)) + " and holds " + listed(names_in(satl)) + ", " + what_was_said());
    // A SECOND CONSOLE OF THE DAY finds the folders made, and uses them as they are -- and claims the
    // number after the first's (KEY-2).
    ConsoleFiles second(on_2026_10_20, home.c_str(), kCache, count_it);
    second.entered("y");
    check(names_in(console) == std::vector<std::string>{"2026-10-20"} && mode_of(console + "/2026-10-20") == 0700 &&
              names_in(console + "/2026-10-20") == std::vector<std::string>{"console-001.c4", "console-002.c4"} &&
              said == 0,
          "a second console of the day finds the day's folder there, claims console-002.c4, and says nothing",
          "console holds " + listed(names_in(console)) + ", the day " + listed(names_in(console + "/2026-10-20")) +
              ", " + what_was_said());
    // A ~/.satl THAT IS A LINK -- to a data drive, say -- is the person's own arrangement, and is followed
    // as config.ini and satellite.log follow it (console_files.cpp); only the two folders below it are
    // never links. The link points inside the room.
    const std::string linked = home_for("linked_satl"), real = linked + "/real_satl";
    ::mkdir(real.c_str(), 0755);
    if (::symlink("real_satl", (linked + "/.satl").c_str()) != 0)
        check(false, "setup: a link at " + linked + "/.satl", std::strerror(errno));
    forget_what_was_said();
    ConsoleFiles through(on_2026_10_20, linked.c_str(), kCache, count_it);
    through.entered("z");
    check(is_link(linked + "/.satl") && is_folder(real + "/console/2026-10-20") &&
              mode_of(real + "/console/2026-10-20") == 0700 && said == 0,
          "a ~/.satl that is a link is followed, as config.ini and satellite.log follow it",
          "what it points at holds " + listed(names_in(real)) + ", " + what_was_said());
}

// ---------------------------------------------------------------------------
// 5. A LINK, OR NOT A FOLDER, IS S017, AND NOTHING IS WRITTEN THROUGH IT
// ---------------------------------------------------------------------------
void links_are_refused()
{
    {
        const std::string home = home_for("linked_root");
        const std::string satl = home + "/.satl", console = satl + "/console", elsewhere = home + "/elsewhere";
        ::mkdir(satl.c_str(), 0755);
        ::mkdir(elsewhere.c_str(), 0700);
        if (::symlink("../elsewhere", console.c_str()) != 0)
            check(false, "setup: a link at " + console, std::strerror(errno));
        forget_what_was_said();
        ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
        store.entered("x");
        check(said == 1 && said_last.code == "S017" && said_last.name == "CONSOLE_NOT_SAVED" &&
                  said_last.directory == console && is_link(console) && names_in(elsewhere).empty(),
              "a root that is a link is S017, and nothing is written through it",
              what_was_said() + "; what the link points at holds " + listed(names_in(elsewhere)));
        // AFTER S017 THE STORE TRIES NOTHING MORE ON THE DISK THIS RUN: the link taken away, the next
        // Enter makes nothing, and says nothing more.
        ::unlink(console.c_str());
        store.entered("y");
        check(said == 1 && !there(console), "after S017 nothing more is tried on the disk, and nothing more is said",
              what_was_said() + "; ~/.satl holds " + listed(names_in(satl)));
    }
    {
        const std::string home = home_for("linked_day");
        const std::string console = home + "/.satl/console", elsewhere = home + "/elsewhere";
        ::mkdir((home + "/.satl").c_str(), 0755);
        ::mkdir(console.c_str(), 0700);
        ::mkdir(elsewhere.c_str(), 0700);
        if (::symlink("../../elsewhere", (console + "/2026-10-20").c_str()) != 0)
            check(false, "setup: a link at " + console + "/2026-10-20", std::strerror(errno));
        forget_what_was_said();
        ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
        store.entered("x");
        check(said == 1 && said_last.code == "S017" && said_last.directory == console + "/2026-10-20" &&
                  is_link(console + "/2026-10-20") && names_in(elsewhere).empty(),
              "a day's folder that is a link is S017, and nothing is written through it",
              what_was_said() + "; what the link points at holds " + listed(names_in(elsewhere)));
    }
    {
        const std::string home = home_for("file_root");
        const std::string console = home + "/.satl/console";
        ::mkdir((home + "/.satl").c_str(), 0755);
        put(console, "not a folder\n");
        forget_what_was_said();
        ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
        store.entered("x");
        check(said == 1 && said_last.code == "S017" && bytes_of(console) == "not a folder\n",
              "a root that is a file is S017, and the file is left as it was", what_was_said());
    }
}

// ---------------------------------------------------------------------------
// 6. A FOLDER THAT CANNOT BE WRITTEN
// ---------------------------------------------------------------------------
void a_folder_not_writable()
{
    const std::string name = "a ~/.satl that cannot be written is S017, and nothing is made";
    if (::geteuid() == 0) {
        skipped(name, "root writes anyway");
        return;
    }
    const std::string home = home_for("read_only");
    const std::string satl = home + "/.satl";
    ::mkdir(satl.c_str(), 0755);
    ::chmod(satl.c_str(), 0555);
    forget_what_was_said();
    ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
    store.entered("x");
    check(said == 1 && said_last.code == "S017" && said_last.directory == satl + "/console" &&
              said_last.description.find("could not be made (Permission denied)") != std::string::npos &&
              names_in(satl).empty(),
          name, what_was_said());
    ::chmod(satl.c_str(), 0755);
}

// ---------------------------------------------------------------------------
// 7. NO HOME TO USE
// ---------------------------------------------------------------------------
void no_home()
{
    // IN A FOLDER OF ITS OWN, so a store that built a path out of nothing, or out of a relative HOME,
    // would make it here.
    const std::string here = home_for("no_home");
    char was[PATH_MAX];
    if (::getcwd(was, sizeof was) == nullptr || ::chdir(here.c_str()) != 0) {
        check(false, "setup: into " + here, std::strerror(errno));
        return;
    }
    forget_what_was_said();
    ConsoleFiles unset(on_2026_10_20, nullptr, kCache, count_it);
    unset.entered("one");
    unset.entered("two");
    check(said == 1 && said_last.code == "S017" && unset.root().empty() && names_in(here).empty(),
          "HOME unset makes nothing and says S017 once", what_was_said() + "; here holds " + listed(names_in(here)));
    check(notice_line(said_last) == "[satellite] S017 CONSOLE_NOT_SAVED: $HOME is not set, so what is typed into "
                                    "this console is not kept. The console works as before, and nothing more is "
                                    "tried on the disk this run.",
          "... in one line that says why, and that the console works as before", notice_line(said_last));
    // ITS LINES ARE KEPT IN MEMORY ALL THE SAME (KEY-3, rule 8): only the disk is given up.
    check(kept_lines(unset.lines()) == std::vector<std::string>{"one", "two"} && names_in(here).empty(),
          "... and every line is kept among the console's lines all the same, with nothing written",
          "the lines are " + listed_lines(kept_lines(unset.lines())));

    forget_what_was_said();
    ConsoleFiles empty(on_2026_10_20, "", kCache, count_it);
    empty.entered("one");
    empty.entered("two");
    check(said == 1 && said_last.code == "S017" && names_in(here).empty(),
          "HOME empty is HOME unset: nothing made, S017 once", what_was_said());

    ::mkdir("relative", 0755);
    ::mkdir("relative/home", 0755);
    forget_what_was_said();
    ConsoleFiles relative(on_2026_10_20, "relative/home", kCache, count_it);
    relative.entered("one");
    check(said == 1 && said_last.code == "S017" && relative.root().empty() && names_in(here + "/relative/home").empty(),
          "a HOME that is not absolute is refused, not resolved: S017, and nothing made",
          what_was_said() + "; relative/home holds " + listed(names_in(here + "/relative/home")));

    if (::chdir(was) != 0)
        check(false, "setup: back into " + std::string(was), std::strerror(errno));
}

// ---------------------------------------------------------------------------
// 8. ENTERS FROM SIXTEEN THREADS AT ONCE: input() runs on a program's threads
// ---------------------------------------------------------------------------
void enters_at_once(const std::string &home)
{
    ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    for (int i = 0; i < 16; ++i)
        threads.emplace_back([&]() {
            ++ready;
            while (!go.load())
                ;
            store.entered("from a thread");
        });
    while (ready.load() < 16)
        ;
    go = true;
    for (std::thread &thread : threads)
        thread.join();
}

void threads_at_once()
{
    const std::string home = home_for("threads");
    forget_what_was_said();
    enters_at_once(home);
    check(said == 0 && is_folder(home + "/.satl/console/2026-10-20") &&
              names_in(home + "/.satl/console") == std::vector<std::string>{"2026-10-20"} &&
              names_in(home + "/.satl/console/2026-10-20") == std::vector<std::string>{"console-001.c4"},
          "sixteen threads Entering at once make the day's folder and claim one file, and none says S017",
          what_was_said() + "; the day holds " + listed(names_in(home + "/.satl/console/2026-10-20")));
    // ONE S017 AMONG THEM: the work in front of it -- the folders opened down to the link -- is the
    // window two threads with no lock between them would both pass through. The link points inside the
    // room, so a store that followed it could reach nothing else.
    const std::string linked = home_for("threads_linked");
    ::mkdir((linked + "/.satl").c_str(), 0755);
    ::mkdir((linked + "/elsewhere").c_str(), 0700);
    if (::symlink("../elsewhere", (linked + "/.satl/console").c_str()) != 0)
        check(false, "setup: a link at " + linked + "/.satl/console", std::strerror(errno));
    forget_what_was_said();
    enters_at_once(linked);
    check(said == 1 && said_last.code == "S017" && names_in(linked + "/elsewhere").empty(),
          "sixteen threads Entering at once at a root that is a link say S017 once", what_was_said());
}

// ---------------------------------------------------------------------------
// 9. DESCRIPTORS: none left open, 0, 1 and 2 among them
// ---------------------------------------------------------------------------
void descriptors()
{
    const int before = open_descriptors();
    int while_held = -1;
    std::vector<HeldName> held;
    {
        const std::string home = home_for("descriptors_made");
        forget_what_was_said();
        ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
        store.entered("x");
        while_held = open_descriptors();
        held = held_in(home + "/.satl/console/2026-10-20");
    }
    const int after_made = open_descriptors();
    {
        const std::string home = home_for("descriptors_refused");
        ::mkdir((home + "/.satl").c_str(), 0755);
        ::mkdir((home + "/.satl/console").c_str(), 0700);
        put(home + "/.satl/console/2026-10-20", "a file where the day's folder goes\n");
        forget_what_was_said();
        ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
        store.entered("x");
    }
    const int after_refused = open_descriptors();
    check(while_held == before + 1 && held.size() == 1 && after_made == before && after_refused == before && said == 1,
          "the store holds one descriptor, its file's, until it goes; no folder's is left open, made or refused",
          std::to_string(before) + " open before, " + std::to_string(while_held) + " while the store held " +
              listed(held) + ", " + std::to_string(after_made) + " after it went, " + std::to_string(after_refused) +
              " after a folder refused");

    // 0, 1 AND 2 CLOSED: open() hands those numbers out first, so a descriptor of the store left open would
    // be one of them -- and so would the file it keeps open for the run, but for the claim (KEY-2). By the
    // claim, the folders above the day's have closed, and the day's listing has come and gone, so the
    // lowest number is free again when the file is opened. Then only 2 closed, as
    // `SATL_NO_WINDOW=1 satl 2>&-` starts: the file there would take every notice std::cerr writes. A
    // thread writes into each closed number all through the claim, as a program's thread writes a
    // notice, and none of it may reach the file: it is never on one of them, not even between its open
    // and a move above them.
    const auto claimed_with_closed = [](const std::string &name, int from, int to) {
        const std::string home = home_for(name), day = home + "/.satl/console/2026-10-20";
        std::fflush(stdout);
        const pid_t child = ::fork();
        if (child == 0) {
            for (int fd = from; fd <= to; ++fd)
                ::close(fd);
            std::atomic<bool> writing{false}, claimed{false};
            std::thread writer([&]() {
                while (!claimed.load()) {
                    for (int fd = from; fd <= to; ++fd)
                        (void)!::write(fd, "E", 1);
                    writing = true;
                }
            });
            while (!writing.load())
                ;
            forget_what_was_said();
            ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
            store.entered("x");
            claimed = true;
            writer.join();
            bool closed = true;
            for (int fd = from; fd <= to; ++fd)
                closed = closed && ::fcntl(fd, F_GETFD) == -1 && errno == EBADF;
            const std::vector<HeldName> kept = held_in(day);
            const int closed_at_exec = kept.size() == 1 ? ::fcntl(kept[0].fd, F_GETFD) : -1;
            const bool above = kept.size() == 1 && kept[0].name == "console-001.c4" && kept[0].fd > 2 &&
                               closed_at_exec >= 0 && (closed_at_exec & FD_CLOEXEC) != 0;
            ::_exit(closed && said == 0 && above && bytes_of(day + "/console-001.c4") == "x\n" ? 0 : 1);
        }
        int status = 0;
        const bool waited = child > 0 && ::waitpid(child, &status, 0) == child;
        return waited && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    };
    const int all_three = claimed_with_closed("descriptors_closed", 0, 2);
    check(all_three == 0,
          "with 0, 1 and 2 closed, the file is claimed above them, closed at exec, and none of the three is left "
          "open -- and nothing a thread writes into them meanwhile reaches it",
          "the child answered " + std::to_string(all_three));
    const int only_two = claimed_with_closed("descriptors_closed_2", 2, 2);
    check(only_two == 0,
          "with only 2 closed, as `SATL_NO_WINDOW=1 satl 2>&-` starts, the file is claimed above it, closed at exec, "
          "and 2 stays closed -- and nothing a thread writes into 2 meanwhile reaches it",
          "the child answered " + std::to_string(only_two));
}

// ---------------------------------------------------------------------------
// 10. THE CONSOLE'S NUMBER: one more than the highest in the day's folder (KEY-2)
// ---------------------------------------------------------------------------
// The earlier consoles' files are kept as they were found, each line and all.
bool kept_as_found(const std::string &day, const std::vector<std::string> &files)
{
    for (const std::string &file : files)
        if (bytes_of(day + "/" + file) != "a line an earlier console kept\n")
            return false;
    return true;
}

struct Claim {
    const char *name;
    const char *home;
    std::vector<std::string> there;
    const char *want;
};

void claimed_after(const Claim &one)
{
    const std::string day = a_day_holding(one.home, one.there);
    forget_what_was_said();
    ConsoleFiles store(on_2026_10_20, home_of(day).c_str(), kCache, count_it);
    store.entered("x");
    const std::vector<std::string> names = names_in(day);
    check(names == with(one.there, one.want) && kept_as_found(day, one.there) && said == 0, one.name,
          "the day holds " + listed(names) + ", " + what_was_said());
}

void the_number()
{
    for (const Claim &one : {
             Claim{"an empty folder gives console-001.c4", "number_empty", {}, "console-001.c4"},
             Claim{"001 and 003 give 004: a gap is never filled", "number_gap", {"console-001.c4", "console-003.c4"},
                   "console-004.c4"},
             Claim{"004, 009, 001 and 006 give 010, in whatever order the folder lists them", "number_same_length",
                   {"console-004.c4", "console-009.c4", "console-001.c4", "console-006.c4"}, "console-010.c4"},
             Claim{"999 gives console-1000.c4", "number_999", {"console-999.c4"}, "console-1000.c4"},
             Claim{"console-1000.c4 alone gives console-1001.c4: a number past 999 is read back as one",
                   "number_1000", {"console-1000.c4"}, "console-1001.c4"},
         })
        claimed_after(one);
}

// ---------------------------------------------------------------------------
// 11. WHAT IS NOT A CONSOLE'S IS LEFT ALONE, AND WHAT THE ODD NAMES MEAN (console_files.cpp's
//     console_number, which decides them)
// ---------------------------------------------------------------------------
void the_names()
{
    {
        const std::string day = a_day_holding("left_alone", {"console-7.txt"});
        ::mkdir((day + "/notes").c_str(), 0755);
        put(day + "/notes/inside", "the person's own\n");
        forget_what_was_said();
        ConsoleFiles store(on_2026_10_20, home_of(day).c_str(), kCache, count_it);
        store.entered("x");
        check(names_in(day) == std::vector<std::string>{"console-001.c4", "console-7.txt", "notes"} &&
                  kept_as_found(day, {"console-7.txt"}) && is_folder(day + "/notes") &&
                  names_in(day + "/notes") == std::vector<std::string>{"inside"} &&
                  bytes_of(day + "/notes/inside") == "the person's own\n" && said == 0,
              "a console-7.txt and a folder named notes are left alone, and the next is still 001",
              "the day holds " + listed(names_in(day)) + ", notes holds " + listed(names_in(day + "/notes")) + ", " +
                  what_was_said());
    }
    for (const Claim &one : {
             Claim{"a name that is not exactly console-<digits>.c4 counts for nothing, and is left alone",
                   "not_consoles",
                   {"Console-004.c4", "console-005.c4.bak", "console-.c4", "console-6a.c4", "console-+7.c4",
                    "console- 8.c4", "xconsole-9.c4", "console-10.C4", "console-1-.c4"},
                   "console-001.c4"},
             Claim{"console-01.c4 is console 1, so the next is 002", "zeros_in_front", {"console-01.c4"},
                   "console-002.c4"},
             Claim{"zeros in front count for nothing, however many: console-0000009.c4 and console-012.c4 give 013",
                   "many_zeros", {"console-0000009.c4", "console-012.c4"}, "console-013.c4"},
             Claim{"console-0.c4 is 0, so alone the next is 001", "number_zero", {"console-0.c4"}, "console-001.c4"},
             Claim{"no number is too large to count: 002 and twenty nines give a 1 and twenty zeros", "any_length",
                   {"console-002.c4", "console-99999999999999999999.c4"}, "console-100000000000000000000.c4"},
         })
        claimed_after(one);

    // A FOLDER AND A LINK WITH CONSOLES' NAMES count by their names, and nothing behind them is opened,
    // followed or written. They are the day's only names, so a claim that counted files alone would
    // start again at 001. The link points at a file of the room's, outside the day's folder.
    const std::string day = a_day_holding("folder_and_link", {});
    const std::string elsewhere = home_of(day) + "/elsewhere";
    put(elsewhere, "the person's own\n");
    ::mkdir((day + "/console-005.c4").c_str(), 0755);
    put(day + "/console-005.c4/inside", "the person's own\n");
    if (::symlink(elsewhere.c_str(), (day + "/console-006.c4").c_str()) != 0)
        check(false, "setup: a link at " + day + "/console-006.c4", std::strerror(errno));
    forget_what_was_said();
    {
        ConsoleFiles store(on_2026_10_20, home_of(day).c_str(), kCache, count_it);
        store.entered("x");
    }
    check(names_in(day) == std::vector<std::string>{"console-005.c4", "console-006.c4", "console-007.c4"} &&
              is_folder(day + "/console-005.c4") &&
              names_in(day + "/console-005.c4") == std::vector<std::string>{"inside"} &&
              bytes_of(day + "/console-005.c4/inside") == "the person's own\n" && is_link(day + "/console-006.c4") &&
              bytes_of(elsewhere) == "the person's own\n" && said == 0,
          "a folder named console-005.c4 and a link named console-006.c4 count by their names: the next is 007, and "
          "both are left as they were",
          "the day holds " + listed(names_in(day)) + ", what the link points at holds \"" + bytes_of(elsewhere) +
              "\", " + what_was_said());
}

// ---------------------------------------------------------------------------
// 12. CLAIMS AT ONCE IN ONE FOLDER: satls starting together (KEY-2)
// ---------------------------------------------------------------------------
// `count` STORES ON ONE HOME, as `count` satls would be -- each with a lock of its own, and none of the
// others' -- Entering at once, a thread each. Answers what the day's folder holds and what the stores
// hold open in it, both read before the stores go.
struct AtOnce {
    std::vector<std::string> names;
    std::vector<HeldName> held;
};

AtOnce claimed_at_once(const std::string &home, int count)
{
    std::vector<std::unique_ptr<ConsoleFiles>> stores;
    for (int i = 0; i < count; ++i)
        stores.push_back(std::make_unique<ConsoleFiles>(on_2026_10_20, home.c_str(), kCache, count_it));
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    for (const std::unique_ptr<ConsoleFiles> &store : stores)
        threads.emplace_back([&ready, &go, one = store.get()]() {
            ++ready;
            while (!go.load())
                ;
            one->entered("at once");
        });
    while (ready.load() < count)
        ;
    go = true;
    for (std::thread &thread : threads)
        thread.join();
    const std::string day = home + "/.satl/console/2026-10-20";
    return AtOnce{names_in(day), held_in(day)};
}

std::vector<std::string> numbered(int from, int to)
{
    std::vector<std::string> names;
    for (int number = from; number <= to; ++number) {
        char name[32];
        std::snprintf(name, sizeof name, "console-%03d.c4", number);
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// Every file in the folder held by one store, and no file by two.
bool each_held_once(const AtOnce &got)
{
    std::vector<std::string> held;
    for (const HeldName &one : got.held)
        held.push_back(one.name);
    return held == got.names;
}

void claims_at_once()
{
    // TWO, AS TWO satls STARTED TOGETHER, ROUND AFTER ROUND: in a round where both read the folder before
    // either claims, the second's 001 is taken by the first, and it must be passed over to 002 -- where a
    // claim that took EEXIST for S017 says it, and one without O_EXCL shares the first's file.
    const int rounds = 200;
    int wrong = 0;
    std::string first_wrong;
    for (int round = 0; round < rounds; ++round) {
        forget_what_was_said();
        const AtOnce got = claimed_at_once(home_for("two_at_once_" + std::to_string(round)), 2);
        if (said == 0 && got.names == numbered(1, 2) && each_held_once(got))
            continue;
        if (wrong++ == 0)
            first_wrong = "round " + std::to_string(round) + ": the day holds " + listed(got.names) + ", held " +
                          listed(got.held) + ", " + what_was_said();
    }
    check(wrong == 0, "two claims at once in one folder get two numbers, 001 and 002, in each of " +
                          std::to_string(rounds) + " rounds",
          std::to_string(wrong) + " round(s) wrong, the first " + first_wrong);

    forget_what_was_said();
    const AtOnce sixteen = claimed_at_once(home_for("sixteen_at_once"), 16);
    check(said == 0 && sixteen.names == numbered(1, 16) && each_held_once(sixteen),
          "sixteen claims at once in one folder get sixteen numbers, 001 to 016, each held by its own store",
          "the day holds " + listed(sixteen.names) + ", held " + listed(sixteen.held) + ", " + what_was_said());
}

// ---------------------------------------------------------------------------
// 13. THE FILE: 0600 whatever the umask, held open for the run, and nothing in it (KEY-2)
// ---------------------------------------------------------------------------
void the_file()
{
    // 0600 UNDER THREE UMASKS: the common 022, this machine's 002, and none at all -- under which a file
    // made 0644 or 0666 keeps every bit it was made with.
    bool every_one = true;
    std::string modes;
    forget_what_was_said();
    for (const mode_t mask : {mode_t(022), mode_t(002), mode_t(000)}) {
        const std::string day = a_day_holding("umask_" + octal(mask), {});
        ::umask(mask);
        {
            ConsoleFiles store(on_2026_10_20, home_of(day).c_str(), kCache, count_it);
            store.entered("x");
        }
        ::umask(022);
        const std::string file = day + "/console-001.c4";
        struct stat about {};
        every_one = every_one && ::lstat(file.c_str(), &about) == 0 && S_ISREG(about.st_mode) && mode_of(file) == 0600;
        modes += (modes.empty() ? "" : ", ") + std::string("under ") + octal(mask) + " it is " + octal(mode_of(file));
    }
    check(every_one && said == 0, "the file is 0600, under a umask of 022, of 002 and of 000",
          modes + ", " + what_was_said());

    // HELD OPEN FOR THE RUN: write-only, appended to, and closed at exec, so no satl started from this one
    // holds it -- and holding the line its first Enter wrote (KEY-3), and nothing else.
    const std::string day = a_day_holding("held_open", {});
    forget_what_was_said();
    {
        ConsoleFiles store(on_2026_10_20, home_of(day).c_str(), kCache, count_it);
        store.entered("x");
        const std::vector<HeldName> held = held_in(day);
        const int status = held.size() == 1 ? ::fcntl(held[0].fd, F_GETFL) : -1;
        const int closed_at_exec = held.size() == 1 ? ::fcntl(held[0].fd, F_GETFD) : -1;
        check(held.size() == 1 && held[0].name == "console-001.c4" && held[0].fd > 2 && status >= 0 &&
                  (status & O_ACCMODE) == O_WRONLY && (status & O_APPEND) != 0 && closed_at_exec >= 0 &&
                  (closed_at_exec & FD_CLOEXEC) != 0 && bytes_of(day + "/console-001.c4") == "x\n" && said == 0,
              "the file is held open for the run, above 2: write-only, appended to, closed at exec, and holding "
              "its first Enter's line",
              "held " + listed(held) + ", its flags " + std::to_string(status) + " and " +
                  std::to_string(closed_at_exec) + ", \"" + shown(bytes_of(day + "/console-001.c4")) + "\", " +
                  what_was_said());
    }
    check(held_in(day).empty() && names_in(day) == std::vector<std::string>{"console-001.c4"},
          "... and closed when the store goes, the file left where it is",
          "held " + listed(held_in(day)) + ", the day holds " + listed(names_in(day)));
}

// ---------------------------------------------------------------------------
// 14. A FILE THAT CANNOT BE CLAIMED IS S017, AND NOTHING MORE IS TRIED (KEY-2, rule 8)
// ---------------------------------------------------------------------------
void a_file_not_claimed()
{
    const std::string name = "a day's folder the file cannot be made in is S017, said once, and nothing more is tried";
    if (::geteuid() == 0) {
        skipped(name, "root writes anyway");
    } else {
        const std::string day = a_day_holding("not_claimed", {});
        ::chmod(day.c_str(), 0500);
        forget_what_was_said();
        ConsoleFiles store(on_2026_10_20, home_of(day).c_str(), kCache, count_it);
        store.entered("x");
        const bool refused = said == 1 && said_last.code == "S017" && said_last.directory == day + "/console-001.c4" &&
                             said_last.description.find("could not be made (Permission denied)") != std::string::npos &&
                             names_in(day).empty() && held_in(day).empty();
        const std::string first = what_was_said();
        // WRITABLE AGAIN, AND THE NEXT ENTER STILL MAKES NOTHING: the store tries the disk no more this run.
        // Both lines are kept in memory all the same (KEY-3).
        ::chmod(day.c_str(), 0700);
        store.entered("y");
        check(refused && said == 1 && names_in(day).empty() && held_in(day).empty() &&
                  kept_lines(store.lines()) == std::vector<std::string>{"x", "y"},
              name + ", and both lines are kept in memory",
              first + "; after it " + what_was_said() + ", the day holds " + listed(names_in(day)) + ", the lines " +
                  listed_lines(kept_lines(store.lines())));
    }
}

// ---------------------------------------------------------------------------
// 15. EVERY ENTER IS KEPT (KEY-3): a satellite.variable.string among the console's lines, and a line of
//     the console's file -- one write(2), with its newline, before the store answers
// ---------------------------------------------------------------------------
// A STORE ON A DAY'S FOLDER OF ITS OWN, and where its file will be.
struct Kept {
    std::string file;
    std::unique_ptr<ConsoleFiles> store;
};

Kept a_store_in(const std::string &name)
{
    const std::string day = a_day_holding(name, {});
    forget_what_was_said();
    return Kept{day + "/console-001.c4", std::make_unique<ConsoleFiles>(on_2026_10_20, home_of(day).c_str(), kCache, count_it)};
}

void the_rule_3_line()
{
    Kept kept = a_store_in("rule_3_line");
    // EACH ONE THAT GOES STANDS JUST BEFORE ONE THAT STAYS, so a filter that took a neighbour with it is
    // caught: ESC before the c, as though it began a sequence; 0xFF before the emoji's first byte, as
    // though it were a first byte owed three more; U+009B, the 8-bit CSI, before the four spaces, as
    // though they were its parameters.
    kept.store->entered("\x1b" "caf\xC3\xA9" "\xFF" "\xF0\x9F\x98\x80" "\xC2\x9B" "    ");
    const std::string wanted = "caf\xC3\xA9" "\xF0\x9F\x98\x80" "    ";
    const satellite_string line = kept.store->lines().at(0);
    check(kept.store->lines().size() == 1 && line.to_utf8() == wanted && line.size() == 9 &&
              bytes_of(kept.file) == wanted + "\n" && said == 0,
          "a line holding caf\xC3\xA9, an emoji, ESC, a lone 0xFF, U+009B and four spaces keeps caf\xC3\xA9, the emoji and "
          "the spaces, and nothing else -- nine characters of a satellite.variable.string, and the same bytes in the file",
          "the lines are " + listed_lines(kept_lines(kept.store->lines())) + " (" + std::to_string(line.size()) +
              " characters), the file \"" + shown(bytes_of(kept.file)) + "\", " + what_was_said());
}

void the_files_lines()
{
    Kept kept = a_store_in("files_lines");
    ConsoleFiles &store = *kept.store;
    // BY BOTH WAYS A LINE COMES, turn about: as the reader brings one -- added to the lines it was handed,
    // the store's, then handed over with them (line_reader.cpp) -- and as a file run's getline does, with
    // nothing kept yet (console_calls.cpp). A line of spaces and a repeat are lines like any other.
    const std::vector<std::string> entered = {"satellite.console.display(\"one\")", "    ", "same", "same",
                                              "d\xC3\xA9j\xC3\xA0 vu", "a\tb\x1b[31mc", "\xF0\x9F\x9A\x80 go", "last"};
    const std::vector<std::string> wanted = {"satellite.console.display(\"one\")", "    ", "same", "same",
                                             "d\xC3\xA9j\xC3\xA0 vu", "ab[31mc", "\xF0\x9F\x9A\x80 go", "last"};
    bool there_at_once = true;   // checked after every Enter, the store still holding the file open
    for (std::size_t index = 0; index < entered.size(); ++index) {
        if (index % 2 == 0) {
            store.lines().add(entered[index]);
            store.entered(entered[index], &store.lines());
        } else {
            store.entered(entered[index]);
        }
        there_at_once = there_at_once && bytes_of(kept.file) == as_a_file(kept_lines(store.lines()));
    }
    const std::vector<std::string> lines = kept_lines(store.lines());
    check(lines == wanted && bytes_of(kept.file) == as_a_file(lines) && there_at_once && said == 0,
          "the file's lines are the console's lines, by the reader's way and by getline's, each in the file as "
          "soon as it is Entered -- nothing waits in a buffer",
          "the lines are " + listed_lines(lines) + ", the file \"" + shown(bytes_of(kept.file)) + "\"" +
              (there_at_once ? "" : ", and a line was not in the file when its Enter returned") + ", " +
              what_was_said());
}

void an_empty_enter()
{
    Kept kept = a_store_in("empty_enter");
    kept.store->entered("before");
    kept.store->entered("");
    kept.store->entered("after");
    check(kept_lines(kept.store->lines()) == std::vector<std::string>{"before", "", "after"} &&
              kept.store->lines().at(1).empty() && bytes_of(kept.file) == "before\n\nafter\n" && said == 0,
          "an empty Enter is an empty line: an empty satellite.variable.string, and an empty line of the file",
          "the lines are " + listed_lines(kept_lines(kept.store->lines())) + ", the file \"" +
              shown(bytes_of(kept.file)) + "\"");
}

// THE WRITES THIS PROCESS HAS MADE, as /proc/self/io counts them: every write(2) a call; -1 when it
// cannot be read.
long long writes_made()
{
    const std::string io = bytes_of("/proc/self/io");
    const std::size_t at = io.find("syscw: ");
    return at == std::string::npos ? -1 : std::atoll(io.c_str() + at + 7);
}

void one_write_a_line()
{
    if (writes_made() < 0) {
        skipped("each Enter is one write(2), its newline in it", "/proc/self/io cannot be read");
        return;
    }
    Kept kept = a_store_in("one_write");
    // THE FIRST ENTER CLAIMS THE FILE AS WELL, AND CLAIMING WRITES NOTHING; a line of a megabyte is one
    // write to a file on a disk as much as a short one is. Nothing else here writes meanwhile: the
    // harness's own lines wait in stdout's buffer until the check.
    const std::vector<std::string> entered = {"the first, which claims the file", "", std::string(1 << 20, 'm'), "last"};
    std::vector<long long> each;
    for (const std::string &line : entered) {
        const long long before = writes_made();
        kept.store->entered(line);
        each.push_back(writes_made() - before);
    }
    std::string counted;
    for (const long long writes : each)
        counted += (counted.empty() ? "" : ", ") + std::to_string(writes);
    check(each == std::vector<long long>{1, 1, 1, 1} && bytes_of(kept.file) == as_a_file(entered) && said == 0,
          "each Enter is one write(2), its newline in it -- the first, an empty one, and a megabyte alike",
          "the four Enters made " + counted + " writes, " + what_was_said());
}

void kept_once()
{
    Kept kept = a_store_in("kept_once");
    ConsoleFiles &store = *kept.store;
    // THE READER'S WAY: added to the lines it was handed, the store's, then handed over with them.
    store.lines().add("by the reader");
    store.entered("by the reader", &store.lines());
    // GETLINE'S WAY: nothing has kept it, so the store adds it.
    store.entered("by getline");
    // A READER HANDED LINES OF ITS OWN, as prompt_cases hands one: they are not the store's, so the store
    // keeps the line among its own as well.
    prompt::ConsoleLines another;
    another.add("by another reader");
    store.entered("by another reader", &another);
    check(kept_lines(store.lines()) == std::vector<std::string>{"by the reader", "by getline", "by another reader"} &&
              another.size() == 1 && bytes_of(kept.file) == "by the reader\nby getline\nby another reader\n" &&
              said == 0,
          "a line is kept once and written once, whichever way it came: the reader's, getline's, or a reader's "
          "with lines of its own",
          "the store's lines are " + listed_lines(kept_lines(store.lines())) + ", the other reader's " +
              std::to_string(another.size()) + ", the file \"" + shown(bytes_of(kept.file)) + "\"");
}

void lines_from_threads()
{
    Kept kept = a_store_in("lines_threads");
    ConsoleFiles &store = *kept.store;
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    for (int thread = 0; thread < 16; ++thread)
        threads.emplace_back([&, thread]() {
            ++ready;
            while (!go.load())
                ;
            for (int line = 0; line < 100; ++line)
                store.entered(std::to_string(thread) + " " + std::to_string(line) + " " + std::string(200, 'x'));
        });
    while (ready.load() < 16)
        ;
    go = true;
    for (std::thread &thread : threads)
        thread.join();
    // EACH THREAD'S LINES IN ITS OWN ORDER, AND THE FILE THE SAME LINES IN THE SAME ORDER: kept and written
    // under one lock, a line never crosses another in the file, nor comes in a different place from memory.
    const std::vector<std::string> lines = kept_lines(store.lines());
    std::vector<int> next(16, 0);
    bool in_order = lines.size() == 1600;
    for (const std::string &line : lines) {
        int thread = -1, number = -1;
        in_order = in_order && std::sscanf(line.c_str(), "%d %d", &thread, &number) == 2 && thread >= 0 &&
                   thread < 16 && number == next[thread]++ && line.size() > 200;
    }
    check(said == 0 && in_order && bytes_of(kept.file) == as_a_file(lines),
          "sixteen threads Entering a hundred lines each at once: 1,600 lines, each thread's in its order, and the "
          "file holds the same lines in the same order, none crossing another",
          std::to_string(lines.size()) + " lines, " + (in_order ? "in order" : "out of order") + ", the file " +
              (bytes_of(kept.file) == as_a_file(lines) ? "the same" : "different") + ", " + what_was_said());
}

void a_write_that_fails()
{
    Kept kept = a_store_in("write_fails");
    ConsoleFiles &store = *kept.store;
    const std::string day = kept.file.substr(0, kept.file.rfind('/'));
    store.entered("before the disk filled");
    const std::vector<HeldName> held = held_in(day);
    // THE DISK FULL, FOR REAL, AND NOTHING FILLED: /dev/full answers every write ENOSPC, and it is put on
    // the store's own descriptor, where its file was.
    const int full = ::open("/dev/full", O_WRONLY | O_CLOEXEC);
    const bool on_full = held.size() == 1 && full >= 0 && ::dup3(full, held[0].fd, O_CLOEXEC) == held[0].fd;
    if (full >= 0)
        ::close(full);
    store.entered("the line the full disk refused");
    const int said_then = said;
    const CriticalReport refused = said_last;
    // ITS FILE PUT BACK ON THE SAME DESCRIPTOR: a store that wrote on after S017 would be seen in it now.
    const int again = ::open(kept.file.c_str(), O_WRONLY | O_APPEND | O_CLOEXEC);
    const bool back = held.size() == 1 && again >= 0 && ::dup3(again, held[0].fd, O_CLOEXEC) == held[0].fd;
    if (again >= 0)
        ::close(again);
    store.entered("a line after it");
    store.entered("and one more");
    const std::string why = std::string("a line could not be written into the console's file (") + std::strerror(ENOSPC) + ")";
    check(on_full && back && said_then == 1 && said == 1 && refused.code == "S017" &&
              refused.directory == kept.file && refused.description.find(why) == 0 &&
              bytes_of(kept.file) == "before the disk filled\n" &&
              kept_lines(store.lines()) == std::vector<std::string>{"before the disk filled", "the line the full disk refused",
                                                                    "a line after it", "and one more"},
          "after a failed write -- /dev/full on the file's descriptor -- S017 is said once, naming the file, nothing "
          "more is written even with the file back, and every line is still kept in memory",
          what_was_said() + " (" + std::to_string(said_then) + " at the failure), the file \"" +
              shown(bytes_of(kept.file)) + "\", the lines " + listed_lines(kept_lines(store.lines())));
}

volatile sig_atomic_t alarms = 0;

void on_alarm(int)
{
    alarms = alarms + 1;
}

void a_write_interrupted()
{
    Kept kept = a_store_in("write_interrupted");
    ConsoleFiles &store = *kept.store;
    const std::string day = kept.file.substr(0, kept.file.rfind('/'));
    store.entered("first");
    const std::vector<HeldName> held = held_in(day);
    // A PIPE ON THE FILE'S DESCRIPTOR, made as small as a pipe can be, and read slowly, four KB at a time
    // with a rest between: the write blocks again and again, and a signal every 0.2 ms with no SA_RESTART
    // interrupts it -- with part of the line taken, a short write; with none, EINTR. A store that took
    // either for the end would lose the rest of the line and say S017.
    int ends[2];
    if (held.size() != 1 || ::pipe2(ends, O_CLOEXEC) != 0) {
        check(false, "setup: a pipe on the console file's descriptor", std::strerror(errno));
        return;
    }
    ::fcntl(ends[1], F_SETPIPE_SZ, 4096);
    const bool on_pipe = ::dup3(ends[1], held[0].fd, O_CLOEXEC) == held[0].fd;
    ::close(ends[1]);
    std::string line(1 << 18, ' ');
    for (std::size_t at = 0; at < line.size(); ++at)
        line[at] = static_cast<char>('a' + at % 26);
    std::string got;
    std::thread reader([&]() {
        sigset_t alarm;
        sigemptyset(&alarm);
        sigaddset(&alarm, SIGALRM);
        pthread_sigmask(SIG_BLOCK, &alarm, nullptr);   // the signals are the writer's
        char piece[4096];
        while (got.size() < line.size() + 1) {
            const ssize_t read = ::read(ends[0], piece, sizeof piece);
            if (read <= 0)
                break;
            got.append(piece, static_cast<std::size_t>(read));
            ::usleep(500);
        }
    });
    struct sigaction interrupting {}, before {};
    interrupting.sa_handler = on_alarm;
    sigemptyset(&interrupting.sa_mask);
    interrupting.sa_flags = 0;   // no SA_RESTART: a blocked write comes back early
    ::sigaction(SIGALRM, &interrupting, &before);
    alarms = 0;
    itimerval every{{0, 200}, {0, 200}}, off{};
    ::setitimer(ITIMER_REAL, &every, nullptr);
    store.entered(line);
    ::setitimer(ITIMER_REAL, &off, nullptr);
    const int landed = alarms;
    // IGNORED BEFORE IT IS PUT BACK, which drops one still pending: its default would end the harness.
    ::signal(SIGALRM, SIG_IGN);
    ::sigaction(SIGALRM, &before, nullptr);
    // THE FILE BACK ON ITS DESCRIPTOR, which closes the pipe's last writing end: the reader sees the end
    // of it, so a store that stopped short of the line fails this case instead of leaving it waiting.
    const int again = ::open(kept.file.c_str(), O_WRONLY | O_APPEND | O_CLOEXEC);
    if (again >= 0) {
        ::dup3(again, held[0].fd, O_CLOEXEC);
        ::close(again);
    }
    reader.join();
    ::close(ends[0]);
    check(on_pipe && said == 0 && got == line + "\n" && landed >= 10,
          "a line of 256 KB written into a pipe that fills, with a signal every 0.2 ms, arrives whole: the write is "
          "retried on EINTR and finished after each short write",
          std::to_string(got.size()) + " of " + std::to_string(line.size() + 1) + " bytes arrived" +
              (got == line + "\n" || got.empty() ? "" : got.compare(0, got.size(), line, 0, got.size()) == 0
                                                             ? " as written"
                                                             : " out of order") +
              ", " + std::to_string(landed) + " signals landed, " + what_was_said());
}

// THE FILTER BY ANOTHER ROAD, to hold what_is_kept to: each character's number decoded first and judged
// by the number -- fewer bytes than it needs is overlong, D800 to DFFF a surrogate, past 10FFFF too
// large -- where what_is_kept judges by Table 3-7's byte ranges. A byte that starts no whole character
// goes alone; a control character goes whole.
std::string kept_by_number(const std::string &entered)
{
    static const char32_t least[] = {0, 0, 0x80, 0x800, 0x10000};
    const unsigned char *const bytes = reinterpret_cast<const unsigned char *>(entered.data());
    const std::size_t size = entered.size();
    std::string kept;
    for (std::size_t at = 0; at < size;) {
        const unsigned char first = bytes[at];
        const std::size_t length = first < 0x80             ? 1
                                   : (first & 0xE0) == 0xC0 ? 2
                                   : (first & 0xF0) == 0xE0 ? 3
                                   : (first & 0xF8) == 0xF0 ? 4
                                                            : 0;
        char32_t code = length == 1 ? first : length == 2 ? (first & 0x1Fu) : length == 3 ? (first & 0x0Fu) : (first & 0x07u);
        bool whole = length != 0 && at + length <= size;
        for (std::size_t after = 1; whole && after < length; ++after) {
            whole = (bytes[at + after] & 0xC0) == 0x80;
            code = (code << 6) | (bytes[at + after] & 0x3Fu);
        }
        if (!whole || code < least[length] || (code >= 0xD800 && code <= 0xDFFF) || code > 0x10FFFF) {
            ++at;
            continue;
        }
        if (!(code < 0x20 || (code >= 0x7F && code <= 0x9F)))
            kept.append(entered, at, length);
        at += length;
    }
    return kept;
}

// ONE LINE THROUGH BOTH: the same answer, which from_utf8 takes and to_utf8 gives back byte for byte.
bool filtered_alike(const std::string &entered, std::string &wrong)
{
    const std::string kept = prompt::what_is_kept(entered), wanted = kept_by_number(entered);
    satellite_string line;
    std::size_t bad = 0;
    const bool taken = satellite_string::from_utf8(kept, line, bad) == 0;
    if (kept == wanted && taken && line.to_utf8() == kept)
        return true;
    if (wrong.empty())
        wrong = "\"" + shown(entered) + "\" kept \"" + shown(kept) + "\", by number \"" + shown(wanted) + "\"" +
                (taken ? "" : ", and from_utf8 refused it at byte " + std::to_string(bad));
    return false;
}

void the_filter()
{
    std::string c0, c1;
    for (int code = 0; code < 0x20; ++code)
        c0 += static_cast<char>(code);
    c0 += '\x7f';
    for (int code = 0x80; code <= 0x9F; ++code)
        c1 += std::string("\xC2") + static_cast<char>(code);
    struct Filtered {
        const char *name;
        std::string entered, wanted;
    };
    for (const Filtered &one : {
             Filtered{"a TAB, a CR, a NUL and DEL go, and the letters between them stay",
                      std::string("a\tb\rc\0d\x7f" "e", 9), "abcde"},
             Filtered{"all 33 of U+0000 to U+001F and U+007F go", c0 + "ok", "ok"},
             Filtered{"all 32 of U+0080 to U+009F go, and U+00A0 after them stays", c1 + "\xC2\xA0", "\xC2\xA0"},
             Filtered{"a stray continuation byte goes alone", "\x80" "a" "\xBF" "b", "ab"},
             Filtered{"a first byte with too few continuations goes alone, and the character after it stays",
                      "\xE2\x82" "A" "\xF0\x9F\x98" "\xC3\xA9", "A\xC3\xA9"},
             Filtered{"overlong forms go: C0 80, C1 BF, E0 80 80, E0 9F BF, F0 80 80 80, F0 8F BF BF",
                      "\xC0\x80" "\xC1\xBF" "\xE0\x80\x80" "\xE0\x9F\xBF" "\xF0\x80\x80\x80" "\xF0\x8F\xBF\xBF" "x", "x"},
             Filtered{"the shortest forms stay: U+00A0, U+0800, U+10000", "\xC2\xA0" "\xE0\xA0\x80" "\xF0\x90\x80\x80",
                      "\xC2\xA0" "\xE0\xA0\x80" "\xF0\x90\x80\x80"},
             Filtered{"a surrogate goes, ED A0 80 and ED BF BF, and U+D7FF and U+E000 either side of them stay",
                      "\xED\xA0\x80" "\xED\xBF\xBF" "\xED\x9F\xBF" "\xEE\x80\x80", "\xED\x9F\xBF" "\xEE\x80\x80"},
             Filtered{"past U+10FFFF goes, F4 90 80 80, F5 80 80 80 and F7 BF BF BF, and U+10FFFF itself stays",
                      "\xF4\x90\x80\x80" "\xF5\x80\x80\x80" "\xF7\xBF\xBF\xBF" "\xF4\x8F\xBF\xBF", "\xF4\x8F\xBF\xBF"},
             Filtered{"F5 to FF go, each alone", "\xF5" "a" "\xF8" "b" "\xFE" "c" "\xFF" "d", "abcd"},
             Filtered{"U+9C40, whose number is satellite_string's own wide value, stays", "\xE9\xB1\x80", "\xE9\xB1\x80"},
         }) {
        prompt::ConsoleLines lines;
        const std::string answered = lines.add(one.entered);
        check(answered == one.wanted && lines.at(0).to_utf8() == one.wanted &&
                  prompt::what_is_kept(one.entered) == one.wanted,
              std::string("the filter: ") + one.name,
              "\"" + shown(one.entered) + "\" kept \"" + shown(answered) + "\", the line holds \"" +
                  shown(lines.at(0).to_utf8()) + "\"");
    }
    {
        prompt::ConsoleLines lines;
        lines.add("\xE9\xB1\x80");
        check(lines.at(0).size() == 1, "the filter: ... and is one character of the satellite.variable.string",
              std::to_string(lines.at(0).size()) + " characters");
    }

    // EVERY LINE OF ONE AND TWO BYTES, THEN A MILLION OF ONE TO EIGHT, against the filter by number. Half
    // the million's bytes are drawn from the edges of Table 3-7's ranges and the controls, the rest from
    // all 256; the draw is a fixed xorshift, so a failure comes back the same.
    std::string wrong;
    long long alike = 0, lines_wrong = 0;
    prompt::ConsoleLines two_bytes;
    for (int first = 0; first < 256; ++first) {
        const std::string one(1, static_cast<char>(first));
        alike += filtered_alike(one, wrong);
        for (int second = 0; second < 256; ++second) {
            const std::string both = one + static_cast<char>(second);
            alike += filtered_alike(both, wrong);
            lines_wrong += two_bytes.add(both) != prompt::what_is_kept(both);
        }
    }
    static const unsigned char edges[] = {0x00, 0x09, 0x0A, 0x0D, 0x1B, 0x1F, 0x20, 'a', 0x7E, 0x7F, 0x80, 0x8F, 0x90,
                                          0x9F, 0xA0, 0xBF, 0xC0, 0xC1, 0xC2, 0xDF, 0xE0, 0xE1, 0xEC, 0xED, 0xEE, 0xEF,
                                          0xF0, 0xF1, 0xF3, 0xF4, 0xF5, 0xF7, 0xF8, 0xFF};
    unsigned long long state = 0x9E3779B97F4A7C15ull;
    const auto next = [&state]() {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    };
    for (int count = 0; count < 1000000; ++count) {
        std::string drawn(1 + next() % 8, '\0');
        for (char &byte : drawn) {
            const unsigned long long pick = next();
            byte = static_cast<char>((pick & 1) != 0 ? edges[(pick >> 1) % sizeof edges] : (pick >> 8) & 0xFF);
        }
        alike += filtered_alike(drawn, wrong);
    }
    const long long all = 256 + 65536 + 1000000;
    check(alike == all && lines_wrong == 0 && two_bytes.size() == 65536,
          "the filter keeps exactly what a decoder by number keeps -- every line of one and two bytes and a million "
          "drawn of one to eight -- and from_utf8 takes all it keeps, to_utf8 giving the same bytes back",
          std::to_string(all - alike) + " of " + std::to_string(all) + " differ, the first " + wrong + "; " +
              std::to_string(lines_wrong) + " lines kept otherwise by ConsoleLines");
}

// ---------------------------------------------------------------------------
// 16. THE WALK (KEY-5): the dated folders and the console files in them, newest first
// ---------------------------------------------------------------------------

// A CONSOLE FILE AN EARLIER CONSOLE LEFT, folders made as the store makes them: ~/.satl/console/<day>/<name>.
void left_by_an_earlier_console(const std::string &home, const std::string &day, const std::string &name,
                                const std::string &bytes = "a line an earlier console kept\n")
{
    ::mkdir((home + "/.satl").c_str(), 0755);
    ::mkdir((home + "/.satl/console").c_str(), 0700);
    ::mkdir((home + "/.satl/console/" + day).c_str(), 0700);
    if (!put(home + "/.satl/console/" + day + "/" + name, bytes))
        check(false, "setup: " + day + "/" + name, std::strerror(errno));
}

// THE WALK'S LIST AS day/name, newest first.
std::vector<std::string> walked_names(const ConsoleFiles &store)
{
    std::vector<std::string> names;
    for (const ConsoleFiles::Found &found : store.walked())
        names.push_back(found.day + "/" + found.name);
    return names;
}

std::string in_order(const std::vector<std::string> &names)
{
    std::string out = "[";
    for (const std::string &name : names)
        out += (out.size() > 1 ? ", " : "") + name;
    return out + "]";
}

void the_walk()
{
    const std::string home = home_for("walk_order");
    for (const char *day : {"2026-12-30", "2026-12-31", "2027-01-03"})
        left_by_an_earlier_console(home, day, "console-001.c4");
    left_by_an_earlier_console(home, "2027-01-05", "console-001.c4");
    left_by_an_earlier_console(home, "2027-01-05", "console-002.c4");
    forget_what_was_said();
    ConsoleFiles store(on_2027_01_05, home.c_str(), kCache, count_it);
    const std::vector<std::string> walked = walked_names(store);
    std::vector<std::string> days;
    for (const ConsoleFiles::Found &found : store.walked())
        if (days.empty() || days.back() != found.day)
            days.push_back(found.day);
    check(days == std::vector<std::string>{"2027-01-05", "2027-01-03", "2026-12-31", "2026-12-30"} && said == 0,
          "on 2027-01-05, the order is 2027-01-05, 2027-01-03, 2026-12-31, 2026-12-30",
          "the walk's list is " + in_order(walked) + ", " + what_was_said());
    check(walked.size() == 5 && walked[0] == "2027-01-05/console-002.c4" && walked[1] == "2027-01-05/console-001.c4",
          "console-002 comes before console-001", "the walk's list is " + in_order(walked));
    check(names_in(home + "/.satl/console/2027-01-05") == std::vector<std::string>{"console-001.c4", "console-002.c4"},
          "the walk writes nothing, and the store claims nothing before the first Enter",
          "the day holds " + listed(names_in(home + "/.satl/console/2027-01-05")));
    store.entered("this console's first line");
    check(walked_names(store) == walked && store.earlier().size() == 5,
          "this console's own file is never on the walk's list: the walk ran before it was claimed",
          "the walk's list is now " + in_order(walked_names(store)));

    // WHAT IS NOT A CONSOLE'S, OR IS REACHED THROUGH A LINK, IS NOT LISTED: a day's folder that is a link, a
    // console file that is a link, a stray file, a folder named notes and one named 10-20 -- each holding
    // a console file -- a file with a day's name, a folder with a console's name, and console-7.txt.
    const std::string odd = home_for("walk_odd"), console = odd + "/.satl/console", elsewhere = odd + "/elsewhere";
    left_by_an_earlier_console(odd, "2026-10-20", "console-001.c4");
    left_by_an_earlier_console(odd, "notes", "console-001.c4");
    left_by_an_earlier_console(odd, "10-20", "console-001.c4");
    left_by_an_earlier_console(odd, "2026-10-200", "console-001.c4");
    ::mkdir(elsewhere.c_str(), 0700);
    put(elsewhere + "/console-001.c4", "reached through a link\n");
    ::symlink(elsewhere.c_str(), (console + "/2026-10-19").c_str());
    ::symlink((elsewhere + "/console-001.c4").c_str(), (console + "/2026-10-20/console-002.c4").c_str());
    put(console + "/stray.c4", "a stray file\n");
    put(console + "/2026-10-18", "a file with a day's name\n");
    put(console + "/2026-10-20/console-7.txt", "not a console's\n");
    ::mkdir((console + "/2026-10-20/console-003.c4").c_str(), 0700);
    ConsoleFiles strays(on_2026_10_20, odd.c_str(), kCache, count_it);
    check(walked_names(strays) == std::vector<std::string>{"2026-10-20/console-001.c4"} && said == 0,
          "a link, a stray file, a folder named notes and a folder named 10-20 are not listed",
          "the walk's list is " + in_order(walked_names(strays)) + ", " + what_was_said());

    // A ROOT THAT IS A LINK IS NOT WALKED, and a store with no HOME walks nothing -- and neither says S017,
    // which is the first Enter's to say.
    const std::string linked = home_for("walk_linked_root"), real = linked + "/real_console";
    left_by_an_earlier_console(linked, "2026-10-20", "console-001.c4");
    ::rename((linked + "/.satl/console").c_str(), real.c_str());
    ::symlink(real.c_str(), (linked + "/.satl/console").c_str());
    ConsoleFiles through(on_2026_10_20, linked.c_str(), kCache, count_it);
    ConsoleFiles no_home(on_2026_10_20, nullptr, kCache, count_it);
    check(through.walked().empty() && no_home.walked().empty() && through.earlier().size() == 0 && said == 0,
          "a root that is a link, and a store with no HOME, list nothing and say nothing",
          std::to_string(through.walked().size()) + " and " + std::to_string(no_home.walked().size()) + " listed, " +
              what_was_said());
}

// ---------------------------------------------------------------------------
// 17. READ BACK (KEY-5): through KEY-3's filter, and only while it is the file the walk listed
// ---------------------------------------------------------------------------
std::vector<std::string> lines_of(const prompt::ConsoleLines *lines)
{
    return lines != nullptr ? kept_lines(*lines) : std::vector<std::string>{"<passed over>"};
}

void read_back()
{
    const std::string home = home_for("read_back");
    left_by_an_earlier_console(home, "2026-10-19", "console-001.c4", "\x1b" "caf\xC3\xA9" "\xFF" "\n");
    left_by_an_earlier_console(home, "2026-10-20", "console-001.c4", "first\n\nlast, with no newline");
    forget_what_was_said();
    ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
    const prompt::ConsoleLines *newest = store.earlier().lines(0), *older = store.earlier().lines(1);
    check(lines_of(older) == std::vector<std::string>{"caf\xC3\xA9"},
          "a file holding ESC, caf\xC3\xA9 and a lone 0xFF reads back as caf\xC3\xA9",
          "it read back as " + listed_lines(lines_of(older)));
    check(lines_of(newest) == std::vector<std::string>{"first", "", "last, with no newline"} &&
              newest->stops() == 2,
          "one line to each newline, an empty line kept, and the piece after the last newline a line too",
          "it read back as " + listed_lines(lines_of(newest)));
    ::unlink((home + "/.satl/console/2026-10-20/console-001.c4").c_str());
    check(store.earlier().lines(0) == newest && lines_of(store.earlier().lines(0)).size() == 3 && said == 0,
          "a console read once stays read for the run, though its file goes",
          "it is now " + listed_lines(lines_of(store.earlier().lines(0))) + ", " + what_was_said());

    // REMOVED AFTER THE WALK, OR REPLACED BY ANOTHER OF ITS NAME -- written beside it and renamed over it,
    // so it is another inode -- is passed over; the console beside them is read as it was.
    const std::string gone = home_for("read_back_gone"), day = gone + "/.satl/console/2026-10-20";
    left_by_an_earlier_console(gone, "2026-10-20", "console-001.c4", "the walk listed this one\n");
    left_by_an_earlier_console(gone, "2026-10-20", "console-002.c4", "removed after the walk\n");
    left_by_an_earlier_console(gone, "2026-10-20", "console-003.c4", "left as it was\n");
    ConsoleFiles after(on_2026_10_20, gone.c_str(), kCache, count_it);
    ::unlink((day + "/console-002.c4").c_str());
    put(day + "/replacement", "put in its place\n");
    ::rename((day + "/replacement").c_str(), (day + "/console-001.c4").c_str());
    check(after.earlier().size() == 3 && lines_of(after.earlier().lines(0)) == std::vector<std::string>{"left as it was"} &&
              after.earlier().lines(1) == nullptr && after.earlier().lines(2) == nullptr && said == 0,
          "a file removed after the walk, or replaced by another of its name, is passed over",
          "console-003 read back as " + listed_lines(lines_of(after.earlier().lines(0))) + ", console-002 as " +
              listed_lines(lines_of(after.earlier().lines(1))) + ", console-001 as " +
              listed_lines(lines_of(after.earlier().lines(2))) + ", " + what_was_said());
}

// ---------------------------------------------------------------------------
// 18. LEFT'S LIST (KEY-6): the newest consoles up to the cache; the walk's list is all of them
// ---------------------------------------------------------------------------
void lefts_list()
{
    // 40 MB: forty consoles of 1 mb each, ten a day over four days, each a line naming it and then
    // nothing -- a file of holes past its line, so the room takes a few blocks and not 40 MB.
    const std::string home = home_for("forty_mb");
    std::vector<std::string> newest_first;
    for (int day = 4; day >= 1; --day)
        for (int number = 10; number >= 1; --number) {
            const std::string folder = "2026-10-1" + std::to_string(day);
            const std::string name = std::string("console-0") + (number < 10 ? "0" : "") + std::to_string(number) + ".c4";
            left_by_an_earlier_console(home, folder, name, folder + "/" + name + "\n");
            if (::truncate((home + "/.satl/console/" + folder + "/" + name).c_str(), static_cast<off_t>(kMb)) != 0)
                check(false, "setup: " + folder + "/" + name + " made 1 mb", std::strerror(errno));
            newest_first.push_back(folder + "/" + name);
        }
    forget_what_was_said();
    ConsoleFiles store(on_2026_10_20, home.c_str(), kCache, count_it);
    unsigned long long int all = 0;
    for (const ConsoleFiles::Found &found : store.walked())
        all += found.size;
    const prompt::ConsoleLines *newest = store.earlier().lines(0), *last = store.earlier().lines(31);
    check(walked_names(store) == newest_first && all == 40 * kMb && store.earlier().size() == 32 &&
              newest != nullptr && newest->stops() == 1 && newest->stop_at(0) == newest_first[0] && last != nullptr &&
              last->stops() == 1 && last->stop_at(0) == newest_first[31] && store.earlier().lines(32) == nullptr &&
              said == 0,
          "a 40 MB store gives a Left's list of its newest 32 MB, and a walk's list of all of it",
          "the walk listed " + std::to_string(store.walked().size()) + " consoles of " + std::to_string(all) +
              " bytes, Left reaches " + std::to_string(store.earlier().size()) + ", the newest reading " +
              listed_lines(lines_of(newest)) + " and the 32nd " + listed_lines(lines_of(last)) + ", " +
              what_was_said());

    // THE LIST ENDS AT THE FIRST CONSOLE THAT WOULD PASS THE CACHE: a cache of 2.5 mb reaches the two
    // newest, and the half mb left reaches nothing.
    ConsoleFiles small(on_2026_10_20, home.c_str(), 2 * kMb + kMb / 2, count_it);
    ConsoleFiles none(on_2026_10_20, home.c_str(), 0, count_it);
    check(small.earlier().size() == 2 && small.walked().size() == 40 && none.earlier().size() == 0 &&
              none.walked().size() == 40,
          "Left's list ends at the first console that would pass the cache, and a cache of 0 reaches none",
          std::to_string(small.earlier().size()) + " and " + std::to_string(none.earlier().size()) + " reached");
}

int remove_one(const char *path, const struct stat *, int, struct FTW *)
{
    return ::remove(path);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: console_files_cases <a folder to work in>\n");
        return 2;
    }
    std::string pattern = std::string(argv[1]) + "/console_files_cases.XXXXXX";
    std::vector<char> name(pattern.begin(), pattern.end());
    name.push_back('\0');
    if (::mkdtemp(name.data()) == nullptr) {
        std::printf("FAIL setup: could not make a folder in %s: %s\n", argv[1], std::strerror(errno));
        return 1;
    }
    // ABSOLUTE, so the case that changes directory still finds what it made.
    char *whole = ::realpath(name.data(), nullptr);
    room = whole != nullptr ? whole : name.data();
    std::free(whole);
    // HOME INTO THE ROOM BEFORE ANYTHING ELSE: every store here is handed a home of its own, and anything
    // that looked HOME up for itself would find this one, never a person's.
    ::setenv("HOME", room.c_str(), 1);
    // A UMASK OF ITS OWN, so the modes below are the store's and not the shell's.
    ::umask(022);
    // A ZONE OF ITS OWN, twelve hours behind UTC, so the day below is local time wherever the machine
    // is: a store that read it in UTC would name every folder a day late.
    ::setenv("TZ", "XST12", 1);
    ::tzset();

    the_day_is_the_clocks();
    the_day_is_read_once();
    the_folders();
    what_was_there();
    links_are_refused();
    a_folder_not_writable();
    no_home();
    threads_at_once();
    descriptors();
    the_number();
    the_names();
    claims_at_once();
    the_file();
    a_file_not_claimed();
    the_rule_3_line();
    the_files_lines();
    an_empty_enter();
    one_write_a_line();
    kept_once();
    lines_from_threads();
    a_write_that_fails();
    a_write_interrupted();
    the_filter();
    the_walk();
    read_back();
    lefts_list();

    ::nftw(room.c_str(), remove_one, 16, FTW_DEPTH | FTW_PHYS);
    check(!there(room), "cleanup: the folder it made is gone", room + " is still there");

    std::printf("%d ok, %d failed: %s\n", passed, failed,
                failed == 0 ? "every console store case passed" : "a console store case FAILED");
    return failed == 0 ? 0 : 1;
}
