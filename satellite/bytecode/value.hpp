#pragma once
// satellite/bytecode/value.hpp -- what one expression is worth, and where a
// capsule's variables live.
//
// THIS IS THE VALUE TYPE PROGRESS §6.5 SAID WAS OWED, and it was owed as the
// third of three things: "`display(42)` still has nowhere to put its argument.
// `Call`'s kind/text/count/flag wants to become one type over satellite_number
// and satellite_string. This is the real next piece."
//
// WHAT CHANGED FROM THE PLACEHOLDER. The old Value had a `count`, an
// `unsigned long long int`, because a library scenario takes one. That made the
// interpreter's idea of a number 64 bits wide while satellite.variable.number
// had no width at all, and it is why `display(99999999999999999999999)` answered
// "a number too large" (check.sh) in a language whose whole number type cannot
// BE too large. The count is gone; `number` is a satellite_number, and the
// narrowing to an unsigned long long happens at the one place it must -- the
// call into a library -- and nowhere else.
//
// A MACHINE CODE IS A NUMBER TOO. A word answers its machine code as a value, so
// display(display("x")) prints x and then prints 0 -- that is unchanged, it is
// just carried in a satellite_number now, built by from_signed().
//
// THERE ARE NO GLOBALS (the author, 2026-09-16): "we begin exe inside of main,
// and end exe inside of main... the only globals are the includes, other files".
// So a VariableTable belongs to ONE running body and is created by run_body when
// that body starts. A capsule cannot see its caller's variables, because it is
// handed a different table -- and that is the rule being enforced by
// construction rather than by a check that could be forgotten.
//
// A VARIABLE REMEMBERS THE WORD THAT DECLARED IT. `satellite.variable.number n`
// stores the code of satellite.variable.number beside the value, so a later
// `n = "text"` is refused with types_do_not_meet (27) instead of quietly making
// n a string. satellite is typed by its declaration, and this is where that
// survives past the line that wrote it.

#include "token_codes.hpp"
#include "../satellite_variable_number/satellite_number.hpp"

#include <string>
#include <unordered_map>

namespace satellite004 {

struct Value {
    enum class Kind { nothing, text, flag, number };

    Kind kind = Kind::nothing;
    std::string text;
    satellite_number number;
    bool flag = false;

    static Value of_text(std::string from)
    {
        Value value;
        value.kind = Kind::text;
        value.text = std::move(from);
        return value;
    }
    static Value of_number(satellite_number from)
    {
        Value value;
        value.kind = Kind::number;
        value.number = std::move(from);
        return value;
    }
    static Value of_flag(bool from)
    {
        Value value;
        value.kind = Kind::flag;
        value.flag = from;
        return value;
    }
    // A machine code, as the value a word's call is worth.
    static Value of_code(signed long long int code) { return of_number(satellite_number::from_signed(code)); }

    // The name of the kind, for a refusal a person has to act on.
    const char *kind_name() const
    {
        switch (kind) {
        case Kind::text: return "a string";
        case Kind::flag: return "a bool";
        case Kind::number: return "a number";
        case Kind::nothing: break;
        }
        return "nothing";
    }
};

struct Variable {
    token::Code declared = 0;   // the word code of satellite.variable.number, .string, ...
    Value value;
};

// One running body's variables. Created per body, never shared: see the header.
using VariableTable = std::unordered_map<std::string, Variable>;

} // namespace satellite004
