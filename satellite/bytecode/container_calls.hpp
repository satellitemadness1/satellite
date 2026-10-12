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

#include <string>
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
    case token::insert_token:
    case token::set_token: return 2;           // a map's .set(k, v), a token since 2026-10-11
    case token::get_token:                     // a map's .get(k), the same day
    case token::append_token:
    case token::join_token:
    case token::reserve_token:
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
    case token::sum_token:
    case token::max_token:
    case token::min_token:
    case token::reverse_token: return 0;
    default: return -1;
    }
}

// `.sort` ALONE TAKES NOTHING OR ONE THING. The author, 2026-09-26: *"let's have .sort accept a
// string or a string object, so we type sort("key" or "value")"*, and *".sort() which is just an
// alias for sort by key"*. On a map `.sort()` is `.sort("key")`, and "key" or "value" says which
// half the entries are put in order by (satellite_map_sort.hpp); on a list it takes nothing, and
// .by_name() or .by_value() after it does the ordering. container_arity says 0 for it, which is
// what lets `m.sort` be written without brackets.
inline bool container_given_fits(token::Code method, std::size_t given)
{
    return given == static_cast<std::size_t>(container_arity(method)) || (method == token::sort_token && given == 1);
}

// "takes 1 argument, and was given 2" -- or .sort's own sentence, which is not a count, and a map's .get's
// and .set's, which said what each takes when they were matched by name (expression.cpp, 2026-09-26).
inline std::string container_takes(token::Code method, int wanted)
{
    if (method == token::sort_token)
        return " takes nothing, or \"key\" or \"value\" on a map";
    if (method == token::get_token)
        return "(key) takes one key, and reads the value under it";
    if (method == token::set_token)
        return "(key, value) takes a key and a value, and files one under the other";
    return " takes " + std::to_string(wanted) + (wanted == 1 ? " argument" : " arguments");
}

// WHAT `.sort(word)` SAYS TO A WORD IT DOES NOT TAKE, or "" for "key" and "value" on a map. One
// sentence for the checker, which sees a word written in quotes, and the walker, which sees a
// string however it was made.
std::string sort_word_refused(const std::string &word, bool on_a_map, const std::string &name);

// THE ONES THAT CHANGE THE CONTAINER. Everything else answers a new value, and
// the header above says why the line is drawn here: a statement that is nothing
// but `names.clear` has to mean the list emptied, or it is a line that does
// nothing.
//
// `.reserve(n)` IS ONE OF THEM THOUGH NO ITEM MOVES: the room it makes belongs to
// the list it was made in, so it has to reach the variable's own -- room made in
// a copy is room nothing will ever append into.
//
// IN THE HEADER because one_operand reads it too: a chain ending in one of these
// is walked a SECOND time, by reference, so `grid[1].append(3)` reaches the real
// item instead of a copy of it. Reading is left on the copy, which is what stops
// a read from cloning a shared list.
//
// A MAP'S .set(k, v) IS ONE OF THEM (a token since 2026-10-11): it files a value, so `rows[1].satellite.set(k, v)`
// reaches the real map in the list as `rows[1].satellite.clear()` does -- matched by name until then, it was a
// word of a name's own map and nothing else's.
inline bool changes_a_container(token::Code method)
{
    switch (method) {
    case token::append_token:
    case token::set_token:
    case token::clear_token:
    case token::insert_token:
    case token::remove_token:
    case token::remove_at_token:
    case token::remove_first_token:
    case token::remove_last_token:
    case token::reserve_token:
    case token::truncate_token: return true;
    default: return false;
    }
}

// THE NUMBER A LIST'S METHOD RUNS -- the code of satellite.container.list.<it>, 1 4 2 n (the
// author, 2026-10-10: "we have to give numbers to things that don't have numbers yet ... and it
// actually runs that number when we do it") -- or 0 for a method a list does not have. Every
// container method but .keys and .values is a list's.
token::Code list_number_of(token::Code method);

// THE NUMBER A MAP'S METHOD RUNS -- the code of satellite.container.map.<it>, 1 4 1 n (the author,
// 2026-10-11: "we still need to get the word satellite onto everything ... everything builtin needs the word
// satellite still") -- or 0 for a method a map does not have. ONE VALUE KIND UNDER FOUR TYPE WORDS: a map is
// an index (type_shape.hpp), and so is what satellite.container.index declares, the arguments variable and an
// info's every item -- a value does not know which word declared it, so these numbers are every one's, and
// none of the four has rows of its own. .search(pattern) keeps its number, 1 4 1 10, with no code: a map has
// no positions to search along, and is refused as before.
token::Code map_number_of(token::Code method);

