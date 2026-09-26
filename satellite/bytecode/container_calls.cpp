// satellite/bytecode/container_calls.cpp -- the header says which of these
// change a name and which answer a new value, and why the split is where it is.

#include "container_calls.hpp"

#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_object/string_and_string_compare.hpp"
#include "../satellite_object/fast_paths.hpp"
#include "type_shape.hpp"
#include "word_codes.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace satellite004 {
namespace {

namespace fast = number_fast_path;

const char *name_of(token::Code method)
{
    const char *named = token::method_name_of(method);   // the registry's one table (INF-1)
    return named[0] != '\0' ? named : "that";
}

Value a_count(std::size_t n)
{
    return Value::of_number(satellite_number(static_cast<unsigned long long int>(n)));
}

// THE ITEMS OF WHATEVER THIS IS, for the methods that treat a list and an index
// alike. An index answers its KEYS, because a key is what `contains` is asked
// about and what `by_name` would order -- the values are reached through the
// key, and always were.
const std::vector<satelliteObject> *items_of(const Value &value, std::vector<satelliteObject> &borrowed)
{
    if (const ListHandle *list = value.as_list()) {
        const satelliteList *held = list->get();
        return held == nullptr ? &borrowed : &held->items;
    }
    if (const IndexHandle *index = value.as_index()) {
        const satelliteIndex *held = index->get();
        if (held != nullptr)
            for (const std::pair<satelliteObject, satelliteObject> &entry : held->entries)
                borrowed.push_back(entry.first);
        return &borrowed;
    }
    return &borrowed;
}

// EVERY ITEM AS TEXT, worked out ONCE. `by_name` on ten thousand items would
// otherwise convert each of them the twenty times a sort compares it.
bool text_of_each(const std::vector<satelliteObject> &items, std::vector<satellite_string> &out, std::string &why)
{
    out.reserve(items.size());
    for (const satelliteObject &item : items) {
        satellite_string text;
        if (item.to_string(text, why) != success) {
            why = "by_name orders by what each item reads as, and one of them has no text: " + why;
            return false;
        }
        out.push_back(std::move(text));
    }
    return true;
}

// CAN EVERY ITEM BE ORDERED AGAINST EVERY OTHER? Asked BEFORE sorting, never
// during. A comparison that refuses halfway through a sort is an inconsistent
// ordering, and the sort's answer means nothing. So the question is settled first.
//
// EVERY PAIR OF KINDS PRESENT IS ASKED, and asking item 1 against the rest was not
// enough (the review, 2026-09-23). Whether two items have an order is decided by
// their KINDS -- compare routes on the pair of tags -- and "1 has an order with b10
// and with 9.5" says nothing about b10 against 9.5, which has none yet. So
// {1, b10, 9.5}.max answered b10 with exit 0, and .sort().by_value() left the list
// as it was and said nothing. Now one item of each kind meets one of every other
// kind, and a second of its own kind when there is one (two lists have no order
// with each other): at most 36 items, however long the list.
//
// Answers success, or compare's own machine code -- not_built_yet for a pair whose
// order is decided and not built, as `b10 < 9.5` says -- with `why` naming the two
// items. `asking` is which method wants to know, so the sentence names the line the
// person wrote.
signed long long int all_comparable(const std::vector<satelliteObject> &items, const char *asking, std::string &why)
{
    constexpr std::size_t none = static_cast<std::size_t>(-1);
    std::size_t first_of[satelliteObject::how_many_kinds], second_of[satelliteObject::how_many_kinds];
    for (std::size_t kind = 0; kind < satelliteObject::how_many_kinds; ++kind) first_of[kind] = second_of[kind] = none;
    for (std::size_t at = 0; at < items.size(); ++at) {
        const std::size_t kind = items[at].kind();
        if (first_of[kind] == none) first_of[kind] = at;
        else if (second_of[kind] == none) second_of[kind] = at;
    }
    std::vector<std::size_t> asked;
    for (std::size_t kind = 0; kind < satelliteObject::how_many_kinds; ++kind) {
        if (first_of[kind] != none) asked.push_back(first_of[kind]);
        if (second_of[kind] != none) asked.push_back(second_of[kind]);
    }
    std::sort(asked.begin(), asked.end());           // so the sentence names the earlier item first
    for (std::size_t one = 0; one < asked.size(); ++one) {
        for (std::size_t other = one + 1; other < asked.size(); ++other) {
            int order = 0;
            std::string inner;
            const signed long long int code = items[asked[one]].compare(items[asked[other]], order, inner);
            if (code != success) {
                why = std::string(asking) + " by what each item is worth, and item " + std::to_string(asked[one] + 1) +
                      " and item " + std::to_string(asked[other] + 1) + " have no order between them: " + inner;
                return code;
            }
        }
    }
    return success;
}

// WHAT `.sum` ADDS: THE KINDS THAT ARE NUMBERS. `+` also joins two strings and mixes
// two colours, and a list of either "summed" would be an answer nobody asked `.sum`
// for -- 003 held sum to numbers for the same reason. Strings have `.join`.
bool a_number_kind(const satelliteObject &item)
{
    return item.is_number() || item.is_float() || item.is_fraction() || item.is_binary() ||
           item.is_hexadecimal() || item.is_percentage() || item.is_infinity();
}

// THE ONES THAT ASK WHAT A LIST HOLDS, TAKEN FROM 003 ON 2026-09-23. An index holds
// keys AND values, and each of these could mean either -- the sum of the scores or
// of the names? -- so an index is told to say which rather than having one chosen.
bool asks_what_it_holds(token::Code method)
{
    return method == token::sum_token || method == token::max_token || method == token::min_token ||
           method == token::join_token;
}


// THE ONES A DICT HAS NO MEANING FOR. Every one of these names a POSITION, and
// an index has no positions -- it has keys. Refusing is better than inventing an
// order for them to count along, because the order a dict keeps is the order
// things were PUT IN, and `remove_at(2)` against that would quietly depend on
// the history of the program rather than on anything visible in the line.
bool a_position_method(token::Code method)
{
    return method == token::insert_token || method == token::remove_at_token ||
           method == token::truncate_token || method == token::index_of_token ||
           method == token::search_token || method == token::append_token;
}

// TAKE AN ENTRY OUT OF AN INDEX, keeping the insertion order of the rest.
//
// THE POSITIONS AFTER IT ALL MOVE, so the lookup table has to be repaired --
// every entry after the hole is now one place earlier. Doing it here, once, is
// what keeps `where` and `entries` from silently disagreeing; an index whose
// table points one past itself answers the WRONG VALUE for a key rather than
// failing, which is the worst way for this to break.
void take_entry_out(satelliteIndex &index, std::size_t at)
{
    std::string key_name;
    if (key_name_of(index.entries[at].first, key_name))
        index.where.erase(key_name);
    index.entries.erase(index.entries.begin() + static_cast<std::ptrdiff_t>(at));
    for (std::pair<const std::string, std::size_t> &row : index.where)
        if (row.second > at) --row.second;
}

} // namespace

