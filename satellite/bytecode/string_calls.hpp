#pragma once
// satellite/bytecode/string_calls.hpp -- WHAT A STRING ANSWERS (M16).
//
// MILESTONES M16: "The list is already frozen -- 23 methods at `1 6 1 n` -- so none of
// this is a design question." These are the fifteen it names, less upper and lower
// (built 2026-09-25, expression.cpp's own branch), plus `find` once its receiver is a
// string, so that one ruling covers every method that takes text:
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

#include "expression.hpp"
#include "token_codes.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

// HOW MANY ARGUMENTS EACH OF A STRING'S METHODS TAKES, and -1 for a method a string does
// not have. In the header so the checker and the walker read one list, as
// container_arity is (container_calls.hpp says what a second hand-kept list cost).
// upper and lower are not here: they have their own branch, older than this file.
int string_method_arity(token::Code method);

// append and clear CHANGE the string they are called on; every other one answers a new
// value. A literal has no name to change, so `"a".append("b")` is refused, as
// `{1}.append(2)` is.
bool changes_a_string(token::Code method);

// THE CHECKER'S HALF: `name.method(...)` on a name declared satellite.variable.string,
// judged before anything runs. `open` is the code after the method (a `(` or not);
// `bracketed` and `given` are what program_check.cpp's brackets_at found there. A
// literal that can never be right is refused here too -- text where a position goes, a
// binary, hex or percentage where text goes -- so the lines above it never print first.
signed long long int string_method_check(const std::vector<std::bitset<16>> &row, std::size_t open,
                                         bool bracketed, std::size_t given, token::Code method,
                                         const std::string &spelling, std::string &why);

// THE WALKER'S HALF. `home` is the variable's own value when the chain is still on a
// name, and nullptr otherwise: append and clear need it, everything else ignores it.
Value call_string_method(token::Code method, const Value &receiver, Value *home, const std::vector<Value> &arguments,
                         bool had_parentheses, const std::string &name, ExpressionContext &context);

// `s[n]` -- ONE CHARACTER, READ. The same answer and the same refusals as s.at(n), with
// the bracket's own spelling in them. `where` is the `[` for the caret.
Value character_of(const Value &text, const Value &index, const std::string &what, std::size_t where,
                   ExpressionContext &context);

} // namespace satellite004