// WHAT A LIST AND A MAP HAVE, for every refusal that lists them: both kinds' own words are under satellite
// (NM-8, a list's 2026-10-10 and a map's 2026-10-11).
const char *what_containers_have();

// A LIST'S METHOD, RUN BY ITS NUMBER: the count and the brackets refused as they always were, then
// the function its file under satellite-numbers/words/ put in the number's row. `number` is
// list_number_of(method), already asked, never 0. `home` is the variable's own object when the
// receiver came straight from a name, and nullptr otherwise. IT MUST BE THE VARIABLE'S OWN OBJECT
// AND NEVER A COPY OF IT: copy-on-write asks `use_count() == 1`, and a copy on the way here makes
// that answer no every time, so every append would copy the whole list -- the quadratic append
// 003 shipped for months (satellite_list.hpp tells that story).
//
// `shape` IS WHAT THE NAME WAS DECLARED AS, or nullptr when the receiver has no
// declaration to answer to. It is here because a type between < and > that is
// enforced on `a[1] = x` and NOT on `a.append(x)` is not a type, it is a
// decoration -- and that is exactly what shipped for an hour:
// `satellite.container.list<satellite.variable.number> ns` took ns.append("zoe")
// and refused ns[1] = "zoe", which is the same wrong value arriving by two doors
// with only one of them locked. .append and .insert ask it (1.4.2.1 and 1.4.2.20). No method of
// an index puts a value in -- a value goes into an index by `m[key] = x`, which checks the
// <type> itself -- so call_container_method no longer takes it.
Value run_list_number(token::Code method, token::Code number, const Value &receiver, Value *home,
                      const TypeShape *shape, const std::vector<Value> &arguments, bool had_parentheses,
                      const std::string &name, ExpressionContext &context);

// A MAP'S METHOD, RUN BY ITS NUMBER: the count and the brackets refused as call_container_method refused
// them, then the function its file under satellite-numbers/words/ put in the number's row (method_call).
// `number` is map_number_of(method), already asked, never 0. `home` and `shape` as run_list_number's: the
// variable's own object, never a copy -- .clear, .remove and .set change it in place -- and what the name
// was declared as, which .set's value must fit.
Value run_map_number(token::Code method, token::Code number, const Value &receiver, Value *home,
                     const TypeShape *shape, const std::vector<Value> &arguments, bool had_parentheses,
                     const std::string &name, ExpressionContext &context);

// THE PIECES A LIST'S NUMBERED METHODS SHARE WITH WHAT IS LEFT IN container_calls.cpp, in a
// namespace of their own.
struct satelliteList;   // satellite_object/satellite_list.hpp

namespace container_parts {

// A count as a number.
Value a_count(std::size_t n);

// THE ITEMS OF WHATEVER THIS IS: a list's own, or an index's keys copied into `borrowed`.
const std::vector<satelliteObject> *items_of(const Value &value, std::vector<satelliteObject> &borrowed);

// EVERY ITEM AS TEXT, worked out once -- by_name's; false, with `why` said, when one has none.
bool text_of_each(const std::vector<satelliteObject> &items, std::vector<satellite_string> &out, std::string &why);

// CAN EVERY ITEM BE ORDERED AGAINST EVERY OTHER? success, or compare's own code with `why`.
signed long long int all_comparable(const std::vector<satelliteObject> &items, const char *asking, std::string &why);

// WHAT `.sum` ADDS: the kinds that are numbers.
bool a_number_kind(const satelliteObject &item);

// THE OPENING EVERY ONE OF A LIST'S CHANGES SHARES: the name's own list, made its own before
// anything else is asked; nullptr, refused, when there is no name or it holds no list. `method` is
// the method's spelling, for the refusals.
satelliteList *list_to_change(Value *home, const std::string &name, const char *method, ExpressionContext &context);

// A POSITION OR A COUNT GOING IN (.insert, .remove_at, .truncate; .reserve's count of items): a
// whole number no less than 0, and one too big for a machine word is past the end of every list.
bool position_argument(const Value &given, bool a_count_of_items, const std::string &name, const char *method,
                       unsigned long long int &position, std::string &written, ExpressionContext &context);

} // namespace container_parts

