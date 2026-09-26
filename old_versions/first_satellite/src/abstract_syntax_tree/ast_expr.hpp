#pragma once

#include "abstract_syntax_tree/ast_span.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "satellite_string/satellite_string.hpp"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace satellite {

// --- types -----------------------------------------------------------------

// A type is never a value. `satellite.variable.*` and `satellite.container.*`
// are reserved as type-only namespaces, which is what keeps '<' unambiguous:
// without that rule `satellite.container.list<satellite.variable.string> argz`
// has the same token stream as the chained comparison
// ((satellite.container.list < satellite.variable.string) > argz), which is the
// C++98 a<b>c problem and is not fixable by lookahead.
//
// A SPACESUIT type is the one type spelled with a bare name, because §1 says a
// user-owned thing is bare and a spacesuit is named by the user. That does not
// reopen §4: §4's ambiguity is created by '<', and a spacesuit takes no generic
// arguments — `my_class<T>` is not grammar, so the token stream that made
// `list<string> argz` ambiguous cannot be produced. What makes a bare type
// parse at all is §5's own observation that TWO ADJACENT WORDS are a shape no
// expression can produce; the parser requires them on one line, exactly as it
// already does for `satellite.variable.number x`.
struct Type {
    std::string space;          // "variable" or "container"; empty for both the
                                // bare `satellite` singleton and a spacesuit
    std::string name;           // "time", "string", "list", or a spacesuit name
    std::vector<Type> args;     // generic arguments: list<string> has one
    Span span;

    // Empty space AND empty name: the bare `satellite` singleton type.
    bool is_singleton() const { return space.empty() && name.empty(); }

    // Empty space with a name: `my_class_name`, a user-declared spacesuit.
    bool is_spacesuit() const { return space.empty() && !name.empty(); }
};

// --- expressions -----------------------------------------------------------

struct Expr;
using ExprPtr = std::shared_ptr<const Expr>;

struct NumberLit {
    Number value;
    std::string text;           // as written, so 3.10 does not unparse as 3.1
};

// `100ms`, and `100 ms` with a space: one literal either way, because the two
// lex identically -- a word may not start with a digit, so the number token
// ends at the 'm' whether or not a space separates them, and the parser sees
// Number then Word('ms') in both cases.
//
// It is a DURATION and not a number, and the difference is the point: 100 is a
// value a program can add to, and 100ms is an amount of time that only means
// something where a length of time is being asked for. The one place that asks
// today is satellite.console.display(100ms), which sets the printer's pace;
// anywhere else this node is an error, named at the point it appears rather
// than silently becoming 100.
//
// §8.2's decision stands: there is no satellite.variable.duration, because a
// duration VALUE would first have to answer .plus, .size, its type name and its
// identity as a map key before it earned a slot in the Value variant. This is a
// literal, and a literal needs none of that.
// The one unit the language spells, in nanoseconds. It lives here, beside the
// node whose field it fills, because the parser converting `100ms` and the
// evaluator handing a Console nanoseconds are two readers of ONE number.
inline constexpr long long kNsPerMs = 1000000;

struct DurationLit {
    // Nanoseconds, converted at parse time from whatever unit was written, so
    // nothing downstream re-reads the spelling to learn what it meant. A second
    // unit is another multiplier in the parser and no change here.
    Number nanoseconds;

    // As written, so `3.10ms` does not unparse as `3.1ms` -- the same reason
    // NumberLit keeps its text. The space in `100 ms` is not preserved; unparse
    // normalises spacing everywhere and this is not the exception.
    std::string text;
};

struct StringLit {
    SatString value;            // escapes already expanded
    std::string source;         // as written, without the quotes
};

// The bare word `satellite`: the runtime singleton, as a value. It is what
// satellite.include(satellite) includes and what satellite.return(satellite)
// returns, and treating it as an object is also what makes satellite.time.now()
// ordinary member access rather than a special path form.
struct SatelliteLit {};

// Where a resolved name lives. Filled in by resolve() (env.hpp); see the
// contract on Name::slot below.
//
// The non-negative half of the int is a frame slot index, so everything else
// has to live in the negative half. Three of the four are single sentinels; a
// FIELD is not, because a spacesuit has one field slot per field and the index
// has to travel with the name — hence a base to count down from rather than a
// second int on every Name.
inline constexpr int SLOT_GLOBAL  = -1;   // satellite.library.<namespace>.<name>
inline constexpr int SLOT_CAPSULE = -2;   // names a capsule, not a variable
inline constexpr int SLOT_METHOD  = -3;   // names a method of the enclosing suit
inline constexpr int SLOT_SUIT    = -4;   // names a spacesuit, not a variable
inline constexpr int SLOT_FIELD   = -5;   // field 0; field i is SLOT_FIELD - i

inline bool is_field_slot(int slot) { return slot <= SLOT_FIELD; }
inline int  field_slot(int index)   { return SLOT_FIELD - index; }
inline int  field_index(int slot)   { return SLOT_FIELD - slot; }

