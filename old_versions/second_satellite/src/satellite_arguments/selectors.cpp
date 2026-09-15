// The ten selectors on `satellite.container.arguments` `1 4 3` -- PLAN M20,
// settled by the author 2026-09-11. See satellite_arguments/arguments.hpp for
// the module's seam.
//
// THESE ARE METHODS AND THE FACTS ARE NOT, WHICH IS THE WHOLE DIVISION IN THIS
// DIRECTORY. handlers.cpp installs thirty-six rows that take NO receiver:
// `arguments.machine.threads` is a fact about the process, compiled as a module
// constant, and the object is not involved in answering it. Every row here
// binds its receiver, because these are questions about the OBJECT -- how long
// its command line is, what it holds, what it looks like written down -- and
// there is no answering them without one.
//
// TWO CONTAINERS AT ONCE, AND EACH ROW SAYS WHICH ONE IT IS ASKING ABOUT.
// `length()`, `first()`, `last()` and `contains(x)` are the COMMAND LINE:
// every program that takes arguments writes
// `for (i = 1; i < args.length(); i = i + 1)`, and a length that counted the
// machine's facts would walk that loop off the user's words and start reading
// the kernel release as though it had been typed. `count()`, `keys()`,
// `has(k)` and `get(k)` are EVERY entry. That is why `length` and `count` are
// two words rather than one `size`, which words.def's note argues.
//
// AND THE FOUR COMMAND-LINE ROWS NEVER CALL entries_of(), WHICH IS THE
// LAZINESS HOLDING. The thirty-two machine facts are assembled on the first
// ask; a program that only walks what it was typed must not pay for them, so
// those four read `body.words` and nothing else. rows.hpp says so from the
// other side.

#include "satellite_arguments/arguments.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_arguments/rows.hpp"
#include "satellite_containers/containers.hpp"
#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_value/render.hpp"
#include "satellite_value/value_arguments.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <utility>
#include <vector>

namespace satellite::arguments {

namespace {

using words::NodeId;

std::string asked(eval::Machine &m)
{
    return std::string(m.text_of(m.here()));
}

// THE RECEIVER RE-ASKED AT RUN TIME, which every module's methods do and for
// methods_internal.hpp's reason: the compiler proved the DECLARATION was the
// right type, and a declaration is not a value. The one difference here is
// that a program cannot construct one of these -- the language hands it over
// -- so the sentence says where it comes from rather than how to make one.
bool arguments_at(eval::Machine &m, const Value *a, uint32_t who,
                  const Arguments **out)
{
    if (const Arg *handle = std::get_if<Arg>(&a[who]); handle && *handle) {
        *out = handle->get();
        return true;
    }
    if (a[who].is_nothing() && who == 0) {
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), asked(m),
        "a `satellite.container.arguments` -- the object `satellite.main` is "
        "handed, under whatever name the program gave its parameter",
        type_name(a[who])));
    return false;
}

// A NAME, WHICH IS A STRING AND NOTHING ELSE. `has(k)` and `get(k)` take the
// names `keys()` lists, so a number is the wrong kind of question rather than
// a key that is missing -- S0711 and not S0731.
bool name_at(eval::Machine &m, const Value *a, uint32_t who, std::string *out)
{
    const Str *text = std::get_if<Str>(&a[who]);
    if (text == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), asked(m),
            "a `satellite.variable.string` naming an entry", type_name(a[who])));
        return false;
    }
    *out = *text ? decode(**text) : std::string();
    return true;
}

// --- the command line -------------------------------------------------------

bool arguments_length(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Arguments *self = nullptr;
    if (!arguments_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number::from_u64(self->words.size()));
    return true;
}

// THE EMPTY CASE CANNOT HAPPEN AND IS WRITTEN ANYWAY. argv[0] is always there
// when a program runs, so `words` is never empty in `satl`; it IS empty in a
// test binary that installed the rows without a command line, and a front() on
// a vector nobody filled is the kind of crash SCRATCH.md/NO_LIMITS.md §1 says
// is never a limit and always a bug.
bool empty_refusal(eval::Machine &m)
{
    m.refuse(errors::make<errors::Code::EVAL_EMPTY_LIST>(
        m.span_of(m.here()), asked(m)));
    return false;
}

bool arguments_first(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Arguments *self = nullptr;
    if (!arguments_at(m, a, 0, &self))
        return false;
    if (self->words.empty())
        return empty_refusal(m);
    *answer = self->words.front().value;
    return true;
}

bool arguments_last(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Arguments *self = nullptr;
    if (!arguments_at(m, a, 0, &self))
        return false;
    if (self->words.empty())
        return empty_refusal(m);
    *answer = self->words.back().value;
    return true;
}

