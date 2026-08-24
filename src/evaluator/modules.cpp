// satellite.<module>.* — the module function surface.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "evaluator/eval_internal.hpp"

#include <iostream>

#include "console_output/console.hpp"
#include "random_numbers/random.hpp"

namespace satellite {

ValuePtr Evaluator::call_module(const std::vector<std::string> &path,
                                const std::vector<ValuePtr> &argv, Span span)
{
    const std::string full = join_path(path);

    // The one constructor the language has that is not a spacesuit's: an
    // instant is read off the clock, never written down as a literal.
    //
    // system_clock rather than steady_clock, because §8.2 fixes the type as an
    // absolute instant since the Unix epoch and steady_clock's epoch is
    // unspecified. The cost is that a clock adjustment can move it; the benefit
    // is that the value means something outside this process.
    if (full == "satellite.time.now") {
        if (!argv.empty()) {
            fail(span, arity_message("satellite.time", "now", 0, argv.size()));
            return nullptr;
        }
        const auto since = std::chrono::system_clock::now().time_since_epoch();
        const auto ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(since);
        return make_value(Time{static_cast<long long>(ns.count())});
    }

    // satellite.file.open(path, mode) — the only way a file value comes into
    // existence, and the reason a declaration cannot make one: opening needs a
    // path and a place to report failure, and a declaration has neither.
    //
    // Modes are strings rather than flags because the language has no enum and
    // wants none; "read", "write" and "append" say what they do at the call
    // site, which a bitmask never does.
    if (full == "satellite.file.open") {
        if (argv.size() != 2) {
            fail(span, arity_message("satellite.file", "open", 2, argv.size()));
            return nullptr;
        }
        const SatString *path = as_string(*argv[0]);
        const SatString *mode = as_string(*argv[1]);
        if (!path || !mode) {
            fail(span, "satellite.file.open wants two "
                       "satellite.variable.string arguments");
            return nullptr;
        }

        const std::string name = decode(*path);
        const std::string how = decode(*mode);

        int flags = 0;
        bool writable = false;
        if (how == "read") {
            flags = O_RDONLY;
        } else if (how == "write") {
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            writable = true;
        } else if (how == "append") {
            flags = O_WRONLY | O_CREAT | O_APPEND;
            writable = true;
        } else {
            fail(span, "satellite.file.open mode must be \"read\", \"write\" "
                       "or \"append\", not \"" + how + "\"");
            return nullptr;
        }

        FilePtr handle = std::make_shared<FileHandle>();
        handle->path = name;
        handle->writable = writable;

        const int fd = ::open(name.c_str(), flags, 0644);
        if (fd < 0) {
            // A failed open is a VALUE, not an error: the caller asks .ok() and
            // decides. Failing here instead would make "does this file exist"
            // unanswerable without crashing the program that asked.
            handle->last_error.store(errno);
        } else {
            handle->fd.store(fd);
        }
        return make_value(Value(std::move(handle)));
    }

    // satellite.directory.* — the working directory.
    //
    // NOT satellite.terminal, however much "cd" feels like a terminal thing:
    // `satl --run script.satl` has no terminal at all and still has a working
    // directory, so naming it after one would be a lie in the headless case,
    // which is the common case. It is also not something the terminal could
    // do — §9 puts the window in a separate process that spawns this one, so
    // the interpreter has no way to tell it anything.
    if (full == "satellite.help") {
        if (argv.empty())
            return make_value(encode_raw(help_overview()));
        if (argv.size() == 1)
            return make_value(encode_raw(help_for(*argv[0])));
        fail(span, arity_message("satellite", "help", 1, argv.size()));
        return nullptr;
    }

    // satellite.directory() -- a module asked what it answers to. Two segments
    // and no third, which today was "no such module function: satellite.
    // directory": an answer that is true and useless, since the reason to type
    // it is not knowing the third segment yet. The bare form without
    // parentheses is handled beside satellite.help in helpers.cpp, so both
    // spellings work for the same reason help's do.
    if (path.size() == 2 && path[0] == "satellite") {
        const std::string listing = help_for_module(path[1]);
        if (!listing.empty()) {
            if (!argv.empty()) {
                fail(span, "satellite." + path[1] +
                           " takes no arguments -- it names a module, and "
                           "answers with what that module can do");
                return nullptr;
            }
            return make_value(encode_raw(listing));
        }
    }

    // satellite.analyze(path) -- what is in a spaceship. Directly under
    // satellite rather than under a module, because the subject is a FILE of
    // the language rather than any one module's business: satellite.help's
    // shape, not satellite.directory's. The walk itself is in analyze.cpp.
    if (full == "satellite.analyze") {
        if (argv.size() != 1) {
            fail(span, arity_message("satellite", "analyze", 1, argv.size()));
            return nullptr;
        }
        const SatString *where = as_string(*argv[0]);
        if (!where) {
            fail(span, "satellite.analyze wants a satellite.variable.string, "
                       "got " + to_string(*argv[0]));
            return nullptr;
        }
        const std::string file = decode(*where);
        std::string error;
        const std::string report = analyze_spaceship(file, error);
        if (!error.empty()) {
            // Loud, for satellite.directory.list's reason: the report is a
            // value with no room in it for "there was no file", and an empty
            // one would read as a spaceship that declares nothing.
            fail(span, "satellite.analyze " + error + " \"" + file + "\"");
            return nullptr;
        }
        return make_value(encode_raw(report));
    }

    // b, kb, mb, gb, tb -- STRINGS, and not for want of trying the other way.
    // A bare `tb` is a name the user owns (§1), so
    // satellite.system.memory.used(tb) asks for a variable called tb and is
    // told there is none; making it mean a unit instead would take a second
    // reserved word, which is the one thing this language spends its entire
    // design not doing. satellite.file.open settled the shape already with
    // "read", "write" and "append": the word says at the call site what it is,
    // and no enum has to exist for it to.
    //
    // 1024 and not 1000. Every divisor here is a power of two, which is the
    // whole reason the answers stay exact: a decimal division by 2^n
    // terminates, so bytes in terabytes is a finite decimal and not the
    // 34-digit approximation §8.1 would otherwise have to carry.
    const auto unit_divisor = [](const std::string &unit,
                                 unsigned long long &out) {
        std::string u;
        for (char c : unit)
            u += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        if (u == "b")  { out = 1ULL; return true; }
        if (u == "kb") { out = 1024ULL; return true; }
        if (u == "mb") { out = 1024ULL * 1024; return true; }
        if (u == "gb") { out = 1024ULL * 1024 * 1024; return true; }
        if (u == "tb") { out = 1024ULL * 1024 * 1024 * 1024; return true; }
        return false;
    };

    // satellite.system.memory.* -- what the machine has, what it is using, and
    // what the firmware will admit about the sticks themselves.
    if (path.size() >= 4 && path[0] == "satellite" && path[1] == "system" &&
        path[2] == "memory") {
        const std::string &what = path[3];
        const bool swap_used_form =
            path.size() == 5 && what == "swap" && path[4] == "used";
        // satellite.system.memory.this.* -- the running THREAD, which owns
        // exactly one thing: its stack. Used is how far down it, available is
        // how big it is allowed to get, and free is the difference -- the
        // headroom a recursion still has. See system.hpp on why heap cannot be
        // asked per thread.
        const bool this_form =
            path.size() == 5 && what == "this" &&
            (path[4] == "used" || path[4] == "free" || path[4] == "available");

        // frequency and bit are not quantities of memory, so no unit applies to
        // either: one is MT/s and the other is a count of wires.
        if (path.size() == 4 && (what == "frequency" || what == "bit")) {
            if (!argv.empty()) {
                fail(span, arity_message("satellite.system.memory", what, 0,
                                         argv.size()));
                return nullptr;
            }
            const unsigned long long answer =
                what == "frequency" ? mem_frequency_mhz() : mem_width_bits();
            // 0 is the answer, not a failure: SMBIOS is mode 0400 root and
            // Linux publishes these nowhere else, so an ordinary run cannot
            // know. Failing would make a program that asks unrunnable as a
            // normal user, which is every run of it.
            return make_value(Number::from_u64(answer));
        }

        if (path.size() == 4 || swap_used_form || this_form) {
            unsigned long long bytes = 0;
            bool known = true;
            if (this_form) {
                unsigned long long used = 0;
                unsigned long long total = 0;
                if (!thread_stack_bytes(&used, &total)) {
                    // 0, the way frequency() answers 0: the platform declined,
                    // and a program that asks should not die of it.
                    used = 0;
                    total = 0;
                }
                if (path[4] == "used")
                    bytes = used;
                else if (path[4] == "available")
                    bytes = total;
                else
                    bytes = total > used ? total - used : 0;
            } else if (swap_used_form)
                bytes = swap_used_bytes();
            else if (what == "used")
                bytes = mem_used_bytes();
            else if (what == "free")
                bytes = mem_available_bytes();
            else if (what == "total")
                bytes = mem_total_bytes();
            else if (what == "swap")
                bytes = swap_total_bytes();
            // main() is this process, not the machine: what the interpreter
            // running this program is holding at the moment the line runs.
            // Under satl-term that is still satl, because the window is a
            // separate process that spawns this one (§9) and holds none of
            // the program's memory.
            else if (what == "main")
                bytes = process_memory_bytes();
            else
                known = false;

            if (known) {
                // Megabytes unless told otherwise, because that is the unit a
                // person reads a machine's memory in.
                std::string unit = "mb";
                if (argv.size() == 1) {
                    const SatString *given = as_string(*argv[0]);
                    if (!given) {
                        fail(span, "satellite.system.memory." + what +
                                   " wants a unit as a satellite.variable."
                                   "string -- \"b\", \"kb\", \"mb\", \"gb\" or "
                                   "\"tb\" -- got " + to_string(*argv[0]));
                        return nullptr;
                    }
                    unit = decode(*given);
                } else if (argv.size() > 1) {
                    fail(span, "satellite.system.memory." + what +
                               " takes 0 or 1 arguments, got " +
                               std::to_string(argv.size()));
                    return nullptr;
                }

                unsigned long long divisor = 0;
                if (!unit_divisor(unit, divisor)) {
                    fail(span, "satellite.system.memory." + what +
                               " unit must be \"b\", \"kb\", \"mb\", \"gb\" or "
                               "\"tb\", not \"" + unit + "\"");
                    return nullptr;
                }
                return make_value(Number::divide(Number::from_u64(bytes),
                                                 Number::from_u64(divisor),
                                                 read_division_digits()));
            }
        }
    }

    // satellite.system.home() -- $HOME, from the same system_facts that
    // satellite_string's \home escape and the prompt already read it through.
    // Under `system` because it is a machine fact and not a directory
    // operation: satellite.directory answers about directories, and where this
    // user's home is happens to be one.
    if (full == "satellite.system.home") {
        if (!argv.empty()) {
            fail(span, arity_message("satellite.system", "home", 0, argv.size()));
            return nullptr;
        }
        return make_value(encode_raw(home_dir()));
    }

    // satellite.system.delete(x) -- unlink a file, rmdir an EMPTY directory.
    //
    // Under `system`, and that is a different argument from the one .home makes
    // above. .home is here because $HOME is a machine fact and system_facts is
    // where the machine facts live. This is here because it is ONE verb over
    // TWO kinds of thing. satellite.file.delete and satellite.directory.delete
    // would each be a true half of the answer, and each would send the other
    // half of its callers to look in the wrong module -- and a caller holding a
    // path very often does not know which of the two it holds, which is exactly
    // the case a split surface cannot serve. The operation that spans both
    // modules belongs above both of them.
    //
    // Not a method on satellite.variable.file either, for a reason §8.3 already
    // fixed: a file is a reference type whose methods act through an open
    // descriptor, and unlink acts on a NAME. .close() and .delete() would look
    // like the same kind of thing and be nothing alike -- one ends this
    // handle's access, the other ends everybody's.
    if (full == "satellite.system.delete") {
        if (argv.size() != 1) {
            fail(span, arity_message("satellite.system", "delete", 1,
                                     argv.size()));
            return nullptr;
        }

        // Two shapes of argument, ONE question -- satellite.directory.list's
        // situation and not satellite.random's, so they are one name.
        //
        // A string is the path, whether it was written in quotes at the call
        // site or arrived through a variable; those are indistinguishable by
        // the time the argument is a value, which is the point. An open
        // satellite.variable.file answers with the path it was opened on, so a
        // program that already holds a handle does not have to write
        // .delete(f.path()) to say a thing it has already said.
        //
        // Everything else is REFUSED rather than stringified. to_string(7) is
        // "7", which is a filename the filesystem would accept -- so a number
        // that reached here by mistake would delete a file named after itself
        // and answer true.
        std::string name;
        if (const SatString *text = as_string(*argv[0])) {
            name = decode(*text);
        } else if (const FilePtr *held = std::get_if<FilePtr>(argv[0].get())) {
            // A FilePtr alternative holding nothing is not how an unopened file
            // is spelled -- default_of leaves a declared file NIL, and that
            // lands in the arm below -- but the null is checked anyway, because
            // the cost is one branch and the alternative is dereferencing it.
            if (!*held) {
                fail(span, "satellite.system.delete was given a file that "
                           "holds nothing, so there is no path to delete");
                return nullptr;
            }
            name = (*held)->path;
        } else if (std::holds_alternative<std::monostate>(*argv[0])) {
            // Nil gets its own sentence. The general message below would say
            // "got satellite", which is true and tells the reader nothing about
            // how they got here -- and the way they got here is almost always a
            // satellite.variable.file that was declared and never opened, which
            // §8.3 makes nil on purpose.
            fail(span, "satellite.system.delete has nothing to delete: nil "
                       "names no path, and a satellite.variable.file is nil "
                       "until satellite.file.open gives it one");
            return nullptr;
        } else {
            fail(span, "satellite.system.delete wants a "
                       "satellite.variable.string or an open "
                       "satellite.variable.file, got " + to_string(*argv[0]));
            return nullptr;
        }

        // lstat, NEVER stat. A symlink pointing at a directory is the one case
        // where the two disagree and the disagreement is destructive: stat
        // follows the link and answers "directory", which would aim rmdir at
        // the TARGET -- so deleting a link would either remove somebody else's
        // directory or, more often, fail with ENOTDIR and leave the link
        // standing. The name given is the link, so the link is what goes.
        struct stat info;
        if (::lstat(name.c_str(), &info) != 0)
            return make_value(false);

        // rmdir for a directory and unlink for everything else, which is the
        // whole of the dispatch: POSIX has two calls and each refuses the
        // other's argument, so choosing between them is not an optimisation.
        //
        // True or false, NEVER an error -- satellite.directory.change's
        // contract, for its reason: a program that asks has to survive being
        // told no, and a destructive call is the last place to hand a caller an
        // exception they did not ask to catch. False collapses several answers
        // and three of them are worth naming, because they are the three a
        // caller actually meets:
        //
        //   - nothing was there. Already handled above by the failed lstat.
        //     Deleting what is already gone is false and not an error, so a
        //     cleanup step is safe to run twice.
        //   - the directory is not empty (ENOTEMPTY). This call does NOT
        //     recurse and there is no flag to ask it to: the language has no
        //     flags, so one name gets one behaviour, and a delete that
        //     silently descends is the single mistake in this file nobody gets
        //     to take back. A caller who means a tree walks
        //     satellite.directory.list and says each name out loud.
        //   - it is not ours to delete (EACCES, EPERM, a sticky /tmp).
        //
        // False is a TRUE answer in every one of those -- "it is still there"
        // -- which is what separates this from the silent wrong answer
        // satellite.directory.list refuses above. There is no third state to
        // lose: .open has a handle to hang .error() on and this has nothing,
        // and inventing one for a call whose whole result is one bit would cost
        // more than it tells.
        const bool gone = S_ISDIR(info.st_mode) ? ::rmdir(name.c_str()) == 0
                                                : ::unlink(name.c_str()) == 0;

        // Nothing is closed on the way out, and an open handle is left open.
        // POSIX keeps a descriptor valid across the unlink of its name -- the
        // inode outlives the directory entry -- so a program that deletes a
        // file it is still reading keeps reading it, and the space is returned
        // when the last descriptor closes. Closing it here would be this
        // function deciding something it was not asked about.
        return make_value(gone);
    }

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

    // satellite.random.<tier>(digits) and satellite.random.<tier>.range(lo, hi)
    // — §18. Three tiers, two shapes each, and the tier is a segment rather
    // than an argument for the reason §17 records: the format's arity table
    // keys one arity per path, so `ultra(digits)` and a two-argument
    // `ultra(min, max)` would be one path with two arities, which the format
    // cannot encode. `.range` is a path of its own and costs one word instead.
    if (path.size() >= 3 && path[0] == "satellite" && path[1] == "random") {
        RandomTier tier = RandomTier::Fast;
        const bool ranged = path.size() == 4 && path[3] == "range";
        if ((path.size() == 3 || ranged) && random_tier(path[2], tier)) {
            const std::string module = "satellite.random." + path[2];

            if (!ranged) {
                if (argv.size() != 1) {
                    fail(span, arity_message("satellite.random",
                                             path[2], 1, argv.size()));
                    return nullptr;
                }
                const Number *width = std::get_if<Number>(argv[0].get());
                long long digits = 0;
                if (!width || !width->to_integer(digits)) {
                    fail(span, module + " wants a whole number of digits, got " +
                               to_string(*argv[0]));
                    return nullptr;
                }
                if (digits < 0 || digits > Number::MAX_RANDOM_DIGITS) {
                    fail(span, module + " draws between 0 and " +
                               std::to_string(Number::MAX_RANDOM_DIGITS) +
                               " digits, not " + std::to_string(digits));
                    return nullptr;
                }

                // Uniform over [0, 10^digits), which is what the argument
                // names — so about one draw in ten of a 40-digit request
                // prints 39 digits or fewer, because a leading zero is not
                // printed. §18 says so out loud rather than rounding it away:
                // the alternative is a draw that is not uniform.
                Number drawn;
                if (!random_digits(tier, static_cast<int>(digits), drawn)) {
                    fail(span, module + " could not draw " +
                               std::to_string(digits) + " digits");
                    return nullptr;
                }
                return make_value(std::move(drawn));
            }

            if (argv.size() != 2) {
                fail(span, arity_message(module.c_str(), "range", 2,
                                         argv.size()));
                return nullptr;
            }
            const Number *low = std::get_if<Number>(argv[0].get());
            const Number *high = std::get_if<Number>(argv[1].get());
            if (!low || !high) {
                fail(span, module + ".range wants two "
                           "satellite.variable.number arguments");
                return nullptr;
            }
            // Whole numbers, because the answer is one: there is no uniform
            // draw over the reals between 1 and 100, and quietly rounding the
            // bounds would answer a question nobody asked.
            if (!low->is_integer() || !high->is_integer()) {
                fail(span, module + ".range wants whole numbers, got " +
                           low->to_string() + " and " + high->to_string());
                return nullptr;
            }
            // Inclusive at both ends, so low == high is a range of one and is
            // legal. low > high is empty, and there is nothing to answer with.
            if (Number::compare(*low, *high) > 0) {
                fail(span, module + ".range is inclusive at both ends, so " +
                           low->to_string() + " to " + high->to_string() +
                           " is empty");
                return nullptr;
            }

            Number drawn;
            if (!random_range(tier, *low, *high, drawn)) {
                fail(span, module + ".range spans more than " +
                           std::to_string(Number::MAX_RANDOM_DIGITS) +
                           " digits");
                return nullptr;
            }
            return make_value(std::move(drawn));
        }
    }

    // The value form. The PACE form — satellite.console.display(100ms) — never
    // reaches here: its argument is a duration and not a value, so
    // src/evaluator/expr.cpp takes it before the arguments are evaluated at all.
    if (full == "satellite.console.display") {
        if (argv.size() != 1) {
            fail(span, arity_message("satellite.console", "display", 1,
                                     argv.size()));
            return nullptr;
        }
        // One emit and not two, because the unit that reaches the Console is
        // the unit that cannot be torn in half by another thread displaying at
        // the same time. Appending the text and then the newline separately
        // would queue two elements and let a line arrive without its ending.
        emit(to_string(*argv[0]) + "\n");
        return make_value(std::monostate{});
    }

    // satellite.console.input([prompt]) — the VALUE form, which hands the line
    // back like any other call. The two-argument form
    // satellite.console.input(prompt, target) writes into a variable instead
    // and never reaches here: its second argument is a place and not a value,
    // so src/evaluator/expr.cpp takes it before the arguments are evaluated,
    // exactly as it does for display's `end`.
    if (full == "satellite.console.input") {
        if (argv.size() > 1) {
            fail(span, arity_message("satellite.console", "input", 1,
                                     argv.size()));
            return nullptr;
        }
        return console_input(argv.empty() ? std::string()
                                          : to_string(*argv[0]), span);
    }

    fail(span, "no such module function: " + full);
    return nullptr;
}

// Reads one line from standard input, after making sure everything already
// displayed has actually reached the terminal.
//
// THE DRAIN IS THE WHOLE REASON THIS IS A FUNCTION. Output goes through the
// Console's printer thread, so a prompt written with
// satellite.console.display(">>", end="") is QUEUED rather than printed, and a
// read that did not wait for it would block on an empty-looking terminal while
// the prompt sat behind it. drain() is exactly the "everything queued is
// written AND flushed" barrier console.hpp already provides for the error
// report, and this is the second caller with the same need.
//
// Returns false at end of input, which is a real condition and not a failure of
// this function: a program run with its stdin closed reaches it immediately, and
// the caller decides what that means.
bool Evaluator::read_input_line(std::string &line)
{
    if (console_)
        console_->drain();
    else
        std::cout.flush();

    return static_cast<bool>(std::getline(std::cin, line));
}

// satellite.console.input's value, for both of its shapes.
//
// The prompt is displayed with NO trailing newline, because a prompt whose
// cursor sits on the line below it is not a prompt. That makes this the second
// caller of the same unnewlined write display(text, end="") uses, and both go
// through emit() as ONE piece so a prompt cannot be torn in half.
ValuePtr Evaluator::console_input(const std::string &prompt, Span span)
{
    if (!prompt.empty())
        emit(prompt);

    std::string line;
    if (!read_input_line(line)) {
        // Loud rather than an empty string. An empty line and no line at all
        // are different answers — the first is the user pressing return, the
        // second is there being nobody there — and a program that cannot tell
        // them apart loops forever on a closed stdin.
        fail(span, "satellite.console.input reached the end of input");
        return nullptr;
    }

    return make_value(encode_raw(line));
}

// satellite.console.input(prompt, target) — read a line, write it into
// `target`.
//
// THE ONLY OUT PARAMETER IN THE LANGUAGE, and it is deliberately not the
// beginning of a general facility: no user capsule can declare one, because
// nothing in a capsule's parameter list can say "this one is written back".
// This exists because it is the shape a program reaches for when it asks for a
// line, and because the value form alone would make
// `satellite.console.input("", answer)` a mystery rather than a mistake.
//
// The value form is the one to prefer and this is written in terms of it, so
// there is one place that prompts, one place that drains and one place that
// reads.
ValuePtr Evaluator::console_input_into(const Expr &prompt, const Expr &target,
                                       Span span)
{
    // The PLACE first, before the prompt is printed and before anything is
    // read: a target that cannot be written to is a mistake in the program, and
    // discovering it after the user has already typed an answer would throw
    // that answer away.
    Slot slot = slot_of(target);
    if (!slot.valid) {
        fail(target.span, "satellite.console.input writes its answer into its "
                          "second argument, so that argument has to be a "
                          "variable");
        return nullptr;
    }

    ValuePtr text = eval(prompt);
    if (failed() || !text)
        return nullptr;

    ValuePtr line = console_input(to_string(*text), span);
    if (failed() || !line)
        return nullptr;

    // The declared type is checked the same way an assignment's is, and with
    // the same message, because this IS an assignment — it just gets its value
    // from the terminal. A line is always a string, so this fires whenever the
    // target is not one.
    const Type *declared = declared_type(slot);
    if (declared && !matches(*declared, *line)) {
        fail(target.span, "cannot assign " + to_string(*line) + " to " +
                          unparse(*declared) + " " + slot.name);
        return nullptr;
    }

    if (!write_slot(slot, line, target.span))
        return nullptr;

    // Nil, not the line. The out-parameter form is used as a statement, and
    // handing back the value as well would give one call two ways to be read.
    return make_value(std::monostate{});
}

// One message for every wrong use of a named argument, so the reader learns the
// whole rule from any of them. Named after duration_misuse, which exists for
// exactly the same reason and is written the same way: name the form that
// works rather than only refusing the one that does not.
std::string named_arg_misuse(const std::string &name)
{
    return "named arguments are not part of the language; " + name +
           "= is understood only as satellite.console.display(text, end=\"\")"
           ", which displays text with the ending given instead of a newline";
}

// satellite.console.display(text, end=<expr>).
//
// The ending is a VALUE and not a flag, so end="" is a prompt, end=" " puts two
// displays on one line separated by a space, and end="\n" is the default
// spelled out. A boolean `newline=false` could not do the middle one.
ValuePtr Evaluator::display_with_end(const Expr &text, const Expr &ending,
                                     Span span)
{
    (void)span;

    // Left to right, because both can be calls that display something of their
    // own and the order is observable.
    ValuePtr value = eval(text);
    if (failed() || !value)
        return nullptr;
    ValuePtr tail = eval(ending);
    if (failed() || !tail)
        return nullptr;

    // ONE emit, for the reason the plain display arm gives: the unit handed to
    // the Console is the unit another thread cannot tear in half. This one is
    // not a whole line, and that is the caller's choice — but it is still one
    // piece, so a prompt cannot arrive split around another thread's output.
    emit(to_string(*value) + to_string(*tail));
    return make_value(std::monostate{});
}

// satellite.console.display(100ms) — a setting, not a display.
//
// What it buys is stated in console.hpp: the pause is taken by the printer
// thread, so the program does not wait for it. The walk carries on building the
// next line while the previous one is still being spaced out on the terminal,
// and the queue between them is what absorbs the difference.
ValuePtr Evaluator::set_display_pace(const DurationLit &node, Span span)
{
    // floor() first: the pace is nanoseconds and a fraction of one is not a
    // wait anybody can observe, so 0.0000001ms is zero rather than an error.
    long long ns = 0;
    if (!node.nanoseconds.floor().to_integer(ns) || ns < 0) {
        fail(span, "a pace of " + node.text +
                   " is not a length of time this can wait");
        return nullptr;
    }

    // Null with no Console attached, which is every test that reads output()
    // back: there is no printer thread there, and so nothing between two lines
    // to pause. Accepted rather than refused, because the same source has to
    // run under `satl --run` where it does pace.
    if (!console_)
        return make_value(std::monostate{});

    // At the PROMPT, say what changed. A setting whose entire effect is a delay
    // between two future lines is invisible at the moment it is made, and the
    // report it earned was "I entered satellite.console.display(100ms) and it
    // didn't work" — from a session where it had worked and had nothing to show
    // for itself.
    //
    // `echo_` is the REPL's own flag, the same one that makes `x` print 1, so a
    // program run with --run stays silent.
    //
    // AFTER the new pace is applied, which is only safe because the pace is a
    // minimum gap: the seconds spent typing the command count toward it, so the
    // acknowledgement of a 1500ms pace still appears at once. Emitting it first
    // instead would stamp it with the pace being replaced, and the one line
    // that must never be slow — `display(0ms)`, the way out — would be the one
    // line still paying the old pace.
    console_->pace(ns);

    if (echo_)
        emit(ns > 0 ? "display paced: " + node.text + " between lines\n"
                    : "display paced: off\n");

    // Nil, like the display it is spelled as. `0ms` is how a program turns the
    // pacing back off, and it needs no second word to do it.
    return make_value(std::monostate{});
}

// ---------------------------------------------------------------------------
// Capsule calls
// ---------------------------------------------------------------------------

} // namespace satellite
