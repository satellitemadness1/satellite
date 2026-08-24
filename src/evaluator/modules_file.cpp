#include "evaluator/eval_internal.hpp"

#include <iostream>
#include <optional>

#include "console_output/console.hpp"
#include "random_numbers/random.hpp"

// satellite.time.now, satellite.file.open and satellite.file.new
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

namespace {

// The mode words, in ONE table, because satellite.file.open and
// satellite.file.new ask the same question and a second copy is a second chance
// for the two to disagree about what "append" means.
//
// §8.3.1 fixed modes as WORDS and not flags — "the language has no enum and
// wants none: the three words say at the call site what a bitmask never does" —
// so read-and-write arrives as a FOURTH WORD and not as a third argument. That
// choice is the one worth recording, because the obvious alternative was to
// hang a second axis off the second slot:
//
//   * The second argument already means the access mode, in every call written
//     since §8.3.1. Giving it a second meaning would silently reinterpret
//     satellite.file.open(p, "write") the day a "text"/"binary" word collided
//     with a mode word, and would leave a reader unable to tell which axis a
//     given string was on.
//   * An optional THIRD argument for text/binary was the other candidate, and
//     it dies for the reason recorded at file_mode_error below: there is no
//     text/binary distinction for satellite to make, so the third slot would
//     have carried a flag that does nothing.
//
// A fourth word costs one row here and one id in format.def, and it reads at
// the call site exactly as the other three do.
struct FileMode {
    const char *word;
    int flags;
    bool readable;
    bool writable;
};

constexpr FileMode kFileModes[] = {
    {"read",   O_RDONLY,                      true,  false},
    {"write",  O_WRONLY | O_CREAT | O_TRUNC,  false, true },
    {"append", O_WRONLY | O_CREAT | O_APPEND, false, true },

    // O_RDWR | O_APPEND, and never a bare O_RDWR. That is forced rather than
    // preferred: POSIX gives one file offset per open file description and the
    // language has no .seek() (§8.3 has no seek in it, and none is deferred in
    // §12 either), so on a bare O_RDWR handle a write would land wherever the
    // last read left that shared offset — a position no satellite program can
    // observe, name or control. O_APPEND removes the question: a write goes on
    // the END, and the kernel does the seek-and-write atomically, so two
    // snapshots of the same handle cannot interleave into each other's bytes.
    //
    // It is also, exactly, what the request asked for — "my_file.write(...)
    // appends the line to the file" — for a handle that can also read.
    // .read() rewinds (methods.cpp), so the two directions do not fight over
    // the one offset they share.
    {"read_append", O_RDWR | O_CREAT | O_APPEND, true, true},
};

const FileMode *find_file_mode(const std::string &word)
{
    for (const FileMode &mode : kFileModes)
        if (word == mode.word)
            return &mode;
    return nullptr;
}

// The list of mode words, built from the table so the message cannot fall
// behind the code the way a hand-written list of three did when a fourth
// landed.
std::string file_mode_list()
{
    std::string out;
    const size_t count = sizeof(kFileModes) / sizeof(kFileModes[0]);
    for (size_t i = 0; i < count; i++) {
        if (i)
            out += (i + 1 == count) ? " or " : ", ";
        out += std::string("\"") + kFileModes[i].word + "\"";
    }
    return out;
}

// What to say about a word that is not a mode — and the one word that needs
// more than a list.
//
// THERE IS NO "text" AND NO "binary", AND THE REFUSAL IS LOUD. The language
// refuses rather than pretends: §18 refuses a digit count past
// MAX_RANDOM_DIGITS instead of quietly clamping it, and §8.6 makes a missing
// key an error instead of returning a nil that would be indistinguishable from
// a stored one. A mode flag that did nothing would be the same mistake with a
// friendlier face, and it would be worse than no flag, because a program that
// passed "binary" would believe something had been arranged for it.
//
// Three pieces of evidence, none of them taste:
//
//   1. POSIX has no newline translation to switch off. fopen's "b" is
//      documented as having no effect on a POSIX system, and open(2) has no
//      equivalent bit at all. There is nothing for the flag to control.
//   2. A satellite string is ALREADY a byte container. encode_byte
//      (satellite_string.cpp) maps every one of the 256 possible bytes to
//      exactly one SatChar — the code table for the ones it names, §8.5's raw
//      area for the rest — and decode() maps each back. .read() calls
//      encode_raw, so it round-trips arbitrary bytes today. A "binary" mode
//      would be spelled the same way "text" is, byte for byte.
//   3. The ONE transformation that would differ is encode(), which expands
//      backslash escapes — and §3.3 has caught that exact substitution three
//      times, most recently inside .read() itself, where reading a program
//      containing \home rewrote it to the reader's home directory. Making it a
//      mode would be shipping a known defect as a feature.
std::string file_mode_error(const char *fn, const std::string &how)
{
    if (how == "text" || how == "binary")
        return std::string(fn) + " has no \"" + how + "\" mode, and needs "
               "none: a satellite.variable.string already holds arbitrary "
               "bytes, .read() maps one byte to one character and back, and "
               "POSIX has no newline translation to turn off. The mode must be "
               + file_mode_list();
    return std::string(fn) + " mode must be " + file_mode_list() + ", not \"" +
           how + "\"";
}

// One handle, opened once, with the failure recorded rather than raised.
//
// §8.3.1: "A failed open is a value, not an error. The handle comes back
// holding errno and the caller asks .ok(). Failing at the call site instead
// would make 'does this file exist' unanswerable without killing the program
// that asked." Both constructors go through here so that neither can drift out
// of that contract on its own.
//
// `flags` is what to open with NOW; `mode->flags` is what .open() reopens with
// later, and satellite.file.new is why they are two arguments and not one — see
// FileHandle::reopen_flags in value.hpp.
FilePtr open_file_handle(const std::string &name, const FileMode &mode,
                         int flags)
{
    FilePtr handle = std::make_shared<FileHandle>();
    handle->path = name;
    handle->readable = mode.readable;
    handle->writable = mode.writable;
    handle->reopen_flags = mode.flags;

    const int fd = ::open(name.c_str(), flags, 0644);
    if (fd < 0)
        handle->last_error.store(errno);
    else
        handle->fd.store(fd);
    return handle;
}

} // namespace

