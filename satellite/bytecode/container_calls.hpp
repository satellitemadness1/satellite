#pragma once
// satellite/bytecode/container_calls.hpp -- WHAT A LIST AND AN INDEX ANSWER.
//
// The author, 2026-09-18: *".append, .size, .sort, .contains and .contain as an
// alias, for sort we will have sort(list).by_name() (always a to z,) and
// .by_value always the smallest value first, and we will build an additional
// command -- .reverse() so you can type .sort(list).by_name().reverse() instead
// of having the old way of .sort("down") and .sort("up")"*.
//
//     names.append("zoe")
//     names.size                       -- how many
//     names.contains("zoe")            -- or .contain, the author's alias
//     names.sort().by_name()           -- A to Z, always
//     names.sort().by_value()          -- smallest first, always
//     names.sort().by_name().reverse() -- Z to A, and that is the ONLY way to get it
//
// ---------------------------------------------------------------------------
// ONE RULE DECIDES WHICH OF THESE CHANGE A NAME AND WHICH ANSWER A NEW VALUE,
// AND IT IS WORTH STATING BEFORE THE CODE:
//
//     `.append` CHANGES THE LIST. Everything else ANSWERS A NEW VALUE.
//
// ---------------------------------------------------------------------------
//
// WHY THAT SPLIT. `names.append("zoe")` written as a statement on its own has to
// mean something, and the only thing it can mean is that names grew -- a version
// that answered a longer list and left names alone would be a line that does
// nothing, which is the worst thing a language can let somebody write.
//
// `.sort()` IS THE OPPOSITE, and that falls out of the author's own spelling.
// `names.sort().by_name().reverse()` reads as a pipeline, and a pipeline whose
// FIRST step quietly rewrote the thing it was given would make
// `satellite.console.display(names.sort().by_name())` change names as a side
// effect of printing it. So `.sort()` takes a copy, and every step after it
// works on that copy. `names = names.sort().by_name()` is how a name is
// reordered, and it says so.
//
// A LIST IS A VALUE (satellite_list.hpp), so a copy here is a handle and costs
// nothing until something writes through it.
//
// ---------------------------------------------------------------------------
// `.reverse()` IS ONE COMMAND ON EVERY TYPE THAT HAS AN ORDER, which is what the
// author asked for -- *"add .reverse() anywhere we can reverse something,
// strings, numbers, binary numbers hex"*:
//
//     {1, 2, 3}.reverse()   {3, 2, 1}        the items, back to front
//     "abc".reverse()       "cba"            the characters, by CHARACTER and
//                                            never by byte -- satellite_string
//                                            is 16/32-bit, so a reversed string
//                                            is still the same letters
//     123.reverse()         321              the digits
//     b1010.reverse()       b0101            the bits, width kept
//
// A NUMBER'S SIGN DOES NOT MOVE: -123 reverses to -321, because the minus is not
// a digit. And a number ENDING IN ZERO loses it -- 120 reverses to 21, not 021 --
// which is not a bug to fix but arithmetic: 021 IS 21, and a number type that
// remembered a leading zero would be a string pretending to be a number.
//
// THE FLOAT RULE IS RECORDED AND CANNOT BE BUILT YET. The author asked for
// something specific and unusual: *"float reverse exchanges the decimal numbers
// for the whole numbers, and float_object.reverse().reverse() will be the only
// command you can reverse TWICE, it shall switch the numbers, AND reverse the
// numbers if you reverse it twice"*. **004 HAS NO FLOAT TYPE AT ALL** -- there is
// no arm for it (satellite_object.hpp lists it as still to come) and
// `to_string` says so in as many words. So the rule is written down in
// PLAN.md's red notes with the author's sentence, to be built with the type, and
// nothing here pretends to implement it.

#include "expression.hpp"

#include <vector>

namespace satellite004 {

// A LIST'S OR AN INDEX'S METHOD. `home` is the variable's own object when the
// receiver came straight from a name, and nullptr otherwise -- `.append` needs
// it, because a list is a value and appending to a copy changes nothing.
//
// IT MUST BE THE VARIABLE'S OWN OBJECT AND NEVER A COPY OF IT. Copy-on-write
// asks `use_count() == 1`, and a copy on the way here makes that answer no every
// time, so every append would copy the whole list -- the quadratic append 003
// shipped for months. satellite_list.hpp tells that story.
Value call_container_method(token::Code method, Value &receiver, Value *home,
                            const std::vector<Value> &arguments, bool had_parentheses,
                            const std::string &name, ExpressionContext &context);

// `.reverse()` ON THE TYPES THAT ARE NOT CONTAINERS: a string, a number, a
// binary. Answers `handled` false when this kind has no reverse, so the caller
// refuses in its own words.
Value reverse_of(const Value &receiver, bool &handled, std::string &why);

} // namespace satellite004
