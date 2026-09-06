// The list's rows, less the sort family -- `1 4 2 1` through `1 4 2 25` with
// `1 4 2 3`..`1 4 2 7` next door in list_sorting.cpp, plus the minted
// `search(pattern)` `1 4 2 26`. See satellite_containers/handlers.hpp.
//
// THE SEMANTICS WERE DECIDED AT M16, the way the string's sixteen were at
// M11: the words came to the numbering from QUAD with signatures and no
// meanings -- v1's list answered only size, first, last and contains -- so
// the milestone that built them chose, and MILESTONES/M16.md §2 carries every
// decision with its overrule point. The rules they all follow are M11's:
// positions count from 0; where another language hands back a sentinel,
// satellite refuses (S0725, S0728, S0729 -- each names the question that
// answers instead of stopping); a mutating method's answer is its receiver's
// new value, written back by the machine (dispatch.hpp's `mutates`).
//
// EVERY MUTATION COPIES THE WHOLE BODY. That is the containers' immutability
// contract -- built, then frozen, so a value two slots see can never change
// under either -- and DESIGN §12 records the in-place fast path as
// deliberately deferred, with the condition a future milestone must meet.

#include "error_reporter/report.hpp"
#include "satellite_containers/methods_internal.hpp"
#include "satellite_containers/search.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace satellite::containers {
namespace {

// The empty-list refusal, from the five rows that raise it.
bool empty_refusal(eval::Machine &m)
{
    m.refuse(errors::make<errors::Code::EVAL_EMPTY_LIST>(m.span_of(m.here()),
                                                         asked(m)));
    return false;
}

// A position checked against this list's length -- S0725 names both numbers.
bool inside(eval::Machine &m, const List &self, unsigned long long at,
            unsigned long long limit)
{
    if (at < limit)
        return true;
    m.refuse(errors::make<errors::Code::EVAL_OUTSIDE_THE_LIST>(
        m.span_of(m.here()), asked(m), std::to_string(at),
        std::to_string(self.size())));
    return false;
}

bool list_append(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    List next = *self;
    next.push_back(a[1]);
    *answer = Value::list(std::move(next));
    return true;
}

bool list_size(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number(static_cast<long long>(self->size())));
    return true;
}

bool list_contains(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    for (const Value &item : *self)
        if (same(item, a[1])) {
            *answer = Value::boolean(true);
            return true;
        }
    *answer = Value::boolean(false);
    return true;
}

// The first position holding `x`, by the language's own equality -- so a
// number finds the float it equals, same()'s M15 note. A miss refuses
// naming `contains` (S0728): a -1 written into programs is forever.
bool list_index_of(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    for (size_t i = 0; i < self->size(); i++)
        if (same((*self)[i], a[1])) {
            *answer = Value::number(Number(static_cast<long long>(i)));
            return true;
        }
    m.refuse(errors::make<errors::Code::EVAL_NOT_IN_THE_LIST>(
        m.span_of(m.here()), text_of(a[1]), asked(m)));
    return false;
}

bool list_empty(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    *answer = Value::boolean(self->empty());
    return true;
}

bool list_clear(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    *answer = Value::list(List{});
    return true;
}

bool list_first(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    if (self->empty())
        return empty_refusal(m);
    *answer = self->front();
    return true;
}

bool list_last(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    if (self->empty())
        return empty_refusal(m);
    *answer = self->back();
    return true;
}

// Keep the first n. A LARGER n IS A NO-OP AND NOT AN ERROR, because truncate
// is the ring-buffer half of DESIGN §12's set-and-deque decision -- "bounded
// ring buffers, which remove_first and truncate answer" -- and a bound that
// refused a list already under it would have to be guarded at every call.
bool list_truncate(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    unsigned long long keep = 0;
    if (!list_at(m, a, 0, &self) || !position_at(m, a, 1, &keep))
        return false;
    List next(self->begin(),
              self->begin() + static_cast<long>(std::min(
                                  keep, static_cast<unsigned long long>(
                                            self->size()))));
    *answer = Value::list(std::move(next));
    return true;
}

// A capacity hint, kept honest under copy-on-write: the content is unchanged
// and the body's vector reserves. What it buys today is small; the row
// exists because the numbering assigned it.
bool list_reserve(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    unsigned long long room = 0;
    if (!list_at(m, a, 0, &self) || !position_at(m, a, 1, &room))
        return false;
    List next;
    next.reserve(static_cast<size_t>(room) > self->size()
                     ? static_cast<size_t>(room)
                     : self->size());
    next.assign(self->begin(), self->end());
    *answer = Value::list(std::move(next));
    return true;
}

bool list_remove_first(eval::Machine &m, const Value *a, uint32_t,
                       Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    if (self->empty())
        return empty_refusal(m);
    *answer = Value::list(List(self->begin() + 1, self->end()));
    return true;
}

bool list_remove_last(eval::Machine &m, const Value *a, uint32_t,
                      Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    if (self->empty())
        return empty_refusal(m);
    *answer = Value::list(List(self->begin(), self->end() - 1));
    return true;
}

