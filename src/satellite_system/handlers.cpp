// The four dials, read and retuned. See satellite_system/handlers.hpp for
// why this is a module and what proves the mechanism it rides on.
//
// PLAN §4.5.3, COMPLETED AT LAST: "the file seeds the namespace at startup
// and the namespace is what everything reads afterwards". The seeding is
// policy_from_the_limits() -- the config file, read once -- and the
// namespace's run-time half is the machine's Policy: every read below
// answers from it, every write below stores into it, and the next division
// or rounding simply observes the new count. Nothing here re-reads a file
// and nothing is process-wide, so M22's many-programs future inherits a dial
// per machine rather than a fight over one.

#include "satellite_system/handlers.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "machine_limits/limits.hpp"
#include "satellite_system/host_methods.hpp"
#include "satellite_system/memory_methods.hpp"

#include <atomic>
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::system {

namespace {

// --- the reads -- module constants, answering the machine's own Policy -----

bool read_division_digits(eval::Machine &m, const Value *, uint32_t,
                          Value *answer)
{
    *answer = Value::number(Number(
        static_cast<long long>(m.policy().division_digits)));
    return true;
}

bool read_float_digits(eval::Machine &m, const Value *, uint32_t,
                       Value *answer)
{
    *answer = Value::number(Number(
        static_cast<long long>(m.policy().float_digits)));
    return true;
}

bool read_max_depth(eval::Machine &m, const Value *, uint32_t, Value *answer)
{
    // BYTES, AND ALREADY RESOLVED: unset meant the machine at seed time
    // (limits::max_depth_bytes' fallback), so what a program reads is the
    // ceiling actually being enforced, never a zero standing in for a story.
    *answer = Value::number(Number::from_u64(m.policy().max_depth));
    return true;
}

bool read_min_free_mb(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    // THE ONE DIAL WHOSE HOME IS NOT THE POLICY: the watchdog's floor, read
    // from where the watchdog reads it. NOT WATCHED ANSWERS NOTHING -- M12's
    // askable state, and the honest one: an unwatched floor means the
    // machine's free memory is not looked at at all, and a 0 here would be a
    // number claiming it is.
    //
    // AND IT IS THE LIVE WORD SINCE M20, NOT THE SEEDED DIAL, so a program
    // reads back what it just wrote. The dial still records where the seed
    // came from, which is what `satl --limits` prints.
    unsigned long long floor = 0;
    *answer = limits::watched_floor(&floor)
                  ? Value::number(Number::from_u64(floor))
                  : Value::nothing();
    return true;
}

// --- the writes -- the retune rows ------------------------------------------

// A dial takes a whole number a machine word holds, or the row says which of
// those it is not -- S0713, the same shape scalars' shift_count uses, because
// at run time the caret is the statement's and the file-and-line answers
// S0807/S0808 give belong to the config file.
bool dial_count(eval::Machine &m, const Value &value, const char *dial,
                unsigned long long least, unsigned long long most,
                unsigned long long *out)
{
    const Number *count = std::get_if<Number>(&value);
    long long wide = 0;
    if (count == nullptr || !count->is_integer() ||
        !count->to_integer(wide) || wide < 0 ||
        static_cast<unsigned long long>(wide) < least ||
        static_cast<unsigned long long>(wide) > most) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), dial,
            "a whole number a machine word holds",
            count == nullptr ? type_name(value)
                             : "`" + count->to_string() + "`"));
        return false;
    }
    *out = static_cast<unsigned long long>(wide);
    return true;
}

bool retune_division_digits(eval::Machine &m, const Value *arguments,
                            uint32_t, Value *)
{
    // The floor is 1 for Number::divide's reason -- a zero count keeps no
    // digits and has nothing to answer -- and the ceiling is the word the
    // count is carried in. The same pair S0807/S0808 hold the config file to.
    unsigned long long count = 0;
    if (!dial_count(m, arguments[0], "division_digits",
                    limits::kDivisionDigitsLeast,
                    limits::kDivisionDigitsMost, &count))
        return false;
    m.retune_division_digits(static_cast<unsigned>(count));
    return true;
}

