#pragma once
// satellite/satellite_object/satellite_map_sort.hpp -- A satellite.container.map PUT IN ORDER:
// .sort(), .sort("key") and .sort("value").
//
// The author, 2026-09-26: *"let's have .sort accept a string or a string object, so we type
// sort("key" or "value") key returns a list with the smallest by key at position[0] and value
// returns the smallest value at position[0]"*; *"we need a total of 4 new functions just for map's
// .sort() function, one that sorts by width, one that sorts a - z and 0 - 9, and then it either does
// it by key or by value, which, both are just satellite.container.multiple's so we only need 2 new
// functions for sorting, one that sorts by "key" and one that sorts by "value"*; and on
// 2026-09-26 again: *"I wanted 3 different functions, .sort() which is just an alias for sort by
// key, and .sort("value") which sorts by value"*.
//
// So here are his four: by_width and by_a_to_z_0_to_9, and sorted_by<key> and sorted_by<value> --
// ONE template, because a key and a value are the same thing (satellite_map_entry.hpp: each is a
// satellite.container.multiple), told which half of the entry and which width to read.
//
// ---------------------------------------------------------------------------
// THE ORDER: *"we sort by a - z and by size at the same time, so 10 is always bigger than 9, it's 2
// digits"* -- the width first, then the characters.
// ---------------------------------------------------------------------------
//
//   two strings        the shorter first, then a to z by HIS character table (a-z, A-Z, 0-9, ...;
//                      character_table.hpp) -- so "file2" comes before "file10", as 9 before 10,
//                      and "bo" before "zoe" before "alice"
//   two whole numbers  the width, then 0 to 9: 9, 10, 100. That IS what they are worth, always.
//                      The minus sign is not in the width (as a float's sign is its own bool), so it
//                      is looked at first: every negative before every other, and among negatives
//                      the WIDER is the smaller -- -100, -10, -9, 3
//   anything else      by what it is worth, satl's own compare (a float, a fraction, a percentage,
//                      a binary, a bool...). A float's width counts its decimal places, so width
//                      first would put 1.25 (4 wide) after 9.5 (3 wide).
//
// TWO THAT TIE KEEP THE ORDER THEY WENT IN (a stable sort), so .sort("value") on a word count keeps
// the words that share a count in the order they were counted.
//
// WHAT HAS NO ORDER IS NOT GUESSED AT. A number key and a string key, or two list values, have no
// order between them in satl's compare, and the caller asks that of every kind present BEFORE this
// sorts (container_calls.cpp) -- as .sort().by_value() does for a list -- and refuses. That is also
// what keeps this a true order: mixing the string rule and the number rule in one sort could go
// round in a circle, and a sort handed a circle answers nonsense.
//
// ---------------------------------------------------------------------------
// WHAT IT ANSWERS: THE MAP, its entries in the new order -- *"the smallest ... at position[0]"*,
// which is satellite's [1] (S413). A COPY, as a list's .sort() is, so
// `display(scores.sort("value"))` leaves scores as it was and `scores = scores.sort("value")` keeps
// the order. Every entry keeps its key, its value and both widths; only the order changes.
// ---------------------------------------------------------------------------

#include "satellite_index.hpp"
#include "string_and_string_compare.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace satellite004 {

// 1. BY WIDTH -- for two strings and two whole numbers only, and 0 (a tie, so the next function
// decides) for anything else. A whole number's sign goes first, being the one thing its width
// leaves out.
inline int by_width(const satelliteObject &left, const satelliteMapWidth &left_width,
                    const satelliteObject &right, const satelliteMapWidth &right_width)
{
    if (left.is_string() && right.is_string())
        return satellite_number::compare(left_width, right_width);
    const satellite_number *left_number = left.as_number();
    const satellite_number *right_number = right.as_number();
    if (left_number == nullptr || right_number == nullptr)
        return 0;
    if (left_number->negative() != right_number->negative())
        return left_number->negative() ? -1 : 1;
    const int wider = satellite_number::compare(left_width, right_width);
    return left_number->negative() ? -wider : wider;
}

// 2. BY a - z AND 0 - 9 -- once the widths are equal. Two strings, character by character in his
// table; two whole numbers, digit by digit (the same width and the same sign, so that is their
// worth); anything else, satl's own compare, which the caller has already asked of every kind here.
inline int by_a_to_z_0_to_9(const satelliteObject &left, const satelliteObject &right)
{
    const satellite_string *left_text = left.as_string();
    const satellite_string *right_text = right.as_string();
    if (left_text != nullptr && right_text != nullptr)
        return string_and_string_compare(*left_text, *right_text);
    const satellite_number *left_number = left.as_number();
    const satellite_number *right_number = right.as_number();
    if (left_number != nullptr && right_number != nullptr)
        return satellite_number::compare(*left_number, *right_number);
    int order = 0;
    std::string ignored;
    left.compare(right, order, ignored);
    return order;
}

