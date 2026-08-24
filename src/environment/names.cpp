// Resolving a name and a call against the scopes and the tables.
//
// Part of src/environment/, split from an 854-line env.cpp.

#include "environment/env_internal.hpp"

namespace satellite {

namespace {

// The dotted path an expression spells, when it is nothing but bare words:
// `ultra` yes, `random.ultra` yes, `f().x` and `satellite.random` no.
//
// Deliberately NOT src/evaluator/'s flatten_path, which also accepts a
// `satellite` root and a call anywhere in the chain. This asks a narrower
// question, and it has to be asked here because src/environment/ does not
// include src/evaluator/ -- the dependency runs the other way and this is not
// the change that should reverse it.
bool bare_dotted_path(const Expr &expr, std::string &out)
{
    if (const Name *name = std::get_if<Name>(&expr)) {
        out = name->text;
        return true;
    }
    if (const Member *member = std::get_if<Member>(&expr)) {
        if (!member->target || !bare_dotted_path(*member->target, out))
            return false;
        out += "." + member->name;
        return true;
    }
    return false;
}

// `satellite.help`, which is the only shape that spelling can have: a Member
// named help whose target is the runtime singleton.
bool is_help_target(const Expr &target)
{
    const Member *member = std::get_if<Member>(&target);
    return member && member->name == "help" && member->target &&
           std::holds_alternative<SatelliteLit>(*member->target);
}

} // namespace

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

    // TRUE and FALSE, and ONLY once every scope above has been asked and
    // answered no.
    //
    // §8.4 gives the booleans the spellings satellite.bool.true and
    // satellite.bool.false, and those remain the canonical ones. These two are
    // accepted as well because they are what a program writes — the shape is
    // universal and reaching for it is not a mistake anybody learns out of.
    //
    // §1 IS NOT WEAKENED, and the position in this function is the whole reason
    // why. A bare name is still the user's: a local, a field, a method, a
    // capsule or a spacesuit called TRUE has already won above, so this is
    // reached only for a word that names nothing the user owns. Making them
    // literals in the LEXER — the obvious implementation — is what would break
    // §1, by taking the two words away from the user everywhere and for good.
    //
    // SLOT_GLOBAL and not a sentinel of their own: the read falls through to
    // the same run-time lookup a top-level name uses, and that is what lets a
    // satellite.library variable of the same name still win at the last moment.
    // See read_slot() in src/evaluator/slots.cpp, which supplies the value.
    if (name.text == "TRUE" || name.text == "FALSE") {
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
    // Inside satellite.help(...) a bare word is allowed to name nothing,
    // because it may be naming a TOPIC instead. Reached only here, after every
    // scope above has been asked and answered no, so the suppression cannot
    // hide a name the user actually owns. The evaluator decides what the word
    // means and reports an unknown topic itself, with the topics listed.
    if (help_topic_) {
        name.slot = SLOT_GLOBAL;
        return;
    }

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

    // satellite.help(random), satellite.help(random.ultra) -- a bare word or a
    // bare dotted path, which may be a TOPIC rather than a variable.
    //
    // The argument is resolved exactly as it always was; the only difference is
    // that failing to resolve is not an error here. That ordering is what keeps
    // §1: a variable, field, method, capsule or spacesuit called `ultra` still
    // wins, and satellite.help(ultra) still answers about the value. The
    // evaluator repeats the judgement on the same expression before evaluating
    // anything (src/evaluator/expr.cpp), which is what makes the two agree.
    if (call.args.size() == 1 && call.args[0] && is_help_target(*call.target)) {
        std::string topic;
        if (bare_dotted_path(*call.args[0], topic)) {
            resolve_expr(*call.target);

            const bool outer = help_topic_;
            help_topic_ = true;
            resolve_expr(*call.args[0]);
            help_topic_ = outer;
            return;
        }
    }

    resolve_expr(*call.target);
    for (const ExprPtr &arg : call.args)
        if (arg)
            resolve_expr(*arg);
}

} // namespace satellite
