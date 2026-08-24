#include "evaluator/eval_internal.hpp"

#include <optional>

// The method table for satellite.variable.file.
//
// One arm of Evaluator::call_method, which was 715 lines in a single function
// before the 2026-08-24 split. The branch bodies below are UNCHANGED.
//
// std::nullopt means "this receiver is not mine"; an ENGAGED optional means the
// arm answered, and the ValuePtr inside may be null because that is how a failed
// method call reports itself once fail() has run. Every `return <expr>;` in the
// moved code therefore became an engaged optional with no edit at all.
//
// `arity` and `number_arg` were lambdas in the old prologue. They are still
// lambdas with the SAME NAMES and the same call syntax, so not one call site
// changed; each now forwards to the one shared implementation in methods.cpp.

namespace satellite {

std::optional<ValuePtr> Evaluator::method_file(
    const ValuePtr &recv, const std::string &name,
    const std::vector<ValuePtr> &argv, const char *module, Span span)
{
    auto arity = [&](size_t want) {
        return method_arity(module, name, argv, want, span);
    };

    // --- file ---------------------------------------------------------------
    if (const FilePtr *self = std::get_if<FilePtr>(recv.get())) {
        const FilePtr &file = *self;
        if (!file) {
            fail(span, "nil has no methods, so ." + name + "() has no receiver");
            return nullptr;
        }

        auto fail_reason = [&](const char *what) {
            const int code = file->last_error.load();
            fail(span, std::string(what) + " " + file->path +
                       (code ? ": " + std::string(strerror(code)) : ""));
        };

        // The direction a handle was opened for, checked BEFORE the syscall.
        //
        // This is what FileHandle::readable and ::writable are for, and until
        // "read_append" they were not worth having: while every mode went one
        // way, asking for the other produced EBADF and "Bad file descriptor" —
        // a sentence about a descriptor that is in perfect health, printed at a
        // program that asked the wrong handle. Four modes make the mistake
        // ordinary, so the message names the mode that would have worked.
        auto wrong_direction = [&](bool allowed, const char *verb,
                                   const char *modes) {
            if (allowed)
                return false;
            fail(span, "cannot " + std::string(verb) + " " + file->path +
                       ": this satellite.variable.file was not opened for it — "
                       "open it with " + modes);
            return true;
        };

        if (name == "ok")
            return arity(0) ? make_value(file->fd.load() >= 0) : nullptr;
        if (name == "path")
            return arity(0) ? make_value(encode_raw(file->path)) : nullptr;
        if (name == "error") {
            if (!arity(0))
                return nullptr;
            const int code = file->last_error.load();
            return make_value(encode_raw(code ? strerror(code) : ""));
        }

        if (name == "read") {
            if (!arity(0))
                return nullptr;
            if (wrong_direction(file->readable, "read",
                                "\"read\" or \"read_append\""))
                return nullptr;
            const int fd = file->fd.load();
            if (fd < 0) {
                fail_reason("cannot read from a closed file:");
                return nullptr;
            }

            // .read() IS THE WHOLE FILE, FROM THE BEGINNING, on every handle.
            //
            // It always was, and nothing said so. Every open starts at offset
            // 0 and nothing in the language moved it, so "from the current
            // offset to EOF" and "the whole file" were the same sentence — with
            // one reachable exception, a SECOND .read() on the same handle,
            // which returned "" because the offset was already at the end.
            // Measured before this change: two .read().length() calls on a
            // six-byte file printed 6 then 0. "" is how an EMPTY FILE is
            // spelled, so the old answer was not merely unhelpful, it was a
            // different fact wearing the same string.
            //
            // "read_append" is what forced the decision rather than merely
            // permitting it: write-then-read-back on one handle is the whole
            // point of the mode, and O_APPEND leaves the shared offset at the
            // new end of the file, so without this the read-back would be that
            // same misleading "". Rewinding is the only answer available —
            // there is no .seek() to ask for anything else with, so "read from
            // wherever the offset happens to be" is a question no satellite
            // program can pose, and an answer no satellite program can use.
            //
            // ESPIPE is the honest exception: a pipe or a FIFO has no
            // beginning to return to, and there "read on from here" is the only
            // meaning the call can have. Any other lseek failure is a real
            // failure and is reported as one.
            if (::lseek(fd, 0, SEEK_SET) < 0 && errno != ESPIPE) {
                file->last_error.store(errno);
                fail_reason("cannot read");
                return nullptr;
            }

            std::string bytes;
            char buffer[65536];
            for (;;) {
                const ssize_t got = ::read(fd, buffer, sizeof buffer);
                if (got == 0)
                    break;
                if (got < 0) {
                    if (errno == EINTR)
                        continue;
                    file->last_error.store(errno);
                    fail_reason("cannot read");
                    return nullptr;
                }
                bytes.append(buffer, static_cast<size_t>(got));
            }

            // encode_raw, NEVER encode. §3.3 found this defect twice already —
            // encode() expands backslash escapes everywhere, so reading a
            // source file containing \home would rewrite it to the user's home
            // directory before the lexer ever saw it. Reading a file is the
            // third home of that same bug and the most damaging, because the
            // file being read is usually a program.
            return make_value(encode_raw(bytes));
        }

        // .write(s) PUTS EXACTLY THE BYTES OF s, and appends no newline of its
        // own. That is unchanged, and the request that it "append the line" is
        // met — but by the MODE and not by a hidden character.
        //
        // Verified by running it before any of this landed: two f.write("one")
        // / f.write("two") calls on one handle produced the six bytes `onetwo`,
        // so the call has always appended to what the handle had already
        // written rather than overwriting it, and has never added a separator.
        // The only thing that ever discarded existing bytes was O_TRUNC at open
        // time, which is what "write" mode means and what .clear() now spells
        // explicitly.
        //
        // ADDING A NEWLINE WOULD HAVE BEEN THE WRONG CHANGE, on three counts.
        // It is breaking — every byte count any existing program computes moves
        // by one per call. It removes something the caller cannot get back: a
        // file with no trailing newline, or a line assembled from several
        // writes, both become unwritable. And the language already has the
        // character, since §3.4 added a real \n escape, so f.write("x\n") says
        // what it means at the call site and needs no rule to be remembered.
        // §8.7's precedent is the same shape — a word means one thing, and
        // "write these bytes" is one thing.
        if (name == "write") {
            if (!arity(1))
                return nullptr;
            const SatString *text = as_string(*argv[0]);
            if (!text) {
                fail(span, "satellite.variable.file.write wants a "
                           "satellite.variable.string, got " +
                           to_string(*argv[0]));
                return nullptr;
            }
            if (wrong_direction(file->writable, "write to",
                                "\"write\", \"append\" or \"read_append\""))
                return nullptr;
            const int fd = file->fd.load();
            if (fd < 0) {
                fail_reason("cannot write to a closed file:");
                return nullptr;
            }

            const std::string bytes = decode(*text);
            size_t sent = 0;
            while (sent < bytes.size()) {
                const ssize_t put =
                    ::write(fd, bytes.data() + sent, bytes.size() - sent);
                if (put < 0) {
                    if (errno == EINTR)
                        continue;
                    file->last_error.store(errno);
                    return make_value(false);
                }
                sent += static_cast<size_t>(put);
            }
            return make_value(true);
        }

        // .clear() — the file back to zero bytes, through the handle already
        // holding it.
        //
        // It TRUNCATES AND REWINDS, and the second half is not decoration.
        // ftruncate() moves the file's length and leaves the offset where it
        // was, so on a "write" handle that had already written a kilobyte the
        // next .write() would land at offset 1024 and the kernel would fill the
        // gap with NUL bytes — a "cleared" file that is longer than it was and
        // full of zeros. Nobody asking for .clear() means that. On an O_APPEND
        // handle the seek is redundant, since every write re-seeks to the end
        // anyway; it is done unconditionally because a rule with an exception
        // is one more thing for the next mode to get wrong.
        //
        // The split between an error and a false is the one .write() already
        // draws: a PROGRAM mistake — the wrong mode, a closed handle — is a
        // language-level error, and a RUNTIME failure of the syscall is a value
        // the caller can test.
        if (name == "clear") {
            if (!arity(0))
                return nullptr;
            if (wrong_direction(file->writable, "clear",
                                "\"write\", \"append\" or \"read_append\""))
                return nullptr;
            const int fd = file->fd.load();
            if (fd < 0) {
                fail_reason("cannot clear a closed file:");
                return nullptr;
            }
            if (::ftruncate(fd, 0) < 0) {
                file->last_error.store(errno);
                return make_value(false);
            }
            if (::lseek(fd, 0, SEEK_SET) < 0) {
                file->last_error.store(errno);
                return make_value(false);
            }
            return make_value(true);
        }

        // .open() — reopen a handle that is closed, or retry one whose open
        // failed.
        //
        // §8.3 SAYS "NEVER SILENTLY REOPEN", AND THIS DOES NOT VIOLATE IT.
        // Read the sentence in place: "After close, other snapshots see a
        // closed handle and get a clean language-level error; never silently
        // reopen." It is the answer to what the RUNTIME does when a snapshot
        // uses a closed handle, and that answer is unchanged — .read(),
        // .write() and .clear() above each refuse a closed fd and say so, and
        // not one of them reopens anything on the program's behalf. What the
        // rule governs is the reopen nobody asked for. This one is a line of
        // source with a status the caller reads, which is the opposite of
        // silent.
        //
        // The wish it reconciles with — "let's automatically open the file
        // sometimes, and not even require .open() for new files" — is met
        // WITHOUT weakening any of that, because both constructors already hand
        // back an open handle: satellite.file.open and satellite.file.new open
        // the file themselves, so .open() is never REQUIRED, on a new file or
        // an old one. It exists for the two cases a constructor cannot cover,
        // which are the ones that come after it: a handle that was closed, and
        // a handle whose open failed and whose reason has since gone away.
        //
        // ALREADY OPEN IS TRUE AND DOES NOTHING, which is the same answer
        // .close() gives to a second close, and the only answer that keeps the
        // snapshot contract intact. Opening again would leak the live
        // descriptor and swap the file description out from under every other
        // snapshot of the handle with no close having happened — which is the
        // thing §8.3's sentence is really protecting, arrived at from the other
        // side.
        if (name == "open") {
            if (!arity(0))
                return nullptr;
            if (file->fd.load() >= 0)
                return make_value(true);

            const int fd = ::open(file->path.c_str(), file->reopen_flags, 0644);
            if (fd < 0) {
                file->last_error.store(errno);
                return make_value(false);
            }

            // Two snapshots racing to reopen: the loser closes the descriptor
            // it just opened rather than overwriting the winner's, for
            // .close()'s reason one step earlier — a handle must never hold a
            // number the OS has handed to something else, and it must never
            // drop one nothing will close.
            int closed = -1;
            if (!file->fd.compare_exchange_strong(closed, fd)) {
                ::close(fd);
                return make_value(true);
            }

            // The stale errno goes. .error() means "the most recent failure",
            // and everywhere else that stays true after a success — a failed
            // .write() describes one write and does not stop the handle being
            // open. An open failure describes THE HANDLE, so once the handle is
            // open the failure describes a state that no longer exists, and
            // .ok() answering true beside .error() answering "No such file or
            // directory" would be two true sentences that read as a
            // contradiction.
            file->last_error.store(0);
            return make_value(true);
        }

        if (name == "close") {
            if (!arity(0))
                return nullptr;
            // §8.3 requires this to return a status rather than be a
            // destructor: close() is where buffered writes commit, and it is
            // where ENOSPC and EIO are reported. A destructor has nobody left
            // to tell.
            //
            // exchange(), so two snapshots of the same file racing to close it
            // cannot both close the descriptor — the loser would be closing a
            // number the OS had already handed to something else.
            const int fd = file->fd.exchange(-1);
            if (fd < 0)
                return make_value(true);   // closing twice is not a failure
            if (::close(fd) < 0) {
                file->last_error.store(errno);
                return make_value(false);
            }
            return make_value(true);
        }
    }
    return std::nullopt;
}

} // namespace satellite
