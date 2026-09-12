// The machine's bracket arms -- `l[i]`, `m[k]`, `s[i]`, `l[a:b]`, and the
// index assignment `l[i] = v`. PLAN M16.
//
// TWO EXPRESSION ARMS AND TWO STATEMENT ARMS IN ONE FILE, which is the one
// place this tree's expression/statement split is crossed on purpose. The
// rules that decide what a subscript MEANS -- a whole number is positional, a
// negative counts from the end, out of range refuses, anything else is a
// search pattern -- have to be identical on the read and the write side or
// `l[i]` and `l[i] = v` disagree about which element they are talking about.
// v1 discovered that from the other end: its assign_index says "the same
// rules the READ side uses, deliberately identical" and had to say it because
// the two lived in different files. Here they are one file and share the
// helpers, so drift is not something a reader has to check for.
//
// WHAT A SUBSCRIPT DOES IS DECIDED BY THE SUBSCRIPT AND NOT BY THE RECEIVER,
// which is v1's DECISION 6a and the one rule in this file that must never
// move. `l[0]` is positional in every program ever written in this language;
// a subscript whose meaning depended on what the receiver turned out to be is
// action at a distance. So: a whole number indexes, and everything else is a
// pattern for the search power. On a MAP the question is asked one step
// earlier -- a key is consulted before the number rule, because a map key is
// not an index and `m[12]` means the key 12 -- and a subscript that could
// never have been a key at all (a pair, a list) falls through to the search,
// which is the dead end v1 found and filled.

#include "evaluator/evaluator_internal.hpp"

#include "satellite_containers/containers.hpp"
#include "satellite_containers/search.hpp"
#include "satellite_number/bignum.hpp"
#include "satellite_value/render.hpp"
#include "satellite_value/value_arguments.hpp"

#include <string>
#include <utility>

