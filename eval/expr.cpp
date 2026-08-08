// Expressions: names, members, calls.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

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

    if (std::holds_alternative<SatelliteLit>(expr))
        return make_value(std::monostate{});   // the runtime singleton

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

ValuePtr Evaluator::eval_call(const Call &node, Span span)
{
    if (!node.target) {
        fail(span, "call has no target");
        return nullptr;
    }

    // A capsule call. Which name is a capsule was settled by resolve(), so a
    // local of the same name shadows one, exactly as it shadows anything else.
    if (const Name *name = std::get_if<Name>(node.target.get())) {
        if (name->slot >= 0 || is_field_slot(name->slot)) {
            fail(span, name->text + " is a variable, not a capsule");
            return nullptr;
        }

        // my_class_name(...): construction. The one call whose target is a
        // type, and what `my_class_name x(...)` is written into by the parser.
        if (name->slot == SLOT_SUIT) {
            const SpacesuitInfo *suit = resolved_.find_suit(name->text);
            if (!suit) {
                fail(span, "no such spacesuit: " + name->text);
                return nullptr;
            }
            std::vector<ValuePtr> ctor_argv;
            if (!eval_args(node.args, ctor_argv))
                return nullptr;
            return construct(*suit, ctor_argv, span);
        }

        // A bare call inside a method is a call on the SAME receiver, so it
        // dispatches on that receiver's spacesuit like any other method call —
        // an override in a subclass is what an inherited method reaches.
        if (name->slot == SLOT_METHOD) {
            if (!current_self_) {
                fail(span, "internal: " + name->text +
                           " resolved to a method with no receiver");
                return nullptr;
            }
            std::vector<ValuePtr> method_argv;
            if (!eval_args(node.args, method_argv))
                return nullptr;
            // Copied, not referenced: the call may reassign the field the
            // receiver came from, and the object has to survive that.
            ObjectPtr self = current_self_;
            return call_object_method(self, name->text, method_argv, span);
        }

        const CapsuleInfo *info = resolved_.find(name->text);
        if (!info) {
            fail(span, "no such capsule: " + name->text);
            return nullptr;
        }
        // Arguments are evaluated in the CALLER's frame, before the callee's
        // frame exists. That ordering is what makes a recursive call see its
        // own argument rather than the activation it is about to create — the
        // failure §6 measured as fact() returning 1 for every input.
        std::vector<ValuePtr> capsule_argv;
        if (!eval_args(node.args, capsule_argv))
            return nullptr;
        return call_capsule(*info, name->text, capsule_argv, span);
    }

    // Every argument is fully reduced to a Value BEFORE anything that could
    // take a write lock. §7 is explicit that no satellite code may run inside
    // Library::update: std::mutex is not recursive, so my_list.append(
    // my_list.size()) would deadlock on the same variable.
    std::vector<ValuePtr> argv;
    if (!eval_args(node.args, argv))
        return nullptr;

    std::vector<std::string> path;
    if (const Member *m = std::get_if<Member>(node.target.get())) {
        if (m->target && flatten_path(*m->target, path)) {
            path.push_back(m->name);
            // satellite.library.<ns>.<var> is a variable; a call on it is a
            // method call on its value, not a module function.
            if (!(path.size() >= 2 && path[1] == "library")) {
                // ...and so is a call on a module CONSTANT:
                // satellite.bool.true.and(x) is `and` on the value
                // satellite.bool.true, not the module function
                // satellite.bool.true.and. Nothing in the shape of the path
                // distinguishes the two, so the constant table is what decides.
                const std::vector<std::string> receiver(path.begin(),
                                                        path.end() - 1);
                if (ValuePtr constant = module_constant(receiver))
                    return call_method(constant, *m->target, m->name, argv,
                                       span);
                return call_module(path, argv, span);
            }
        }

        // A spacesuit may name a method `append`, and its own method has to
        // win. The receiver's DECLARED type settles that without evaluating
        // it, which matters because the mutator path needs storage rather than
        // a value — evaluating first to find out would run the receiver's side
        // effects before rejecting `foo().append(x)`.
        if (is_mutator(m->name)) {
            const Slot slot = slot_of(*m->target);
            const Type *declared = slot.valid ? declared_type(slot) : nullptr;
            if (!declared || !declared->is_spacesuit())
                return call_mutator(*m->target, m->name, argv, span);
        }

        ValuePtr recv = eval(*m->target);
        if (failed() || !recv)
            return nullptr;
        return call_method(recv, *m->target, m->name, argv, span);
    }

    if (flatten_path(*node.target, path))
        return call_module(path, argv, span);

    fail(span, "this expression is not callable");
    return nullptr;
}

// ---------------------------------------------------------------------------
// Methods
// ---------------------------------------------------------------------------

} // namespace satellite
