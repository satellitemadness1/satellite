// `satellite.system.memory` `1 22 4` and everything under it -- thirteen rows
// that answer one question in five units. See satellite_system/units.hpp for
// the unit table and why it is a string.
//
// A FILE OF ITS OWN AND NOT handlers.cpp's, WHICH IS PLAN §3 ARRIVING ON TIME.
// v1 answered all of `satellite.system` from modules_system.cpp at 321 lines,
// and PLAN M20 names that file as the reason this milestone "splits by
// subject" rather than porting it whole. The subjects really are separate:
// this one is a quantity of bytes converted into a unit, and `home`,
// `environment` and the dials are not quantities of anything.
//
// THE THIRTEEN ARE THREE QUESTIONS ASKED OF THREE THINGS, and the table at the
// bottom is the whole of the arrangement:
//
//     what                which          bare            with a unit
//     the machine's RAM   total/free/used  1 22 4 6-8     1 22 4 10-12
//     this process        main            1 22 4 3       1 22 4 9
//     the swap file       total/free/used  1 22 4 4 1-3   1 22 4 4 4-7
//     this thread's stack available/free/used 1 22 4 5 1-3 1 22 4 5 4-6
//
// SO EVERY ROW IS `bytes, in a unit` AND ONE FUNCTION DOES IT. What differs
// between rows is which reader supplies the bytes and which argument, if any,
// supplies the unit -- and both are data, so the rows below are a table and
// not thirteen functions that each divide by 1024 somewhere.

#include "satellite_system/memory_methods.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_system/units.hpp"
#include "satellite_value/render.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"
#include "system_facts/facts.hpp"

#include <string>

namespace satellite::system {

namespace {

std::string asked(eval::Machine &m)
{
    return std::string(m.text_of(m.here()));
}

// The unit an argument names, or a refusal and 0.
//
// ONE SITE, SO S1301 IS RAISED IN ONE PLACE -- satellite_file/handlers.cpp's
// mode_at() is the precedent and its reason is the same: thirteen rows that
// each spelled the refusal would be thirteen chances for "gb" to mean
// something different.
unsigned long long unit_at(eval::Machine &m, const Value *arguments,
                           uint32_t count)
{
    // NO ARGUMENT IS NOT A MISTAKE, it is the bare shape asking for the
    // default -- `memory.total()` and `memory.total("mb")` are two numbers in
    // WORD_NUMBERS §2.2 and one answer here.
    if (count == 0)
        return unit_divisor(kDefaultUnit);

    const Str *text = std::get_if<Str>(&arguments[0]);
    if (!text) {
        // A UNIT GIVEN AS A NUMBER IS A WRONG TYPE AND NOT A BAD UNIT, so it
        // gets S0711 and not S1301. `memory.total(3)` did not misspell a word;
        // it handed over the wrong kind of thing entirely, and a message
        // listing five spellings would be answering a question nobody asked.
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), asked(m),
            "a `satellite.variable.string` unit", type_name(arguments[0])));
        return 0;
    }

    const std::string word = *text ? decode(**text) : std::string();
    if (const unsigned long long divisor = unit_divisor(word))
        return divisor;

    m.refuse(errors::make<errors::Code::SYSTEM_BAD_UNIT>(
        m.span_of(m.here()), asked(m), "\"" + word + "\""));
    return 0;
}

// Bytes, reported in the unit the call names.
//
// EXACT, AND THE DIVISION NEVER ROUNDS. Every divisor is a power of 1024, so
// the decimal terminates and `division_digits` -- which is read only by "a
// division that does not end" -- never applies. units.hpp carries the
// measurement that says so.
bool answer_in_unit(eval::Machine &m, const Value *arguments, uint32_t count,
                    unsigned long long bytes, Value *answer)
{
    const unsigned long long divisor = unit_at(m, arguments, count);
    if (divisor == 0)
        return false;               // unit_at has already refused

    // BYTES ARE HANDED BACK WHOLE rather than divided by one, so `("b")`
    // answers an integer and not an integer with an exact fractional zero.
    if (divisor == 1) {
        *answer = Value::number(Number::from_u64(bytes));
        return true;
    }
    *answer = Value::number(Number::divide(Number::from_u64(bytes),
                                           Number::from_u64(divisor),
                                           m.policy().division_digits));
    return true;
}

