// The sort family -- `sort()` `1 4 2 3` through `sort_up(key)` `1 4 2 7`, and
// `max` `1 4 2 24` / `min` `1 4 2 25`, which are the same ordering asked once
// instead of n log n times.
//
// THE SORT PRIMITIVE IS NOT THE SEARCH POWER, and PLAN §8's M16 entry says so
// in as many words: these five rows are DESIGN §12's "sorting needs one
// primitive, not comparators" -- all seven of QUAD's sort comparators are one
// numeric key with ties broken deterministically -- and the search power is
// v1's comparator ladder next door in search_score.cpp. Two different things
// that land in one milestone.
//
// WHAT ORDERS, DECIDED AT M16: numbers (floats with them, by M15's rule that
// the two numeric arms compare as values) and strings, by their stored codes.
// Nothing else, and a MIXED list is refused rather than ordered by some rule
// nobody chose. The floor is the language's own: S0712 says `<` "orders
// numbers, and nothing else does". Strings are the one addition, and they are
// not a liberty -- `satellite.directory.list()` `1 18 4` answers a list of
// names that has to come back sorted, so a language whose sort could not
// order strings would need a second sort at M19. THE OVERRULE POINT is an
// instant: a list of `satellite.variable.time` is plainly ordered and is
// refused here, because M13 gave an instant exactly one ability and its
// methods are M29's. One arm, the day somebody wants it.
//
// TIES KEEP THEIR PLACE, because the sort is stable. QUAD's comparators carry
// an explicit `// deterministic ties` tie-break for the opposite reason --
// `std::sort` is not stable, so its programs had to add a second key to stay
// deterministic. A stable sort makes that unnecessary rather than optional:
// two elements the ordering cannot separate come back in the order they went
// in, so a satellite program needs no tie-break field at all.

#include "error_reporter/report.hpp"
#include "satellite_containers/methods_internal.hpp"
#include "satellite_float/satellite_float.hpp"
#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace satellite::containers {
namespace {

// What kind of order a value has, or None. The two numeric arms share one
// kind, because M15 made them compare as values and a sort that put every
// float after every number would disagree with `==` on the very same pair.
enum class Order { None, Numeric, Text };

Order order_of(const Value &value)
{
    if (value.is_number() || value.is_float())
        return Order::Numeric;
    if (value.is_string())
        return Order::Text;
    return Order::None;
}

Number as_number(const Value &value)
{
    if (const Number *exact = std::get_if<Number>(&value))
        return *exact;
    const Flo &held = std::get<Flo>(value);
    return held ? held->to_number() : Number();
}

// -1, 0 or 1. Only ever asked of two values of the SAME kind -- `orderable`
// below is what guarantees it, so this cannot fail and the comparator handed
// to stable_sort is a real strict weak ordering.
int order_between(const Value &a, const Value &b)
{
    if (order_of(a) == Order::Numeric) {
        // BOTH FLOATS GO THROUGH Float::compare rather than through the
        // number, so a float's own precision decides -- to_number() is exact,
        // but routing the common case through it would allocate twice per
        // comparison for an answer the float already has.
        if (a.is_float() && b.is_float()) {
            const Flo &x = std::get<Flo>(a);
            const Flo &y = std::get<Flo>(b);
            if (x && y)
                return Float::compare(*x, *y);
        }
        return Number::compare(as_number(a), as_number(b));
    }
    const Str &x = std::get<Str>(a);
    const Str &y = std::get<Str>(b);
    static const SatString empty;
    const SatString &left = x ? *x : empty;
    const SatString &right = y ? *y : empty;
    // BY THE STORED CODES AND NEVER BY DECODED TEXT, map_key_of's rule one
    // module over: a live code expands at display time, so ordering decoded
    // text would make a sorted list re-order itself when the working
    // directory changed.
    return left < right ? -1 : (right < left ? 1 : 0);
}

// The whole list is orderable and of one kind, or a refusal naming what was
// found. Checked BEFORE anything is moved, so a list that cannot be sorted is
// left exactly as it was rather than half-ordered.
bool orderable(eval::Machine &m, const List &items)
{
    Order kind = Order::None;
    for (const Value &item : items) {
        const Order here = order_of(item);
        if (here == Order::None) {
            m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
                m.span_of(m.here()), asked(m),
                "a list of `satellite.variable.number` or of "
                "`satellite.variable.string`, which are what this language "
                "orders",
                type_name(item)));
            return false;
        }
        if (kind == Order::None) {
            kind = here;
            continue;
        }
        if (kind != here) {
            m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
                m.span_of(m.here()), asked(m),
                "a list whose elements are all one kind -- numbers, or "
                "strings",
                type_name(item)));
            return false;
        }
    }
    return true;
}

// The value one element is sorted BY: the entry under `key` when the element
// is a map, refused by name otherwise. This is DESIGN §12's primitive --
// "one numeric key descending" -- and a map is the shape a satellite program
// has for a record with named fields until M26 gives it spacesuits.
bool keyed(eval::Machine &m, const Value &element, const std::string &canonical,
           const Value &spelled, const Value **out)
{
    const MapBody *body = as_map(element);
    if (body == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), asked(m),
            "a list of `satellite.container.map`, since a key names an entry "
            "in one",
            type_name(element)));
        return false;
    }
    const auto found = body->index.find(canonical);
    if (found == body->index.end()) {
        m.refuse(errors::make<errors::Code::EVAL_NO_SUCH_KEY>(
            m.span_of(m.here()), text_of(spelled)));
        return false;
    }
    *out = &body->entries[found->second].value;
    return true;
}

