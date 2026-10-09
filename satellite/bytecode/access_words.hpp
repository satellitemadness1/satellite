#pragma once
// satellite/bytecode/access_words.hpp -- WHAT satellite.access SAYS A SHAPE IS, in words and
// as a value a program would write: "a list of maps (string -> list of numbers)", {"key": {1}}.
// Split from access_calls.cpp (the line rule); access_calls.hpp says what access shows.

#include "type_shape.hpp"
#include "value.hpp"

#include <string>

namespace satellite004 {
namespace access_words {

// an info is reached as the list of maps it is (info_calls.hpp)
const TypeShape &as_reached(const TypeShape &shape);
inline bool a_list(const TypeShape &shape) { return shape.word == word::code_of(1, 4, 2); }
inline bool a_multiple(const TypeShape &shape) { return shape.word == word::code_of(1, 4, 6); }
bool reached_into(const TypeShape &shape);                  // a level under it can be reached

std::string last_part(token::Code word);                     // number, string, map, index
std::string noun(const TypeShape &declared, bool many);      // "list of maps (string -> number)"
std::string short_noun(const TypeShape &declared, bool many);  // "list of maps"
std::string with_article(const std::string &said);
const TypeShape &example_arm(const TypeShape &shape);        // the type a multiple's fill line puts in
std::string example_of(const TypeShape &declared);           // {"key": {1}}, or "" when none is written
std::string position_letter(std::size_t level);              // n, m, p, q ...
std::string key_placeholder(const TypeShape &key, std::size_t level);   // "key", "key2" ... or key
std::string value_shown(const Value &value);                 // what display shows, quoted and cut short

} // namespace access_words
} // namespace satellite004
