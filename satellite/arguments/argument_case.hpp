#pragma once
// satellite/arguments/argument_case.hpp -- ONE ARGUMENT, WRAPPED, AND THE
// REGISTERS THAT SPELL ITS NAME. SATELLITE_ARGUMENTS A11-A15.
//
// The author's brief, 2026-09-17: "we wrap the objects in a special class --
// this class just has specific std::string to it called argument_name and it =
// arguments.name.name.name UP TO 3 specific, special std::string
// argument_name_r1 for register #1 and r2, and r3 and then we can just add
// additional registers as we need them for arguments, HERE is the code that
// calls return_name_register1 return_name_regsiter2 and these registers are set
// to VOID for objects that do not employ that register, so it's kinda like the 0
// in our number system, so the argument system has it's own satellite.numbering
// system".
//
// RULING R3 -- FOUR REGISTERS, NOT THREE. The author's own value list overflows
// three: arguments.system.memory.satellite.free is FOUR parts after `arguments`
// (system, memory, satellite, free). r4 is built now; A15 below is how r5 is
// added, written down so the next one is a diff and not a decision.
//
// RULING R4 -- "VOID" IS THE ZERO, and it is a literal std::string compared with
// ==, never a pointer, and never left empty. An empty register and a VOID
// register must not become two ways to say one thing: is_a_real_argument()
// compares registers, and "" == "" would make arguments.memory and
// arguments.memory.total the same name.

#include "argument_value.hpp"

#include <cstddef>
#include <string>

namespace satellite004 {

// The zero of the argument numbering system. One definition, one spelling.
inline const std::string &argument_register_void()
{
    static const std::string void_register = "VOID";
    return void_register;
}

// How many registers a name may use. A15: RAISING THIS IS THE WHOLE JOB.
//   1. kArgumentRegisters = 5
//   2. add `std::string argument_name_r5 = argument_register_void();` below,
//      next to r4, and its return_name_register5()
//   3. add the r5 line to register_at() and set_register_at()'s switches
// Nothing else reads a register by name: full_name(), is_a_real_argument() and
// find_argument() all walk register_at(), so they follow on their own. The
// static_assert under set_register_at() fails until step 3 is done, so a
// half-raised count does not compile.
inline constexpr std::size_t kArgumentRegisters = 4;

// WHERE A NAME CAME FROM, AND IT IS NOT A DETAIL -- IT IS THE DIFFERENCE BETWEEN
// THE TWO THINGS `arguments` HOLDS. Measured against the tree 2026-09-18: a real
// gather produces 29 names and only 11 of them are words in `words.tsv`.
//
//   word   the LANGUAGE has this name. `arguments.memory.total` is `1 14 1 1 2 1`
//          and a program can read it. The word table is the authority and a name
//          it does not have is a typo.
//   satl   SATL fills this in, and the language has no word for it.
//          `arguments.threads_startup` is a config.ini row; `arguments.file` is
//          what satl was told to run; `arguments.argument_1` exists only on a run
//          that was given a word. None of them are words and none of them are
//          mistakes, and `arguments.file` in particular is how the interpreter
//          knows what to run -- refusing it stops satl from starting at all.
//
// SO THE WORD-TABLE CHECK APPLIES TO `word` AND NOT TO `satl`. That is the whole
// reason this enum exists, and SATELLITE_ARGUMENTS A26-A32 cannot be done
// without it: those milestones move the old store's readers onto this class, and
// the old store is eighteen-twenty-ninths names this class would have refused.
//
// THE DEFAULT IS `word`, so nothing that does not say otherwise gets a weaker
// check than it had.
enum class ArgumentOrigin { word, satl };

// ONE ARGUMENT: a value out of the one variant, and the registers that name it.
// The vector inside satelliteArguments holds THESE and not bare satelliteObjects
// (ruling R2) -- the brief's literal field was std::vector<satelliteObject>, but
// the same brief wraps the objects in the register-carrying class, and the
// registers have nowhere else to live.
class satelliteArgumentCase {
public:
    satelliteArgumentCase() = default;

    // arguments.memory.total  ->  r1 "memory", r2 "total", r3/r4 VOID.
    // The leading "arguments" is not a register: every argument has it, so
    // storing it four hundred times says nothing. from_name() below strips it.
    explicit satelliteArgumentCase(const std::string &dotted_name, ArgumentValue value_input,
                                   ArgumentOrigin origin_input = ArgumentOrigin::word)
        : value_(std::move(value_input)), origin_(origin_input)
    {
        set_name(dotted_name);
    }

    // A14 -- the readers the brief names by name.
    const std::string &return_name_register1() const { return argument_name_r1; }
    const std::string &return_name_register2() const { return argument_name_r2; }
    const std::string &return_name_register3() const { return argument_name_r3; }
    const std::string &return_name_register4() const { return argument_name_r4; }