// --- where the bytes come from ----------------------------------------------
//
// ONE FUNCTION PER FACT AND NOT PER ROW, because the bare row and the unit row
// ask the same question -- which is exactly what WORD_NUMBERS §2.2 is saying by
// giving them two numbers and one name.

unsigned long long machine_total() { return facts::mem_total_bytes(); }
unsigned long long machine_free()  { return facts::mem_available_bytes(); }
unsigned long long machine_used()  { return facts::mem_used_bytes(); }

unsigned long long process_main()  { return facts::process_memory_bytes(); }

unsigned long long swap_total() { return facts::swap_total_bytes(); }
unsigned long long swap_free()  { return facts::swap_free_bytes(); }
unsigned long long swap_used()  { return facts::swap_used_bytes(); }

// THIS THREAD'S STACK, AND `available` IS THE ONE WORD THAT IS NOT `free`.
// facts::thread_stack_bytes() answers used and total together, so the three
// are one read taken apart three ways -- and when the platform will not say,
// all three answer 0 rather than guessing from a limit that may not apply.
unsigned long long stack_used()
{
    unsigned long long used = 0, total = 0;
    return facts::thread_stack_bytes(&used, &total) ? used : 0;
}

// NO `stack_total`, AND THERE WAS ONE UNTIL M20's FOURTH COMMIT. It was
// written on 2026-09-11 beside these two and never installed, because
// `satellite.system.memory.this` has no `total` row -- its children are
// `available`, `free` and `used` and their unit forms, and that is all. An
// uninstalled reader is a -Wunused-function warning in a tree built with
// -Wall -Wextra, which is how it was found.
unsigned long long stack_free()
{
    unsigned long long used = 0, total = 0;
    if (!facts::thread_stack_bytes(&used, &total))
        return 0;
    return total > used ? total - used : 0;
}

} // namespace

// --- the rows ----------------------------------------------------------------

namespace {

// One handler per reader, made by the compiler.
//
// A ROW CANNOT CARRY THE READER AS DATA, and finding that out is what this
// template is. `eval::HandlerFn` is a plain function pointer -- no captures,
// no state -- so a table of {path, reader, arity} would need the dispatcher to
// hand the row back to the handler, and nothing in dispatch.hpp does or should:
// a handler is reached BY ITS NUMBER and the number is the whole of what the
// machine knows. Making the reader a TEMPLATE ARGUMENT instead puts the choice
// at compile time, where it already was in every other sense -- the result is
// twenty-one distinct function pointers, each three instructions long, and the
// table below stays the flat list of rows the subject actually is.
template <unsigned long long (*Bytes)()>
bool report(eval::Machine &m, const Value *arguments, uint32_t count,
            Value *answer)
{
    return answer_in_unit(m, arguments, count, Bytes(), answer);
}

// THE FIRMWARE PAIR IS NOT REPORTED IN A UNIT, because neither is a quantity of
// memory: `bit` is how wide the bus is and `frequency` is how fast the sticks
// are clocked. Dividing either by 1024 would be nonsense with a number attached,
// so they take no unit argument and are written out rather than templated.
// v1 says the same thing in the same place.
//
// BOTH ANSWER 0 FOR AN ORDINARY USER AND 0 IS THE TRUTHFUL ANSWER -- the SMBIOS
// entries are root-only. system_facts/firmware_facts.cpp carries the receipt,
// and PLAN M20's done-when requires the demonstration to SAY that rather than
// treat it as a failure.
bool read_bit(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::number(Number::from_u64(facts::mem_width_bits()));
    return true;
}

bool read_frequency(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::number(Number::from_u64(facts::mem_frequency_mhz()));
    return true;
}

struct Row {
    words::NodeId path;
    eval::HandlerFn fn;
    uint32_t arity;             // 0 is the bare shape, 1 is (unit)
};

} // namespace

