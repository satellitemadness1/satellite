#include "evaluator/eval_internal.hpp"

#include <iostream>
#include <optional>

#include "console_output/console.hpp"
#include "random_numbers/random.hpp"

// satellite.directory.* -- the working directory
//
// One arm of Evaluator::call_module, which was 771 lines in a single function
// before the 2026-08-24 split. The branch bodies below are UNCHANGED -- they
// were moved, not rewritten.
//
// The return type is what makes that possible. `std::nullopt` means "not mine,
// keep looking"; an ENGAGED optional means this arm handled the call, and the
// ValuePtr inside may still be null because a null return is how a failed call
// reports itself after fail() has run (§8.3.1's rule that a failure is a value).
// So every `return <expr>;` in the moved code converts to an engaged optional on
// its own and needed no edit at all.
//
// Part of src/evaluator/modules.cpp -- see eval_internal.hpp for why an
// anonymous namespace could not simply be split.

namespace satellite {

std::optional<ValuePtr> Evaluator::module_directory(
    const std::string &full, const std::vector<std::string> &path,
    const std::vector<ValuePtr> &argv, Span span)
{
    // satellite.directory.* — the working directory.
    //
    // NOT satellite.terminal, however much "cd" feels like a terminal thing:
    // `satl --run script.satl` has no terminal at all and still has a working
    // directory, so naming it after one would be a lie in the headless case,
    // which is the common case. It is also not something the terminal could
    // do — §9 puts the window in a separate process that spawns this one, so
    if (full == "satellite.directory.current") {
        if (!argv.empty()) {
            fail(span, arity_message("satellite.directory", "current", 0,
                                     argv.size()));
            return nullptr;
        }
        return make_value(encode_raw(cwd()));
    }

    if (full == "satellite.directory.exists" ||
        full == "satellite.directory.change") {
        const bool changing = full == "satellite.directory.change";
        const char *what = changing ? "change" : "exists";
        if (argv.size() != 1) {
            fail(span, arity_message("satellite.directory", what, 1,
                                     argv.size()));
            return nullptr;
        }
        const SatString *path = as_string(*argv[0]);
        if (!path) {
            fail(span, std::string("satellite.directory.") + what +
                       " wants a satellite.variable.string, got " +
                       to_string(*argv[0]));
            return nullptr;
        }

        const std::string name = decode(*path);
        struct stat info;
        const bool is_dir =
            ::stat(name.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
        if (!changing)
            return make_value(is_dir);

        // Checked before the move rather than reporting chdir's failure after,
        // because "not a directory" and "no such directory" are the two
        // answers a caller wants and chdir collapses several more into errno.
        // Like a failed satellite.file.open, this is a VALUE and not an error:
        // asking whether somewhere is reachable must not kill the program that
        // asked.
        if (!is_dir)
            return make_value(false);
        return make_value(::chdir(name.c_str()) == 0);
    }

    // satellite.directory.list([dir]) — what is IN a directory, sorted.
    //
    // Variadic like satellite.help, and for the same reason: the question with
    // no argument is the one people actually ask, and making them write
    // satellite.directory.list(satellite.directory.current()) to ask it would
    // be a worse surface than the word it replaces. The two forms are one
    // question about two directories -- which §17 calls P_HELP's situation and
    // not §18's, where the two shapes of satellite.random ask different
    // questions and earned separate names.
    if (path.size() == 3 && full == "satellite.directory.list") {
        std::string name;
        if (argv.empty()) {
            name = cwd();
        } else if (argv.size() == 1) {
            const SatString *where = as_string(*argv[0]);
            if (!where) {
                fail(span, "satellite.directory.list wants a "
                           "satellite.variable.string, got " +
                           to_string(*argv[0]));
                return nullptr;
            }
            name = decode(*where);
        } else {
            // Spelled out rather than through arity_message, which renders one
            // number and this function has two. satellite.help has the same
            // shape and does use the helper; the helper is what is wrong there,
            // and saying "1" about a function that is happy with none is a
            // message that sends the reader back to the documentation.
            fail(span, "satellite.directory.list takes 0 or 1 arguments, got " +
                       std::to_string(argv.size()));
            return nullptr;
        }

        // This is the one place in the module where failure is an ERROR and not
        // a value, and the departure is deliberate. .open answers with a handle
        // that says .ok() and .change answers false, because each has somewhere
        // to put the bad news. A list has nowhere: the empty list is already a
        // true answer about an empty directory, so spending it on "there was no
        // directory" would make those two indistinguishable -- the same defect
        // §8.6 rejects when it refuses nil-for-absent, and the one plans/
        // missing.txt calls the worst kind, a silent wrong answer that survives
        // every other fix. Loud costs a caller one .exists() call; quiet costs
        // them a program that lists a mistyped path as empty and does nothing.
        //
        // stat first, so the two answers a caller actually wants stay apart --
        // the reason .change gives above for not simply reporting chdir's
        // errno. Then opendir, whose failure is a third thing: the directory is
        // there and cannot be read. satellite.directory.exists() answers true
        // for that case, because stat needs only search permission on the
        // PARENT -- so .exists is a safe way to ask about ENOENT and is not a
        // promise that .list will succeed.
        struct stat info;
        if (::stat(name.c_str(), &info) != 0) {
            fail(span, "satellite.directory.list has no such directory \"" +
                       name + "\" -- satellite.directory.exists asks that "
                       "without failing");
            return nullptr;
        }
        if (!S_ISDIR(info.st_mode)) {
            fail(span, "satellite.directory.list wants a directory, and \"" +
                       name + "\" is a file");
            return nullptr;
        }

        DIR *dir = ::opendir(name.c_str());
        if (!dir) {
            fail(span, "satellite.directory.list cannot read \"" + name +
                       "\": " + std::strerror(errno));
            return nullptr;
        }

        std::vector<SatString> names;
        int failure = 0;
        for (;;) {
            // Cleared before EVERY call, because a null return means both "the
            // directory ended" and "the read failed", and errno is the only
            // thing that tells them apart. readdir does not touch errno on
            // success, so a stale value from any earlier syscall would read as
            // a failure that never happened.
            errno = 0;
            const struct dirent *entry = ::readdir(dir);
            if (!entry) {
                failure = errno;
                break;
            }
            const std::string leaf = entry->d_name;
            // "." and ".." are not contents. "." IS this directory and ".." is
            // its parent, so neither is a thing in it -- and a caller who joins
            // each name onto the directory it came from and walks down gets an
            // infinite descent rather than a walk. Real dotfiles stay: a
            // language with no flags has one answer, so it has to be the true
            // one, and there would be no second way to ask for them.
            if (leaf == "." || leaf == "..")
                continue;
            // encode_raw and never encode. A POSIX filename is arbitrary bytes,
            // valid UTF-8 or not, and encode() expands the backslash names
            // §3.3 lists -- a file called "\cwd" would list as the working
            // directory instead of as itself. encode_raw maps one byte to one
            // code and decode() maps it back, parking whatever the table has no
            // character for in the raw area, so every byte survives the round
            // trip and satellite.file.open on the result opens that same file.
            names.push_back(encode_raw(leaf));
        }
        // Before closedir, which may fail and set errno itself and would
        // otherwise get to decide whether the loop above worked.
        ::closedir(dir);
        if (failure != 0) {
            // A half-read directory is worse than none: its .length() is a
            // number the caller would believe.
            fail(span, "satellite.directory.list stopped part way through \"" +
                       name + "\": " + std::strerror(failure));
            return nullptr;
        }

        // Sorted, because readdir hands entries back in whatever order the
        // filesystem stores them -- hash order on ext4 -- so the same directory
        // lists differently on two machines, and §8.5 already refused that for
        // the map: unstable iteration order makes a program's output
        // unfalsifiable, and eval_test compares output as an exact string.
        //
        // Sorted as SatStrings, NOT as decoded bytes: `<` on two satellite
        // strings is this exact comparison over the code table, so any other
        // order would hand back a list whose own elements the language's own
        // operator calls out of order. Two consequences worth naming rather
        // than discovering: dotfiles sort last, because the dot is punctuation
        // in that table, and a capitalised name sorts after a lowercase one.
        // Byte order was considered, and rejected for exactly that reason.
        std::sort(names.begin(), names.end());

        List entries;
        entries.reserve(names.size());
        for (SatString &leaf : names)
            entries.push_back(make_value(std::move(leaf)));
        return make_value(std::move(entries));
    }

    return std::nullopt;
}

} // namespace satellite
