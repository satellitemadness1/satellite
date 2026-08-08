// Small helpers shared across the evaluator.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

namespace satellite {

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

// Bounds the C++ recursion the tree walk costs, so a pathological tree raises
// a satellite error instead of segfaulting the C++ stack (§6).
// satellite.library.main.x -> {"satellite", "library", "main", "x"}.
// False for anything that is not a pure SatelliteLit/Member chain, which is
// what separates a language path from an expression with members on it.
bool flatten_path(const Expr &expr, std::vector<std::string> &out)
{
    if (std::holds_alternative<SatelliteLit>(expr)) {
        out.push_back("satellite");
        return true;
    }
    if (const Member *m = std::get_if<Member>(&expr)) {
        if (!m->target || !flatten_path(*m->target, out))
            return false;
        out.push_back(m->name);
        return true;
    }
    return false;
}

std::string join_path(const std::vector<std::string> &path)
{
    std::string out;
    for (size_t i = 0; i < path.size(); i++) {
        if (i)
            out += ".";
        out += path[i];
    }
    return out;
}

// A declaration with no initialiser still gets a value of its declared type,
// so `satellite.container.list<...> l` is an empty list you can append to
// rather than a nil you cannot. The bare `satellite` type has no such value
// and stays nil.
//
// A SPACESUIT type is nil here, and the declaration statement builds the
// instance instead (exec, below). The split is not arbitrary: a spacesuit's
// default is an ALLOCATION, so this function would have to be able to fail and
// to run satellite code, and a field of a suit's own type would default-
// construct forever. A variable is not part of any object's layout, so it has
// no such regress and gets a real instance; a field is, so it starts as nil and
// a method fills it with my_class_name().
Value default_of(const Type &type)
{
    if (type.is_singleton() || type.is_spacesuit())
        return std::monostate{};
    if (type.space == "variable") {
        if (type.name == "bool")
            return false;
        if (type.name == "number")
            return Number();
        if (type.name == "string")
            return make_string(SatString{});
        // The epoch. A time has no "empty" the way a string does, and the epoch
        // is the one instant that is a fact rather than a choice.
        if (type.name == "time")
            return Time{};
        // A file is nil until it is opened, and nil is a real answer rather
        // than a placeholder: satellite.variable.file is a reference type
        // (§8.3), and a declaration cannot open anything because it has no path
        // to open and nowhere to report that opening failed.
    }
    if (type.space == "container" && type.name == "list")
        return make_list(List{});
    return std::monostate{};
}

// Structural, not pointer, equality: two lists holding equal values are equal
// even when they share no children. The variant's own operator== would compare
// the shared_ptrs inside a List, so the List arm has to come first.
//
// An OBJECT is the exception, and it needs no arm: the variant's own operator==
// compares the ObjectPtrs, which is identity, and identity is what equality
// means for a reference type. Two instances with equal fields are two
// instances — a spacesuit that wants them equal says so with a method, because
// only it knows which of its fields are part of what it means to be equal.
bool value_equals(const Value &a, const Value &b)
{
    if (a.index() != b.index())
        return false;
    if (const List *la = as_list(a)) {
        const List &lb = *as_list(b);
        if (la->size() != lb.size())
            return false;
        for (size_t i = 0; i < la->size(); i++) {
            const ValuePtr &x = (*la)[i];
            const ValuePtr &y = lb[i];
            if (!x || !y) {
                if (x != y)
                    return false;
                continue;
            }
            if (!value_equals(*x, *y))
                return false;
        }
        return true;
    }
    // Strings compare by CONTENT, and this arm is not optional.
    //
    // The variant's own operator== compares alternatives, and the string
    // alternative is a shared_ptr since strings moved behind a handle. That
    // compares POINTERS, so "x" == "x" was false whenever the two sides were
    // built separately — which is every interesting case. It shipped, and all
    // eleven test binaries passed, because nothing in the suite compared two
    // independently constructed strings. The satellite lexer written in
    // satellite is what caught it: its whitespace test stopped matching and
    // spaces started coming out as punctuation tokens.
    //
    // Any future alternative that is a handle to something with value semantics
    // needs an arm here too. The fallback below is correct only for
    // alternatives whose own operator== already means what the language means.
    if (const SatString *sa = as_string(a))
        return *sa == *as_string(b);

    return static_cast<const ValueBase &>(a) == static_cast<const ValueBase &>(b);
}

// An index must be a whole number: 1.5 is a bug in the program, not a
// silently truncated 1.
bool as_index(const Value &v, long long &out)
{
    // Number::to_integer answers both halves at once — is it whole, and does it
    // fit — so there is no isfinite/floor dance any more, and no way for a
    // value that merely rounds to an integer to pass as one.
    const Number *n = std::get_if<Number>(&v);
    return n && n->to_integer(out);
}

// Half-open, negative bounds Python-style, out-of-range clamps, hi < lo is
// empty (§7).
void clamp_range(long long &lo, long long &hi, long long len)
{
    if (lo < 0)
        lo += len;
    if (hi < 0)
        hi += len;
    lo = std::max(0LL, std::min(lo, len));
    hi = std::max(0LL, std::min(hi, len));
    if (hi < lo)
        hi = lo;
}

ValuePtr make_value(Value v)
{
    return std::make_shared<const Value>(std::move(v));
}

// A string and a list are stored behind a handle (value.hpp), so neither
// converts to a Value implicitly any more. These two overloads keep that a
// detail of the value model rather than something every call site restates.
ValuePtr make_value(SatString s)
{
    return make_value(make_string(std::move(s)));
}

ValuePtr make_value(List items)
{
    return make_value(make_list(std::move(items)));
}


// The whole language, on one screen.
//
// That is only possible because the language IS one screen: two dozen methods,
// seven module functions, four statement forms. A reference that fits is worth
// more than a reference that is complete, and here they are the same thing --
// so this is checked against the code below rather than written once and left
// to rot.

ValuePtr module_constant(const std::vector<std::string> &path)
{
    // Deliberately reachable WITHOUT parentheses. Someone who needs help is by
    // definition someone who may not remember the calling syntax, and making
    // them get it right first is the one place a language can least afford to.
    if (path.size() == 2 && path[0] == "satellite" && path[1] == "help")
        return make_value(encode_raw(help_overview()));

    if (path.size() == 3 && path[0] == "satellite" && path[1] == "bool") {
        if (path[2] == "true")
            return make_value(true);
        if (path[2] == "false")
            return make_value(false);
    }
    return nullptr;
}

// Methods that write back through their receiver. They are the reason a
// receiver has to name a storage slot.
bool is_mutator(const std::string &name)
{
    return name == "append";
}

// What the depth guard is protecting, measured rather than guessed.
//
// One capsule activation costs exactly 3 depth units — eval(Call) -> eval_call
// -> call_capsule -> exec(body) -> exec_block -> exec(return) -> eval — and
// 3169 bytes of C++ stack at -O2 (clang 24, x86-64). Three units is the fewest
// any body can cost, so that is the WORST ratio the guard has to survive: a
// longer body spends more units per byte, not fewer. The cost does not grow
// with the number of locals either, because a frame's slots are a vector.
//
// Where the C++ stack actually ends, bisected on an 8 MB stack with the guard
// disabled:
//
//     -O2    2600 levels return, 2800 segfault   -> ~7950 units
//     -O0    1200 levels return, 1300 segfault   -> ~3750 units
//
// §6's suggested default of 10000 was written when nothing recursed, and it is
// past BOTH cliffs — so the guard could never have fired, and a runaway
// recursion was a segfault rather than the error the guard exists to produce.
// 2000 units is a quarter of the optimised stack and half the unoptimised one.
// It is also what the resolver walks to (MAX_RESOLVE_DEPTH, env.cpp), so
// neither pass is the one that dies first.
constexpr int DEFAULT_MAX_DEPTH = 2000;

// The ceiling on satellite.library.system.max_depth. The knob raises the limit
// toward the stack and cannot raise it past: above the cliff a larger setting
// does not buy deeper recursion, it buys a segfault instead of an error
// message. 3000 stays under the -O0 cliff as well as the -O2 one, because an
// unoptimised build is exactly where losing the error message costs most.
constexpr int MAX_MAX_DEPTH = 3000;

int read_max_depth()
{
    ValuePtr v = Library::instance().get("system", "max_depth");
    long long set = 0;
    if (v)
        if (const Number *n = std::get_if<Number>(v.get()))
            if (n->floor().to_integer(set) && set >= 1)
                return static_cast<int>(std::min<long long>(set, MAX_MAX_DEPTH));
    return DEFAULT_MAX_DEPTH;
}

// The significant digits a non-terminating division keeps (§8.1). A knob for
// the same reason max_depth is one: the right answer depends on the program,
// and the default is only a default.
int read_division_digits()
{
    ValuePtr v = Library::instance().get("system", "division_digits");
    long long set = 0;
    if (v)
        if (const Number *n = std::get_if<Number>(v.get()))
            if (n->floor().to_integer(set) && set >= 1)
                return static_cast<int>(
                    std::min<long long>(set, Number::MAX_DIVISION_DIGITS));
    return Number::DEFAULT_DIVISION_DIGITS;
}

} // namespace satellite