bool retune_float_digits(eval::Machine &m, const Value *arguments, uint32_t,
                         Value *)
{
    // INT_MAX and not UINT_MAX -- limits.hpp carries why: a fractional place
    // lives in a Number's int exponent, so a wider right half has no Number
    // to be rounded in.
    unsigned long long count = 0;
    if (!dial_count(m, arguments[0], "float_digits",
                    limits::kFloatDigitsLeast, limits::kFloatDigitsMost,
                    &count))
        return false;
    m.retune_float_digits(static_cast<unsigned>(count));
    return true;
}

bool retune_max_depth(eval::Machine &m, const Value *arguments, uint32_t,
                      Value *)
{
    // ANY NUMBER OF BYTES IS A NUMBER OF BYTES -- the dial has no range for
    // config_internal.hpp's reason, and that holds here too: zero refuses
    // the next growth and says so about recursion, which is a working answer.
    unsigned long long bytes = 0;
    if (!dial_count(m, arguments[0], "max_depth", 0, ~0ULL, &bytes))
        return false;
    m.retune_max_depth(bytes);
    return true;
}

// THE ROW THAT REFUSED UNTIL M20, AND WHAT IT WAS WAITING FOR WAS ONE WORD.
// It used to say "the watchdog reads its floor from the config's store on a
// thread of its own, and a live handoff to it is undesigned" -- which was true
// and was the right refusal while it lasted: a torn read on that thread kills
// a healthy process, the worst thing a watchdog can do. The design is a single
// atomic `unsigned long long`, written whole here and read once per wake-up
// there, and machine_limits/limits.hpp carries why that is all of it.
//
// SO §4.5.3's SENTENCE IS TRUE FOR THE FIRST TIME: "the file is where a
// machine's settings live before a program starts; the namespace is how a
// running program reads and changes them." Both halves, one dial.
//
// "disabled" TURNS THE WATCH OFF AND `0` DOES NOT, settled by the author on
// 2026-09-11. A floor of zero is a real floor that is never crossed -- no
// machine has less than 0 MB free -- and reading it as "stop watching" would
// be the language guessing at a number's meaning, which is the conversion
// DESIGN §1.1 does not have. So the off switch is a WORD, and it is quoted
// because a bare one is a variable name in this language: the four file modes
// at S1201 are the same shape and the same decision.
bool retune_min_free_mb(eval::Machine &m, const Value *arguments, uint32_t,
                        Value *)
{
    if (const Str *word = std::get_if<Str>(&arguments[0])) {
        const std::string said = *word ? decode(**word) : std::string();
        if (said != "disabled") {
            m.refuse(errors::make<errors::Code::SYSTEM_BAD_DIAL_WORD>(
                m.span_of(m.here()), "min_free_mb", "\"" + said + "\""));
            return false;
        }
        limits::stop_watching_free_memory();
        return true;
    }

    // ANY NUMBER OF MEGABYTES IS A NUMBER OF MEGABYTES, which is max_depth's
    // rule one row up and holds here for the same reason: a floor above what
    // the machine has stops the run within the second, and that is a working
    // answer to somebody who asked for it rather than a mistake to refuse.
    unsigned long long megabytes = 0;
    if (!dial_count(m, arguments[0], "min_free_mb", 0, limits::kFloorMost,
                    &megabytes))
        return false;
    limits::watch_floor(megabytes);
    return true;
}

} // namespace

// --- satellite.system.persist -------------------------------------------------