// THE PIECES A MAP'S NUMBERED METHODS SHARE (satellite-numbers/words/1.4.1.n.cpp), as container_parts
// are a list's.
struct satelliteIndex;   // satellite_object/satellite_index.hpp

namespace map_method_parts {

// THE OPENING EVERY ONE OF A MAP'S CHANGES SHARES -- .clear, .remove(k), .remove_first(), .remove_last(),
// .set(k, v) -- as call_container_method made it until 2026-10-11: a name to change, and that name's own
// map, made its own (copy-on-write) before anything else is asked of it. nullptr, refused. `method` is the
// method's spelling, for the refusals.
satelliteIndex *map_to_change(Value *home, const std::string &name, const char *method, ExpressionContext &context);

// TAKE AN ENTRY OUT, keeping the insertion order of the rest, and the lookup table repaired with it.
void take_entry_out(satelliteIndex &index, std::size_t at);

// CAN EVERY KEY -- OR EVERY VALUE -- BE ORDERED AGAINST EVERY OTHER? .sort("key")'s and .sort("value")'s
// question, asked where each sits in the entries: success, or compare's own code with `why` said.
signed long long int entries_comparable(const satelliteIndex &held, bool by_key, std::string &why);

} // namespace map_method_parts

// A LIST'S AND A MAP'S REFUSALS, AND NOTHING ELSE SINCE 2026-10-11: each comes here only with a method it
// does not have -- a list with .keys or .values, a map with .append or a position, either with anything not
// a container's -- and is refused before anything runs; every method each has is its number
// (run_list_number, run_map_number). `home` is the variable's own object when the receiver came straight
// from a name, and nullptr otherwise.
Value call_container_method(token::Code method, Value &receiver, Value *home, const std::vector<Value> &arguments,
                            bool had_parentheses, const std::string &name, ExpressionContext &context);

// ---------------------------------------------------------------------------
// `satellite.container.list()` -- AN EMPTY LIST, MADE BY NAMING THE TYPE.
// ---------------------------------------------------------------------------
//
// 003's constructor (the author, 2026-09-05, 003 MILESTONES/M16.md §2), taken into
// 004 on 2026-09-23 because 272 lines of his programs are written with it:
//
//     satellite.container.list<madness_type> type_dna = satellite.container.list()
//
// It is `= {}` in other words, and answers exactly what `{}` answers -- a list of
// nothing, which fits every list<type> because there is nothing in it not to fit.
// In 003 it was the ONLY way to get a list (a declaration there held nothing); in
// 004 a bare declaration and `{}` already make one, so this is the third spelling
// of the same value and not a new idea.
//
// NO LIBRARY, for file_calls.hpp's reason: a list is a HANDLE and a library cannot
// answer one. So the checker knows the word by is_container_word(), as it knows
// satellite.infinity() and satellite.file's words.
//
// TWO CODES, ONE WORD. `satellite.container.list()` lexes to `1 4 2 0`; with
// anything between the brackets it falls back to `1 4 2`, which is also the TYPE
// word in `satellite.container.list<...> name`. A type word is followed by `<` or
// a name and never by `(`, and only a call reaches these functions, so the two
// uses never meet.
bool is_container_word(token::Code code);

// "" when the call is one that runs, or what to say instead: the constructor takes
// nothing, because a list that holds something is written `{1, 2}`.
std::string container_word_refused(token::Code code, std::size_t given);

// The empty list. The arguments are already evaluated (and there are none).
Value call_container_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

// WHAT AN INDEX SAYS TO .sum .max .min .join AND .reserve, or "" for any other
// method: which half to ask (`scores.satellite.values.satellite.sum`), or that room is a list's -- and
// to .by_name() and .by_value(), that a map is put in order by .satellite.sort("key") and
// .satellite.sort("value"). ONE
// SENTENCE FOR THE CHECKER AND THE WALKER, so a name declared an index is refused
// before anything runs and a value that turns out to be one is refused in the same
// words.
std::string index_refuses(token::Code method, const std::string &name);

// `.reverse()` ON THE TYPES THAT ARE NOT CONTAINERS: a string, a number, a
// binary. Answers `handled` false when this kind has no reverse, so the caller
// refuses in its own words.
Value reverse_of(const Value &receiver, bool &handled, std::string &why);

} // namespace satellite004