std::string index_refuses(token::Code method, const std::string &name)
{
    if (asks_what_it_holds(method)) {
        const std::string called = std::string(name_of(method)) + (method == token::join_token ? "(separator)" : "");
        return "an index holds keys and values, so say which: " + name + ".values." + called + " or " + name +
               ".keys." + called;
    }
    if (method == token::reserve_token)
        return "an index makes its room as keys arrive; .reserve(n) is a list's";
    return "";
}

// ---------------------------------------------------------------------------
// `satellite.container.list()` -- the header says why it has no library.
// ---------------------------------------------------------------------------
// AND satellite.container.map() (1 4 1 0), since 2026-09-26: the map is an index
// (type_shape.hpp), so map() is the empty one, as list() is the empty list.
bool is_map_word(token::Code code)
{
    return code == word::code_of(1, 4, 1) || code == word::code_of(1, 4, 1, 0);
}

bool is_container_word(token::Code code)
{
    return code == word::code_of(1, 4, 2) || code == word::code_of(1, 4, 2, 0) || is_map_word(code);
}

std::string container_word_refused(token::Code code, std::size_t given)
{
    if (!is_container_word(code) || given == 0)
        return "";
    if (is_map_word(code))
        return "satellite.container.map() makes a map of nothing and takes nothing -- a map that holds "
               "something is written with braces, {\"key\": value}";
    return "satellite.container.list() makes a list of nothing and takes nothing -- a list that holds "
           "something is written with braces, {1, 2}";
}

Value call_container_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context)
{
    const std::string refused = container_word_refused(code, arguments.size());
    if (!refused.empty()) {
        context.refuse(satl_line_not_understood, refused);
        return Value();
    }
    if (is_map_word(code))
        return Value::of_index(make_index());
    return Value::of_list(make_list());
}

