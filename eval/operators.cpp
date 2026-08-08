// Indexing, slicing, and the unary and binary operators.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

namespace satellite {

ValuePtr Evaluator::eval_index(const Index &node, Span span)
{
    if (!node.target || !node.subscript) {
        fail(span, "malformed index");
        return nullptr;
    }
    ValuePtr target = eval(*node.target);
    if (failed() || !target)
        return nullptr;
    ValuePtr sub = eval(*node.subscript);
    if (failed() || !sub)
        return nullptr;

    long long i = 0;
    if (!as_index(*sub, i)) {
        fail(node.subscript->span,
             "index must be a whole satellite.variable.number, got " +
             to_string(*sub));
        return nullptr;
    }

    // An out-of-range INDEX is an error; only a slice clamps (§7).
    if (const List *list = as_list(*target)) {
        long long len = static_cast<long long>(list->size());
        if (i < 0)
            i += len;
        if (i < 0 || i >= len) {
            fail(node.subscript->span, "index " + to_string(*sub) +
                                       " is outside a list of length " +
                                       std::to_string(len));
            return nullptr;
        }
        // O(1) and no copy at all: the child pointer is returned as-is.
        ValuePtr item = (*list)[static_cast<size_t>(i)];
        return item ? item : make_value(std::monostate{});
    }

    if (const SatString *s = as_string(*target)) {
        // Selects satellite characters, not display characters: encode("hi
        // \home!") is 4 SatChars that decode to 16 display bytes, so s[2] is
        // one unit that displays as a whole home directory. It is the only
        // O(1), stable rule, and it is unique to satellite (§7).
        long long len = static_cast<long long>(s->size());
        if (i < 0)
            i += len;
        if (i < 0 || i >= len) {
            fail(node.subscript->span, "index " + to_string(*sub) +
                                       " is outside a string of length " +
                                       std::to_string(len));
            return nullptr;
        }
        return make_value(SatString(1, (*s)[static_cast<size_t>(i)]));
    }

    fail(span, to_string(*target) + " cannot be indexed");
    return nullptr;
}

ValuePtr Evaluator::eval_slice(const Slice &node, Span span)
{
    if (!node.target) {
        fail(span, "malformed slice");
        return nullptr;
    }
    ValuePtr target = eval(*node.target);
    if (failed() || !target)
        return nullptr;

    long long len = 0;
    if (const List *list = as_list(*target))
        len = static_cast<long long>(list->size());
    else if (const SatString *s = as_string(*target))
        len = static_cast<long long>(s->size());
    else {
        fail(span, to_string(*target) + " cannot be sliced");
        return nullptr;
    }

    auto bound = [&](const ExprPtr &e, long long fallback, long long &out) {
        if (!e) {
            out = fallback;
            return true;
        }
        ValuePtr v = eval(*e);
        if (failed() || !v)
            return false;
        if (!as_index(*v, out)) {
            fail(e->span, "slice bound must be a whole "
                          "satellite.variable.number, got " + to_string(*v));
            return false;
        }
        return true;
    };

    long long lo = 0;
    long long hi = 0;
    if (!bound(node.lo, 0, lo) || !bound(node.hi, len, hi))
        return nullptr;
    clamp_range(lo, hi, len);

    if (const List *list = as_list(*target)) {
        // A full slice is the same list: no copy, and pointer identity is
        // preserved so `l[:]` is genuinely free.
        if (lo == 0 && hi == len)
            return target;
        // Slicing copies POINTERS only — the children stay shared.
        return make_value(List(list->begin() + lo, list->begin() + hi));
    }

    const SatString &s = *as_string(*target);
    if (lo == 0 && hi == len)
        return target;
    // Unlike a list slice, a string slice really does copy characters.
    return make_value(s.substr(static_cast<size_t>(lo),
                               static_cast<size_t>(hi - lo)));
}

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