namespace satellite {
namespace eval {

namespace {

// A subscript as a position, resolved against a length: false means "this was
// not a whole number", which is the search's cue and never an error by
// itself. `out` is only meaningful when the answer is true AND `inside` is.
bool as_index(const Value &value, long long length, long long *out,
              bool *inside)
{
    const Number *number = std::get_if<Number>(&value);
    long long narrow = 0;
    if (number == nullptr || !number->is_integer() ||
        !number->to_integer(narrow))
        return false;
    // NEGATIVE COUNTS FROM THE END -- v1's rule, kept. `l[-1]` is the last
    // element, which is the spelling every reader of this file's neighbours
    // already expects, and it is why this cannot be position_at's unsigned
    // check: a negative subscript is a legal question with an answer.
    if (narrow < 0)
        narrow += length;
    *out = narrow;
    *inside = narrow >= 0 && narrow < length;
    return true;
}

// The searching half of every subscript arm, written once. The threshold is
// the machine's dial (`satellite.system.threshold`), read per search so a
// program that moves it mid-run is obeyed by the very next bracket.
void search_subscript(Machine &m, const Value &target, const Value &pattern)
{
    Value found = containers::search_collect(target, pattern, false,
                                             m.search_threshold());
    m.done();
    m.fold(std::move(found));
}

// `l[i]`, `m[k]`, `s[i]`. Two children: the target, then the subscript.
void index_read(Machine &m, OpIndex subscript_op)
{
    // THE CARET GOES UNDER THE SUBSCRIPT AND NOT UNDER THE `[`. An Index
    // node's anchor token is its bracket (ast.hpp's table), so the obvious
    // span_of(here()) draws the caret on punctuation -- and every sentence
    // below is ABOUT the subscript: which position, which key. This is the
    // same correction M12 made to text_of one layer along.
    const errors::Span where = m.span_of(subscript_op);
    const Value &target = m.value_from_top(1);
    const Value &subscript = m.value_from_top(0);

    // A MAP IS CONSULTED BEFORE THE SUBSCRIPT IS COERCED, and the order is
    // the whole of what v1 got wrong first: demanding a whole number up front
    // made `m["bolt"]` fail with "index must be a whole number" whatever the
    // receiver turned out to be -- a true sentence about the wrong question.
    if (const MapBody *body = as_map(target)) {
        std::string canonical;
        if (map_key_of(subscript, canonical)) {
            const auto found = body->index.find(canonical);
            // A MISS IS AN ERROR, symmetric with `.get(k)` and for its
            // reason: `nothing` is a value an entry can hold, so absent and
            // present-but-nothing must not be one answer.
            if (found == body->index.end()) {
                m.refuse(errors::make<errors::Code::EVAL_NO_SUCH_KEY>(
                    where, text_of(subscript)));
                return;
            }
            Value answer = body->entries[found->second].value;
            m.done();
            m.fold(std::move(answer));
            return;
        }
        search_subscript(m, target, subscript);
        return;
    }

    if (const List *list = as_list(target)) {
        const long long length = static_cast<long long>(list->size());
        long long at = 0;
        bool inside = false;
        if (!as_index(subscript, length, &at, &inside)) {
            search_subscript(m, target, subscript);
            return;
        }
        if (!inside) {
            m.refuse(errors::make<errors::Code::EVAL_OUTSIDE_THE_LIST>(
                where, m.text_of(m.here()), text_of(subscript),
                std::to_string(length)));
            return;
        }
        Value answer = (*list)[static_cast<size_t>(at)];
        m.done();
        m.fold(std::move(answer));
        return;
    }

    if (const Str *text = std::get_if<Str>(&target)) {
        static const SatString empty;
        const SatString &codes = *text ? **text : empty;
        const long long length = static_cast<long long>(codes.size());
        long long at = 0;
        bool inside = false;
        // A STRING IS NOT SEARCHABLE BY SUBSCRIPT, so a non-numeric one is a
        // wrong question rather than a pattern: the search power's domain is
        // the two containers (its walker knows lists and maps and nothing
        // else), and `s["a"]` reading as a search would be a fourth meaning
        // for one bracket. `contains(x)` and `find(x)` are how a string is
        // asked that, and they have been since M11.
        if (!as_index(subscript, length, &at, &inside)) {
            m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
                where, m.text_of(m.here()),
                "a whole `satellite.variable.number` position -- a string is "
                "asked about its content with `contains(x)` and `find(x)`",
                type_name(subscript)));
            return;
        }
        if (!inside) {
            m.refuse(errors::make<errors::Code::EVAL_PAST_THE_END>(
                where, m.text_of(m.here()), text_of(subscript),
                std::to_string(length)));
            return;
        }
        // SELECTS SATELLITE CHARACTERS, NOT DISPLAY CHARACTERS -- v1's rule,
        // and M11's `at(n)` already answers the same way: a live code is ONE
        // character that displays as a whole home directory. It is the only
        // O(1), stable rule, and it is unique to satellite.
        Value answer =
            Value::string(SatString(1, codes[static_cast<size_t>(at)]));
        m.done();
        m.fold(std::move(answer));
        return;
    }

    // THE ARGUMENTS OBJECT INDEXES ITS COMMAND LINE AND NOTHING ELSE -- PLAN
    // M20, confirmed by the author 2026-09-09: "index 0 is the program name
    // ... `.length()` and numeric `[i]` cover the command line and nothing
    // else". That is the whole reason the body stores the words apart from the
    // machine's facts: a program writes `for (i = 1; i < args.length(); ...)`,
    // and a `[i]` that walked into the facts would start handing back the
    // kernel release as though it had been typed.
    if (const Arg *held = std::get_if<Arg>(&target)) {
        static const Arguments nothing_yet;
        const Arguments &body = *held ? **held : nothing_yet;
        const long long length = static_cast<long long>(body.words.size());
        long long at = 0;
        bool inside = false;
        // NOT A SEARCH AND NOT A KEY. The search power's walker knows lists
        // and maps and nothing else -- the string arm above declines for the
        // same reason -- and the facts are reached by their NAMES, which are
        // registry paths a program writes out: `arguments.machine.threads`.
        if (!as_index(subscript, length, &at, &inside)) {
            m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
                where, m.text_of(m.here()),
                "a whole `satellite.variable.number` position -- the machine's "
                "facts are reached by name, as `arguments.machine.threads`",
                type_name(subscript)));
            return;
        }
        if (!inside) {
            m.refuse(errors::make<errors::Code::EVAL_OUTSIDE_THE_LIST>(
                where, m.text_of(m.here()), text_of(subscript),
                std::to_string(length)));
            return;
        }
        Value answer = body.words[static_cast<size_t>(at)].value;
        m.done();
        m.fold(std::move(answer));
        return;
    }

    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), m.text_of(m.here()),
        "a `satellite.container.list`, a `satellite.container.map`, a "
        "`satellite.variable.string` or the `arguments` object -- the four "
        "things a `[` can ask about",
        type_name(target)));
}