// ---------------------------------------------------------------------------
// `.reverse()` ON THE TYPES THAT ARE NOT CONTAINERS.
// ---------------------------------------------------------------------------
Value reverse_of(const Value &receiver, bool &handled, std::string &why)
{
    handled = true;

    // A STRING, BY CHARACTER AND NEVER BY BYTE. satellite_string holds 16/32-bit
    // characters, so walking it backwards gives the same letters in the other
    // order -- where reversing UTF-8 bytes would give a string that is not text
    // at all. This is the whole reason the language has its own string type.
    if (const satellite_string *text = receiver.as_string()) {
        // WALKED ONCE FORWARD, THEN WRITTEN BACWARDS. Indexing it from the end
        // instead -- code_at_unchecked(n) for n = size-1 down to 0 -- is O(1) per
        // character only while the string is narrow; the moment it holds ONE wide
        // character every index walks from the start, and reversing a name with
        // an accent in it becomes quadratic. The units are the storage, so one
        // pass over them is one pass over the string whatever it holds.
        std::vector<char32_t> codes;
        codes.reserve(text->size());
        for (std::size_t unit = 0; unit < text->units(); ) {
            std::size_t width = 1;
            codes.push_back(text->code_at_unit(unit, width));
            unit += width;
        }
        satellite_string out;
        for (std::size_t at = codes.size(); at > 0; --at) {
            if (out.append_code(codes[at - 1]) != success) {
                why = "a character in it could not be written back";
                handled = false;
                return Value();
            }
        }
        return Value::of_string(std::move(out));
    }

    // A NUMBER: ITS DIGITS. The sign does not move, because a minus is not a
    // digit -- -123 reverses to -321. A number ending in zero LOSES it (120 ->
    // 21), and that is arithmetic rather than a bug: 021 IS 21, and a number
    // that remembered a leading zero would be a string wearing a number's name.
    if (const satellite_number *number = receiver.as_number()) {
        std::string digits = fast::to_text(*number);
        const bool below_zero = !digits.empty() && digits[0] == '-';
        if (below_zero) digits.erase(digits.begin());
        std::reverse(digits.begin(), digits.end());
        satellite_number made;
        const signed long long int read = fast::from_token_text(digits, fast::kDecimal, made);
        if (read != success) {
            why = "its digits reversed are not a number this can read";
            handled = false;
            return Value();
        }
        if (below_zero) made = -made;
        return Value::of_number(std::move(made));
    }

    // A BINARY: ITS BITS, AND THE WIDTH IS KEPT. b1010 reverses to b0101, which
    // is worth 5 -- the leading zero survives here precisely because a binary IS
    // its width (003 DESIGN 8.5), which a number is not.
    if (const satellite_binary_number *bits = receiver.as_binary()) {
        std::string written = bits->written();          // "b1010", or "-b1010"
        const bool below_zero = !written.empty() && written[0] == '-';
        const std::size_t from = written.find('b');
        if (from == std::string::npos) {
            why = "it has no bits to reverse";
            handled = false;
            return Value();
        }
        std::string digits = written.substr(from + 1);
        std::reverse(digits.begin(), digits.end());
        satellite_binary_number made;
        if (satellite_binary_number::from_digits(digits, made) != success) {
            why = "its bits reversed are not a binary this can read";
            handled = false;
            return Value();
        }
        return Value::of_binary(below_zero ? made.negated() : std::move(made));
    }

    // A FLOAT'S .reverse() IS DECIDED AND NOT BUILT (SATELLITE_INFINITY.md FLT-4:
    // `12.34.reverse()` is 34.12), so it says so rather than that a float has no
    // order to reverse -- which would be untrue (2026-09-22).
    if (receiver.is_float()) {
        why = "a float's .reverse() is not built yet (SATELLITE_INFINITY.md FLT-4)";
        handled = false;
        return Value();
    }

    // A FRACTION HAS AN ORDER -- 1/3 < 1/2 -- so "no order to reverse" would be
    // untrue of one too (satellite.variable.fraction, 2026-09-22). Whether its
    // .reverse() turns 1/3 into 3/1 is not decided, and it is not built.
    if (receiver.is_fraction()) {
        why = "a fraction's .reverse() is not built yet -- a fraction has .numerator, .denominator and .string so far";
        handled = false;
        return Value();
    }

    // satellite.variable.hex (2026-09-22): ITS DIGITS, AND THE WIDTH IS KEPT, as a
    // binary's bits are -- the author asked for .reverse() on "binary numbers hex".
    // x00FF reverses to xFF00; the sign stays where it is.
    if (const satellite_hexadecimal_number *hex = receiver.as_hexadecimal()) {
        std::string digits = (hex->negative() ? hex->negated() : *hex).digits();
        std::reverse(digits.begin(), digits.end());
        satellite_hexadecimal_number made;
        if (satellite_hexadecimal_number::from_digits(digits, made) != success) {
            why = "its digits reversed are not a hex this can read";
            handled = false;
            return Value();
        }
        return Value::of_hexadecimal(hex->negative() ? made.negated() : std::move(made));
    }

    // AN INFINITY HAS NO DIGITS TO TURN ROUND (SATELLITE_INFINITY.md Part 3). It is
    // larger than every number, and reversing the digits of some number standing in
    // for it would be an answer that is wrong and does not say so.
    if (receiver.is_infinity()) {
        why = "an infinity has no digits to turn round";
        handled = false;
        return Value();
    }

    handled = false;
    return Value();
}