struct Name {                                               // my_time, argz, fact
    std::string text;

    // A frame slot index when >= 0, otherwise one of the sentinels above.
    //
    // `mutable` because the tree is shared as shared_ptr<const Expr>, and that
    // makes the contract load-bearing rather than incidental: resolve() must
    // finish, on one thread, before any evaluation begins. Resolving lazily
    // during eval would be a data race the moment two threads walk one shared
    // Program — which is exactly what §6's 8-thread test does.
    mutable int slot = SLOT_GLOBAL;
};

struct Member { ExprPtr target; std::string name; };        // <expr> . name
struct Call   { ExprPtr target; std::vector<ExprPtr> args; };
struct Index  { ExprPtr target; ExprPtr subscript; };       // <expr> [ i ]

// <expr> [ lo : hi ] — either bound may be null, so l[:], l[2:], l[:5] and
// l[2:5] are all this node. Bounds are half-open: len(l[a:b]) == b - a, which
// is a subtraction with no +1 and maps straight onto List(begin+a, begin+b).
struct Slice { ExprPtr target; ExprPtr lo; ExprPtr hi; };

struct Unary  { std::string op; ExprPtr operand; };
struct Binary { std::string op; ExprPtr left; ExprPtr right; };

// { a, b, c } — a list written down.
//
// §8.6's containers could be BUILT before this node existed (declare empty,
// then .append()) but could not be WRITTEN, which made a list of four strings
// five statements. This is the literal, and it is deliberately only a sequence
// of expressions: the ELEMENT TYPE is not here, because the type belongs to the
// variable or parameter the literal is being handed to, not to the literal. A
// bare `{1, 2}` in a position with no declared type is a list of whatever its
// elements evaluated to, which is the same rule every other expression follows.
//
// Empty is legal and is spelled `{}`. It is the one list literal whose element
// type CANNOT be inferred from its contents, which is exactly why the type
// lives on the declaration.
struct ListLit {
    std::vector<ExprPtr> elements;
};

// DurationLit is appended rather than inserted next to NumberLit, for the
// reason stated on Str/ListRef in value.hpp: help_for() and module_of() switch
// on raw variant indices there, and an AST alternative that moved would
// silently renumber every ExprBase index the same way. It also costs nothing to
// add -- NumberLit is already the widest alternative at 64 bytes and
// DurationLit is the same shape, so the static_assert below still holds.
// name = <expr>, in an argument list only.
//
// The language has no keyword arguments and this node is NOT the beginning of
// them: it is the grammar for one, so that the ONE capsule that wants a named
// argument can have it and every other position can refuse it by name instead
// of by a parse error about a missing ')'. Which names are understood is the
// evaluator's business, not the parser's — exactly like DurationLit, which
// parses anywhere and is accepted in one place.
//
// `name` is a bare word. `satellite.x = 1` in an argument list is not this;
// only an unqualified word followed by a single '=' is.
struct NamedArg {
    std::string name;
    ExprPtr value;
};

// `x00FF`, `b1010` — §21's hex and binary literals.
//
// ONE STRING, NOT TWO, and that is a size decision rather than a taste one.
// NumberLit keeps both its Number and its spelling and is 64 bytes, which makes
// it the widest alternative and therefore the one that sets sizeof(Expr) at 96.
// A BitsLit holding both normalised digits and the original spelling would be
// two std::strings — 72 bytes — and would push every Expr in the tree wider,
// which the static_assert below exists to catch. So the spelling is what is
// stored and the digits are derived at evaluation, where the work is one
// substr on a literal that is evaluated once.
//
// Appended, not inserted, for the reason DurationLit and ListLit were.
struct BitsLit {
    unsigned radix = 0;         // 2 or 16
    std::string text;           // as written, prefix and all: "x00ff"
};

// ListLit is appended for the same reason DurationLit was, and the note above
// applies to it unchanged: a vector is 24 bytes, so the widest alternative is
// still NumberLit at 64 and sizeof(Expr) does not move.
using ExprBase = std::variant<NumberLit, StringLit, SatelliteLit, Name,
                              Member, Call, Index, Slice, Unary, Binary,
                              DurationLit, ListLit, NamedArg, BitsLit>;

struct Expr : ExprBase {
    using ExprBase::ExprBase;
    // Inherited constructors deliberately skip the base's own copy and move,
    // so building an Expr out of an already-formed ExprBase needs this.
    Expr(ExprBase base) : ExprBase(std::move(base)) {}
    Span span;
};

// See the note on Span. Expr is kept separate from Value on measured grounds
// (§10): folding them grows every Value from 40 to 96, which takes a
// million-element number list from 38 MB to 91 MB. This assert is half of what
// makes that measurement stay true.
static_assert(sizeof(void *) != 8 || sizeof(Expr) == 96,
              "Expr must stay 96 bytes on 64-bit — see the note on Span");

ExprPtr make_expr(ExprBase node, Span span);

} // namespace satellite
