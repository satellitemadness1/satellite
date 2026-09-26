#include "evaluator/eval_internal.hpp"

#include <iostream>
#include <optional>

#include "console_output/console.hpp"
#include "random_numbers/random.hpp"

// satellite.system.* -- memory, the machine, and deleting a file
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

std::optional<ValuePtr> Evaluator::module_system(
    const std::string &full, const std::vector<std::string> &path,
    const std::vector<ValuePtr> &argv, Span span)
{
    // Moved here from the middle of call_module by the 2026-08-24 split. It sat
    // between the `analyze` branch and the memory branches and reads as though
    // it belonged to neither; it belongs to these, which are its only caller.
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
        // satellite.system.memory.swap.<used|free|total>. Swap now answers the
        // same three words the machine's own memory does, and for the same
        // reason §1 gives: a word means one thing everywhere, so a reader who
        // learns memory.free() has already learned memory.swap.free().
        //
        // free is computed here rather than read, because Linux publishes
        // SwapTotal and SwapFree in /proc/meminfo but the pair this code
        // already had is total and USED. Subtracting keeps the two answers
        // consistent with each other by construction: total - used - free == 0
        // always, which would not be guaranteed if free came from a separate
        // read taken a moment later.
        const bool swap_form =
            path.size() == 5 && what == "swap" &&
            (path[4] == "used" || path[4] == "free" || path[4] == "total");
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

        if (path.size() == 4 || swap_form || this_form) {
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
            } else if (swap_form) {
                const unsigned long long swap_total = swap_total_bytes();
                const unsigned long long swap_used = swap_used_bytes();
                if (path[4] == "used")
                    bytes = swap_used;
                else if (path[4] == "total")
                    bytes = swap_total;
                else
                    bytes = swap_total > swap_used ? swap_total - swap_used : 0;
            }
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

    return std::nullopt;
}

} // namespace satellite