void install_memory()
{
    using words::NodeId;

    static const Row rows[] = {
        // --- the machine's memory: bare, then the same three with a unit ---
        {NodeId::SYSTEM_MEMORY_FREE_0,          report<machine_free>,  0},
        {NodeId::SYSTEM_MEMORY_TOTAL_0,         report<machine_total>, 0},
        {NodeId::SYSTEM_MEMORY_USED_0,          report<machine_used>,  0},
        {NodeId::SYSTEM_MEMORY_FREE_UNIT,       report<machine_free>,  1},
        {NodeId::SYSTEM_MEMORY_TOTAL_UNIT,      report<machine_total>, 1},
        {NodeId::SYSTEM_MEMORY_USED_UNIT,       report<machine_used>,  1},

        // --- this process's own resident set ---
        {NodeId::SYSTEM_MEMORY_MAIN_0,          report<process_main>,  0},
        {NodeId::SYSTEM_MEMORY_MAIN_UNIT,       report<process_main>,  1},

        // --- the swap file ---
        //
        // `swap.used` 1 22 4 4 3 TAKES NO ARGUMENT AND `swap.used(unit)`
        // 1 22 4 4 7 DOES, and that second number is one this milestone minted:
        // §2.2 had written `used` without the parens its siblings carry while
        // v1's unit block answered both. The two rows here are what that fix
        // looks like once it is built.
        {NodeId::SYSTEM_MEMORY_SWAP_FREE_0,     report<swap_free>,     0},
        {NodeId::SYSTEM_MEMORY_SWAP_TOTAL_0,    report<swap_total>,    0},
        {NodeId::SYSTEM_MEMORY_SWAP_USED,       report<swap_used>,     0},
        {NodeId::SYSTEM_MEMORY_SWAP_FREE_UNIT,  report<swap_free>,     1},
        {NodeId::SYSTEM_MEMORY_SWAP_TOTAL_UNIT, report<swap_total>,    1},
        {NodeId::SYSTEM_MEMORY_SWAP_USED_UNIT,  report<swap_used>,     1},

        // `swap(unit)` 1 22 4 4 6 IS SWAP'S OWN OPTIONAL UNIT -- the bare
        // object asked for in a unit, which is the total. §2.2 names it
        // "swap's own optional unit" and that is what it answers.
        {NodeId::SYSTEM_MEMORY_SWAP_UNIT,       report<swap_total>,    1},

        // --- this thread's stack ---
        //
        // `available` AND `free` ARE TWO WORDS FOR ONE FACT, which is v1's
        // shape and not a redundancy introduced here: §2.2 gives both numbers
        // and DESIGN §1's "a word means one thing everywhere" is why they must
        // answer the same. A reader who learned `memory.free()` has already
        // learned this one.
        {NodeId::SYSTEM_MEMORY_THIS_AVAILABLE_0,    report<stack_free>, 0},
        {NodeId::SYSTEM_MEMORY_THIS_FREE_0,         report<stack_free>, 0},
        {NodeId::SYSTEM_MEMORY_THIS_USED,           report<stack_used>, 0},
        {NodeId::SYSTEM_MEMORY_THIS_AVAILABLE_UNIT, report<stack_free>, 1},
        {NodeId::SYSTEM_MEMORY_THIS_FREE_UNIT,      report<stack_free>, 1},
        {NodeId::SYSTEM_MEMORY_THIS_USED_UNIT,      report<stack_used>, 1},
    };

    for (const Row &row : rows)
        eval::Handlers::table().install(static_cast<words::PathId>(row.path),
                                        {row.fn, false, row.arity, "M20"});

    eval::Handlers::table().install(
        static_cast<words::PathId>(NodeId::SYSTEM_MEMORY_BIT),
        {read_bit, false, 0, "M20"});
    eval::Handlers::table().install(
        static_cast<words::PathId>(NodeId::SYSTEM_MEMORY_FREQUENCY),
        {read_frequency, false, 0, "M20"});
}

} // namespace satellite::system