// THE ONE PROCESS-WIDE THING IN THIS FILE, and the note at the top says nothing
// here is process-wide -- so this is a departure and it is worth the sentence.
// The four dials belong to a Machine because a program obeys them; this belongs
// to the PROMPT, which outlives every Machine it makes, and a per-run home for
// it would be forgotten between the run that set it and the run that reads it.
// That is the same argument evaluator/dispatch.hpp makes for the handler table
// being a property of the build rather than of a run, one layer up.
std::atomic<bool> keeping{true};

// `satellite.system.persist()` `1 22 7` -- what the setting is.
bool read_persist(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::boolean(persisting());
    return true;
}

// `satellite.system.persist(x)` `1 22 8` -- set it, and answer what it now is.
//
// IT ANSWERS THE NEW SETTING RATHER THAN `nothing`, which is a small decision
// with a reason: `satellite.console.display(satellite.system.persist(false))`
// then says `false`, so the one call can be read as well as written and the
// prompt's banner has something to print without asking twice.
bool write_persist(eval::Machine &machine, const Value *arguments,
                   uint32_t count, Value *answer)
{
    // A REFUSAL AND NOT A COERCION. `persist(1)` is not `persist(true)` here
    // for the reason DESIGN §1.1 gives everywhere else: the language has no
    // conversions, so a number where a bool was asked for is a mistake to name
    // rather than a thing to guess the meaning of.
    if (count != 1 || !arguments[0].is_bool()) {
        machine.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            machine.span_of(machine.here()),
            std::string(machine.text_of(machine.here())),
            "a `satellite.variable.bool`",
            count == 1 ? type_name(arguments[0]) : "nothing"));
        return false;
    }
    set_persisting(std::get<bool>(arguments[0]));
    *answer = Value::boolean(persisting());
    return true;
}

void install_handlers()
{
    using words::NodeId;

    struct Row {
        NodeId path;
        eval::HandlerFn read;
        eval::HandlerFn write;
    };
    // READ AND WRITE INSTALLED TOGETHER, one row per dial, so the two tables
    // cannot disagree about which paths are the dials.
    static constexpr Row rows[] = {
        {NodeId::LIBRARY_SYSTEM_DIVISION_DIGITS, read_division_digits,
         retune_division_digits},
        {NodeId::LIBRARY_SYSTEM_MAX_DEPTH, read_max_depth, retune_max_depth},
        {NodeId::LIBRARY_SYSTEM_MIN_FREE_MB, read_min_free_mb,
         retune_min_free_mb},
        {NodeId::LIBRARY_SYSTEM_FLOAT_DIGITS, read_float_digits,
         retune_float_digits},
    };
    for (const Row &row : rows) {
        eval::Handlers::table().install(
            static_cast<words::PathId>(row.path), {row.read, false, 0, "M15"});
        eval::Assigners::table().install(
            static_cast<words::PathId>(row.path), {row.write, "M15"});
    }

    // `satellite.system`'s FIRST BUILT CHILD, and WORD_NUMBERS §2.7 says so:
    // that node has carried thirty numbered paths since the 2026-08-28
    // transcription and no milestone had reached any of them.
    eval::Handlers::table().install(
        static_cast<words::PathId>(words::NodeId::SYSTEM_PERSIST_0),
        {read_persist, false, 0, "M22"});
    eval::Handlers::table().install(
        static_cast<words::PathId>(words::NodeId::SYSTEM_PERSIST_X),
        {write_persist, false, 1, "M22"});

    // M20 -- THE REST OF `satellite.system`, INSTALLED FROM THEIR OWN FILES.
    // The node above had two built children and thirty numbered ones until
    // 2026-09-11; these two calls are most of the difference. They are separate
    // files because they are separate subjects (PLAN M20 splits v1's one
    // 321-line modules_system.cpp by exactly this seam) and one install because
    // a caller still installs `satellite.system` once.
    install_memory();
    install_host();
}

bool persisting()
{
    return keeping.load(std::memory_order_relaxed);
}

void set_persisting(bool on)
{
    keeping.store(on, std::memory_order_relaxed);
}

} // namespace satellite::system
