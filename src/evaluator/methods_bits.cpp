#include "evaluator/eval_internal.hpp"

#include <optional>

// The method table for binary and hexadecimal, §21.
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

std::optional<ValuePtr> Evaluator::method_bits(
    const ValuePtr &recv, const std::string &name,
    const std::vector<ValuePtr> &argv, const char *module, Span span)
{
    auto arity = [&](size_t want) {
        return method_arity(module, name, argv, want, span);
    };

    // --- binary and hex, §21 ------------------------------------------------
    //
    // ONE BLOCK FOR BOTH, because they answer the same questions and differ
    // only in the base their digits are written in — which is the §8.7 test for
    // whether two things share a method set. The radix is read where it
    // matters and nowhere else.
    if (const Bits *self = as_bits(*recv)) {
        // The WIDTH, not the magnitude. `x0009999CCC`.digits() is 10, and that
        // is the number a program needs to know a value survived a round trip.
        if (name == "digits")
            return arity(0) ? make_value(Number::from_u64(self->digits.size()))
                            : nullptr;

        // What it costs on a socket. Rounds up, because there is no seven-bit
        // byte: `b101`.bytes() is 1.
        if (name == "bytes")
            return arity(0)
                       ? make_value(Number::from_u64(bits_byte_count(*self)))
                       : nullptr;

        // THE LOSSY DIRECTION, and the program has to ask for it by name.
        // x0009 -> 9. The leading zeros are gone because a number has no width
        // (§8.1), which is the whole reason §21 is not just a number literal in
        // another base.
        if (name == "to_number") {
            if (!arity(0))
                return nullptr;
            Number out;
            Number::parse(bits_to_decimal(self->radix, self->digits), out);
            return make_value(out);
        }

        if (name == "to_hex" || name == "to_binary") {
            if (!arity(0))
                return nullptr;
            const Bits converted =
                bits_convert(*self, name == "to_hex" ? 16u : 2u);
            return make_value(make_bits(converted.radix, converted.digits));
        }

        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;

        // Same radix only, and that is a refusal rather than a silent
        // conversion. Joining a hex value to a binary one has two defensible
        // answers — convert the argument, or convert the receiver — and §18's
        // posture is that a call with two defensible answers is a call the
        // program should write out. .to_hex() on the argument says which.
        if (name == "concat") {
            if (!arity(1))
                return nullptr;
            const Bits *other = as_bits(*argv[0]);
            if (!other) {
                fail(span, std::string(module) +
                               ".concat wants a value of the same type, got " +
                               to_string(*argv[0]));
                return nullptr;
            }
            if (other->radix != self->radix) {
                fail(span, std::string(module) +
                               ".concat wants the same radix; convert with "
                               ".to_hex() or .to_binary() first");
                return nullptr;
            }
            return make_value(
                make_bits(self->radix, self->digits + other->digits));
        }
    }
    return std::nullopt;
}

} // namespace satellite
