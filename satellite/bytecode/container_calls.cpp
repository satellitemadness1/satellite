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
// during. A comparison that refuses halfway through std::sort is an inconsistent
// ordering, which is undefined behaviour -- the sort may read past the end of
// its own range. So the question is settled first, against item 1, which is
// enough: ordering is transitive for every pair the object model allows.
bool all_comparable(const std::vector<satelliteObject> &items, std::string &why)
{
    for (std::size_t at = 1; at < items.size(); ++at) {
        int order = 0;
        std::string inner;
        if (items[0].compare(items[at], order, inner) != success) {
            why = "by_value orders by what each item is worth, and item 1 and item " + std::to_string(at + 1) +
                  " have no order between them: " + inner;
            return false;
        }
    }
    return true;
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
                           ".clear, .truncate, .keys, .values, .sort().by_name(), .sort().by_value() and .reverse())");
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
            std::string unfit;
            if (shape != nullptr && shape->word != 0 && !shape->parameters.empty() &&
                !value_fits(shape->parameters[0], going_in, unfit)) {
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
        if (method == token::insert_token || method == token::remove_at_token || method == token::truncate_token) {
            const satellite_number *number = arguments.front().as_number();
            if (number == nullptr) {
                context.refuse(types_do_not_meet,
                               what + " takes an item number, and was given " + arguments.front().kind_name());
                return Value();
            }
            if (number->negative()) {
                context.refuse(not_a_position, what + " was given " + fast::to_text(*number) +
                                                   ", and items count from 1");
                return Value();
            }
            const unsigned long long int position = fast::fits_a_count(*number) ? fast::as_count(*number) : 0;

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
                               what + "(" + std::to_string(position) + "): " +
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
            if (!all_comparable(items, why)) {
                context.refuse(types_do_not_meet, what + ": " + why);
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