bool sorted_plainly(eval::Machine &m, const Value *a, bool down, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    if (!orderable(m, *self))
        return false;
    List next = *self;
    std::stable_sort(next.begin(), next.end(),
                     [down](const Value &x, const Value &y) {
                         const int side = order_between(x, y);
                         return down ? side > 0 : side < 0;
                     });
    *answer = Value::list(std::move(next));
    return true;
}

// The keyed halves, which are one function because `sort_up(key)` and
// `sort_down(key)` differ in a bool and in nothing else.
bool sorted_by_key(eval::Machine &m, const Value *a, bool down, Value *answer)
{
    const List *self = nullptr;
    std::string canonical;
    if (!list_at(m, a, 0, &self) || !key_at(m, a, 1, &canonical))
        return false;

    // Every element's key value pulled out ONCE, checked, and sorted
    // alongside its element -- rather than looked up inside the comparator,
    // where a map lookup would run n log n times and a refusal would have
    // nowhere to go.
    std::vector<std::pair<const Value *, const Value *>> keyed_pairs;
    List by;
    keyed_pairs.reserve(self->size());
    by.reserve(self->size());
    for (const Value &element : *self) {
        const Value *value = nullptr;
        if (!keyed(m, element, canonical, a[1], &value))
            return false;
        keyed_pairs.push_back({&element, value});
        by.push_back(*value);
    }
    if (!orderable(m, by))
        return false;

    std::stable_sort(keyed_pairs.begin(), keyed_pairs.end(),
                     [down](const auto &x, const auto &y) {
                         const int side = order_between(*x.second, *y.second);
                         return down ? side > 0 : side < 0;
                     });

    List next;
    next.reserve(keyed_pairs.size());
    for (const auto &pair : keyed_pairs)
        next.push_back(*pair.first);
    *answer = Value::list(std::move(next));
    return true;
}

bool list_sort(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    return sorted_plainly(m, a, false, answer);
}

bool list_sort_down(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    return sorted_plainly(m, a, true, answer);
}

// `sort(direction)` `1 4 2 4` -- THE GENERAL FORM, AND IT ONLY EVER RUNS WHEN
// THE DIRECTION IS A VARIABLE. A literal folds at resolve to `sort_down()` or
// (since the author's 2026-09-05 decision) back to `sort()`, so a program that
// spells the word never reaches this row. What arrives here is
// `my_list.sort(which)`, and the word is checked at run time because that is
// the cost of the readability DESIGN §1.1 asks for.
bool list_sort_direction(eval::Machine &m, const Value *a, uint32_t,
                         Value *answer)
{
    const Str *word = std::get_if<Str>(&a[1]);
    const std::string spelled = word && *word ? live_text(**word) : std::string();
    if (spelled != "up" && spelled != "down") {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), asked(m),
            "either `\"up\"` or `\"down\"`, the two directions a sort has",
            word ? "`" + spelled + "`" : std::string(type_name(a[1]))));
        return false;
    }
    return sorted_plainly(m, a, spelled == "down", answer);
}

bool list_sort_down_key(eval::Machine &m, const Value *a, uint32_t,
                        Value *answer)
{
    return sorted_by_key(m, a, true, answer);
}

bool list_sort_up_key(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    return sorted_by_key(m, a, false, answer);
}

// `max` and `min` are the ordering asked once. They are NOT a sort that keeps
// one end: a sort is n log n and copies the body, and asking which element is
// largest is a single pass over it.
bool extreme(eval::Machine &m, const Value *a, bool largest, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    if (self->empty()) {
        m.refuse(errors::make<errors::Code::EVAL_EMPTY_LIST>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    if (!orderable(m, *self))
        return false;
    const Value *best = &self->front();
    for (const Value &item : *self) {
        const int side = order_between(item, *best);
        if (largest ? side > 0 : side < 0)
            best = &item;
    }
    *answer = *best;
    return true;
}

bool list_max(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    return extreme(m, a, true, answer);
}

bool list_min(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    return extreme(m, a, false, answer);
}

} // namespace

void install_list_sorting()
{
    eval::Handlers &table = eval::Handlers::table();
    const auto path = [](words::NodeId id) {
        return static_cast<words::PathId>(id);
    };

    // EVERY SORT MUTATES, which is the decision worth naming: `l.sort()`
    // orders the list it was asked of, and the answer is the receiver's new
    // value (dispatch.hpp's `mutates`), so `l.sort()` in statement position
    // sorts `l` and `m = l.sort()` means what it reads as. The alternative --
    // a sort that answered a new list and left the receiver alone -- would
    // make `l.sort()` on its own line a statement that did nothing, which is
    // the shape a person writes first in every language that has one.
    table.install(path(words::NodeId::CONTAINER_LIST_SORT_0),
                  {list_sort, true, 1, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_SORT_DIRECTION),
                  {list_sort_direction, true, 2, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_SORT_DOWN_0),
                  {list_sort_down, true, 1, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_SORT_DOWN_KEY),
                  {list_sort_down_key, true, 2, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_SORT_UP),
                  {list_sort_up_key, true, 2, "M16", true});

    // max AND min DO NOT MUTATE. They answer an ELEMENT and not a list, so
    // the write-back column would store one element over the whole list --
    // which is why the column is per row and not per family.
    table.install(path(words::NodeId::CONTAINER_LIST_MAX),
                  {list_max, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_LIST_MIN),
                  {list_min, true, 1, "M16"});
}

} // namespace satellite::containers
