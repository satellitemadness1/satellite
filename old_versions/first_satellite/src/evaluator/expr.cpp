// Expressions: names, members, calls.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share. The call form -- Evaluator::eval_call, and the
// bare_dotted_path helper only it uses -- is next door in expr_call.cpp, moved
// there when this file passed 400 lines. What is left here is the dispatch it
// arrives through, member access, and the argument evaluation it calls.

#include "evaluator/eval_internal.hpp"

namespace satellite {

ValuePtr Evaluator::eval(const Expr &expr)
{
    if (depth_ >= max_depth_) {
        fail(expr.span, "expression nests deeper than satellite.library."
                        "system.max_depth (" + std::to_string(max_depth_) + ")");
        return nullptr;
    }
    DepthGuard guard(depth_);

    if (const NumberLit *n = std::get_if<NumberLit>(&expr))
        return make_value(n->value);

    if (const StringLit *s = std::get_if<StringLit>(&expr))
        return make_value(s->value);

    // §21. Normalisation happens HERE and not in the lexer, because the lexer
    // reports what was written and `x00ff` was written in lower case — unparse
    // needs that spelling back. The VALUE is the upper-cased form, so `x00ff`
    // and `x00FF` are one value and `==` does not depend on how it was typed.
    if (const BitsLit *b = std::get_if<BitsLit>(&expr))
        return make_value(
            make_bits(b->radix, bits_normalise(b->radix, b->text.substr(1))));

    // Reached only where a duration is NOT allowed, because the one place it is
    // — the argument of satellite.console.display — is taken by eval_call
    // below before it evaluates its arguments. So this is the error for
    // `x = 100ms`, `l.append(100ms)`, `(100ms).plus(1)` and every other
    // position, and it names the form that works instead of saying "no".
    if (const DurationLit *d = std::get_if<DurationLit>(&expr)) {
        fail(expr.span, duration_misuse(d->text));
        return nullptr;
    }

    // Reached only where a named argument is NOT allowed, for the same reason
    // the DurationLit arm above is: the one place it IS allowed is taken by
    // eval_call before it evaluates its arguments. So this is the error for
    // `x = end="!"`, `l.append(end="!")` and every other position.
    if (const NamedArg *named = std::get_if<NamedArg>(&expr)) {
        fail(expr.span, named_arg_misuse(named->name));
        return nullptr;
    }

    if (std::holds_alternative<SatelliteLit>(expr))
        return make_value(std::monostate{});   // the runtime singleton

    // A list literal, evaluated LEFT TO RIGHT. The order is observable — an
    // element can be a call that displays something — so it is fixed here
    // rather than left to whatever the loop happens to do.
    //
    // The list is built complete and then frozen by make_list, which is the
    // same contract every other list in the language has (value.hpp): nothing
    // holds a reference to the vector while it is still growing, so publishing
    // it is safe with no lock.
    if (const ListLit *literal = std::get_if<ListLit>(&expr)) {
        List items;
        items.reserve(literal->elements.size());
        for (const ExprPtr &element : literal->elements) {
            // A null element means the parser already failed on this literal
            // and reported it; there is nothing left to evaluate and nothing
            // useful to add.
            if (!element)
                return nullptr;
            ValuePtr value = eval(*element);
            if (!value)
                return nullptr;   // the element's own error already stands
            items.push_back(std::move(value));
        }
        return make_value(make_list(std::move(items)));
    }

    if (const Name *name = std::get_if<Name>(&expr)) {
        if (name->slot == SLOT_CAPSULE) {
            fail(expr.span, "capsule " + name->text +
                            " cannot be used as a value");
            return nullptr;
        }
        if (name->slot == SLOT_METHOD) {
            fail(expr.span, "method " + name->text +
                            " cannot be used as a value; call it with " +
                            name->text + "()");
            return nullptr;
        }
        if (name->slot == SLOT_SUIT) {
            fail(expr.span, "spacesuit " + name->text +
                            " cannot be used as a value; build one with " +
                            name->text + "()");
            return nullptr;
        }
        return read_slot(slot_of(expr), name->text, expr.span);
    }

    if (const Member *m = std::get_if<Member>(&expr))
        return eval_member(*m, expr.span);
    if (const Call *c = std::get_if<Call>(&expr))
        return eval_call(*c, expr.span);
    if (const Index *i = std::get_if<Index>(&expr))
        return eval_index(*i, expr.span);
    if (const Slice *s = std::get_if<Slice>(&expr))
        return eval_slice(*s, expr.span);
    if (const Unary *u = std::get_if<Unary>(&expr))
        return eval_unary(*u, expr.span);
    if (const Binary *b = std::get_if<Binary>(&expr))
        return eval_binary(*b, expr.span);

    fail(expr.span, "unhandled expression");
    return nullptr;
}

ValuePtr Evaluator::eval_member(const Member &node, Span span)
{
    // Flattened from the parts rather than by rebuilding an Expr around the
    // node, which would allocate on every member access.
    std::vector<std::string> path;
    if (node.target && flatten_path(*node.target, path)) {
        path.push_back(node.name);

        if (path.size() >= 2 && path[1] == "library") {
            if (path.size() != 4) {
                fail(span, "a library path is satellite.library."
                           "<namespace>.<name>, got " + join_path(path));
                return nullptr;
            }
            return read_slot(Slot{SLOT_GLOBAL, path[2], path[3], true},
                             join_path(path), span);
        }

        if (ValuePtr constant = module_constant(path))
            return constant;

        // satellite.console.display without the call parentheses, and any
        // other module path used as a value.
        fail(span, join_path(path) + " is a module path, not a value");
        return nullptr;
    }

    // THE ONE VALUE THAT ANSWERS A BARE SELECTOR: the arguments object.
    //
    // `args.cxx_compiler` with no parentheses, which is the spelling that was
    // asked for and the one that reads right for a bag of named strings. It is
    // a NARROWING of the refusal below by exactly one case, not a lifting of
    // it: this object has no fields a program could confuse a name with, no
    // way for a program to construct one, and a method set that is checked
    // disjoint from its entry names. Every other value still gets the message
    // underneath, unchanged.
    //
    // Evaluated here rather than lowered in the resolver because the receiver
    // is an expression and its type is not known until it is evaluated --
    // `f().cxx_compiler` has to work if `args.cxx_compiler` does.
    if (node.target) {
        ValuePtr target = eval(*node.target);
        if (failed())
            return nullptr;
        if (target) {
            if (const Arguments *args = as_arguments(*target))
                return argument_named(*args, node.name, span);
        }
    }

    // Bare field access is deliberately not in the language: accessor methods
    // only, so `my_window.height()` is the one spelling (§8.3, §12).
    fail(span, "no bare field access in satellite; call " + node.name +
               "() instead");
    return nullptr;
}

bool Evaluator::eval_args(const std::vector<ExprPtr> &args,
                          std::vector<ValuePtr> &argv)
{
    argv.reserve(args.size());
    for (const ExprPtr &arg : args) {
        if (!arg)
            return false;
        ValuePtr v = eval(*arg);
        if (failed() || !v)
            return false;
        argv.push_back(std::move(v));
    }
    return true;
}

} // namespace satellite