bool list_remove_at(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    unsigned long long at = 0;
    if (!list_at(m, a, 0, &self) || !position_at(m, a, 1, &at))
        return false;
    if (!inside(m, *self, at, self->size()))
        return false;
    List next = *self;
    next.erase(next.begin() + static_cast<long>(at));
    *answer = Value::list(std::move(next));
    return true;
}

// The first occurrence of `x`, by value -- and a miss refuses exactly as
// index_of's does, because "remove what is not there" is the same wrong
// program as "where is what is not there".
bool list_remove(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    for (size_t i = 0; i < self->size(); i++)
        if (same((*self)[i], a[1])) {
            List next = *self;
            next.erase(next.begin() + static_cast<long>(i));
            *answer = Value::list(std::move(next));
            return true;
        }
    m.refuse(errors::make<errors::Code::EVAL_NOT_IN_THE_LIST>(
        m.span_of(m.here()), text_of(a[1]), asked(m)));
    return false;
}

// `insert(n, x)` puts x AT position n, so n may equal the size -- that is the
// append position, and refusing it would make insert unable to say what
// append says. Beyond it is S0725, with the count in the sentence.
bool list_insert(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    unsigned long long at = 0;
    if (!list_at(m, a, 0, &self) || !position_at(m, a, 1, &at))
        return false;
    if (!inside(m, *self, at, self->size() + 1))
        return false;
    List next = *self;
    next.insert(next.begin() + static_cast<long>(at), a[2]);
    *answer = Value::list(std::move(next));
    return true;
}

// One string from every element. A string element contributes its STORED
// codes -- the render/store boundary is the printer's, not a method's (M11's
// rule for the string methods, held here) -- and any other element
// contributes what display would print for it.
bool list_join(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    const Str *separator = std::get_if<Str>(&a[1]);
    if (separator == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), asked(m), "a `satellite.variable.string` "
            "separator", type_name(a[1])));
        return false;
    }
    SatString out;
    for (size_t i = 0; i < self->size(); i++) {
        if (i && *separator)
            out += **separator;
        if (const Str *text = std::get_if<Str>(&(*self)[i]))
            out += *text ? **text : SatString();
        else
            out += encode_raw(text_of((*self)[i]));
    }
    *answer = Value::string(std::move(out));
    return true;
}

bool list_reverse(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    List next(self->rbegin(), self->rend());
    *answer = Value::list(std::move(next));
    return true;
}

// The sum of a list of numbers, and the empty list's sum is 0 -- not a
// sentinel but the sum of no numbers, the one row here where "nothing to
// answer with" has an answer. A non-number element refuses by name; whether
// floats join later is recorded as an overrule point in MILESTONES/M16.md.
bool list_sum(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    Number total;
    for (const Value &item : *self) {
        const Number *number = std::get_if<Number>(&item);
        if (number == nullptr) {
            m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
                m.span_of(m.here()), asked(m),
                "a list holding only `satellite.variable.number` elements",
                type_name(item)));
            return false;
        }
        total = Number::add(total, *number);
    }
    *answer = Value::number(std::move(total));
    return true;
}

bool list_search(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const List *self = nullptr;
    if (!list_at(m, a, 0, &self))
        return false;
    *answer = search_collect(a[0], a[1], true, m.search_threshold());
    return true;
}

} // namespace

void install_list_methods()
{
    eval::Handlers &table = eval::Handlers::table();
    const auto path = [](words::NodeId id) {
        return static_cast<words::PathId>(id);
    };

    table.install(path(words::NodeId::CONTAINER_LIST_APPEND),
                  {list_append, true, 2, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_SIZE),
                  {list_size, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_LIST_CONTAINS),
                  {list_contains, true, 2, "M16"});
    table.install(path(words::NodeId::CONTAINER_LIST_INDEX_OF),
                  {list_index_of, true, 2, "M16"});
    table.install(path(words::NodeId::CONTAINER_LIST_EMPTY),
                  {list_empty, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_LIST_CLEAR),
                  {list_clear, true, 1, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_FIRST),
                  {list_first, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_LIST_LAST),
                  {list_last, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_LIST_TRUNCATE),
                  {list_truncate, true, 2, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_RESERVE),
                  {list_reserve, true, 2, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_REMOVE_FIRST_0),
                  {list_remove_first, true, 1, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_REMOVE_LAST_0),
                  {list_remove_last, true, 1, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_REMOVE_AT),
                  {list_remove_at, true, 2, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_REMOVE),
                  {list_remove, true, 2, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_INSERT),
                  {list_insert, true, 3, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_JOIN),
                  {list_join, true, 2, "M16"});
    table.install(path(words::NodeId::CONTAINER_LIST_REVERSE),
                  {list_reverse, true, 1, "M16", true});
    table.install(path(words::NodeId::CONTAINER_LIST_SUM),
                  {list_sum, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_LIST_SEARCH),
                  {list_search, true, 2, "M16"});
}

} // namespace satellite::containers
