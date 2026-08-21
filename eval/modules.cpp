// satellite.<module>.* — the module function surface.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

#include "../random.hpp"

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

    if (full == "satellite.console.display") {
        if (argv.size() != 1) {
            fail(span, arity_message("satellite.console", "display", 1,
                                     argv.size()));
            return nullptr;
        }
        output_ += to_string(*argv[0]);
        output_ += "\n";
        return make_value(std::monostate{});
    }

    fail(span, "no such module function: " + full);
    return nullptr;
}

// ---------------------------------------------------------------------------
// Capsule calls
// ---------------------------------------------------------------------------

} // namespace satellite
