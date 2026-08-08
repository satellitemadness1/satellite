// Resolving a name and a call against the scopes and the tables.
//
// Part of env/, split from an 854-line env.cpp.

#include "env_internal.hpp"

namespace satellite {

void Resolver::resolve_name(const Name &name, Span span)
{
    // Innermost outwards: a local, then a field of the enclosing spacesuit,
    // then a method of it, then a capsule. Each shadows the next, which is the
    // same rule an inner block already follows against an outer one.
    if (const int *slot = lookup(name.text)) {
        name.slot = *slot;
        return;
    }

    if (int index = lookup_field(name.text); index >= 0) {
        name.slot = field_slot(index);
        return;
    }

    if (suit_ && suit_->find_method(name.text)) {
        name.slot = SLOT_METHOD;
        return;
    }

    if (out_.capsules.count(name.text)) {
        name.slot = SLOT_CAPSULE;
        return;
    }

    if (out_.suits.count(name.text)) {
        name.slot = SLOT_SUIT;
        return;
    }

    if (!info_ && !suit_) {
        // Top level. An unknown name here is NOT an error: the REPL declares a
        // variable on one line and reads it on the next, and those are two
        // separate Programs that no single resolve() call can see at once. It
        // resolves against satellite.library at run time.
        name.slot = SLOT_GLOBAL;
        return;
    }

    // Inside a capsule the opposite holds. A name that is not a parameter, not
    // a local and not a capsule is an error here rather than an implicit read
    // of a global — that is what makes capsules lexically closed, and it is
    // what makes §6's eight-thread result mean anything. Globals are still
    // reachable, by the four-segment satellite.library path.
    //
    // A spacesuit member is closed the same way, and for a second reason: a
    // name that silently fell through to a global would make a typo'd field
    // read a variable somewhere else in the program.
    fail(span, suit_ ? "unknown variable in spacesuit " + suit_->name + ": " +
                           name.text
                     : "unknown variable in capsule: " + name.text);
    name.slot = SLOT_GLOBAL;
}

void Resolver::resolve_call(const Call &call, Span span)
{
    if (!call.target) {
        for (const ExprPtr &arg : call.args)
            if (arg)
                resolve_expr(*arg);
        return;
    }

    if (const Name *callee = std::get_if<Name>(call.target.get())) {
        // The scope lookup comes FIRST so a local shadows a capsule of the
        // same name, matching how every other name resolves.
        if (!lookup(callee->text)) {
            // Arity is knowable for both kinds, so a wrong call count is a
            // static error instead of something the tree walk trips over later.
            auto arity = [&](const std::string &what, size_t want) {
                if (call.args.size() != want)
                    fail(span, what + " takes " + std::to_string(want) +
                               (want == 1 ? " argument, got "
                                          : " arguments, got ") +
                               std::to_string(call.args.size()));
                for (const ExprPtr &arg : call.args)
                    if (arg)
                        resolve_expr(*arg);
            };

            // A bare call inside a spacesuit means a method of that spacesuit
            // before it means a top-level capsule, so a suit can name a method
            // whatever it likes without colliding with the rest of the program.
            if (suit_) {
                if (const MethodInfo *method = suit_->find_method(callee->text)) {
                    callee->slot = SLOT_METHOD;
                    arity(callee->text, method->info->param_count);
                    return;
                }
            }

            auto found = out_.capsules.find(callee->text);
            if (found != out_.capsules.end()) {
                callee->slot = SLOT_CAPSULE;
                arity(callee->text, found->second.param_count);
                return;
            }

            // my_class_name(...) builds an instance, and its arguments are the
            // arguments of the suit's constructor. `my_suit x("data")` is the
            // same call, written by the parser.
            if (const SpacesuitInfo *built = out_.find_suit(callee->text)) {
                callee->slot = SLOT_SUIT;
                arity(callee->text, built->ctor_params());
                return;
            }
        }
    }

    resolve_expr(*call.target);
    for (const ExprPtr &arg : call.args)
        if (arg)
            resolve_expr(*arg);
}

} // namespace satellite