// The clamped bounds of a slice. A SLICE CLAMPS AND AN INDEX DOES NOT, which
// is deliberate on both sides: `l[0]` on an empty list is a mistake in the
// program and `l[0:5]` on a two-element list is the ordinary way to say "up
// to five of them". v1 drew the line in the same place.
void clamp_range(long long &lo, long long &hi, long long length)
{
    if (lo < 0)
        lo += length;
    if (hi < 0)
        hi += length;
    lo = lo < 0 ? 0 : (lo > length ? length : lo);
    hi = hi < 0 ? 0 : (hi > length ? length : hi);
    if (hi < lo)
        hi = lo;
}

} // namespace

void op_index(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        m.again(1);
        // THE SUBSCRIPT PUSHED FIRST SO IT RUNS SECOND: the target is written
        // first and must be evaluated first, and a stack runs what is on top.
        m.push(op.b);
        m.push(op.a);
        return;
    }
    index_read(m, op.b);
}

void op_slice(Machine &m, const Op &op, uint32_t step)
{
    // AN ABSENT BOUND IS NOT PUSHED AND NOT DEFAULTED TO A CONSTANT, which is
    // what keeps `l[:]` free: the count of children varies, so the arm reads
    // its own operands to know how many answers are waiting. `d` carries that
    // count, decided by the compiler, because counting again here would be
    // the same decision made in two places.
    if (step == 0) {
        m.again(1);
        if (op.c != kNoOp)
            m.push(op.c);
        if (op.b != kNoOp)
            m.push(op.b);
        m.push(op.a);
        return;
    }

    const uint32_t bounds = op.d;
    const Value &target = m.value_from_top(bounds);

    long long length = 0;
    const List *list = as_list(target);
    const Str *text = std::get_if<Str>(&target);
    static const SatString empty;
    const SatString *codes = text ? (*text ? text->get() : &empty) : nullptr;
    if (list)
        length = static_cast<long long>(list->size());
    else if (codes)
        length = static_cast<long long>(codes->size());
    else {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), m.text_of(m.here()),
            "a `satellite.container.list` or a `satellite.variable.string` -- "
            "the two things that have a run of elements to take",
            type_name(target)));
        return;
    }

    // The bounds sit above the target in the order they were pushed, so the
    // low bound is the deeper of the two whenever both are present.
    long long lo = 0;
    long long hi = length;
    uint32_t depth = bounds;
    const auto bound = [&](long long &into) {
        const Value &value = m.value_from_top(--depth);
        const Number *number = std::get_if<Number>(&value);
        long long narrow = 0;
        if (number == nullptr || !number->is_integer() ||
            !number->to_integer(narrow)) {
            m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
                m.span_of(m.here()), m.text_of(m.here()),
                "a whole `satellite.variable.number` slice bound",
                type_name(value)));
            return false;
        }
        into = narrow;
        return true;
    };
    if (op.b != kNoOp && !bound(lo))
        return;
    if (op.c != kNoOp && !bound(hi))
        return;
    clamp_range(lo, hi, length);

    Value answer;
    if (list) {
        // A FULL SLICE IS THE SAME LIST: no copy, and the handle is shared,
        // so `l[:]` is genuinely free. Any other slice copies the VALUES,
        // whose own bodies stay shared behind their handles.
        answer = (lo == 0 && hi == length)
                     ? target
                     : Value::list(List(list->begin() + lo, list->begin() + hi));
    } else {
        answer = (lo == 0 && hi == length)
                     ? target
                     : Value::string(codes->substr(
                           static_cast<size_t>(lo),
                           static_cast<size_t>(hi - lo)));
    }

    m.done();
    // The target and every bound come off together and the answer replaces
    // them -- fold() only collapses two, so a slice with two bounds pops the
    // extra one itself.
    for (uint32_t i = 0; i < bounds; i++)
        m.pop_value();
    m.set_top(std::move(answer));
}

