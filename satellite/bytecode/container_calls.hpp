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

// HOW MANY ARGUMENTS EACH CONTAINER METHOD TAKES, AND WHICH METHODS THERE ARE.
// -1 means "not a container method at all".
//
// IT IS IN THE HEADER SO THE CHECKER AND THE WALKER READ THE SAME LIST. The
// checker kept its own hand-written set of container methods and it went stale
// the moment this file grew: `n.first` was refused before the program ran as
// "not built for satellite.container.list yet" while container_calls.cpp had
// implemented it. That is the third time in this session a second hand-kept list
// has contradicted the first (the two feedback spellings, then the checker's
// type words), so this one is asked rather than copied.
//
// It mirrors file_method_arity deliberately: a method spelled the same on a file
// and on a list must take the same arguments, or a person has to remember which
// receiver they are holding.
inline int container_arity(token::Code method)
{
    switch (method) {
    case token::insert_token: return 2;
    case token::append_token:
    case token::contains_token:
    case token::index_of_token:
    case token::search_token:
    case token::remove_token:
    case token::remove_at_token:
    case token::truncate_token: return 1;
    case token::size_token:
    case token::empty_token:
    case token::first_token:
    case token::last_token:
    case token::clear_token:
    case token::remove_first_token:
    case token::remove_last_token:
    case token::keys_token:
    case token::values_token:
    case token::sort_token:
    case token::by_name_token:
    case token::by_value_token:
    case token::reverse_token: return 0;
    default: return -1;
    }
}

// THE ONES THAT CHANGE THE CONTAINER. Everything else answers a new value, and
// the header above says why the line is drawn here: a statement that is nothing
// but `names.clear` has to mean the list emptied, or it is a line that does
// nothing.
//
// IN THE HEADER because one_operand reads it too: a chain ending in one of these
// is walked a SECOND time, by reference, so `grid[1].append(3)` reaches the real
// item instead of a copy of it. Reading is left on the copy, which is what stops
// a read from cloning a shared list.
inline bool changes_a_container(token::Code method)
{
    switch (method) {
    case token::append_token:
    case token::clear_token:
    case token::insert_token:
    case token::remove_token:
    case token::remove_at_token:
    case token::remove_first_token:
    case token::remove_last_token:
    case token::truncate_token: return true;
    default: return false;
    }
}

// A LIST'S OR AN INDEX'S METHOD. `home` is the variable's own object when the
// receiver came straight from a name, and nullptr otherwise -- `.append` needs
// it, because a list is a value and appending to a copy changes nothing.
//
// IT MUST BE THE VARIABLE'S OWN OBJECT AND NEVER A COPY OF IT. Copy-on-write
// asks `use_count() == 1`, and a copy on the way here makes that answer no every
// time, so every append would copy the whole list -- the quadratic append 003
// shipped for months. satellite_list.hpp tells that story.
// `shape` IS WHAT THE NAME WAS DECLARED AS, or nullptr when the receiver has no
// declaration to answer to. It is here because a type between < and > that is
// enforced on `a[1] = x` and NOT on `a.append(x)` is not a type, it is a
// decoration -- and that is exactly what shipped for an hour:
// `satellite.container.list<satellite.variable.number> ns` took ns.append("zoe")
// and refused ns[1] = "zoe", which is the same wrong value arriving by two doors
// with only one of them locked.
Value call_container_method(token::Code method, Value &receiver, Value *home, const TypeShape *shape,
                            const std::vector<Value> &arguments, bool had_parentheses,
                            const std::string &name, ExpressionContext &context);

// `.reverse()` ON THE TYPES THAT ARE NOT CONTAINERS: a string, a number, a
// binary. Answers `handled` false when this kind has no reverse, so the caller
// refuses in its own words.
Value reverse_of(const Value &receiver, bool &handled, std::string &why);

} // namespace satellite004
