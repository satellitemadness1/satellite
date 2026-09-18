#pragma once
// satellite/bytecode/type_shape.hpp -- WHAT A DECLARATION SAYS A NAME MAY HOLD.
//
// The author, 2026-09-18: *"we need to build it so we can use any container with
// any container, so we can do satellite.container.multiple<type1, type2> and
// type2 is a list, and any possible combination"*.
//
//     satellite.container.list<satellite.variable.string> names
//     satellite.container.index<satellite.variable.string, satellite.variable.number> scores
//     satellite.container.multiple<satellite.variable.string, satellite.container.list> either
//     satellite.container.list<satellite.container.list<satellite.variable.number>> grid
//
// A SHAPE IS A WORD AND ITS PARAMETERS, AND A PARAMETER IS A SHAPE. That one
// sentence is what "any possible combination" needs -- a flat list of allowed
// words would do `multiple<string, list>` and stop at `list<list<number>>`,
// which is the combination a person reaches for first.
//
// ---------------------------------------------------------------------------
// `multiple` IS NOT A CONTAINER, AND THIS FILE IS WHERE THAT SHOWS.
// ---------------------------------------------------------------------------
//
// The author: *"multiple is a std::variant"*. It is -- and satelliteObject IS
// that variant already, over every type the language has. So `multiple<A, B>`
// adds no arm and no storage: it is a name that accepts two shapes instead of
// one, checked here when something is assigned. The value inside is an ordinary
// string or an ordinary list, indistinguishable from one held by a name declared
// the plain way.
//
// ---------------------------------------------------------------------------
// `>>` IS ONE TOKEN, and that is C++'s oldest parsing wart, inherited here.
// ---------------------------------------------------------------------------
//
// `list<list<number>>` ends in a `>>`, which the lexer has already made into
// shift_right_token (token_codes.hpp 0x0608) because `>>` is a shift everywhere
// else. C++ lived with `> >` until C++11 and then taught its parser to split the
// token; this does the same, with `pending_closes`. It is not worth making a
// person type a space to close two brackets.

#include "word_codes.hpp"
#include "token_codes.hpp"
#include "../satellite_object/satellite_object.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

struct TypeShape {
    token::Code word = 0;                  // satellite.variable.string, satellite.container.list, ...
    std::vector<TypeShape> parameters;     // what is between its < and >
};

// EVERY WORD THAT NAMES A TYPE, and the arm it means. One function, so the
// checker and the walker cannot come to disagree about what a word accepts --
// which is the same reason index_into and write_through_index share position_of.
inline satelliteObject::Kind kind_of_type_word(token::Code word)
{
    if (word == word::code_of(1, 6, 4)) return satelliteObject::number;
    if (word == word::code_of(1, 6, 1)) return satelliteObject::string;
    if (word == word::code_of(1, 6, 5)) return satelliteObject::binary;
    if (word == word::code_of(1, 6, 16)) return satelliteObject::percentage;
    if (word == word::code_of(1, 6, 2)) return satelliteObject::file;
    if (word == word::code_of(1, 6, 6)) return satelliteObject::boolean;
    if (word == word::code_of(1, 4, 2)) return satelliteObject::list;
    if (word == word::code_of(1, 4, 5)) return satelliteObject::index;
    return satelliteObject::how_many_kinds;          // not a type word
}

inline bool is_a_type_word(token::Code word)
{
    return kind_of_type_word(word) != satelliteObject::how_many_kinds ||
           word == word::code_of(1, 4, 6);           // multiple, which is every type at once
}

// HOW MANY PARAMETERS A WORD TAKES. -1 means "any number", which is what
// `multiple` is: the whole point of it is that the author picks how many.
inline int parameters_wanted(token::Code word)
{
    if (word == word::code_of(1, 4, 2)) return 1;    // list<of what>
    if (word == word::code_of(1, 4, 5)) return 2;    // index<key, value>
    if (word == word::code_of(1, 4, 6)) return -1;   // multiple<a, b, c, ...>
    return 0;                                        // a plain type takes none
}

// ---------------------------------------------------------------------------
// DOES THIS VALUE FIT THIS SHAPE?
// ---------------------------------------------------------------------------
//
// AN EMPTY PARAMETER LIST MEANS "ANY", and that is deliberate rather than lax:
// `satellite.container.list names = {1, "two"}` is a list of whatever, which is
// what the braced literal already makes, and a person who wants it checked says
// so by writing `<satellite.variable.number>`. Requiring the parameter would
// have made every list declaration longer to say nothing new.
bool value_fits(const TypeShape &shape, const satelliteObject &value, std::string &why);

inline bool any_of_fits(const std::vector<TypeShape> &shapes, const satelliteObject &value, std::string &why)
{
    for (const TypeShape &one : shapes) {
        std::string ignored;
        if (value_fits(one, value, ignored))
            return true;
    }
    std::string names;
    for (std::size_t at = 0; at < shapes.size(); ++at) {
        if (at != 0) names += at + 1 == shapes.size() ? " or " : ", ";
        names += word::spelling_of(shapes[at].word);
    }
    why = "it holds " + std::string(value.kind_name()) + ", and this name takes " + names;
    return false;
}

// READ `satellite.container.index<a, b>` OFF THE TOKENS, `at` on the word.
// Leaves `at` on whatever follows -- the name, in a declaration.
//
// `pending_closes` MUST START AT 0 and is the `>>` fix: a caller passes it in,
// and a non-zero value coming back would mean a `>` was found with nothing left
// to close. Both defined in type_shape.cpp.
bool read_type_shape(const std::vector<std::bitset<16>> &row, std::size_t &at, TypeShape &out,
                     unsigned int &pending_closes, std::string &why);

} // namespace satellite004
