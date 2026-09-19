// satellite/bytecode/type_shape.cpp -- does a value fit a declared shape.
//
// SEPARATE FROM THE HEADER because checking a list's items means reaching into
// satelliteList and an index's pairs into satelliteIndex, and both of those are
// defined AFTER satelliteObject is complete -- which is the same ordering that
// put the list and index arms behind handles in the first place.

#include "type_shape.hpp"

#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"

namespace satellite004 {

bool value_fits(const TypeShape &shape, const satelliteObject &value, std::string &why)
{
    // `multiple<a, b>` -- ANY ONE OF THEM. The value is an ordinary value of
    // whichever it matched; nothing is wrapped and nothing remembers which.
    if (shape.word == word::code_of(1, 4, 6)) {
        if (shape.parameters.empty())
            return true;                         // `multiple` with no types is every type
        return any_of_fits(shape.parameters, value, why);
    }

    const satelliteObject::Kind wanted = kind_of_type_word(shape.word);
    if (wanted == satelliteObject::how_many_kinds)
        return true;                             // not a type word: nothing to check

    // A NUMBER NAME TAKES A BINARY, because it always did: `satellite.variable.number n = b1010`
    // was a number before binary was a type of its own, and run_assignment converts
    // it. The shape has to agree with that or the conversion is refused before it
    // happens.
    if (wanted == satelliteObject::number && value.is_binary())
        return true;

    // AN INFINITY NAME TAKES A PLAIN NUMBER, the way `multiple` takes each type it
    // lists (SATELLITE_INFINITY.md Q24): when an infinity goes away, "the name keeps
    // what is left, a plain number" -- `x = x - x` leaves 0 in x, under the same name
    // (INF-3). So the name is declared for the whole family and what it can come down
    // to; the level declared is a synonym, not a bound. A binary or a percentage is not
    // a plain number, and a name of the family refuses one.
    if (wanted == satelliteObject::infinity && value.is_number())
        return true;

    if (value.kind() != wanted) {
        // JUST WHAT IT HOLDS. Every caller has already said what the name was
        // declared as, so naming the wanted type again reads as
        // "declared a string, and this name takes a string" -- which tells a
        // person nothing and reads like the machine arguing with itself.
        // `multiple` is the exception and says its list, because the declaration
        // alone does not name the types it accepts.
        why = "it holds " + std::string(value.kind_name());
        return false;
    }

    if (shape.parameters.empty())
        return true;                             // `list` with no <> is a list of anything

    // A LIST'S ITEMS. EVERY one of them, because a parameter that is checked for
    // the first item only is a parameter that says nothing.
    if (wanted == satelliteObject::list) {
        const satelliteList *held = value.as_list()->get();
        if (held == nullptr) return true;
        for (std::size_t at = 0; at < held->items.size(); ++at) {
            std::string inner;
            if (!value_fits(shape.parameters[0], held->items[at], inner)) {
                why = "item " + std::to_string(at + 1) + " of it does not fit: " + inner;
                return false;
            }
        }
        return true;
    }

    // AN INDEX'S KEYS AND VALUES, both, for the same reason.
    if (wanted == satelliteObject::index) {
        const satelliteIndex *held = value.as_index()->get();
        if (held == nullptr) return true;
        for (std::size_t at = 0; at < held->entries.size(); ++at) {
            std::string inner;
            if (!value_fits(shape.parameters[0], held->entries[at].first, inner)) {
                why = "a key of it does not fit: " + inner;
                return false;
            }
            if (shape.parameters.size() > 1 &&
                !value_fits(shape.parameters[1], held->entries[at].second, inner)) {
                why = "a value of it does not fit: " + inner;
                return false;
            }
        }
        return true;
    }

    return true;
}

// ---------------------------------------------------------------------------
// READING `<a, b>` OFF THE TOKENS.
// ---------------------------------------------------------------------------
//
// `pending_closes` IS THE `>>` FIX. When a `>` is wanted and shift_right_token
// is found, that token closes TWO levels: this one, and one for whoever called
// us. Recording the second rather than consuming it is exactly how C++11 stopped
// making people write `> >`.
bool read_type_shape(const std::vector<std::bitset<16>> &row, std::size_t &at, TypeShape &out,
                     unsigned int &pending_closes, std::string &why)
{
    const token::Code word = static_cast<token::Code>(at < row.size() ? row[at].to_ulong() : 0);
    if (!word::is_word_code(word)) {
        why = "a type was expected here, and this is not one";
        return false;
    }
    if (!is_a_type_word(word)) {
        why = std::string(word::spelling_of(word)) + " is not a type a name can be declared as";
        return false;
    }
    out.word = word;
    ++at;

    const token::Code next = static_cast<token::Code>(at < row.size() ? row[at].to_ulong() : 0);
    if (next != token::less_than_token)
        return true;                              // no <>, which means "of anything"
    ++at;

    for (;;) {
        TypeShape inner;
        if (!read_type_shape(row, at, inner, pending_closes, why))
            return false;
        out.parameters.push_back(inner);

        if (pending_closes > 0) {                 // a `>>` below us closed this one too
            --pending_closes;
            break;
        }
        const token::Code after = static_cast<token::Code>(at < row.size() ? row[at].to_ulong() : 0);
        if (after == token::comma_token) { ++at; continue; }
        if (after == token::greater_than_token) { ++at; break; }
        if (after == token::shift_right_token) {  // `>>` closes this one and one above
            ++at;
            pending_closes = 1;
            break;
        }
        why = std::string(word::spelling_of(out.word)) +
              "<...> was opened with < and never closed with > on this line";
        return false;
    }

    const int wanted = parameters_wanted(out.word);
    if (wanted >= 0 && out.parameters.size() != static_cast<std::size_t>(wanted)) {
        why = std::string(word::spelling_of(out.word)) + " takes " + std::to_string(wanted) +
              (wanted == 1 ? " type between < and >, and was given " : " types between < and >, and was given ") +
              std::to_string(out.parameters.size());
        return false;
    }
    if (wanted < 0 && out.parameters.size() < 2) {
        // `multiple<x>` is a name that holds one type, which is what declaring it
        // that type already says. Saying so is friendlier than accepting a
        // declaration that means nothing.
        why = "satellite.container.multiple takes two or more types between < and >, and was given " +
              std::to_string(out.parameters.size());
        return false;
    }
    return true;
}

} // namespace satellite004
