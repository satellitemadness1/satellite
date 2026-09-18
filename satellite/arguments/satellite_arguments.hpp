#pragma once
// satellite/arguments/satellite_arguments.hpp -- THE CLASS THE BRIEF ASKS FOR.
// SATELLITE_ARGUMENTS A16-A23.
//
// The author's brief, 2026-09-17:
//
//     class satelliteArguments
//     {
//         protected:
//             std::vector<satelliteObject> argument_cases;
//             bool add_argument(satelliteObject object_input) { ... }
//     };
//
// "after it has passed through checking that the name is a real list from our
// list of argument names, we check that a value exists that is within bounds --
// this value is a std::variant and has templates for each value type -- when the
// type is... a satellite_number, then we use a template for that type to check
// that the value is in bounds. If after it passes the name and template value
// checks, we can finally add it to our list".
//
// So add_argument is TWO CHECKS AND A PUSH, in that order, and both checks are
// below in the order the brief gives them.
//
// RULING R2: the vector holds satelliteArgumentCase, not satelliteObject --
// the wrapper is what carries the registers, and the brief's own next sentence
// wraps the object in it.
//
// THE ACCEPTANCE TEST IS THE COMPILER (the author): "none of this will be tested
// or checked or anything -- we just write it, make sure it compiles, and if it
// compiles then we just accept it as the gospel!" There is no check.sh row and
// no pty test for this file. Behaviour is checked once the values exist (B7-B15).

#include "argument_case.hpp"
#include "../bytecode/word_codes.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace satellite004 {

// A17 -- IS THIS A REAL ARGUMENT NAME? The registers are spelled back out and
// looked up in the word table.
//
// THE TABLE IS words.tsv AND IT IS NOT COPIED HERE, which is A16's whole point:
// word_codes.hpp is GENERATED from words.tsv (through words_004.tsv and
// make_words.py, and check.sh regenerates it and compares byte for byte), so a
// hand-written list of names in this file would be a second table free to fall
// behind the first. code_of_spelling() is a binary search over 367 sorted
// spellings -- it is not a cost worth avoiding on a path that runs once per
// argument gathered.
//
// Answers false for arguments.memory.satellite.free until that row exists, which
// is correct and is the point: a name is real when the language has the word.
inline bool is_a_real_argument(const satelliteArgumentCase &argument_case)
{
    if (argument_case.depth() == 0) return false;   // bare `arguments` names no value

    // A NAME SATL FILLS IN IS NOT CHECKED AGAINST THE WORD TABLE, because the
    // language has no word for it and is not supposed to. `arguments.file`,
    // `arguments.threads_startup`, `arguments.argument_1` -- eighteen of the
    // twenty-nine a real gather produces. What checks THOSE is the thing that
    // produces them: a config row is checked by `gather_config()` against the
    // author's own `return_arguments_vector()`, and a fact satl fills in is
    // checked by `filled_in_by_satl()`, which `arguments_cases.cpp` already
    // asserts stays in step with what `gather()` really adds.
    //
    // SO NOTHING IS UNCHECKED. It is checked somewhere else, by the code that
    // knows what it means -- which is why this is a narrowing of THIS check and
    // not a hole in it.
    if (argument_case.origin() == ArgumentOrigin::satl) return true;

    return word::code_of_spelling(argument_case.word_table_name()) != 0;
}

inline bool is_a_real_argument(const std::string &dotted_name,
                               ArgumentOrigin origin = ArgumentOrigin::word)
{
    satelliteArgumentCase named(dotted_name, ArgumentValue{}, origin);
    return is_a_real_argument(named);
}

// A18 -- THE BOUNDS TEMPLATE, WHICH ACCEPTS BY DEFAULT. Every type that has no
// bound to check answers true, so an arm added to the variant is accepted rather
// than silently refused by a check nobody wrote for it.
template <typename T>
inline bool value_in_bounds(const T &)
{
    return true;
}

// A19 -- SATELLITE_NUMBER, AND WHAT "IN BOUNDS" HONESTLY MEANS HERE.
//
// IT IS NOT A CEILING. satellite_number has no upper limit by design -- DESIGN's
// own rule, and the author's: a bound is never the fix. A number with four
// thousand digits is a satellite_number doing its job, and refusing it here
// would put a limit in the one language that promises none.
//
// WHAT IS REAL IS THE FLOOR. Every argument the machine reports is a COUNT of
// something -- bytes, cores, threads, seconds -- and a count below zero is not a
// small number, it is a reader that failed and returned -1. That is the bound
// worth checking, and it is the one that catches a real mistake.
//
// A signed argument that MAY go under zero is the signed_count arm, which takes
// the default above and is accepted whatever it holds.
template <>
inline bool value_in_bounds<satellite_number>(const satellite_number &value)
{
    return value.negative() == false;
}

