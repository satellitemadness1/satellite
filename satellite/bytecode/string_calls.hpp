#pragma once
// satellite/bytecode/string_calls.hpp -- WHAT A STRING ANSWERS (M16).
//
// MILESTONES M16: "The list is already frozen -- 23 methods at `1 6 1 n` -- so none of
// this is a design question." These are the fifteen it names, less upper and lower
// (built 2026-09-25 in a branch of expression.cpp's own, and 1 6 1 8 and 9 since
// 2026-10-10), plus `find` once its receiver is a string, so that one ruling covers every
// method that takes text:
//
//     s.size   s.empty                    how many characters; whether there are none
//     s.contains(x)  s.starts_with(x)  s.ends_with(x)   true or false
//     s.find(x)                            where x begins, counting from 0 (as it always has)
//     s.at(n)   s[n]                       character n, counting from 1, as a string
//     s.substring(start, end)              characters start to end, both from 1, both kept
//     s.split(separator)                   a satellite.container.list of strings
//     s.replace(a, b)  s.trim  s.resolved  a new string; s keeps its own
//     s.append(x)  s.clear                 CHANGE s, as a list's .append changes the list
//
// POSITIONS COUNT FROM 1, as a list's items and a file's lines do (expression.cpp's
// position_of: "one set of rules for one bracket"). So s[1] is the first character and
// s[s.size] the last, and substring's end is a character too: "hello".substring(2, 4) is
// ell. 003 counted from 0 with the end left out; the same piece is 003's
// substring(start - 1, end), so the END number is the one both write. An empty piece is
// substring(n + 1, n), which is 003's substring(n, n), and anything further backwards is
// refused as 003 refused it (S412).
//
// A NUMBER WHERE TEXT IS EXPECTED IS ITS DIGITS, and satellite.log says so -- the author,
// 2026-09-16: "just convert the number to the string and run that piece, obviously the
// programmer meant convert to string, but record the warning in satellite.log" (S020,
// warn_number_taken_as_text). Anything else where text goes is refused (S301).
//
// A CHARACTER IS A CHARACTER, NEVER A BYTE: "héllo".size is 5 and "日本語"[3] is 語
// (satellite_string.hpp). The loops are string_pieces.hpp's.
//
// EVERY ONE OF THEM RUNS BY ITS NUMBER (the author, 2026-10-10: "we have to give numbers to
// things that don't have numbers yet, like .find() and .replace and .append() ... and it actually
// runs that number when we do it"). A string's method is the word satellite.variable.string.<it>,
// 1 6 1 n in words/words.tsv; its code is a file under satellite-numbers/words/ named by that
// number, and run_string_number calls it through the function table. The ones a string answered
// before they had a number -- .reverse(), .foreground(c), .background(c) and .add(x) -- took the
// next free numbers, 1 6 1 23 to 26, the same day. Where a method had two copies (the 32-bit
// libraries ported from 003, and this walker's), the author kept the one programs ran: this one.

#include "expression.hpp"
#include "token_codes.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

// HOW MANY ARGUMENTS FOURTEEN OF A STRING'S METHODS TAKE -- the ones M16 built -- and -1 for
// any other method. In the header so the checker and the walker read one list, as
// container_arity is (container_calls.hpp says what a second hand-kept list cost). -1 does
// not mean a string lacks it: upper, lower, the conversions, reverse, foreground, background
// and add say their own counts in their own files, in the words they said before they had a
// number. string_number_of is what says whether a string has a method.
int string_method_arity(token::Code method);

// THE NUMBER A STRING'S METHOD RUNS -- the code of satellite.variable.string.<it>, 1 6 1 n --
// or 0 for a method a string does not have.
token::Code string_number_of(token::Code method);

// append and clear CHANGE the string they are called on; every other one answers a new
// value. A literal has no name to change, so `"a".append("b")` is refused, as
// `{1}.append(2)` is.
bool changes_a_string(token::Code method);

// THE CHECKER'S HALF (string_check.cpp): `name.method(...)` on a name declared
// satellite.variable.string, or on a string literal, judged before anything runs. `open` is
// the code after the method (a `(` or not); `bracketed` and `given` are what
// program_check.cpp's brackets_at found there. A literal that can never be right is refused
// here too, so the lines above it never print first: anything but a whole number where a
// position goes, a first position of 0, a binary, hex or percentage where text goes, and ""
// as the text replace looks for -- and a `[` after the chain's answer, which is not built.
signed long long int string_method_check(const std::vector<std::bitset<16>> &row, std::size_t open,
                                         bool bracketed, std::size_t given, token::Code method,
                                         const std::string &spelling, std::string &why);

// AND ONE ARGUMENT OF ANOTHER TYPE'S METHOD JUDGED AS TEXT (the checker gaps of 2026-10-19, OPEN_ITEMS_2026-10-20):
// a file's append, index_of, search, contains and remove read text in their first argument, and insert and
// replace in their second (file_calls.cpp's text_of), so a lone binary, hex, percentage or time literal at
// `slot` is refused before the run, in the walker's sentence. `open` is the method's bracket and `wanted` its
// count. success otherwise -- a float or a bool there too, for this file's reason above.
signed long long int text_literal_refused(const std::vector<std::bitset<16>> &row, std::size_t open, int wanted,
                                          int slot, const std::string &spelling, std::string &why);

// `s[...]` ON A NAME DECLARED A STRING, judged before anything runs: a lone literal in the
// brackets that is not a whole number, or is 0, refused as s[n] refuses it. `open` is the `[`.
signed long long int string_index_check(const std::vector<std::bitset<16>> &row, std::size_t open,
                                        const std::string &name, std::string &why);

// THE WALKER'S HALF: the method's number, run. `number` is string_number_of(method), which the
// walker has already asked (so it is never 0 here). `home` is the variable's own value when the
// chain is still on a name, and nullptr otherwise: append and clear need it, everything else
// ignores it. A count string_method_arity knows is refused here, before the number runs -- and
// that is what lets each number's function read the arguments its word names without asking
// how many there are.
Value run_string_number(token::Code method, token::Code number, const Value &receiver, Value *home,
                        const std::vector<Value> &arguments, bool had_parentheses, const std::string &name,
                        ExpressionContext &context);

// THE PIECES THE NUMBERED METHODS SHARE (string_calls.cpp), in a namespace of their own so a
// name like one_character cannot meet the lexer's.
namespace string_method_parts {

// TEXT GOING IN: the string itself, or a number's digits with S020 in satellite.log. `made`
// holds the digits when there are any, so a string argument is never copied. nullptr, refused.
const satellite_string *text_argument(const Value &value, satellite_string &made, const std::string &what,
                                      ExpressionContext &context);

// A POSITION GOING IN, counting from 1, and `written` as the program wrote it.
bool position_argument(const Value &value, unsigned long long int &out, std::string &written,
                       const std::string &what, ExpressionContext &context);

// "the string is empty" or "it holds 5 characters, counting from 1".
std::string what_it_holds(const satellite_string &text);

// ONE CHARACTER, for s.at(n) and s[n]: `open` and `close` spell the call as it was written.
Value one_character(const satellite_string &text, const Value &index, const std::string &open,
                    const std::string &close, std::size_t where, bool placed, ExpressionContext &context);

} // namespace string_method_parts

// `s[n]` -- ONE CHARACTER, READ. The same answer and the same refusals as s.at(n), with
// the bracket's own spelling in them. `where` is the `[` for the caret.
Value character_of(const Value &text, const Value &index, const std::string &what, std::size_t where,
                   ExpressionContext &context);

} // namespace satellite004
