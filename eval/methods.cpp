// The built-in method surface — every method every type answers to.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

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
    if (const ObjectPtr *self = std::get_if<ObjectPtr>(recv.get()))
        return call_object_method(*self, name, argv, span);

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

    // --- number ------------------------------------------------------------
    if (const Number *self = std::get_if<Number>(recv.get())) {
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
            const int fd = file->fd.load();
            if (fd < 0) {
                fail_reason("cannot read from a closed file:");
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

    fail(span, std::string(module) + " has no method " + name);
    return nullptr;
}

} // namespace satellite