// THE CLASS. The brief's shape, with the brief's protected members.
class satelliteArguments {
public:
    satelliteArguments() = default;
    virtual ~satelliteArguments() = default;

    // A23 -- THE PUBLIC READER THE INTERPRETER CALLS WHILE ACTIVE. Answers
    // nullptr for a name that was never added, which is a different thing from a
    // name that is not real: one is "not gathered on this machine", the other is
    // "not a word of this language".
    const satelliteArgumentCase *find_argument(const std::string &dotted_name) const
    {
        satelliteArgumentCase wanted(dotted_name, ArgumentValue{});
        return find_argument(wanted);
    }

    // A22 -- WALK EVERY CASE, COMPARE THE REGISTERS. The brief asks for exactly
    // this and says why: "when someone calls arguments, we don't instantly
    // access the argument, we search through every single argument until we find
    // the name that was typed in".
    //
    // A LINEAR WALK IS THE RIGHT SHAPE HERE and it is not a shortcut. There are
    // dozens of arguments, not thousands; the compare stops at the first
    // register that differs, so a miss costs one string compare and not four;
    // and a walk has no map to keep in step with the vector. If this ever holds
    // thousands, the registers are already the key a map would need.
    const satelliteArgumentCase *find_argument(const satelliteArgumentCase &wanted) const
    {
        for (const satelliteArgumentCase &argument_case : argument_cases) {
            bool same = true;
            for (std::size_t which = 0; which < kArgumentRegisters; ++which) {
                if (argument_case.register_at(which) != wanted.register_at(which)) {
                    same = false;
                    break;
                }
            }
            if (same == true) return &argument_case;
        }
        return nullptr;
    }

    bool has_argument(const std::string &dotted_name) const { return find_argument(dotted_name) != nullptr; }
    const std::vector<satelliteArgumentCase> &all_arguments() const { return argument_cases; }
    std::size_t argument_count() const { return argument_cases.size(); }
    void clear_arguments() { argument_cases.clear(); }

    // The brief's own add, made public so a gatherer outside the class can feed
    // it (R6: the machine readers become the feeders). The protected one below
    // is the brief's literal signature and both run the same two checks.
    bool add_argument(const std::string &dotted_name, ArgumentValue value_input,
                      ArgumentOrigin origin = ArgumentOrigin::word)
    {
        return add_argument(satelliteArgumentCase(dotted_name, std::move(value_input), origin));
    }

    // The same, spelled so the call site says which kind it is adding. A gatherer
    // reading config.ini or the command line writes this one and a reader of the
    // code can see, without looking anything up, that no word is being claimed.
    bool add_satl_argument(const std::string &dotted_name, ArgumentValue value_input)
    {
        return add_argument(dotted_name, std::move(value_input), ArgumentOrigin::satl);
    }

    // Every name a PROGRAM may read: the words, and not what satl filled in for
    // itself. This is what the `arguments` variable answers from once A26-A32
    // point the readers at it.
    std::vector<const satelliteArgumentCase *> arguments_a_program_can_read() const
    {
        std::vector<const satelliteArgumentCase *> readable;
        for (const satelliteArgumentCase &argument_case : argument_cases)
            if (argument_case.a_program_can_read_it())
                readable.push_back(&argument_case);
        return readable;
    }

protected:
    // A21 -- NAME CHECK, BOUNDS CHECK, THEN push_back, in the brief's order.
    //
    // A NAME ALREADY HELD IS OVERWRITTEN, NOT ADDED TWICE. Two rows for one name
    // would make find_argument's answer depend on which was pushed first, and
    // the brief's search stops at the first match -- so the second would be
    // unreachable and the value silently stale.
    bool add_argument(satelliteArgumentCase argument_case)
    {
        if (is_a_real_argument(argument_case) == false) return false;
        if (value_is_in_bounds(argument_case.value()) == false) return false;

        for (satelliteArgumentCase &held : argument_cases) {
            bool same = true;
            for (std::size_t which = 0; which < kArgumentRegisters; ++which) {
                if (held.register_at(which) != argument_case.register_at(which)) {
                    same = false;
                    break;
                }
            }
            if (same == true) {
                held.set_value(argument_case.value());
                return true;
            }
        }
        argument_cases.push_back(std::move(argument_case));
        return true;
    }

    // The variant's arm decides which value_in_bounds runs. std::visit picks the
    // template instance at compile time, one per arm, which is the brief's
    // "templates for each value type" without a switch to keep in step with the
    // variant.
    static bool value_is_in_bounds(const ArgumentValue &value)
    {
        return std::visit([](const auto &held) { return value_in_bounds(held); }, value);
    }

    // A20 -- the brief's own field, with R2's element type.
    std::vector<satelliteArgumentCase> argument_cases;
};

} // namespace satellite004
