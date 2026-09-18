// satellite/bytecode/container_calls.cpp -- the header says which of these
// change a name and which answer a new value, and why the split is where it is.

#include "container_calls.hpp"

#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_object/string_and_string_compare.hpp"
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
    switch (method) {
    case token::append_token: return "append";
    case token::size_token: return "size";
    case token::contains_token: return "contains";
    case token::sort_token: return "sort";
    case token::by_name_token: return "by_name";
    case token::by_value_token: return "by_value";
    case token::reverse_token: return "reverse";
    default: return "that";
    }
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
        satellite_string out;
        for (std::size_t at = text->size(); at > 0; --at) {
            const signed long long int put = out.append_code(text->code_at_unchecked(at - 1));
            if (put != success) {
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

    handled = false;
    return Value();
}

// ---------------------------------------------------------------------------
// A LIST'S AND AN INDEX'S METHODS.
// ---------------------------------------------------------------------------
Value call_container_method(token::Code method, Value &receiver, Value *home,
                            const std::vector<Value> &arguments, bool had_parentheses,
                            const std::string &name, ExpressionContext &context)
{
    const std::string what = name + "." + name_of(method);
    const bool is_index = receiver.is_index();

    // HOW MANY ARGUMENTS, said before anything is done with them.
    const std::size_t wanted = (method == token::append_token || method == token::contains_token) ? 1 : 0;
    if (arguments.size() != wanted) {
        context.refuse(satl_line_not_understood,
                       what + " takes " + std::to_string(wanted) + (wanted == 1 ? " argument" : " arguments") +
                           ", and was given " + std::to_string(arguments.size()));
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
    case token::append_token: {
        if (is_index) {
            // AN INDEX IS NOT APPENDED TO, and saying which spelling to use is
            // worth more than a bare refusal: appending would have to invent a
            // key, and a key is the one thing a program must say out loud.
            context.refuse(types_do_not_meet,
                           what + " -- an index is filled by its key: write " + name + "[key] = value");
            return Value();
        }
        if (home == nullptr) {
            // `{1, 2}.append(3)` -- a list nothing is holding. The append would
            // be correct and then thrown away, which is a line that does nothing.
            context.refuse(satl_line_not_understood,
                           what + " changes a list, and this one has no name to change -- "
                                  "append to a name that was declared");
            return Value();
        }
        ListHandle *handle = home->as_list();
        if (handle == nullptr) {
            context.refuse(types_do_not_meet, what + " is a list's, and " + name + " is " + home->kind_name());
            return Value();
        }
        about_to_change(*handle).items.push_back(arguments.front());
        return *home;
    }

    // -----------------------------------------------------------------------
    // THE ONES THAT ANSWER A VALUE.
    // -----------------------------------------------------------------------
    case token::size_token: {
        std::vector<satelliteObject> borrowed;
        return a_count(items_of(receiver, borrowed)->size());
    }

    case token::contains_token: {
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