// ---------------------------------------------------------------------------
// A LIST'S AND AN INDEX'S METHODS.
// ---------------------------------------------------------------------------
Value call_container_method(token::Code method, Value &receiver, Value *home, const TypeShape *shape,
                            const std::vector<Value> &arguments, bool had_parentheses,
                            const std::string &name, ExpressionContext &context)
{
    const std::string what = name + "." + name_of(method);
    const bool is_index = receiver.is_index();

    // HOW MANY ARGUMENTS, said before anything is done with them.
    const int arity = container_arity(method);
    if (arity < 0) {
        context.refuse(types_do_not_meet,
                       what + " -- a container has no " + name_of(method) +
                           " (a list and an index have .append, .size, .empty, .first, .last, .contains, "
                           ".index_of, .search, .insert, .remove, .remove_at, .remove_first, .remove_last, "
                           ".clear, .truncate, .reserve, .sum, .max, .min, .join, .keys, .values, .sort().by_name(), "
                           ".sort().by_value() and .reverse())");
        return Value();
    }
    // A DICT HAS NO POSITIONS, so the methods that count along one are refused
    // rather than given an order to count along.
    if (is_index && a_position_method(method)) {
        context.refuse(types_do_not_meet,
                       what + " -- an index has keys and not positions" +
                           (method == token::append_token
                                ? std::string(": write ") + name + "[key] = value"
                                : std::string(", so nothing counts along it. Take its keys first: ") + name +
                                      ".keys." + name_of(method) + "(...)"));
        return Value();
    }
    {
        const std::string refused = index_refuses(method, name);
        if (is_index && !refused.empty()) {
            context.refuse(types_do_not_meet, what + " -- " + refused);
            return Value();
        }
    }
    if ((method == token::keys_token || method == token::values_token) && !is_index) {
        context.refuse(types_do_not_meet,
                       what + " -- .keys and .values are an index's; a list already IS its items");
        return Value();
    }
    const std::size_t wanted = static_cast<std::size_t>(arity);
    if (arguments.size() != wanted) {
        context.refuse(satl_line_not_understood,
                       what + " takes " + std::to_string(wanted) +
                           (wanted == 1 ? " argument" : " arguments") + ", and was given " +
                           std::to_string(arguments.size()));
        return Value();
    }
    if (wanted > 0 && !had_parentheses) {
        context.refuse(satl_line_not_understood, what + " takes an argument and needs a ( after it");
        return Value();
    }

    switch (method) {
    // -----------------------------------------------------------------------
    // THE ONE THAT CHANGES THE NAME.
    // -----------------------------------------------------------------------
    case token::append_token:
    case token::clear_token:
    case token::insert_token:
    case token::remove_token:
    case token::remove_at_token:
    case token::remove_first_token:
    case token::remove_last_token:
    case token::reserve_token:
    case token::truncate_token: {
        // A METHOD THAT CHANGES SOMETHING NEEDS SOMETHING TO CHANGE.
        // `{1, 2}.append(3)` is a list nothing is holding: the append would be
        // perfectly correct and then thrown away, which is a line that does
        // nothing -- the worst thing a language can let a person write.
        if (home == nullptr) {
            context.refuse(satl_line_not_understood,
                           what + " changes a container, and this one has no name to change");
            return Value();
        }

        // AN INDEX'S MUTATORS, on its entries in insertion order.
        if (IndexHandle *keys = home->as_index()) {
            satelliteIndex &body = about_to_change(*keys);
            const std::size_t held = body.entries.size();
            if (method == token::clear_token) {
                body.entries.clear();
                body.where.clear();
                return *home;
            }
            if (held == 0) {
                context.refuse(line_past_the_end, what + ": the index is empty");
                return Value();
            }
            if (method == token::remove_first_token) { take_entry_out(body, 0); return *home; }
            if (method == token::remove_last_token) { take_entry_out(body, held - 1); return *home; }
            // `.remove(key)` -- BY KEY, because that is the only thing an index
            // is asked about. `.contains` reads the keys too, so the pair agree.
            std::string key_name;
            if (!key_name_of(arguments.front(), key_name)) {
                context.refuse(types_do_not_meet,
                               what + " was given " + arguments.front().kind_name() +
                                   " as a key, and a key must be a number, a string, a bool, a binary or a percentage");
                return Value();
            }
            for (std::size_t at = 0; at < body.entries.size(); ++at) {
                std::string here;
                if (key_name_of(body.entries[at].first, here) && here == key_name) {
                    take_entry_out(body, at);
                    return *home;
                }
            }
            context.refuse(text_not_found, what + ": there is no such key in it");
            return Value();
        }

        ListHandle *handle = home->as_list();
        if (handle == nullptr) {
            context.refuse(types_do_not_meet, what + " is a container's, and " + name + " is " + home->kind_name());
            return Value();
        }
        satelliteList &body = about_to_change(*handle);
        const std::size_t held = body.items.size();

        // WHAT GOES IN MUST FIT WHAT THE NAME PROMISED. The same test
        // write_through_index makes for `a[1] = x`, made here for `a.append(x)`
        // and `a.insert(n, x)` -- two doors into one list, and locking one of
        // them is worth nothing.
        if (method == token::append_token || method == token::insert_token) {
            const Value &going_in = method == token::append_token ? arguments.front() : arguments[1];
            // ONLY A LIST'S <type> DESCRIBES AN ITEM. A `multiple<A, B>` name
            // constrains ITSELF and not what is inside it -- write_through_index's
            // rule -- and reading its A as the item type refused
            // `multiple<list, number> m = {1}` then m.append(3), "it holds a number",
            // while m[1] = 3 went in (the review, 2026-09-23).
            // A MULTIPLE IS ASKED WHICH ARM THE LIST IS HELD AS (type_shape.hpp), so
            // `multiple<list<number>, number> m = {1}` then m.append("x") is refused.
            std::string unfit;
            const TypeShape *held = shape != nullptr ? &arm_holding(*shape, receiver) : nullptr;
            if (held != nullptr && held->word == word::code_of(1, 4, 2) && !held->parameters.empty() &&
                !value_fits(held->parameters[0], going_in, unfit)) {
                context.refuse(types_do_not_meet, what + ": " + unfit);
                return Value();
            }
        }

        if (method == token::append_token) {
            body.items.push_back(arguments.front());
            return *home;
        }
        if (method == token::clear_token) {
            body.items.clear();
            return *home;
        }

        // THE ONES THAT NAME A POSITION. `.insert` may name one PAST the last
        // item -- inserting at size + 1 is appending, and refusing it would make
        // a loop that fills a list from the end stop one short for no reason.
        // `.reserve(n)` names a COUNT and not a position, but it is read the same way:
        // a whole number no less than 0.
        if (method == token::insert_token || method == token::remove_at_token || method == token::truncate_token ||
            method == token::reserve_token) {
            const satellite_number *number = arguments.front().as_number();
            if (number == nullptr) {
                context.refuse(types_do_not_meet,
                               what + " takes " +
                                   (method == token::reserve_token ? "a count of items" : "an item number") +
                                   ", and was given " + arguments.front().kind_name());
                return Value();
            }
            if (number->negative()) {
                context.refuse(not_a_position, what + " was given " + fast::to_text(*number) +
                                                   (method == token::reserve_token ? ", and a count is 0 or more"
                                                                                   : ", and items count from 1"));
                return Value();
            }
            // A NUMBER TOO BIG FOR A MACHINE WORD IS PAST THE END OF EVERY LIST. It
            // read as position 0 until 2026-09-23, so `a.truncate(10^29)` -- keep more
            // than there are, which changes nothing -- emptied the list instead.
            const std::string written = fast::to_text(*number);
            const unsigned long long int position = fast::fits_a_count(*number)
                                                        ? fast::as_count(*number)
                                                        : std::numeric_limits<unsigned long long int>::max();

            // `.reserve(n)` -- ROOM FOR n ITEMS, MADE ONCE, 003's list_reserve. No item
            // moves and .size answers what it did; what changes is that the next n
            // appends find the room already there. Room the machine will not give is
            // out_of_memory (48), said here -- never a crash, and never a quiet no.
            if (method == token::reserve_token) {
                const std::string no_room = what + "(" + written + "): ";
                if (position > body.items.max_size()) {
                    context.refuse(out_of_memory, no_room + "no machine holds that many items");
                    return Value();
                }
                try {
                    body.items.reserve(static_cast<std::size_t>(position));
                } catch (const std::bad_alloc &) {
                    context.refuse(out_of_memory, no_room + "the machine would not give satl the memory for that many");
                    return Value();
                } catch (const std::length_error &) {
                    context.refuse(out_of_memory, no_room + "no machine holds that many items");
                    return Value();
                }
                return *home;
            }

            if (method == token::truncate_token) {
                // KEEPING MORE THAN THERE ARE CHANGES NOTHING and is not an
                // error, which is what a file's truncate does. `truncate(0)`
                // empties it, and that is the one place 0 is a real answer
                // rather than a position.
                if (position < held)
                    body.items.resize(static_cast<std::size_t>(position));
                return *home;
            }
            const unsigned long long int most = method == token::insert_token ? held + 1 : held;
            if (position == 0 || position > most) {
                context.refuse(line_past_the_end,
                               what + "(" + written + "): " +
                                   (held == 0 ? std::string("the list is empty")
                                              : "the list holds " + std::to_string(held) +
                                                    (held == 1 ? " item" : " items") + ", counting from 1") +
                                   (method == token::insert_token
                                        ? ", so a new one goes in at 1 to " + std::to_string(most)
                                        : ""));
                return Value();
            }
            const std::size_t where = static_cast<std::size_t>(position - 1);
            if (method == token::insert_token)
                body.items.insert(body.items.begin() + static_cast<std::ptrdiff_t>(where), arguments[1]);
            else
                body.items.erase(body.items.begin() + static_cast<std::ptrdiff_t>(where));
            return *home;
        }

        // THE ONES THAT NAME NOTHING, on an empty list.
        if (held == 0) {
            context.refuse(line_past_the_end, what + ": the list is empty");
            return Value();
        }
        if (method == token::remove_first_token) { body.items.erase(body.items.begin()); return *home; }
        if (method == token::remove_last_token) { body.items.pop_back(); return *home; }

        // `.remove(x)` -- THE FIRST ITEM THAT IS x, and a refusal when there is
        // none. Quietly doing nothing is the alternative, and it is worse: a
        // program that removes the wrong thing tells you, and a program that
        // removes nothing does not. `.contains(x)` is how you ask first.
        for (std::size_t at = 0; at < body.items.size(); ++at) {
            if (body.items[at] == arguments.front()) {
                body.items.erase(body.items.begin() + static_cast<std::ptrdiff_t>(at));
                return *home;
            }
        }
        context.refuse(text_not_found, what + ": there is no such item in it -- " + name +
                                           ".contains(x) asks before removing");
        return Value();
    }

    // -----------------------------------------------------------------------
    // THE ONES THAT ANSWER A VALUE.
    // -----------------------------------------------------------------------
    // AN INDEX ANSWERS FROM ITS OWN COUNT, never by copying every key out to
    // measure the copy. items_of() builds a vector for the methods that walk one,
    // and `.size` does not walk anything.
    case token::size_token: {
        if (const IndexHandle *keys = receiver.as_index()) {
            const satelliteIndex *held = keys->get();
            return a_count(held == nullptr ? 0 : held->entries.size());
        }
        std::vector<satelliteObject> borrowed;
        return a_count(items_of(receiver, borrowed)->size());
    }

    case token::empty_token: {
        std::vector<satelliteObject> borrowed;
        return Value::of_bool(items_of(receiver, borrowed)->empty());
    }

    // `.first` AND `.last` ARE `a[1]` AND `a[a.size]`, said in a word. On an
    // index they are the first and last KEY IN INSERTION ORDER, which is the
    // only order a dict has -- and the order it prints in, so what you read here
    // is what you saw.
    case token::first_token:
    case token::last_token: {
        std::vector<satelliteObject> borrowed;
        const std::vector<satelliteObject> *items = items_of(receiver, borrowed);
        if (items->empty()) {
            context.refuse(line_past_the_end,
                           what + ": " + (is_index ? "the index is empty" : "the list is empty"));
            return Value();
        }
        return method == token::first_token ? items->front() : items->back();
    }

    // `.keys` AND `.values`, AS LISTS, IN INSERTION ORDER -- and the two line up
    // index for index, so `k.keys[2]` and `k.values[2]` are one entry. That is
    // the whole reason they are not two independently-ordered answers.
    case token::keys_token:
    case token::values_token: {
        std::vector<satelliteObject> out;
        const satelliteIndex *held = receiver.as_index()->get();
        if (held != nullptr) {
            out.reserve(held->entries.size());
            for (const std::pair<satelliteObject, satelliteObject> &entry : held->entries)
                out.push_back(method == token::keys_token ? entry.first : entry.second);
        }
        return Value::of_list(make_list(std::move(out)));
    }

    // `.index_of(x)` -- WHERE x IS, COUNTING FROM 1, and 0 when it is nowhere.
    // 0 is a real answer here rather than a refusal, and that is a file's own
    // rule kept: 0 is not a position, so it cannot be confused with one.
    case token::index_of_token: {
        std::vector<satelliteObject> borrowed;
        const std::vector<satelliteObject> *items = items_of(receiver, borrowed);
        for (std::size_t at = 0; at < items->size(); ++at)
            if ((*items)[at] == arguments.front())
                return a_count(at + 1);
        return a_count(0);
    }

    // `.search(x)` -- WHERE AN ITEM CONTAINS x, where `.index_of` wants it to BE
    // x. A file searches the text of a line; a list searches the text an item
    // reads as, so `{12, 345}.search(4)` finds 345 at 2.
    case token::search_token: {
        std::vector<satelliteObject> borrowed;
        const std::vector<satelliteObject> *items = items_of(receiver, borrowed);
        satellite_string needle;
        std::string why;
        if (arguments.front().to_string(needle, why) != success) {
            context.refuse(types_do_not_meet,
                           what + " looks for what an item READS as, and was given " +
                               arguments.front().kind_name() + ", which has no text");
            return Value();
        }
        for (std::size_t at = 0; at < items->size(); ++at) {
            satellite_string text;
            if ((*items)[at].to_string(text, why) != success)
                continue;                       // an item with no text contains nothing
            Value found;
            if (str_find_str(Value::of_string(text), Value::of_string(needle), found) == success)
                return a_count(at + 1);
        }
        return a_count(0);
    }

    case token::contains_token: {
        // AN INDEX ASKS ITS HASH TABLE, which is the entire reason it has one.
        // Copying every key into a vector and walking it made a hash-table
        // lookup cost O(n) plus a full copy -- on the container whose whole
        // point is that asking is cheap.
        if (const IndexHandle *keys = receiver.as_index()) {
            const satelliteIndex *held = keys->get();
            std::string key_name;
            if (held == nullptr || !key_name_of(arguments.front(), key_name))
                return Value::of_bool(false);   // a key it could never hold is a key it does not hold
            return Value::of_bool(value_at(*held, key_name) != nullptr);
        }
        std::vector<satelliteObject> borrowed;
        const std::vector<satelliteObject> *items = items_of(receiver, borrowed);
        for (const satelliteObject &item : *items)
            if (item == arguments.front())
                return Value::of_bool(true);
        return Value::of_bool(false);
    }

    // `.sort()` TAKES A COPY AND ORDERS NOTHING. The ordering is the
    // `.by_name()` or `.by_value()` that follows, which is the author's own
    // spelling -- and the copy is what stops
    // `display(names.sort().by_name())` quietly reordering names.
    case token::sort_token:
        return receiver;

    case token::by_name_token:
    case token::by_value_token: {
        std::vector<satelliteObject> borrowed;
        std::vector<satelliteObject> items = *items_of(receiver, borrowed);
        std::string why;

        if (method == token::by_name_token) {
            std::vector<satellite_string> text;
            if (!text_of_each(items, text, why)) {
                context.refuse(types_do_not_meet, what + ": " + why);
                return Value();
            }
            // THE TEXT IS SORTED BESIDE THE ITEMS rather than looked up again,
            // so each item is converted once (see text_of_each).
            std::vector<std::size_t> order(items.size());
            for (std::size_t at = 0; at < order.size(); ++at) order[at] = at;
            std::stable_sort(order.begin(), order.end(),
                             [&text](std::size_t l, std::size_t r) {
                                 return string_and_string_compare(text[l], text[r]) < 0;
                             });
            std::vector<satelliteObject> sorted;
            sorted.reserve(items.size());
            for (const std::size_t which : order) sorted.push_back(items[which]);
            items.swap(sorted);
        } else {
            const signed long long int comparable = all_comparable(items, "by_value orders", why);
            if (comparable != success) {
                context.refuse(comparable, what + ": " + why);
                return Value();
            }
            std::stable_sort(items.begin(), items.end(),
                             [](const satelliteObject &l, const satelliteObject &r) {
                                 int order = 0;
                                 std::string ignored;
                                 return l.compare(r, order, ignored) == success && order < 0;
                             });
        }
        return Value::of_list(make_list(std::move(items)));
    }

    case token::reverse_token: {
        std::vector<satelliteObject> borrowed;
        std::vector<satelliteObject> items = *items_of(receiver, borrowed);
        std::reverse(items.begin(), items.end());
        return Value::of_list(make_list(std::move(items)));
    }

    // -----------------------------------------------------------------------
    // TAKEN FROM 003, 2026-09-23: .sum .max .min .join(separator). 003 numbered
    // them at M16 and gave each its meaning there; these are those meanings,
    // counted from 1 where a position is said, as everything in 004 is.
    // -----------------------------------------------------------------------

    // `.sum` -- EVERY ITEM ADDED WITH +, left to right, by the object model's own
    // add: a list of floats sums as a float and a fraction stays a fraction, exactly
    // as `a + b + c` written out would. A LIST OF NOTHING SUMS TO 0 -- not a
    // sentinel but the sum of no numbers, 003's one row where "nothing to answer
    // with" has an answer.
    case token::sum_token: {
        std::vector<satelliteObject> borrowed;
        const std::vector<satelliteObject> &items = *items_of(receiver, borrowed);
        if (items.empty())
            return a_count(0);
        for (std::size_t at = 0; at < items.size(); ++at) {
            if (!a_number_kind(items[at])) {
                context.refuse(types_do_not_meet,
                               what + " adds numbers, and item " + std::to_string(at + 1) + " is " +
                                   items[at].kind_name() +
                                   (items[at].is_string() ? " -- .join(separator) makes one string of them" : ""));
                return Value();
            }
        }
        Value total = items.front();
        for (std::size_t at = 1; at < items.size(); ++at) {
            Value next;
            std::string why;
            const signed long long int added = total.add(items[at], next, why);
            if (added != success) {
                context.refuse(added, what + ": item " + std::to_string(at + 1) +
                                          " cannot be added to the items before it -- " + why);
                return Value();
            }
            total = std::move(next);
        }
        return total;
    }

    // `.max` AND `.min` -- THE ORDERING ASKED ONCE. Not a sort that keeps one end: a
    // sort is n log n and copies the list, and asking which item is largest is one
    // pass. Worth is `.sort().by_value()`'s own comparison, so `.max` IS
    // `.sort().by_value().last` and `.min` IS `.sort().by_value().first` -- EVEN
    // BETWEEN EQUAL ITEMS: a stable sort keeps equals in the order they came, so of
    // {2, 1, 2.0} the last is 2.0, and .max answers 2.0 (it answered 2 until the
    // review, 2026-09-23). So .max keeps the LATER of two equals and .min the earlier.
    case token::max_token:
    case token::min_token: {
        std::vector<satelliteObject> borrowed;
        const std::vector<satelliteObject> &items = *items_of(receiver, borrowed);
        const bool largest = method == token::max_token;
        if (items.empty()) {
            context.refuse(line_past_the_end, what + ": the list is empty, so nothing in it is the " +
                                                  (largest ? "largest" : "smallest"));
            return Value();
        }
        std::string why;
        const signed long long int comparable =
            all_comparable(items, largest ? "max finds the largest" : "min finds the smallest", why);
        if (comparable != success) {
            context.refuse(comparable, what + ": " + why);
            return Value();
        }
        // AND EVERY COMPARISON THE PASS MAKES IS STILL ASKED WHETHER IT ANSWERED. The
        // kinds were judged above; a refusal here would be one that depends on the
        // values, and skipping it is how a wrong item came back with exit 0 before.
        const satelliteObject *best = &items.front();
        for (std::size_t at = 1; at < items.size(); ++at) {
            int order = 0;
            std::string inner;
            const signed long long int code = items[at].compare(*best, order, inner);
            if (code != success) {
                context.refuse(code, what + ": item " + std::to_string(at + 1) + " has no order with the " +
                                         (largest ? "largest" : "smallest") + " before it: " + inner);
                return Value();
            }
            if (largest ? order >= 0 : order < 0)
                best = &items[at];
        }
        return *best;
    }

    // `.join(separator)` -- ONE STRING: every item as it reads, with the separator
    // between and nothing at either end. A string item is its own text and anything
    // else is what .string would make of it -- `{1, 2}.join("-")` is "1-2". The
    // separator must be a string, because what goes between text is text.
    case token::join_token: {
        const satellite_string *separator = arguments.front().as_string();
        if (separator == nullptr) {
            context.refuse(types_do_not_meet, what + " puts a string between the items, and was given " +
                                                  arguments.front().kind_name());
            return Value();
        }
        std::vector<satelliteObject> borrowed;
        const std::vector<satelliteObject> &items = *items_of(receiver, borrowed);
        satellite_string out;
        for (std::size_t at = 0; at < items.size(); ++at) {
            if (at != 0)
                out.append(*separator);
            satellite_string text;
            std::string why;
            if (items[at].to_string(text, why) != success) {
                context.refuse(types_do_not_meet,
                               what + ": item " + std::to_string(at + 1) + " has no text to join -- " + why);
                return Value();
            }
            out.append(text);
        }
        return Value::of_string(std::move(out));
    }

    default:
        break;
    }

    context.refuse(not_built_yet,
                   what + " is not built for " + receiver.kind_name() +
                       " yet -- a container has .append, .size, .contains, .sort().by_name(), "
                       ".sort().by_value() and .reverse()");
    return Value();
}

} // namespace satellite004
