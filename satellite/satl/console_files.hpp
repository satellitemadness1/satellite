#pragma once
// satellite/satl/console_files.hpp -- THE CONSOLE'S STORE (KEY_PLAN.md): what is typed into a console,
// kept in a folder of the day the console started, in a file of the console's own. KEY-1 is the folder,
// the author's first milestone of 2026-10-20: "create the month-and-day named folders to store the
// individual console files into" -- under ~/.satl/console ("yes"), and with the year ("include the
// year"). KEY-2 is the file in it, the second: "name the console files console-001.c4". KEY-3 is every
// Enter kept in it, the third: "one enter press per line":
//
//     ~/.satl/                      made only when it is missing, 0755, as write_entry makes it
//         console/                  0700, at the console's first Enter
//             2026-10-20/           0700, the day the console started, in local time
//                 console-001.c4    0600, this console's own, claimed at the same Enter (KEY-2)
//
// ONE STORE A satl, made by run_satl with `new` once the commands that print and exit have returned,
// and only for a console -- stdin and stdout both terminals -- and never deleted: the reader and
// input() reach it until satl exits, from any thread, through machine/console_store.hpp.
//
// NOTHING IS MADE UNTIL THE FIRST ENTER, so a console nobody types into leaves nothing (rule 2). The
// first Enter is a line the reader answers at a terminal, typed or pasted, or a file run's input()
// answered there; never a piped line, a line stopped with Ctrl-C, or the end of the input.
//
// THE FOLDERS ARE 0700 because they will hold everything a person types (rule 2). Each is made with
// mkdirat and opened with openat O_NOFOLLOW|O_DIRECTORY from the folder above, starting from ~/.satl,
// as the walk will read them (KEY-5): a name there that is a link, or not a folder, is S017, and
// nothing is ever written through it. No folder's descriptor is kept past the first Enter.
//
// THE CONSOLE'S FILE IS CLAIMED IN THE SAME STEP (KEY-2), through the day's folder that step has just
// opened, before its descriptor closes. Its number is one more than the highest console-<digits>.c4 in
// the folder -- 001 in an empty one, three digits and more after 999 (console-1000.c4), a gap never
// filled -- and its name is taken with O_CREAT|O_EXCL, as make_new takes one (satellite_file.hpp), so
// a number another satl took first is passed over for the next. It is 0600, and it is held open for
// the run, never on 0, 1 or 2.
//
// EVERY ENTER IS A LINE OF IT (KEY-3), and a satellite.variable.string among the console's lines
// (prompt/console_lines.hpp), which the store owns: the line as Enter sent it, less what
// what_is_kept drops, and its newline, in ONE write(2) to the file held open with O_APPEND -- retried
// on EINTR and finished after a short write, as satellite_log.hpp's write_entry writes an entry. Nothing
// waits in a buffer of satl's, so a hangup's _exit, a second Ctrl-C, SIGKILL or a closed window loses
// no line already Entered: each is the kernel's before its line runs. Nothing kept is ever run again by
// itself (rule 9); the store only keeps and writes.
//
// ONE FILE A CONSOLE, BECAUSE ONE STORE A satl: the prompt's lines and input()'s answers claim one
// file, once. A satl that `run` or `interpret` starts (prompt_run.cpp), and a File > New window
// (console_menu.cpp's start_another_satl), is a satl of its own, with a store of its own and a file of
// its own at its own first Enter; the file is O_CLOEXEC, so no satl started from this one holds it.
//
// A CONSOLE THAT CANNOT SAVE KEEPS WORKING (rule 8): the first Enter that cannot make the folder or
// claim the file in it, or the first line that cannot be written into the file, says S017
// CONSOLE_NOT_SAVED once (machine/s_codes.hpp), and the store tries nothing more on the disk this run --
// so a line written in part is never followed by the next. Its lines are kept in memory all the same.
//
// THE CONSOLES BEFORE THIS ONE ARE FOUND WHEN THE STORE IS MADE (KEY-5), the fourth milestone: "we build
// the code that loads the month-and-day named folders into satellite.variable.string objects". ONE WALK,
// before the prompt or the program reads a line, and before this console has a file, so its own is never
// among them: the dated folders under the root -- YYYY-MM-DD, digits and two dashes -- and the
// console-<digits>.c4 files in each, each with its size and inode from one fstatat. It reads as
// listing_counts.cpp's count_contents reads: one folder open a level, each opened O_NOFOLLOW from the one
// above, no link followed, only regular files listed; nothing else is. What it finds is THE WALK'S LIST,
// newest first: the folders' dates, latest first, and in a day the higher number first.
//
// LEFT'S LIST (KEY-6) is the newest consoles on it whose sizes add up to no more than the cache the store
// is given -- arguments.input.cache, 32 mb, "saving the most recent 32mb worth of files" (the fifth
// milestone) -- and it is what Left reaches: the earlier consoles (prompt/earlier_consoles.hpp), each read
// into satellite.variable.strings the first time Left reaches it, through what_is_kept as a line typed is,
// and only while its file is the one the walk listed. Nothing the walk finds is ever written or removed.

#include "../machine/critical_report.hpp"
#include "../prompt/console_lines.hpp"
#include "../prompt/earlier_consoles.hpp"

#include <ctime>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