std::optional<ValuePtr> Evaluator::module_time_and_file(
    const std::string &full, const std::vector<std::string> & /*path*/,
    const std::vector<ValuePtr> &argv, Span span)
{
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

    // satellite.file.open(path, mode) — one of the two ways a file value comes
    // into existence, and the reason a declaration is neither: opening needs a
    // path and a place to report failure, and a declaration has neither.
    //
    // Modes are strings rather than flags because the language has no enum and
    // wants none; the words say what they do at the call site, which a bitmask
    // never does. THE ARITY AND THE MEANING OF BOTH SLOTS ARE UNCHANGED — every
    // call written against §8.3.1 opens the same file with the same access, and
    // the only thing "read_append" adds is a fourth word the second slot will
    // now accept. That was the whole point of putting read-and-write in the
    // mode word instead of in a new argument; see kFileModes above.
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

        const FileMode *chosen = find_file_mode(how);
        if (!chosen) {
            fail(span, file_mode_error("satellite.file.open", how));
            return nullptr;
        }

        // A failed open is a VALUE, not an error: the caller asks .ok() and
        // decides. Failing here instead would make "does this file exist"
        // unanswerable without crashing the program that asked.
        return make_value(
            Value(open_file_handle(name, *chosen, chosen->flags)));
    }

    // satellite.file.new(path[, mode]) — make a file that is NOT there, and
    // hand back a handle already open on it.
    //
    // IT REFUSES TO CLOBBER, and the argument is not squeamishness. The
    // truncating spelling already exists: satellite.file.open(path, "write") is
    // O_CREAT | O_TRUNC and has been since §8.3.1. A `new` that truncated would
    // therefore be a SECOND spelling of an operation the language already has,
    // while leaving "make this only if it is not there" with no spelling at
    // all — a strictly worse trade than the other way round. And O_EXCL is the
    // kernel answering the question atomically; a check-then-create written in
    // satellite would be a TOCTOU race, which is not a thing to hand a language
    // whose file surface exists to be used from scripts.
    //
    // A PATH THAT EXISTS IS A VALUE, NOT AN ERROR. The handle comes back
    // holding EEXIST and the caller asks .ok(), exactly as §8.3.1 requires of a
    // failed open — "does this file already exist" is a question a script asks
    // constantly and has to survive being answered yes.
    //
    // THE MODE IS OPTIONAL AND DEFAULTS TO "read_append". That default is the
    // whole of "let's automatically open the file sometimes, and not even
    // require .open() for new files": what comes back is already open, in the
    // one mode that can both write the file you just made and read it back, so
    // .open() is never REQUIRED on a fresh handle. Optional rather than a
    // second path name for the reason format.def records over P_HELP:
    // .new(path) and .new(path, mode) are one question about one file, so they
    // are one name with two shapes.
    if (full == "satellite.file.new") {
        if (argv.empty() || argv.size() > 2) {
            fail(span, "satellite.file.new takes 1 or 2 arguments, got " +
                       std::to_string(argv.size()));
            return nullptr;
        }
        const SatString *path = as_string(*argv[0]);
        if (!path) {
            fail(span, "satellite.file.new wants a satellite.variable.string "
                       "path, got " + to_string(*argv[0]));
            return nullptr;
        }

        std::string how = "read_append";
        if (argv.size() == 2) {
            const SatString *mode = as_string(*argv[1]);
            if (!mode) {
                fail(span, "satellite.file.new wants a "
                           "satellite.variable.string mode, got " +
                           to_string(*argv[1]));
                return nullptr;
            }
            how = decode(*mode);
        }

        const FileMode *chosen = find_file_mode(how);
        if (!chosen) {
            fail(span, file_mode_error("satellite.file.new", how));
            return nullptr;
        }

        // O_TRUNC is dropped rather than carried: a file that O_EXCL just
        // created is empty, so truncating it is a no-op that would only confuse
        // anyone reading the flags. O_EXCL is NOT stored in reopen_flags, for
        // the reason value.hpp gives — a reopen of a file this call created
        // would otherwise fail with EEXIST against the program's own work.
        const int create =
            (chosen->flags & ~O_TRUNC) | O_CREAT | O_EXCL;
        return make_value(
            Value(open_file_handle(decode(*path), *chosen, create)));
    }

    return std::nullopt;
}

} // namespace satellite
