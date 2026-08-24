// The built-in method surface — every method every type answers to.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// "satellite.variable.number.plus takes 1 argument, got 2" beats a bare
// "wrong number of arguments" every time.
std::string arity_message(const char *module, const std::string &name,
                          size_t want, size_t got)
{
    std::string out = std::string(module) + "." + name + " takes " +
                      std::to_string(want) +
                      (want == 1 ? " argument, got " : " arguments, got ") +
                      std::to_string(got);
    return out;
}

ValuePtr Evaluator::call_method(const ValuePtr &recv, const Expr &recv_expr,
                                const std::string &name,
                                const std::vector<ValuePtr> &argv, Span span)
{
    (void)recv_expr;

    // An instance answers for its own members before any built-in table is
    // consulted, so a spacesuit may name a method `length` or `to_string`
    // without colliding with the language's.
    //
    // `size` is the one built-in an instance can fall THROUGH to, and the order
    // above is what makes that safe: the suit is asked first, so a spacesuit
    // that already defines size() keeps its own, and adding .size() to the
    // language cannot change what any program that compiles today does. Only a
    // suit with no size() reaches the model in §8.7.
    if (const ObjectPtr *self = std::get_if<ObjectPtr>(recv.get())) {
        const bool own = *self && (*self)->suit &&
                         (*self)->suit->find_method(name) != nullptr;
        if (name == "size" && !own && *self && (*self)->suit) {
            if (!argv.empty()) {
                fail(span, arity_message(suit_name((*self)->suit).c_str(), name,
                                         0, argv.size()));
                return nullptr;
            }
            return make_value(Number(value_bytes(recv)));
        }
        return call_object_method(*self, name, argv, span);
    }

    const char *module = module_of(*recv);
    if (!module) {
        fail(span, "nil has no methods, so ." + name + "() has no receiver");
        return nullptr;
    }

    auto arity = [&](size_t want) {
        if (argv.size() == want)
            return true;
        fail(span, arity_message(module, name, want, argv.size()));
        return false;
    };
    auto number_arg = [&](size_t i, const Number *&out) {
        out = std::get_if<Number>(argv[i].get());
        if (!out) {
            fail(span, std::string(module) + "." + name +
                       " wants a satellite.variable.number, got " +
                       to_string(*argv[i]));
            return false;
        }
        return true;
    };

    // .size() answers for every receiver that has methods at all, so it sits
    // ABOVE the per-type tables rather than being copied into each of them.
    // That placement is the whole reason the answer stays consistent: a type
    // added later gets a correct .size() from the exhaustive visitor in
    // value.cpp without anyone remembering to add a row here.
    //
    // §8.7 is the model, and the short version is that this is bytes and
    // .length() is items — a string of five characters is 5 long and 10 big.
    if (name == "size")
        return arity(0) ? make_value(Number(value_bytes(recv))) : nullptr;

    // --- number ------------------------------------------------------------
    if (const Number *self = std::get_if<Number>(recv.get())) {
        // §8.7: `length` means "how many items" on a string, a list and a map,
        // and a number holds no items. Rather than give the word a second
        // meaning — which format.def refused to do when it made `has` a new
        // word instead of a reuse of `contains` — a number REFUSES length()
        // and names the two methods that answer what the caller probably meant.
        if (name == "length") {
            fail(span, "satellite.variable.number has no method length — "
                       "length() counts the items in a container and a number "
                       "holds none; use .digits() for its decimal digits, or "
                       ".size() for its bytes");
            return nullptr;
        }
        if (name == "digits")
            return arity(0) ? make_value(Number(self->digit_count())) : nullptr;
        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;
        if (name == "abs")
            return arity(0) ? make_value(self->abs()) : nullptr;
        if (name == "floor")
            return arity(0) ? make_value(self->floor()) : nullptr;
        if (name == "ceil")
            return arity(0) ? make_value(self->ceil()) : nullptr;
        if (name == "round")
            return arity(0) ? make_value(self->round()) : nullptr;

        const Number *rhs = nullptr;
        if (name == "plus")
            return arity(1) && number_arg(0, rhs)
                       ? make_value(Number::add(*self, *rhs)) : nullptr;
        if (name == "minus")
            return arity(1) && number_arg(0, rhs)
                       ? make_value(Number::sub(*self, *rhs)) : nullptr;
        if (name == "times")
            return arity(1) && number_arg(0, rhs)
                       ? make_value(Number::mul(*self, *rhs)) : nullptr;
        if (name == "divided_by") {
            if (!arity(1) || !number_arg(0, rhs))
                return nullptr;
            if (rhs->is_zero()) {
                fail(span, "division by zero");
                return nullptr;
            }
            return make_value(Number::divide(*self, *rhs, division_digits_));
        }
        if (name == "modulo") {
            if (!arity(1) || !number_arg(0, rhs))
                return nullptr;
            if (rhs->is_zero()) {
                fail(span, "modulo by zero");
                return nullptr;
            }
            return make_value(Number::modulo(*self, *rhs));
        }
    }

    // --- string ------------------------------------------------------------
    if (const SatString *self = as_string(*recv)) {
        auto string_arg = [&](size_t i, const SatString *&out) {
            out = as_string(*argv[i]);
            if (!out) {
                fail(span, std::string(module) + "." + name +
                           " wants a satellite.variable.string, got " +
                           to_string(*argv[i]));
                return false;
            }
            return true;
        };

        if (name == "length")
            return arity(0) ? make_value(Number(self->size())) : nullptr;
        if (name == "to_string")
            return arity(0) ? make_value(*self) : nullptr;

        const SatString *rhs = nullptr;
        if (name == "concat")
            return arity(1) && string_arg(0, rhs) ? make_value(*self + *rhs)
                                                  : nullptr;
        if (name == "contains")
            return arity(1) && string_arg(0, rhs)
                       ? make_value(self->find(*rhs) != SatString::npos)
                       : nullptr;
        if (name == "starts_with")
            return arity(1) && string_arg(0, rhs)
                       ? make_value(self->rfind(*rhs, 0) == 0)
                       : nullptr;
        if (name == "ends_with") {
            if (!arity(1) || !string_arg(0, rhs))
                return nullptr;
            bool ok = rhs->size() <= self->size() &&
                      self->compare(self->size() - rhs->size(), rhs->size(),
                                    *rhs) == 0;
            return make_value(ok);
        }
    }

    // --- bool --------------------------------------------------------------
    if (const bool *self = std::get_if<bool>(recv.get())) {
        auto bool_arg = [&](size_t i, bool &out) {
            const bool *b = std::get_if<bool>(argv[i].get());
            if (!b) {
                fail(span, std::string(module) + "." + name +
                           " wants a satellite.variable.bool, got " +
                           to_string(*argv[i]));
                return false;
            }
            out = *b;
            return true;
        };

        if (name == "negate")
            return arity(0) ? make_value(!*self) : nullptr;
        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;

        bool rhs = false;
        if (name == "and")
            return arity(1) && bool_arg(0, rhs) ? make_value(*self && rhs)
                                                : nullptr;
        if (name == "or")
            return arity(1) && bool_arg(0, rhs) ? make_value(*self || rhs)
                                                : nullptr;
    }

    // --- time --------------------------------------------------------------
    if (const Time *self = std::get_if<Time>(recv.get())) {
        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;

        // §8.2's whole arithmetic surface: the difference between two instants
        // is a NUMBER of nanoseconds, not a third instant, because
        // satellite.variable.duration is deferred and a time that could mean
        // either would need a mode flag on every value.
        if (name == "minus") {
            if (!arity(1))
                return nullptr;
            const Time *rhs = std::get_if<Time>(argv[0].get());
            if (!rhs) {
                fail(span, "satellite.variable.time.minus wants a "
                           "satellite.variable.time, got " +
                           to_string(*argv[0]));
                return nullptr;
            }
            return make_value(Number(self->ns - rhs->ns));
        }

        // Exact, all 61 bits of it. It was lossy above 2^53 while a number was
        // a double, which is why §8.2 insisted an ELAPSED time be spelled
        // a.minus(b); §8.1's decimal makes both spellings exact, and the
        // preference for minus() is now about durations rather than precision.
        if (name == "nanoseconds")
            return arity(0) ? make_value(Number(self->ns)) : nullptr;
    }

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

    // --- list --------------------------------------------------------------
    if (const List *self = as_list(*recv)) {
        if (name == "length")
            return arity(0) ? make_value(Number(self->size())) : nullptr;
        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;
        // .to_string() is the list as one value -- "[a, b]" -- and .lines() is
        // the list as a listing: one element per line, with what each element
        // is when it names something on disk. Two renderings because they
        // answer two questions, and the REPL echoes a list with this one.
        if (name == "lines")
            return arity(0) ? make_value(encode_raw(list_lines(*self))) : nullptr;
        if (name == "first" || name == "last") {
            if (!arity(0))
                return nullptr;
            if (self->empty()) {
                fail(span, "satellite.container.list." + name +
                           " on an empty list");
                return nullptr;
            }
            return name == "first" ? self->front() : self->back();
        }
        if (name == "contains") {
            if (!arity(1))
                return nullptr;
            for (const ValuePtr &item : *self)
                if (item && value_equals(*item, *argv[0]))
                    return make_value(true);
            return make_value(false);
        }
    }

    // --- map ---------------------------------------------------------------
    // Delegated to src/evaluator/maps.cpp, which owns the key contract. The mutating
    // half (.set / .remove) never arrives here: is_mutator routes it to
    // call_mutator before the receiver is even evaluated, because it has to
    // write back through the receiver's storage slot.
    if (as_map(*recv))
        return call_map_method(recv, name, argv, span);

    fail(span, std::string(module) + " has no method " + name);
    return nullptr;
}

} // namespace satellite
