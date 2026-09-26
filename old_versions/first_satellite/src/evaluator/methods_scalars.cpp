#include "evaluator/eval_internal.hpp"

#include <optional>

// The method table for number, string, bool and time.
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

std::optional<ValuePtr> Evaluator::method_scalars(
    const ValuePtr &recv, const std::string &name,
    const std::vector<ValuePtr> &argv, const char *module, Span span)
{
    auto arity = [&](size_t want) {
        return method_arity(module, name, argv, want, span);
    };
    auto number_arg = [&](size_t i, const Number *&out) {
        return method_number_arg(module, name, argv, i, out, span);
    };

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
    return std::nullopt;
}

} // namespace satellite
