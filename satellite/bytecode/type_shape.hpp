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

inline constexpr std::size_t kNoSuit = static_cast<std::size_t>(-1);

struct TypeShape {
    token::Code word = 0;                  // satellite.variable.string, satellite.container.list, ...
    std::vector<TypeShape> parameters;     // what is between its < and >

    // A SPACESUIT'S NAME IS A TYPE TOO (2026-09-22): `tagged_report.run_log log`,
    // `satellite.container.list<energy_flight.satellite_flight>`. A user's type has no
    // word, so `word` is satellite.spacesuit's own code and the name is kept AS WRITTEN;
    // `suit` is the spacesuit it reached, which only the scope table can say --
    // capsules_in resolves every header's, and the checker and the walker a body's.
    std::vector<std::string> suit_names;   // `run_log`, or `tagged_report.run_log`
    std::size_t suit = kNoSuit;            // its scope in the CapsuleTable, once resolved

    bool is_a_suit() const { return !suit_names.empty(); }
};

// A WORD WITH NOTHING BETWEEN < AND > -- `satellite.variable.number`.
inline TypeShape plain_shape(token::Code word)
{
    TypeShape shape;
    shape.word = word;
    return shape;
}

// A SPACESUIT'S NAME AS WRITTEN, joined -- for a sentence about it.
inline std::string suit_written(const TypeShape &shape)
{
    std::string written;
    for (const std::string &each : shape.suit_names) written += (written.empty() ? "" : ".") + each;
    return written;
}

// WHAT A SHAPE WAS WRITTEN AS, for a sentence: the word, or the spacesuit's name --
// never "satellite.spacesuit", which is the word standing in for every one of them.
inline std::string shape_written(const TypeShape &shape)
{
    return shape.is_a_suit() ? suit_written(shape) : std::string(word::spelling_of(shape.word));
}

// satellite.container.map (1 4 1) IS satellite.container.index (1 4 5), since
// 2026-09-26. The author's word for it has always been map -- M14 names it, his
// satellite.variable.info/README.md draws one, and the first "crazy combination" he
// wrote to try containers declares a list<list<map<...>>> -- while 004 built the
// container and called it index. Map is 003's numbered row and cannot become a
// second spelling in aliases.tsv (a spelling that is already a word is refused
// there), so it stays its own code and is read as the same container here, by
// every test that asks. A refusal names the word as it was written.
inline bool is_an_index_word(token::Code word)
{
    return word == word::code_of(1, 4, 5) || word == word::code_of(1, 4, 1);
}

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
    if (word == word::code_of(1, 6, 17)) return satelliteObject::infinity;   // and every level above (INF-2)
    if (word == word::code_of(1, 4, 2)) return satelliteObject::list;
    if (is_an_index_word(word)) return satelliteObject::index;               // and .map
    if (word == word::code_of(1, 6, 18)) return satelliteObject::window;   // and a button (WIN-3)
    if (word == word::code_of(1, 6, 10)) return satelliteObject::floating;     // and .double (2026-09-22)
    if (word == word::code_of(1, 6, 11)) return satelliteObject::hexadecimal;  // and .hexadecimal
    if (word == word::code_of(1, 6, 19)) return satelliteObject::color;        // and .colour
    if (word == word::code_of(1, 6, 20)) return satelliteObject::fraction;
    if (word == word::code_of(1, 6, 13)) return satelliteObject::thread;       // 2026-09-23 (thread_calls.hpp)
    if (word == word::code_of(1, 6, 21)) return satelliteObject::index;       // the arguments (main_arguments.hpp)
    if (word == word::code_of(1, 6, 22)) return satelliteObject::list;        // an info: a list of indexes (info_calls.hpp)
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
    if (is_an_index_word(word)) return 2;            // index<key, value>, map<key, value>
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

// THE ARM OF A `multiple` THAT A VALUE IS HELD AS, or `shape` itself when it is not
// a multiple (2026-09-26). A write into `x[1][2]` walks the declared shape down beside
// the value, and a multiple on the way used to end the walk: its arms describe the
// NAME, not an item, so everything under it was taken as "anything" and
// `list<multiple<string, list<number>>> x = {{1}}` then `x[1][1] = satellite.bool.true`
// went in -- a value the declaration refuses, accepted one item at a time
// (utility/check_container_shapes.py found 108 such). The walk goes on through the
// arm the value really is: the one of its kind.
//
// TWO ARMS OF ONE KIND -- multiple<list<number>, list<string>> -- ARE NOT CHOSEN BETWEEN,
// and the multiple itself comes back, so below it is unchecked as it always was. Choosing
// "the first the value fits now" was tried and refused what the declaration allows (an
// empty list fits both, so e.append("x") was held to list<number>), and asking whether the
// value fits some arm AFTER each write walks the whole value every time -- 20,000 appends
// took 7.8 s where a list<number> takes 0.07 (the review, 2026-09-26). Checking that one
// case right needs a write that can be taken back; it is written down in CONTAINERS.md.
const TypeShape &arm_holding(const TypeShape &shape, const satelliteObject &value);

// READ `satellite.container.index<a, b>` OFF THE TOKENS, `at` on the word.
// Leaves `at` on whatever follows -- the name, in a declaration.
//
// `pending_closes` MUST START AT 0 and is the `>>` fix: a caller passes it in,
// and a non-zero value coming back would mean a `>` was found with nothing left
// to close. Both defined in type_shape.cpp.
bool read_type_shape(const std::vector<std::bitset<16>> &row, std::size_t &at, TypeShape &out,
                     unsigned int &pending_closes, std::string &why);

} // namespace satellite004
