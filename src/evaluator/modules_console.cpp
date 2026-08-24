#include "evaluator/eval_internal.hpp"

#include <iostream>
#include <optional>

#include "console_output/console.hpp"
#include "random_numbers/random.hpp"

// satellite.console.display and satellite.console.input
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

std::optional<ValuePtr> Evaluator::module_console(
    const std::string &full, const std::vector<std::string> & /*path*/,
    const std::vector<ValuePtr> &argv, Span span)
{
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
    return std::nullopt;
}

} // namespace satellite