namespace {

// `l[i] = v` and `m[k] = v`, over a receiver that names a slot. A STATEMENT,
// so it leaves no value -- and the write is a copy-and-replace rather than an
// in-place poke, because a container is built then frozen: the transform
// builds the next body and the machine publishes it into the slot the same
// way a mutating method's answer is published. Every other list mutation in
// the language already works this way, so this adds a spelling rather than a
// mechanism.
void index_store(Machine &m, const Op &op, uint32_t step, bool global)
{
    if (step == 0) {
        m.again(1);
        m.push(op.c);
        m.push(op.b);
        return;
    }

    const Value &subscript = m.value_from_top(1);
    const Value &value = m.value_from_top(0);
    // A COPY SINCE M23, AND THE TERNARY IS WHY IT IS SPELLED OUT. `m.global`
    // answers by value now (evaluator/globals.hpp: a reference into storage
    // another walk may be writing is a lock protecting the wrong thing), and a
    // `const Value &` bound to a ternary whose arms are a prvalue and an lvalue
    // copies the local arm too, silently. Writing the copy makes the cost
    // visible at the one site that pays it -- `my_list[i] = v` on a global.
    const Value current = global ? m.global(op.a) : m.local(op.a);

    if (const MapBody *body = as_map(current)) {
        // A MAP TAKES A SUBSCRIPT STORE AND v1 REFUSED ONE, and this is the
        // one place M16 overrules it. v1's assign_index answered "a map is
        // written with .set(key, value)" on the grounds that a second
        // spelling of one thing is a cost -- but DESIGN §12's deferral list
        // says `m[k] = v` and `l[i] = v` are ONE entry and that "fixing one
        // fixes both", so a milestone that built the list half and declined
        // the map half would be leaving that entry half-struck. The rule that
        // makes them one is this file's own: assignment resolves a storage
        // slot, and the slot is the CONTAINER's in both cases.
        MapBody next;
        if (!containers::map_with(*body, subscript, value, next)) {
            m.refuse(errors::make<errors::Code::EVAL_NOT_A_KEY>(
                m.span_of(m.here()), type_name(subscript)));
            return;
        }
        Value written = Value::map(std::move(next));
        m.done();
        m.pop_value();
        m.pop_value();
        if (global)
            m.set_global(op.a, std::move(written));
        else
            m.set_local(op.a, std::move(written));
        return;
    }

    const List *list = as_list(current);
    if (list == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), m.text_of(m.here()),
            "a `satellite.container.list` or a `satellite.container.map` -- "
            "the two things a subscript can be assigned into",
            type_name(current)));
        return;
    }

    const long long length = static_cast<long long>(list->size());
    long long at = 0;
    bool inside = false;
    if (!as_index(subscript, length, &at, &inside)) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), m.text_of(m.here()),
            "a whole `satellite.variable.number` position -- a search finds "
            "elements and cannot be assigned into",
            type_name(subscript)));
        return;
    }
    if (!inside) {
        // OUT OF RANGE IS AN ERROR AND NOT A GROW, the read side's rule
        // exactly: an index assignment that extended the list would make
        // `l[5] = x` on an empty list a way to create four elements holding
        // nothing that nobody asked for.
        m.refuse(errors::make<errors::Code::EVAL_OUTSIDE_THE_LIST>(
            m.span_of(m.here()), m.text_of(m.here()), text_of(subscript),
            std::to_string(length)));
        return;
    }

    List next = *list;
    next[static_cast<size_t>(at)] = value;
    Value written = Value::list(std::move(next));
    m.done();
    m.pop_value();
    m.pop_value();
    if (global)
        m.set_global(op.a, std::move(written));
    else
        m.set_local(op.a, std::move(written));
}

} // namespace

void op_index_store(Machine &m, const Op &op, uint32_t step)
{
    index_store(m, op, step, false);
}

void op_index_store_global(Machine &m, const Op &op, uint32_t step)
{
    index_store(m, op, step, true);
}

} // namespace eval
} // namespace satellite