    // The same four, by index, so a loop does not repeat itself four times.
    // Answers VOID for a register this build does not have, which is the same
    // answer as a register the name does not use -- deliberately, because both
    // mean "this name stops before here".
    const std::string &register_at(std::size_t which) const
    {
        switch (which) {
        case 0: return argument_name_r1;
        case 1: return argument_name_r2;
        case 2: return argument_name_r3;
        case 3: return argument_name_r4;
        default: break;
        }
        return argument_register_void();
    }

    bool uses_register(std::size_t which) const { return register_at(which) != argument_register_void(); }

    // How many registers this name actually employs, counted from the front.
    // A VOID register ends the name: arguments.memory.VOID.total is not a name,
    // and set_name() cannot make one.
    std::size_t depth() const
    {
        std::size_t used = 0;
        while (used < kArgumentRegisters && uses_register(used)) ++used;
        return used;
    }

    // "arguments.memory.total" -- rebuilt, because the registers are what is
    // stored and the spelling is what a person typed.
    std::string full_name() const
    {
        std::string name = "arguments";
        for (std::size_t which = 0; which < kArgumentRegisters; ++which) {
            if (!uses_register(which)) break;
            name += '.';
            name += register_at(which);
        }
        return name;
    }

    // The full word-table spelling, which is what the table is keyed on.
    // words.tsv carries satellite.library.main.arguments.memory.total; the
    // author writes arguments.memory.total. Same word, two spellings, and this
    // is the one place that knows it.
    std::string word_table_name() const
    {
        return "satellite.library.main." + full_name();
    }

    ArgumentOrigin origin() const { return origin_; }
    void set_origin(ArgumentOrigin to) { origin_ = to; }

    // Whether a PROGRAM may read this name. Only a word of the language can be
    // written in a program, so this is exactly `origin() == word`.
    bool a_program_can_read_it() const { return origin_ == ArgumentOrigin::word; }

    const ArgumentValue &value() const { return value_; }
    ArgumentValue &value() { return value_; }
    void set_value(ArgumentValue value_input) { value_ = std::move(value_input); }
    ArgumentKindOf kind() const { return kind_of(value_); }

    // Split a dotted name into the registers. A leading "arguments." (or any of
    // its seven aliases) is dropped; everything after it fills r1 onward, and
    // every register the name does not reach is left VOID. Answers false when
    // the name is deeper than this build's registers -- the caller then knows to
    // read A15 rather than to wonder why a name went missing.
    bool set_name(const std::string &dotted_name)
    {
        for (std::size_t which = 0; which < kArgumentRegisters; ++which)
            set_register_at(which, argument_register_void());

        std::size_t at = 0;
        std::size_t filled = 0;
        bool first_part = true;
        while (at <= dotted_name.size()) {
            const std::size_t dot = dotted_name.find('.', at);
            const std::size_t end = (dot == std::string::npos) ? dotted_name.size() : dot;
            const std::string part = dotted_name.substr(at, end - at);
            at = end + 1;
            if (part.empty()) {
                if (dot == std::string::npos) break;
                continue;
            }
            if (first_part == true) {
                first_part = false;
                if (is_arguments_alias(part) == true) {
                    if (dot == std::string::npos) break;
                    continue;
                }
            }
            if (filled >= kArgumentRegisters) return false;
            set_register_at(filled, part);
            ++filled;
            if (dot == std::string::npos) break;
        }
        return true;
    }

    // THE SEVEN ALIASES, A4. The author's list, whole: "arg, argz, args, argv,
    // arguments, argument, argumentz". One table, read here and by the
    // declaration check, so the spellings cannot drift apart.
    static bool is_arguments_alias(const std::string &word)
    {
        for (const char *alias : {"arg", "argz", "args", "argv", "arguments", "argument", "argumentz"})
            if (word == alias) return true;
        return false;
    }

private:
    void set_register_at(std::size_t which, const std::string &to)
    {
        static_assert(kArgumentRegisters == 4, "A15: raise the count, add the register, add it to both switches");
        switch (which) {
        case 0: argument_name_r1 = to; break;
        case 1: argument_name_r2 = to; break;
        case 2: argument_name_r3 = to; break;
        case 3: argument_name_r4 = to; break;
        default: break;
        }
    }

    // A12/A13 -- four registers, every one of them VOID until a name fills it.
    std::string argument_name_r1 = argument_register_void();
    std::string argument_name_r2 = argument_register_void();
    std::string argument_name_r3 = argument_register_void();
    std::string argument_name_r4 = argument_register_void();

    ArgumentValue value_;
    ArgumentOrigin origin_ = ArgumentOrigin::word;
};

} // namespace satellite004
