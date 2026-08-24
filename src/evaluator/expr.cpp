// Expressions: names, members, calls.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "evaluator/eval_internal.hpp"

namespace satellite {

namespace {

// The dotted path an expression spells when it is nothing but bare words.
// Narrower than flatten_path on purpose: that one accepts a `satellite` root
// and a call anywhere in the chain, and neither can be a help topic.
bool bare_dotted_path(const Expr &expr, std::string &out,
                      const Name *&head)
{
    if (const Name *name = std::get_if<Name>(&expr)) {
        out = name->text;
        head = name;
        return true;
    }
    if (const Member *member = std::get_if<Member>(&expr)) {
        if (!member->target || !bare_dotted_path(*member->target, out, head))
            return false;
        out += "." + member->name;
        return true;
    }
    return false;
}

} // namespace

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

    // BEFORE eval_args, and that ordering is the whole mechanism. A duration is
    // not a value (ast.hpp), so it has nothing to be reduced to and would fail
    // as an argument like any other misuse; the pace form is therefore matched
    // on the ARGUMENT EXPRESSION, here, while the tree still says `100ms`.
    //
    // It is also why the check runs before the capsule and method arms above
    // it: `my_capsule(100ms)` has to reach the same complaint as every other
    // wrong position, and not a wrong-type error from inside a frame.
    if (node.args.size() == 1 && node.args[0] &&
        std::holds_alternative<DurationLit>(*node.args[0])) {
        const DurationLit &duration = std::get<DurationLit>(*node.args[0]);
        std::vector<std::string> path;
        if (!flatten_path(*node.target, path) ||
            join_path(path) != "satellite.console.display") {
            fail(node.args[0]->span, duration_misuse(duration.text));
            return nullptr;
        }
        return set_display_pace(duration, span);
    }

    // satellite.help(random), satellite.help(random.ultra) -- a TOPIC.
    //
    // Matched on the ARGUMENT EXPRESSION, before eval_args, for the same reason
    // the duration form above is: a topic is a WORD and not a value, so there
    // is nothing to reduce it to, and by the time argv exists the word is gone.
    //
    // §1 IS NOT WEAKENED, and the order of the tests below is the whole reason.
    // A bare word reaches the topic table only after every scope the user owns
    // has been asked -- resolve() asked the local, field, method, capsule and
    // spacesuit scopes (src/environment/names.cpp) and left the slot global
    // only if all of them said no, and satellite.library is asked here. So a
    // variable called `ultra` still wins its own name and still reaches
    // satellite.help(value). This is §19.7's rule for TRUE and FALSE, applied
    // to a word that is not reserved either.
    //
    // A DOTTED path skips the library question, because `random.ultra` cannot
    // name a variable at all: §10 makes the bare two-segment form illegal in
    // program source, so there is nothing for it to lose to.
    if (node.args.size() == 1 && node.args[0]) {
        std::vector<std::string> target;
        std::string topic;
        const Name *head = nullptr;
        if (flatten_path(*node.target, target) &&
            join_path(target) == "satellite.help" &&
            bare_dotted_path(*node.args[0], topic, head) && head) {
            const bool dotted = topic != head->text;
            const bool owned  = head->slot != SLOT_GLOBAL ||
                                (!dotted &&
                                 Library::instance().get(ns_, head->text));
            if (dotted || !owned) {
                const std::string listing = help_for_topic(topic);
                if (!listing.empty())
                    return make_value(encode_raw(listing));
                fail(node.args[0]->span,
                     "no help topic called " + topic +
                     " -- the topics are random, fast, normal, ultra and "
                     "wide, and satellite.help lists them");
                return nullptr;
            }
        }
    }

    // satellite.console.input(prompt, target) — matched on the ARGUMENT
    // EXPRESSIONS, for the same reason the two forms below and above are: the
    // second argument is a PLACE to write and not a value, so there is nothing
    // for eval_args to reduce it to.
    if (node.args.size() == 2 && node.args[0] && node.args[1] &&
        !std::holds_alternative<NamedArg>(*node.args[1])) {
        std::vector<std::string> path;
        if (flatten_path(*node.target, path) &&
            join_path(path) == "satellite.console.input")
            return console_input_into(*node.args[0], *node.args[1], span);
    }

    // satellite.console.display(x, end=<expr>) — a display that chooses its own
    // ending instead of taking the newline display always appends.
    //
    // Matched on the ARGUMENT EXPRESSIONS for the same reason the duration form
    // above is: a NamedArg is not a value, so it has nothing to be reduced to,
    // and by the time argv exists the name is gone. Being here also means a
    // named argument handed to anything else — a capsule, a method, another
    // module function — reaches ONE complaint that says what named arguments
    // are for, rather than a wrong-type error from inside a frame.
    for (size_t i = 0; i < node.args.size(); i++) {
        if (!node.args[i] || !std::holds_alternative<NamedArg>(*node.args[i]))
            continue;

        const NamedArg &named = std::get<NamedArg>(*node.args[i]);
        std::vector<std::string> path;
        const bool is_display = flatten_path(*node.target, path) &&
                                join_path(path) == "satellite.console.display";

        // The one accepted shape, and it is checked whole: display(value,
        // end=<expr>). A second named argument, a named argument first, or
        // `end` on anything else all fall through to the message below.
        if (is_display && named.name == "end" && i == 1 &&
            node.args.size() == 2 && node.args[0] &&
            !std::holds_alternative<NamedArg>(*node.args[0]))
            return display_with_end(*node.args[0], *named.value, span);

        fail(node.args[i]->span, named_arg_misuse(named.name));
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

        // A spacesuit may name a method `append`, `set` or `remove`, and its
        // own method has to win. The receiver's DECLARED type settles that for
        // free whenever it is known:
        //
        //   declared container -> a container mutation; call_mutator, which
        //                         needs storage rather than a value
        //   declared spacesuit -> the suit's own method; fall through
        //
        // When nothing static settles it — a temporary, or a slot with no
        // recorded type — the VALUE has to decide, which means evaluating the
        // receiver and paying its side effects before a possible error. That is
        // why it is the last resort and not the rule: `foo().append(x)` still
        // fails, now after foo() has run.
        //
        // The previous form of this test asked `!declared ||
        // !declared->is_spacesuit()` and so sent EVERY receiver of unknown type
        // to call_mutator. A spacesuit method named `set` — and two of this
        // repo's own example programs have one — then failed with a message
        // about writing back through a receiver whenever it was called on
        // anything but a declared spacesuit-typed variable. Widening
        // is_mutator to `set` and `remove` for the map is what made that latent
        // bug reachable, so the two changes belong in one commit.
        if (is_mutator(m->name)) {
            const Slot slot = slot_of(*m->target);
            const Type *declared = slot.valid ? declared_type(slot) : nullptr;

            if (declared && declared->space == "container")
                return call_mutator(*m->target, m->name, argv, span);

            if (!declared || !declared->is_spacesuit()) {
                ValuePtr recv = eval(*m->target);
                if (failed() || !recv)
                    return nullptr;
                // An instance answers with its own method. Anything else is a
                // container mutation, and call_mutator reports the storage
                // error when the receiver names none — which is what keeps
                // `"abc"[0:1].append(1)` saying exactly what it always has.
                if (!std::holds_alternative<ObjectPtr>(*recv))
                    return call_mutator(*m->target, m->name, argv, span);
                return call_method(recv, *m->target, m->name, argv, span);
            }
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