namespace satellite004 {

class ConsoleFiles {
public:
    // THE CLOCK: the machine's in satl (the_machines_clock, below); console_files_cases gives the
    // store a clock of its own and never the real one.
    using Clock = std::time_t (*)();
    // HOW S017 IS SAID: print_notice in satl. console_files_cases counts it instead -- print_notice's
    // tally says a notice once a place (its code and directory), and it writes satellite.log too.
    using Say = void (*)(const CriticalReport &report);

    // ONE CONSOLE THE WALK FOUND (KEY-5): where it is, and what the walk saw of it.
    struct Found {
        std::string day;              // "2026-10-20", the folder it is in
        std::string name;             // "console-001.c4"
        std::string number;           // its number's digits, as the claim reads them: "1"
        unsigned long long int size;  // its bytes, when the walk listed it
        dev_t device;                 // and the file it was then, which is the only one read back
        ino_t inode;
    };

    // THE DAY IS READ ONCE, HERE, so a console started at 23:59 keeps its day after midnight. `home`
    // is $HOME as satl was given it, or null: the root is built from it now, absolute, because the
    // prompt may stand in another folder by the first Enter (structured-library.cpp's
    // go_to_the_default_directory, and satellite.directory.change after it).
    // AND THE WALK RUNS HERE (KEY-5), from that root. `cache` is how many bytes of the newest consoles
    // Left reaches (KEY-6): run_satl gives arguments.input.cache's mb as bytes, and console_files_cases
    // values of its own -- it never includes satellite_config.hpp.
    ConsoleFiles(Clock clock, const char *home, unsigned long long int cache, Say say = print_notice);
    // THE FILE IS CLOSED HERE AND NOWHERE ELSE. run_satl never deletes its store, so in satl the file
    // stays open until satl exits; a harness's store closes it as it goes.
    ~ConsoleFiles();

    ConsoleFiles(const ConsoleFiles &) = delete;
    ConsoleFiles &operator=(const ConsoleFiles &) = delete;

    // A LINE WAS ENTERED AT THIS CONSOLE. It is added to the console's lines, unless they are `kept_in`
    // -- the reader that answered it has added it already -- and then written into the console's file,
    // one write(2) with its newline. The first Enter makes the day's folder and claims the file before
    // its line is written, or says S017; after S017 the lines are still kept, and nothing is written.
    // Safe from any thread: a line is kept and written under the store's lock, so the file's lines are
    // in the order the store keeps them, and an Enter on another thread waits its turn.
    void entered(const std::string &line, const prompt::ConsoleLines *kept_in = nullptr);

    // THE CONSOLE'S LINES, oldest first: what the reader is handed (session.cpp, and a file run's in
    // structured-library.cpp), and what Up and Down go through (KEY-4).
    prompt::ConsoleLines &lines() { return lines_; }

    // THE CONSOLES BEFORE THIS ONE, newest first, that Left reaches (KEY-6): what the reader is handed
    // beside the console's lines (session.cpp, and a file run's in structured-library.cpp).
    prompt::EarlierConsoles &earlier() { return earlier_; }
    // THE WALK'S LIST (KEY-5): every console file it found, newest first. Left's list is the start of it.
    const std::vector<Found> &walked() const { return walked_; }

    const std::string &day() const { return day_; }     // "2026-10-20"; "" when the clock gave none
    const std::string &root() const { return root_; }   // "/home/me/.satl/console"; "" with no usable HOME

private:
    // What the first Enter found. `untried` until then; after it, the store is one of the other two:
    // the folder made and the file claimed, or S017 said. A line that cannot be written takes `claimed`
    // to `not_saved`, and nothing takes it back this run.
    enum class Disk { untried, claimed, not_saved };

    // The folders, made and opened from ~/.satl down; the day's is left open in `day`, for the claim,
    // and the caller closes it. False with `why` and `where` when one cannot be.
    bool make_the_days_folder(int &day, std::string &why, std::string &where) const;
    // The console's file, claimed in the day's folder and held in file_. False with `why` and `where`
    // when it cannot be.
    bool claim_the_file(int day, std::string &why, std::string &where);
    // One kept line and its newline into file_. False with `why` when it cannot be written whole.
    bool written(const std::string &kept, std::string &why) const;
    // The walk (KEY-5): walked_ filled and sorted, and earlier_ given the newest `cache` bytes of it.
    // Says nothing: a root that is missing, a link, or cannot be read finds no console.
    void walk(unsigned long long int cache);

    Say say_;
    std::string day_;
    std::string satl_;      // $HOME/.satl
    std::string root_;      // $HOME/.satl/console
    std::string no_root_;   // why there is no root -- HOME unset, or not absolute -- or ""

    prompt::ConsoleLines lines_;   // every line Entered, kept whatever the disk does (KEY-3)
    std::vector<Found> walked_;          // the walk's list, newest first, set once when the store is made (KEY-5)
    prompt::EarlierConsoles earlier_;    // Left's list, each console read when Left first reaches it (KEY-6)

    std::mutex lock_;
    Disk disk_ = Disk::untried;
    int file_ = -1;         // this console's file, from its first Enter; -1 before it, and after a claim's S017
    std::string file_path_; // where it is, for an S017 a line that cannot be written says
};

// THE MACHINE'S CLOCK, the one run_satl gives the store.
std::time_t the_machines_clock();

// THE HOOK run_satl SETS (machine/console_store.hpp's line_entered): the line, to the store.
void a_line_was_entered(const std::string &line, const prompt::ConsoleLines *kept_in);

} // namespace satellite004
