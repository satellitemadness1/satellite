// The unary and binary operators.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share. Indexing and slicing were here too until the
// 2026-08-24 search work; they are next door in subscripts.cpp now, at the seam
// this file's own banner comment already drew.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------

ValuePtr Evaluator::eval_unary(const Unary &node, Span span)
{
    if (!node.operand) {
        fail(span, "malformed unary " + node.op);
        return nullptr;
    }
    ValuePtr v = eval(*node.operand);
    if (failed() || !v)
        return nullptr;

    if (node.op == "-") {
        const Number *n = std::get_if<Number>(v.get());
        if (!n) {
            fail(span, "unary - wants a satellite.variable.number, got " +
                       to_string(*v));
            return nullptr;
        }
        return make_value(n->negated());
    }
    if (node.op == "!") {
        const bool *b = std::get_if<bool>(v.get());
        if (!b) {
            fail(span, "! wants a satellite.variable.bool, got " +
                       to_string(*v));
            return nullptr;
        }
        return make_value(!*b);
    }

    fail(span, "unknown unary operator " + node.op);
    return nullptr;
}

ValuePtr Evaluator::eval_binary(const Binary &node, Span span)
{
    if (!node.left || !node.right) {
        fail(span, "malformed " + node.op);
        return nullptr;
    }
    ValuePtr lhs = eval(*node.left);
    if (failed() || !lhs)
        return nullptr;
    ValuePtr rhs = eval(*node.right);
    if (failed() || !rhs)
        return nullptr;

    // Equality is defined on every pair of values, including across types,
    // where it is simply false. Ordering is not.
    if (node.op == "==")
        return make_value(value_equals(*lhs, *rhs));
    if (node.op == "!=")
        return make_value(!value_equals(*lhs, *rhs));

    const Number *ln = std::get_if<Number>(lhs.get());
    const Number *rn = std::get_if<Number>(rhs.get());
    const SatString *ls = as_string(*lhs);
    const SatString *rs = as_string(*rhs);

    if (node.op == "+" && ls && rs)
        return make_value(*ls + *rs);

    // A number on either side of `+` from a string renders itself, so
    // "count is " + n says what it reads as. This is the ONE coercion in the
    // language and it is deliberately narrow: it fires only for `+`, only when
    // the other operand is already a string, and only for a number. It is not
    // a step toward general coercion — `"3" + 4` is still not 7, because
    // string-to-number does not exist in either direction (§8.1's "one number
    // type, one answer" is about arithmetic, and nothing here is arithmetic).
    //
    // The rendering is `Number::to_string` through the same `to_string(Value)`
    // visitor `.to_string()` itself calls, so the two spellings can never
    // disagree — §8.1.1 fixes how a number renders in exactly one place, and
    // this is not a second one. `encode_raw`, never `encode`: §3.3, and a
    // rendered number holds no backslash to expand anyway.
    if (node.op == "+" && ls && rn)
        return make_value(*ls + encode_raw(to_string(*rhs)));
    if (node.op == "+" && ln && rs)
        return make_value(encode_raw(to_string(*lhs)) + *rs);

    if (ln && rn) {
        if (node.op == "+") return make_value(Number::add(*ln, *rn));
        if (node.op == "-") return make_value(Number::sub(*ln, *rn));
        if (node.op == "*") return make_value(Number::mul(*ln, *rn));
        if (node.op == "/") {
            if (rn->is_zero()) {
                fail(span, "division by zero");
                return nullptr;
            }
            return make_value(Number::divide(*ln, *rn, division_digits_));
        }
        if (node.op == "%") {
            if (rn->is_zero()) {
                fail(span, "modulo by zero");
                return nullptr;
            }
            return make_value(Number::modulo(*ln, *rn));
        }
        const int order = Number::compare(*ln, *rn);
        if (node.op == "<")  return make_value(order < 0);
        if (node.op == "<=") return make_value(order <= 0);
        if (node.op == ">")  return make_value(order > 0);
        if (node.op == ">=") return make_value(order >= 0);
    }

    if (ls && rs) {
        if (node.op == "<")  return make_value(*ls < *rs);
        if (node.op == "<=") return make_value(*ls <= *rs);
        if (node.op == ">")  return make_value(*ls > *rs);
        if (node.op == ">=") return make_value(*ls >= *rs);
    }

    fail(span, node.op + " does not apply to " + to_string(*lhs) + " and " +
               to_string(*rhs));
    return nullptr;
}

} // namespace satellite