// BY THE LANGUAGE'S OWN EQUALITY, which is `same()` and is what
// `satellite.container.list.contains(x)` uses one node over -- so a word found
// here and the same word found in a list are found by one rule.
bool arguments_contains(eval::Machine &m, const Value *a, uint32_t,
                        Value *answer)
{
    const Arguments *self = nullptr;
    if (!arguments_at(m, a, 0, &self))
        return false;
    for (const CommandLineWord &word : self->words)
        if (same(word.value, a[1])) {
            *answer = Value::boolean(true);
            return true;
        }
    *answer = Value::boolean(false);
    return true;
}

// --- every entry ------------------------------------------------------------

bool arguments_count(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Arguments *self = nullptr;
    if (!arguments_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number::from_u64(entries_of(*self).size()));
    return true;
}

// IN DISPLAY ORDER, which is the order the printer uses and the order the
// registry is in -- so a program that walks `keys()` and a person reading
// `display(arguments)` see the same object in the same sequence.
bool arguments_keys(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Arguments *self = nullptr;
    if (!arguments_at(m, a, 0, &self))
        return false;
    const std::vector<Entry> entries = entries_of(*self);
    List out;
    out.reserve(entries.size());
    for (const Entry &entry : entries)
        out.push_back(Value::string(encode_raw(entry.name)));
    *answer = Value::list(std::move(out));
    return true;
}

// `has` IS THE QUESTION `get` REFUSES, which is the map's rule at `1 4 1 3`
// and is kept here for its reason: a program that wants to ask before it asks
// needs an answer rather than a refusal.
bool arguments_has(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Arguments *self = nullptr;
    std::string name;
    if (!arguments_at(m, a, 0, &self) || !name_at(m, a, 1, &name))
        return false;
    for (const Entry &entry : entries_of(*self))
        if (entry.name == name) {
            *answer = Value::boolean(true);
            return true;
        }
    *answer = Value::boolean(false);
    return true;
}

bool arguments_get(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Arguments *self = nullptr;
    std::string name;
    if (!arguments_at(m, a, 0, &self) || !name_at(m, a, 1, &name))
        return false;
    for (const Entry &entry : entries_of(*self))
        if (entry.name == name) {
            *answer = entry.value;
            return true;
        }
    // A MISS IS AN ERROR AND NOT NOTHING -- the map's argument at S0726, and
    // sharper here: two of the object's rows are M25's and answer nothing at
    // all, so "absent" and "present and refusing" must not be one answer.
    m.refuse(errors::make<errors::Code::EVAL_NO_SUCH_ENTRY>(
        m.span_of(m.here()), "\"" + name + "\""));
    return false;
}

// --- the whole of it as text -------------------------------------------------

// TWO WORDS, ONE ANSWER, AND THE SHARED BODY IS THE POINT. `to_string()` and
// `lines()` are two numbers in WORD_NUMBERS §2.2 because both readings are
// natural; they are one function here because two would be a pair that could
// drift. And it is the OBJECT and not the command line: showing only what was
// typed would hide the facts that are the reason the object exists.
bool arguments_text(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Arguments *self = nullptr;
    if (!arguments_at(m, a, 0, &self))
        return false;
    *answer = Value::string(encode_raw(object_text(*self)));
    return true;
}

} // namespace

void install_selectors()
{
    eval::Handlers &table = eval::Handlers::table();

    // EVERY ROW BINDS ITS RECEIVER AND COUNTS IT IN THE ARITY -- argument 0 is
    // the object, per DESIGN §6.4's written-out form. None of them mutates:
    // an Arguments is built before make_shared and frozen, which
    // value_arguments.hpp keeps as MapBody's rule.
    struct Row {
        NodeId path;
        eval::HandlerFn fn;
        uint32_t arity;
    };
    static constexpr Row rows[] = {
        {NodeId::CONTAINER_ARGUMENTS_LENGTH,    arguments_length,   1},
        {NodeId::CONTAINER_ARGUMENTS_COUNT,     arguments_count,    1},
        {NodeId::CONTAINER_ARGUMENTS_KEYS,      arguments_keys,     1},
        {NodeId::CONTAINER_ARGUMENTS_TO_STRING, arguments_text,     1},
        {NodeId::CONTAINER_ARGUMENTS_LINES,     arguments_text,     1},
        {NodeId::CONTAINER_ARGUMENTS_HAS,       arguments_has,      2},
        {NodeId::CONTAINER_ARGUMENTS_GET,       arguments_get,      2},
        {NodeId::CONTAINER_ARGUMENTS_FIRST,     arguments_first,    1},
        {NodeId::CONTAINER_ARGUMENTS_LAST,      arguments_last,     1},
        {NodeId::CONTAINER_ARGUMENTS_CONTAINS,  arguments_contains, 2},
    };
    for (const Row &row : rows)
        table.install(static_cast<words::PathId>(row.path),
                      {row.fn, true, row.arity, "M20"});
}

} // namespace satellite::arguments
