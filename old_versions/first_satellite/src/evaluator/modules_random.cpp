#include "evaluator/eval_internal.hpp"

#include <iostream>
#include <optional>

#include "console_output/console.hpp"
#include "random_numbers/random.hpp"

// satellite.random.<tier>, and the .range form
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

std::optional<ValuePtr> Evaluator::module_random(
    const std::string & /*full*/, const std::vector<std::string> &path,
    const std::vector<ValuePtr> &argv, Span span)
{
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

    return std::nullopt;
}

} // namespace satellite