// ONE ENTRY AS THE SORT READS IT: the half's width, and a whole number's sign and value, beside
// where the entry is. The two functions above, worked out once per entry for a whole number that
// fits one machine word and for a string's width -- so most comparisons never go back to the map.
// Going back is a jump into memory the size of the whole map, and over the twenty million
// comparisons a million entries take, those jumps WERE the sort: 1,000,000 number keys, sorted
// by place alone, cost 0.95 s over the copy (2026-09-27).
struct satelliteMapSortPlace {
    unsigned long long int width = 0;       // a whole number's or a string's, else unread
    unsigned long long int magnitude = 0;   // a whole number's, when it fits one machine word
    std::size_t from = 0;                   // where the entry is in the map
    enum Shape : unsigned char { whole, text, other } shape = other;
    bool negative = false;
};

// by_width, then by_a_to_z_0_to_9 -- the same rules, read from the places where they can be, and
// from the entries themselves where they cannot: a string of the same width, a number wider than
// a machine word, a float, and every other kind.
template <satelliteObject satelliteMapEntry::*half, satelliteMapWidth satelliteMapEntry::*width>
bool comes_before(const satelliteMapSortPlace &l, const satelliteMapSortPlace &r,
                  const std::vector<satelliteMapEntry> &entries)
{
    if (l.shape == satelliteMapSortPlace::whole && r.shape == satelliteMapSortPlace::whole) {
        if (l.negative != r.negative)
            return l.negative;
        if (l.width != r.width)                              // the wider negative is the smaller
            return (l.width < r.width) != l.negative;
        return l.magnitude != r.magnitude && (l.magnitude < r.magnitude) != l.negative;
    }
    if (l.shape == satelliteMapSortPlace::text && r.shape == satelliteMapSortPlace::text && l.width != r.width)
        return l.width < r.width;
    const satelliteMapEntry &left = entries[l.from];
    const satelliteMapEntry &right = entries[r.from];
    int by = by_width(left.*half, left.*width, right.*half, right.*width);
    if (by == 0)
        by = by_a_to_z_0_to_9(left.*half, right.*half);
    return by < 0;
}

// 3 AND 4. BY "key" AND BY "value": one template, told which half of each entry and which width.
template <satelliteObject satelliteMapEntry::*half, satelliteMapWidth satelliteMapEntry::*width>
IndexHandle sorted_by(const satelliteIndex &map)
{
    const std::vector<satelliteMapEntry> &entries = map.entries;
    const std::size_t count = entries.size();

    // THE PLACES ARE SORTED, NOT THE ENTRIES, so where each entry came from is known after -- the
    // hash table from key to place is repaired from that, never rebuilt by writing every key out.
    std::vector<satelliteMapSortPlace> places(count);
    for (std::size_t at = 0; at < count; ++at) {
        satelliteMapSortPlace &place = places[at];
        place.from = at;
        const satelliteObject &held = entries[at].*half;
        const satelliteMapWidth &measured = entries[at].*width;
        if (const satellite_number *number = held.as_number()) {
            if (number->fits_one_limb()) {
                place.shape = satelliteMapSortPlace::whole;
                place.negative = number->negative();
                place.magnitude = number->limb(0);
                place.width = measured.limb(0);
            }
        } else if (held.is_string()) {
            place.shape = satelliteMapSortPlace::text;
            place.width = measured.limb(0);
        }
    }
    std::stable_sort(places.begin(), places.end(),
                     [&entries](const satelliteMapSortPlace &l, const satelliteMapSortPlace &r) {
                         return comes_before<half, width>(l, r, entries);
                     });

    IndexHandle sorted = make_index();
    sorted->entries.reserve(count);
    std::vector<std::size_t> now_at(count);
    for (std::size_t at = 0; at < count; ++at) {
        sorted->entries.push_back(entries[places[at].from]);
        now_at[places[at].from] = at;
    }
    sorted->where = map.where;
    for (std::pair<const std::string, std::size_t> &row : sorted->where)
        row.second = now_at[row.second];
    return sorted;
}

inline IndexHandle sorted_by_key(const satelliteIndex &map)
{
    return sorted_by<&satelliteMapEntry::key, &satelliteMapEntry::key_width>(map);
}

inline IndexHandle sorted_by_value(const satelliteIndex &map)
{
    return sorted_by<&satelliteMapEntry::value, &satelliteMapEntry::value_width>(map);
}

} // namespace satellite004
