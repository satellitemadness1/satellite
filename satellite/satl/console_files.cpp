// The console's store. See console_files.hpp.

#include "console_files.hpp"

#include "../machine/console_store.hpp"
#include "../machine/s_codes.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace satellite004 {

namespace {

// THE CONSOLE'S FILE IS NAMED console-<digits>.c4 (KEY_PLAN.md rule 2): "console-001.c4", the author's
// own spelling of milestone 2.
constexpr char kNameHead[] = "console-";
constexpr char kNameTail[] = ".c4";

// A THOUSAND NUMBERS TAKEN IN A ROW IS NO RACE BETWEEN satls -- a person does not start a thousand
// consoles between one satl's reading of the folder and its claim -- so the claim stops there, and
// the loop has an end on a folder that answers EEXIST to every name.
constexpr int kTakenInARow = 1000;

// HELD BY WHAT CLOSES IT, so no way out of the first Enter leaves a folder open.
struct Folder {
    int fd = -1;
    Folder() = default;
    Folder(const Folder &) = delete;
    Folder &operator=(const Folder &) = delete;
    ~Folder()
    {
        if (fd >= 0)
            ::close(fd);
    }
};

// ONE FOLDER BELOW `above`: made 0700 when it is missing, then opened -- never through a link, and only
// when it is a folder. The umask can take bits from 0700 and never add one.
bool made_and_opened(int above, const char *name, Folder &opened, std::string &why)
{
    if (::mkdirat(above, name, 0700) != 0 && errno != EEXIST) {
        why = std::string("the folder could not be made (") + std::strerror(errno) + ")";
        return false;
    }
    opened.fd = ::openat(above, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (opened.fd >= 0)
        return true;
    const int failed = errno;
    // A LINK IS SAID AS ONE. Linux answers O_DIRECTORY|O_NOFOLLOW on a link with ENOTDIR, as it answers
    // a file, so the name itself is asked which it is.
    struct stat about {};
    if (::fstatat(above, name, &about, AT_SYMLINK_NOFOLLOW) == 0 && S_ISLNK(about.st_mode))
        why = "the folder is a link, and nothing is written through one";
    else if (failed == ENOTDIR)
        why = "something that is not a folder has the folder's name";
    else
        why = std::string("the folder could not be opened (") + std::strerror(failed) + ")";
    return false;
}

// A CONSOLE'S NUMBER, FROM A NAME IN THE DAY'S FOLDER. Only a name that is exactly "console-", one
// digit or more, and ".c4" is a console's; every other name is left alone and not counted --
// console-7.txt, a folder named notes, Console-001.c4, console-1.c4.bak, console-.c4, console-+5.c4.
// THE NUMBER IS KEPT AS ITS DIGITS, the zeros in front taken off, so it has as many as its name gives
// it: no number is too large to count, and none stands in the way (DESIGN.md standard 2).
//
// WHAT THE ODD ONES MEAN, decided here because KEY_PLAN.md does not say:
// - console-01.c4 IS CONSOLE 1, and console-0001.c4 too: zeros in front change no number. So the next
//   is 002, and a day never holds a second console 1 by satl's doing.
// - console-0.c4 IS 0, below every number a console is given, so it changes nothing: alone in the
//   folder, the next is 001.
// - A FOLDER OR A LINK NAMED console-005.c4 IS COUNTED BY ITS NAME, as every name is: the claim reads
//   names and never what is behind them, so nothing there is opened or followed, and the next number
//   is 006. A number already in the folder is never claimed again, whatever holds it.
bool console_number(const char *name, std::string &number)
{
    const std::size_t head = sizeof kNameHead - 1, tail = sizeof kNameTail - 1, length = std::strlen(name);
    if (length <= head + tail || std::strncmp(name, kNameHead, head) != 0 ||
        std::strcmp(name + length - tail, kNameTail) != 0)
        return false;
    for (std::size_t at = head; at < length - tail; ++at)
        if (name[at] < '0' || name[at] > '9')
            return false;
    std::size_t first = head;   // past the zeros in front: console-0.c4 keeps no digit, which is 0
    while (first < length - tail && name[first] == '0')
        ++first;
    number.assign(name + first, name + length - tail);
    return true;
}

// WHICH IS HIGHER, of two numbers as console_number keeps them: the one with more digits, and of two
// with as many, the one whose digits sort later.
bool higher(const std::string &number, const std::string &than)
{
    return number.size() != than.size() ? number.size() > than.size() : number > than;
}

// ONE MORE THAN `number`, written out: each 9 at its end becomes 0 and carries one to the digit in
// front, and a number of nines gains a digit -- 999 gives 1000, and 0, no digit at all, gives 1.
std::string one_more(std::string number)
{
    std::size_t at = number.size();
    while (at > 0 && number[at - 1] == '9')
        number[--at] = '0';
    if (at == 0)
        number.insert(number.begin(), '1');
    else
        ++number[at - 1];
    return number;
}

// A CONSOLE'S NAME, FROM ITS NUMBER: three digits at least, console-001.c4, and as many as it needs
// after 999, console-1000.c4 (KEY_PLAN.md rule 2).
std::string console_name(const std::string &number)
{
    const std::size_t zeros = number.size() < 3 ? 3 - number.size() : 0;
    return std::string(kNameHead) + std::string(zeros, '0') + number + kNameTail;
}

// THE HIGHEST NUMBER IN THE DAY'S FOLDER, 0 (no digit) when it holds none, read through the folder's
// own descriptor and never by its path, which a link put there since the folder was opened could send
// somewhere else. fdopendir takes a duplicate, and closedir closes it; the duplicate is close-on-exec,
// as every descriptor of the store is, because a program's thread may start a child meanwhile.
bool highest_number_in(int day, std::string &highest, std::string &why)
{
    highest.clear();
    const int listed = ::fcntl(day, F_DUPFD_CLOEXEC, 0);
    DIR *folder = listed >= 0 ? ::fdopendir(listed) : nullptr;
    if (folder == nullptr) {
        why = std::string("the day's folder could not be read (") + std::strerror(errno) + ")";
        if (listed >= 0)
            ::close(listed);
        return false;
    }
    int failed = 0;
    std::string number;
    for (;;) {
        errno = 0;
        const dirent *entry = ::readdir(folder);
        if (entry == nullptr) {
            failed = errno;   // 0 at the end of the folder
            break;
        }
        if (console_number(entry->d_name, number) && higher(number, highest))
            highest = number;
    }
    ::closedir(folder);
    if (failed != 0) {
        why = std::string("the day's folder could not be read (") + std::strerror(failed) + ")";
        return false;
    }
    return true;
}

// NEVER 0, 1 OR 2 (KEY_PLAN.md KEY-2): those are the terminal. A satl started at a terminal with one of
// them closed -- `SATL_NO_WINDOW=1 satl 2>&-`; satl's own console has put its pty on all three before
// the store is made -- would be handed that number by an open, and the console's file held there for
// the run would be where std::cerr writes: every notice and report of the run would go into the file,
// among the lines a person typed. claim_the_file fills the free ones while it opens the file, so this is
// the last guard, for one another thread freed in between: the file is moved above them, as
// program_spawn.cpp moves a descriptor it hands a child -- F_DUPFD_CLOEXEC, from 3. -1 when it cannot
// be, with errno saying why.
int above_two(int fd)
{
    if (fd > 2)
        return fd;
    const int moved = ::fcntl(fd, F_DUPFD_CLOEXEC, 3);
    const int failed = errno;
    ::close(fd);
    errno = failed;
    return moved;
}

// A DAY'S FOLDER IS NAMED YYYY-MM-DD (KEY_PLAN.md KEY-5, rule 2): ten characters, digits but for the two
// dashes, so a dated name sorts as its date. Every other name under the root is left alone -- notes,
// 10-20, a stray file -- and nothing in it is listed.
bool a_dated_name(const char *name)
{
    for (int at = 0; at < 10; ++at)
        if ((at == 4 || at == 7) ? name[at] != '-' : (name[at] < '0' || name[at] > '9'))
            return false;   // a shorter name fails at its '\0' here, before anything past it is read
    return name[10] == '\0';
}

// ONE FOLDER BELOW `above`, OPENED AS THE WALK READS IT (KEY-5): never made, never through a link, and only
// when it is a folder. False when it cannot be, and nothing is said: a console the walk cannot reach is
// one Left does not reach.
bool opened_below(int above, const char *name, Folder &opened)
{
    opened.fd = ::openat(above, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    return opened.fd >= 0;
}

// EVERY NAME IN A FOLDER, read through the folder's own descriptor as highest_number_in reads one: a
// close-on-exec copy for fdopendir, which closedir closes. A folder that fails part way gives the names
// read before it failed.
std::vector<std::string> names_in(int folder)
{
    std::vector<std::string> names;
    const int listed = ::fcntl(folder, F_DUPFD_CLOEXEC, 0);
    DIR *const listing = listed >= 0 ? ::fdopendir(listed) : nullptr;
    if (listing == nullptr) {
        if (listed >= 0)
            ::close(listed);
        return names;
    }
    while (const dirent *entry = ::readdir(listing))
        if (std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0)
            names.push_back(entry->d_name);
    ::closedir(listing);
    return names;
}

// A CONSOLE'S FILE, READ BACK THE FIRST TIME LEFT REACHES IT (KEY_PLAN.md KEY-5). It is reached from ~/.satl
// down as the walk reached it -- console/ and the day's folder never through a link, the file never through
// one -- and READ ONLY WHEN IT IS THE FILE THE WALK LISTED, the same inode on the same device: a file
// removed since, or another put in its place, is passed over (false). O_NONBLOCK and O_NOCTTY, so a fifo
// put there since is never waited on: it is not that file, and it is passed over.
//
// AS MANY BYTES AS THE WALK LISTED, and no more: Left's list adds up to no more than the cache by those
// sizes, so what Left reads does too, even of a console still running in another satl and adding to its
// file. Each line was one write(2) with its newline (KEY-3), so those bytes end at a line's end.
//
// ONE LINE TO EACH NEWLINE, and the piece after the last newline is a line too when there is one, as
// satellite.file reads a file's lines -- a file edited by hand may end without one. Each goes through
// ConsoleLines::add, so what_is_kept has it as it has a line typed: a file edited by hand still cannot put
// an ESC on the screen.
bool read_back(const std::string &satl, const ConsoleFiles::Found &found, prompt::ConsoleLines &into)
{
    Folder top, console, day;
    top.fd = ::open(satl.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (top.fd < 0 || !opened_below(top.fd, "console", console) || !opened_below(console.fd, found.day.c_str(), day))
        return false;
    Folder file;   // a file's descriptor, held by what closes it as a folder's is
    file.fd = ::openat(day.fd, found.name.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_NOCTTY | O_CLOEXEC);
    struct stat about {};
    if (file.fd < 0 || ::fstat(file.fd, &about) != 0 || !S_ISREG(about.st_mode) || about.st_dev != found.device ||
        about.st_ino != found.inode)
        return false;
    std::string bytes(found.size, '\0');
    std::size_t got = 0;
    while (got < bytes.size()) {
        const ssize_t took = ::read(file.fd, bytes.data() + got, bytes.size() - got);
        if (took < 0 && errno == EINTR)
            continue;
        if (took < 0)
            return false;
        if (took == 0)
            break;   // shorter than the walk saw it: what is there is read
        got += static_cast<std::size_t>(took);
    }
    bytes.resize(got);
    std::size_t from = 0;
    for (std::size_t newline; (newline = bytes.find('\n', from)) != std::string::npos; from = newline + 1)
        into.add(bytes.substr(from, newline - from));
    if (from < bytes.size())
        into.add(bytes.substr(from));
    return true;
}

} // namespace

ConsoleFiles::ConsoleFiles(Clock clock, const char *home, unsigned long long int cache, Say say) : say_(say)
{
    // THE DAY, as satellite_log.hpp's entry_heading reads the time: localtime_r, and %Y-%m-%d, as every
    // date satl writes is (KEY_PLAN.md rule 2).
    const std::time_t now = clock();
    std::tm local{};
    char written[32] = "";
    if (localtime_r(&now, &local) != nullptr)
        std::strftime(written, sizeof written, "%Y-%m-%d", &local);
    day_ = written;

    // THE ROOT, ABSOLUTE OR NOT AT ALL. HOME unset or empty is a real state (config_file.hpp's folder()):
    // nothing is made, and the first Enter says S017. A HOME THAT IS NOT ABSOLUTE IS REFUSED THE SAME WAY,
    // NOT RESOLVED: the prompt may already stand in arguments.directory.default by now, away from the
    // folder satl started in, which that HOME was relative to -- and a guess would put a person's typing
    // in a folder they never named.
    if (home == nullptr || *home == '\0') {
        no_root_ = "$HOME is not set";
        return;
    }
    if (*home != '/') {
        no_root_ = std::string("$HOME is ") + home + ", which is not an absolute path";
        return;
    }
    std::string base = home;
    while (base.size() > 1 && base.back() == '/')
        base.pop_back();
    satl_ = (base == "/" ? std::string() : base) + "/.satl";
    root_ = satl_ + "/console";
    walk(cache);
}

void ConsoleFiles::walk(unsigned long long int cache)
{
    // ~/.satl AS config.ini AND THE FIRST ENTER REACH IT, a link followed: it is the person's own
    // arrangement. console/ and each day's folder below it are never links.
    Folder satl, console;
    satl.fd = ::open(satl_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (satl.fd < 0 || !opened_below(satl.fd, "console", console))
        return;
    for (const std::string &day : names_in(console.fd)) {
        Folder folder;
        if (!a_dated_name(day.c_str()) || !opened_below(console.fd, day.c_str(), folder))
            continue;
        std::string number;
        for (const std::string &name : names_in(folder.fd)) {
            // A CONSOLE'S NAME, AS THE CLAIM COUNTS ONE (console_number), AND A REGULAR FILE: a link, a
            // folder or a fifo with such a name is a number the claim passes over, and no console's lines.
            struct stat about {};
            if (!console_number(name.c_str(), number) ||
                ::fstatat(folder.fd, name.c_str(), &about, AT_SYMLINK_NOFOLLOW) != 0 || !S_ISREG(about.st_mode))
                continue;
            walked_.push_back(Found{day, name, number, static_cast<unsigned long long int>(about.st_size),
                                    about.st_dev, about.st_ino});
        }
    }
    // NEWEST FIRST: the later date, and in a day the higher number -- by its digits, as the claim compares
    // them, so console-1000.c4 is after console-999.c4. Two names of one number (console-01.c4 beside
    // console-001.c4, which no satl makes) are kept in an order of their names, so a walk is always the same.
    std::sort(walked_.begin(), walked_.end(), [](const Found &one, const Found &other) {
        if (one.day != other.day)
            return one.day > other.day;
        if (one.number != other.number)
            return higher(one.number, other.number);
        return one.name > other.name;
    });
    // LEFT'S LIST: the newest whose sizes add up to no more than the cache, ending at the first that would
    // pass it -- "saving the most recent 32mb worth of files" -- so Left never skips a console to reach an
    // older one. Each is read when Left first reaches it, from where the walk found it.
    unsigned long long int total = 0;
    for (const Found &found : walked_) {
        if (found.size > cache - total)
            break;
        total += found.size;
        earlier_.add([satl = satl_, found](prompt::ConsoleLines &into) { return read_back(satl, found, into); });
    }
}

ConsoleFiles::~ConsoleFiles()
{
    if (file_ >= 0)
        ::close(file_);
}

bool ConsoleFiles::make_the_days_folder(int &day, std::string &why, std::string &where) const
{
    if (!no_root_.empty()) {
        why = no_root_;
        return false;
    }
    where = root_;
    if (day_.empty()) {
        why = "the clock gave no day to name the folder by";
        return false;
    }
    // ~/.satl ITSELF, ONLY WHEN IT IS MISSING, and as write_entry makes it (satellite_log.hpp): one
    // folder, 0755. It is opened as config.ini and satellite.log reach it, following a link -- a ~/.satl
    // that is a link is the person's own arrangement. The two folders below it are the store's, and
    // neither is ever a link.
    where = satl_;
    if (::mkdir(satl_.c_str(), 0755) != 0 && errno != EEXIST) {
        why = std::string("the folder could not be made (") + std::strerror(errno) + ")";
        return false;
    }
    Folder satl;
    satl.fd = ::open(satl_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (satl.fd < 0) {
        why = std::string("the folder could not be opened (") + std::strerror(errno) + ")";
        return false;
    }
    where = root_;
    Folder console;
    if (!made_and_opened(satl.fd, "console", console, why))
        return false;
    where = root_ + "/" + day_;
    Folder opened;
    if (!made_and_opened(console.fd, day_.c_str(), opened, why))
        return false;
    day = std::exchange(opened.fd, -1);
    return true;
}

bool ConsoleFiles::claim_the_file(int day, std::string &why, std::string &where)
{
    const std::string folder = root_ + "/" + day_;
    where = folder;
    std::string highest;
    if (!highest_number_in(day, highest, why))
        return false;
    // EACH NUMBER IS TRIED IN TURN FROM ONE MORE THAN THE HIGHEST, so a gap is never filled. EEXIST is a
    // name another satl took between this one's reading and its claim, and is passed over for the next;
    // anything else is S017. O_NOFOLLOW as well as O_EXCL, which already refuses a link -- nothing is
    // ever opened through one. 0600, from which a umask can take bits and never add one: whatever the
    // umask, nobody else reads what a person typed.
    const std::string first = one_more(highest);
    std::string number = first, name;
    for (int taken = 0;; ++taken, number = one_more(number)) {
        if (taken == kTakenInARow) {
            why = "every name from " + console_name(first) + " to " + name + " was taken as it was claimed";
            where = folder;
            return false;
        }
        name = console_name(number);
        where = folder + "/" + name;
        // NOT ON 0, 1 OR 2 EVEN AS IT IS OPENED: an open takes the lowest free number, so with one of them
        // closed the file would be that number until above_two moved it, and a notice another thread
        // wrote to std::cerr in between would go into the file. Each free one of the three holds a copy
        // of the day's folder while the file is opened -- a folder takes no write, as a closed number
        // takes none -- and is freed after; a copy that comes back above 2 means none is left free.
        int low[3], filled = 0;
        for (int copy; filled < 3 && (copy = ::fcntl(day, F_DUPFD_CLOEXEC, 0)) >= 0;) {
            if (copy > 2) {
                ::close(copy);
                break;
            }
            low[filled++] = copy;
        }
        const int fd =
            ::openat(day, name.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_APPEND | O_CLOEXEC, 0600);
        const int failed = errno;
        while (filled > 0)
            ::close(low[--filled]);
        if (fd < 0 && failed == EEXIST)
            continue;
        if (fd < 0) {
            why = std::string("the console's file could not be made (") + std::strerror(failed) + ")";
            return false;
        }
        // CLAIMED, AND NOT REMOVED IF IT CANNOT BE KEPT: an empty console-NNN.c4 is a console with no
        // lines, and removing a file is KEY-7's, under a lock, never the claim's.
        file_ = above_two(fd);
        if (file_ < 0) {
            why = std::string("the console's file could not be kept above 0, 1 and 2, which are the terminal (") +
                  std::strerror(errno) + ")";
            return false;
        }
        file_path_ = where;
        return true;
    }
}

// ONE LINE, ONE write(2) (KEY_PLAN.md KEY-3), its newline in the same buffer, as write_entry writes an
// entry (satellite_log.hpp): retried when a signal interrupts it before a byte is taken, and finished
// when it takes only part. A write that takes nothing, or fails, is the end: what it took stays, and
// nothing is written after it (entered).
bool ConsoleFiles::written(const std::string &kept, std::string &why) const
{
    const std::string line = kept + '\n';
    std::size_t sent = 0;
    while (sent < line.size()) {
        const ssize_t put = ::write(file_, line.data() + sent, line.size() - sent);
        if (put < 0 && errno == EINTR)
            continue;
        if (put <= 0) {
            why = std::string("a line could not be written into the console's file (") +
                  (put < 0 ? std::strerror(errno) : "it took no byte") + ")";
            return false;
        }
        sent += static_cast<std::size_t>(put);
    }
    return true;
}

void ConsoleFiles::entered(const std::string &line, const prompt::ConsoleLines *kept_in)
{
    std::string why, where;
    {
        const std::lock_guard<std::mutex> held(lock_);
        // KEPT FIRST, WHATEVER THE DISK DOES (rule 8: a console that cannot save keeps its lines in
        // memory), and ONCE: a line the reader has added to these lines already is not added again --
        // only a line from a reader handed no lines is added here, since a file run's input() reads
        // through a reader too (KEY_PLAN.md KEY-4). What is written is the same filter's answer either
        // way, so the file's line is the kept line, byte for byte.
        const std::string kept = kept_in == &lines_ ? prompt::what_is_kept(line) : lines_.add(line);
        if (disk_ == Disk::untried) {
            // THE FOLDER AND THE FILE IN ONE STEP (KEY_PLAN.md KEY-2): the number is read, and the name
            // taken, through the day's folder this step has just made and opened, before its descriptor
            // closes at the end of the step -- so the file is claimed in the folder that was made, never
            // by a path a link could redirect, and nothing is claimed before the first Enter.
            Folder day;
            disk_ = make_the_days_folder(day.fd, why, where) && claim_the_file(day.fd, why, where)
                        ? Disk::claimed
                        : Disk::not_saved;
        } else if (disk_ == Disk::not_saved) {
            return;   // S017 has been said, and nothing more is tried on the disk this run
        }
        // AND THEN WRITTEN, under the same lock, so two threads' lines never cross in the file. A LINE THAT
        // CANNOT BE WRITTEN IS S017 (rule 8), and the file gets nothing more this run: whatever part of
        // the line went in stays the last thing in it, and no later line is joined to it.
        if (disk_ == Disk::claimed) {
            if (written(kept, why))
                return;
            disk_ = Disk::not_saved;
            where = file_path_;
        }
    }
    // SAID OUTSIDE THE STORE'S LOCK: print_notice takes the console's lock (critical_report.hpp), and
    // nothing here holds the two at once. An Enter on another thread already finds the store not_saved.
    CriticalReport report;
    report.code = kConsoleNotSaved.code;
    report.name = kConsoleNotSaved.name;
    report.description = why + ", so " + kConsoleNotSaved.means;
    report.directory = where;
    say_(report);
}

std::time_t the_machines_clock()
{
    return std::time(nullptr);
}

void a_line_was_entered(const std::string &line, const prompt::ConsoleLines *kept_in)
{
    if (ConsoleFiles *store = console_files())
        store->entered(line, kept_in);
}

} // namespace satellite004
