// The built-in method surface — every method every type answers to.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// "satellite.variable.number.plus takes 1 argument, got 2" beats a bare
// "wrong number of arguments" every time.
std::string arity_message(const char *module, const std::string &name,
                          size_t want, size_t got)
{
    std::string out = std::string(module) + "." + name + " takes " +
                      std::to_string(want) +
                      (want == 1 ? " argument, got " : " arguments, got ") +
                      std::to_string(got);
    return out;
}


// The two prologue lambdas, promoted to real functions so that every arm of
// call_method can share ONE implementation. The arms still spell them `arity`
// and `number_arg` and still call them the same way -- each arm declares a
// two-line lambda that forwards here -- which is what let every branch body move
// without a single edit.
bool Evaluator::method_arity(const char *module, const std::string &name,
                             const std::vector<ValuePtr> &argv, size_t want,
                             Span span)
{
    if (argv.size() == want)
        return true;
    fail(span, arity_message(module, name, want, argv.size()));
    return false;
}

bool Evaluator::method_number_arg(const char *module, const std::string &name,
                                  const std::vector<ValuePtr> &argv, size_t i,
                                  const Number *&out, Span span)
{
    out = std::get_if<Number>(argv[i].get());
    if (!out) {
        fail(span, std::string(module) + "." + name +
                   " wants a satellite.variable.number, got " +
                   to_string(*argv[i]));
        return false;
    }
    return true;
}

// call_method was 715 lines in one function. The per-type tables now live in
// files beside this one; what stays here is the part every receiver shares.
//
// ORDER IS PRESERVED. The arms are in fact mutually exclusive -- each tests a
// distinct Value alternative, and an Arguments is NOT a ListRef however much the
// type system lets one stand in for a list<string> -- but the order is kept
// anyway, because "they cannot overlap" is a property that a future alternative
// could quietly take away.
ValuePtr Evaluator::call_method(const ValuePtr &recv, const Expr &recv_expr,
                                const std::string &name,
                                const std::vector<ValuePtr> &argv, Span span)
{
    (void)recv_expr;

    // An instance answers for its own members before any built-in table is
    // consulted, so a spacesuit may name a method `length` or `to_string`
    // without colliding with the language's.
    //
    // `size` is the one built-in an instance can fall THROUGH to, and the order
    // above is what makes that safe: the suit is asked first, so a spacesuit
    // that already defines size() keeps its own, and adding .size() to the
    // language cannot change what any program that compiles today does. Only a
    // suit with no size() reaches the model in §8.7.
    if (const ObjectPtr *self = std::get_if<ObjectPtr>(recv.get())) {
        const bool own = *self && (*self)->suit &&
                         (*self)->suit->find_method(name) != nullptr;
        if (name == "size" && !own && *self && (*self)->suit) {
            if (!argv.empty()) {
                fail(span, arity_message(suit_name((*self)->suit).c_str(), name,
                                         0, argv.size()));
                return nullptr;
            }
            return make_value(Number(value_bytes(recv)));
        }
        return call_object_method(*self, name, argv, span);
    }

    const char *module = module_of(*recv);
    if (!module) {
        fail(span, "nil has no methods, so ." + name + "() has no receiver");
        return nullptr;
    }

    // Only `arity` survives in the driver: .size() below is the one check that
    // runs before any per-type arm, and no arm's lambda can serve it. The old
    // `number_arg` lambda moved to the arm that used it -- the number table was
    // its only caller, all five times.
    auto arity = [&](size_t want) {
        return method_arity(module, name, argv, want, span);
    };

    // .size() answers for every receiver that has methods at all, so it sits
    // ABOVE the per-type tables rather than being copied into each of them.
    // That placement is the whole reason the answer stays consistent: a type
    // added later gets a correct .size() from the exhaustive visitor in
    // value.cpp without anyone remembering to add a row here.
    //
    // §8.7 is the model, and the short version is that this is bytes and
    // .length() is items — a string of five characters is 5 long and 10 big.
    if (name == "size")
        return arity(0) ? make_value(Number(value_bytes(recv))) : nullptr;

    if (auto r = method_scalars(recv, name, argv, module, span))    return *r;
    if (auto r = method_file(recv, name, argv, module, span))       return *r;
    if (auto r = method_containers(recv, name, argv, module, span)) return *r;
    if (auto r = method_bits(recv, name, argv, module, span))       return *r;

    fail(span, std::string(module) + " has no method " + name);
    return nullptr;
}

} // namespace satellite
