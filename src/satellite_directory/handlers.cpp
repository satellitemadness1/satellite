// `satellite.directory.change(d)` `1 18 1` through `list(d)` `1 18 5`, behind
// the table. See satellite_directory/handlers.hpp for what this module is not.

#include "satellite_directory/handlers.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error_reporter/report.hpp"
#include "satellite_file/file_internal.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

namespace satellite::directory {

namespace {

// The working directory, however long it is. NOT a PATH_MAX buffer: DESIGN
// §1.4 refuses a limit the language did not need, and a path longer than
// whatever constant was picked is exactly the case a bound turns into a wrong
// answer. getcwd(nullptr, 0) is the GNU extension that allocates what it needs;
// where it is not available the loop below is the portable form and is what
// runs, because it costs one branch and removes the question.
std::string working_directory()
{
    std::vector<char> buffer(256);
    for (;;) {
        if (::getcwd(buffer.data(), buffer.size()) != nullptr)
            return std::string(buffer.data());
        if (errno != ERANGE)
            return std::string();
        buffer.resize(buffer.size() * 2);
    }
}

// `satellite.directory.current()` `1 18 2`.
bool directory_current(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = file::as_text(working_directory());
    return true;
}

// `satellite.directory.exists(d)` `1 18 3` and `change(d)` `1 18 1`, which ask
// the same question and differ in what they do with the answer.
//
// CHECKED BEFORE THE MOVE rather than reporting chdir's failure after, because
// "not a directory" and "no such directory" are the two answers a caller wants
// and chdir collapses several more into errno. Like a failed
// `satellite.file.open`, this is a VALUE and not an error: asking whether
// somewhere is reachable must not kill the program that asked -- and PLAN M19's
// done-when rests on it, "a `1 18 1` onto a plain file that returns false
// without dying".
bool directory_reach(eval::Machine &m, const Value *arguments, uint32_t,
                     Value *answer, bool changing)
{
    std::string name;
    if (!file::path_at(m, arguments, 0, &name))
        return false;

    struct stat info;
    const bool is_directory =
        ::stat(name.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
    if (!changing || !is_directory) {
        *answer = Value::boolean(is_directory);
        return true;
    }
    *answer = Value::boolean(::chdir(name.c_str()) == 0);
    return true;
}

bool directory_exists(eval::Machine &m, const Value *arguments, uint32_t count,
                      Value *answer)
{
    return directory_reach(m, arguments, count, answer, false);
}

bool directory_change(eval::Machine &m, const Value *arguments, uint32_t count,
                      Value *answer)
{
    return directory_reach(m, arguments, count, answer, true);
}

// `satellite.directory.list()` `1 18 4` and `list(d)` `1 18 5` -- what is IN a
// directory, sorted.
//
// TWO NUMBERS AND NOT ONE VARIADIC ROW, which is where this differs from v1:
// WORD_NUMBERS §1.3 makes the call shape part of the number, so the two forms
// are two rows over one function rather than one row counting its arguments.
// They are still ONE QUESTION about two directories -- §1.5's `P_HELP`
// situation and not `satellite.random`'s, where two shapes ask different things
// and earned separate names.
bool directory_list(eval::Machine &m, const Value *arguments, uint32_t count,
                    Value *answer)
{
    std::string name;
    if (count == 0)
        name = working_directory();
    else if (!file::path_at(m, arguments, 0, &name))
        return false;

    const std::string quoted = "\"" + name + "\"";

    // stat FIRST, so the two answers a caller actually wants stay apart -- the
    // reason `change` gives above for not simply reporting chdir's errno. Then
    // opendir, whose failure is a THIRD thing: the directory is there and
    // cannot be read. `exists(d)` answers true for that case, because stat needs
    // only search permission on the PARENT -- so `exists` is a safe way to ask
    // about ENOENT and is not a promise that `list` will succeed. S1206 is where
    // a reader sent here by S1204 finds that out.
    struct stat info;
    if (::stat(name.c_str(), &info) != 0) {
        m.refuse(errors::make<errors::Code::DIRECTORY_NO_SUCH>(
            m.span_of(m.here()), quoted));
        return false;
    }
    if (!S_ISDIR(info.st_mode)) {
        m.refuse(errors::make<errors::Code::DIRECTORY_NOT_A_DIRECTORY>(
            m.span_of(m.here()), quoted));
        return false;
    }

    DIR *directory = ::opendir(name.c_str());
    if (directory == nullptr) {
        m.refuse(errors::make<errors::Code::DIRECTORY_UNREADABLE>(
            m.span_of(m.here()), quoted, std::strerror(errno)));
        return false;
    }

    std::vector<SatString> names;
    int failure = 0;
    for (;;) {
        // Cleared before EVERY call, because a null return means both "the
        // directory ended" and "the read failed", and errno is the only thing
        // that tells them apart. readdir does not touch errno on success, so a
        // stale value from any earlier syscall would read as a failure that
        // never happened.
        errno = 0;
        const struct dirent *entry = ::readdir(directory);
        if (entry == nullptr) {
            failure = errno;
            break;
        }
        const std::string leaf = entry->d_name;
        // "." AND ".." ARE NOT CONTENTS. "." IS this directory and ".." is its
        // parent, so neither is a thing in it -- and a caller who joins each
        // name onto the directory it came from and walks down gets an infinite
        // descent rather than a walk. REAL DOTFILES STAY: a language with no
        // flags has one answer, so it has to be the true one, and there would be
        // no second way to ask for them.
        if (leaf == "." || leaf == "..")
            continue;
        // encode_raw and never encode, for the reason as_text carries one module
        // over: a POSIX filename is arbitrary bytes and a file called "\cwd"
        // would otherwise list as the working directory instead of as itself.
        names.push_back(encode_raw(leaf));
    }
    // Before closedir, which may fail and set errno itself and would otherwise
    // get to decide whether the loop above worked.
    ::closedir(directory);
    if (failure != 0) {
        // A half-read directory is worse than none: its `.size()` is a number
        // the caller would believe.
        m.refuse(errors::make<errors::Code::DIRECTORY_UNREADABLE>(
            m.span_of(m.here()), quoted, std::strerror(failure)));
        return false;
    }

    // SORTED, because readdir hands entries back in whatever order the
    // filesystem stores them -- hash order on ext4 -- so the same directory
    // lists differently on two machines, and DESIGN §8.4 already refused that
    // for the map: unstable iteration order makes a program's output
    // unfalsifiable, and the tests compare output as an exact string.
    //
    // SORTED AS SatStrings AND NOT AS DECODED BYTES: `<` on two satellite
    // strings is this exact comparison over DESIGN §5's code table, so any other
    // order would hand back a list whose own elements the language's own
    // operator calls out of order. Two consequences worth naming rather than
    // discovering: dotfiles sort last, because the dot is punctuation in that
    // table, and a capitalised name sorts after a lowercase one.
    //
    // AND NOT THROUGH M16's `sort()` `1 4 2 3`. PLAN M19 says the list is
    // M16's and the sorting is not a second sort here -- which is right about
    // the LIST and would be wrong about the order: `sort` is a row a program
    // calls on a value, and this is the order the value is BUILT in. Sorting
    // after the fact would put the same comparison behind a dispatch.
    std::sort(names.begin(), names.end());

    List entries;
    entries.reserve(names.size());
    for (SatString &leaf : names)
        entries.push_back(Value::string(std::move(leaf)));
    *answer = Value::list(std::move(entries));
    return true;
}

} // namespace

void install_handlers()
{
    using words::NodeId;
    auto &table = eval::Handlers::table();

    // No row binds a receiver: `directory` is a namespace, not a value.
    table.install(static_cast<words::PathId>(NodeId::DIRECTORY_CHANGE),
                  eval::Handler{directory_change, false, 1, "M19"});
    table.install(static_cast<words::PathId>(NodeId::DIRECTORY_CURRENT),
                  eval::Handler{directory_current, false, 0, "M19"});
    table.install(static_cast<words::PathId>(NodeId::DIRECTORY_EXISTS),
                  eval::Handler{directory_exists, false, 1, "M19"});
    table.install(static_cast<words::PathId>(NodeId::DIRECTORY_LIST_0),
                  eval::Handler{directory_list, false, 0, "M19"});
    table.install(static_cast<words::PathId>(NodeId::DIRECTORY_LIST_D),
                  eval::Handler{directory_list, false, 1, "M19"});
}

} // namespace satellite::directory
