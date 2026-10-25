// satellite/bytecode/type_shape.cpp -- does a value fit a declared shape.
//
// SEPARATE FROM THE HEADER because checking a list's items means reaching into
// satelliteList and an index's pairs into satelliteIndex, and both of those are
// defined AFTER satelliteObject is complete -- which is the same ordering that
// put the list and index arms behind handles in the first place.

#include "type_shape.hpp"

#include "capsule_scopes.hpp"
#include "suit_layout.hpp"
#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"

#include <mutex>
#include <string>
#include <unordered_map>

namespace satellite004 {

namespace {

// A LIST THIS LONG REMEMBERS THE SHAPE IT FITS (ERRORS4 033); a shorter one is walked again, which costs less than
// numbering the shape does.
constexpr std::size_t kRememberedFrom = 64;

// A SHAPE'S NUMBER FOR satelliteList::fits: each word's code, a spacesuit's scope, and what is between its < and >,
// spelled out and numbered the first time it is asked -- so two shapes share a number only when they are one shape.
// Under a lock, since two threads may ask at once; made once and never destroyed, so a thread ending after main()
// has returned still finds it.
std::uint64_t number_of_shape(const TypeShape &shape)
{
    std::string spelled;
    const auto spell = [&spelled](const auto &self, const TypeShape &each) -> void {
        spelled += std::to_string(each.word);
        if (each.is_a_suit()) spelled += "@" + std::to_string(each.suit);
        if (each.parameters.empty()) return;
        spelled += '<';
        for (const TypeShape &inner : each.parameters) {
            self(self, inner);
            spelled += ',';
        }
        spelled += '>';
    };
    spell(spell, shape);
    static std::mutex *const lock = new std::mutex();
    static std::unordered_map<std::string, std::uint64_t> *const numbers =
        new std::unordered_map<std::string, std::uint64_t>();
    const std::lock_guard<std::mutex> hold(*lock);
    return numbers->emplace(std::move(spelled), numbers->size() + 1).first->second;
}

} // namespace

bool value_fits(const TypeShape &shape, const satelliteObject &value, std::string &why)
{
    // AN OBJECT OF THAT SPACESUIT AND NO OTHER (2026-09-22). Which spacesuit is the
    // SCOPE it resolved to, never the name: two files may each declare a `people`,
    // and an object of one is not an object of the other.
    if (shape.is_a_suit()) {
        // AN OBJECT'S SPACESUIT, OR A POINTER'S -- which is the spacesuit of the object it points
        // at, kept with it after that object is gone (satellite_pointer.hpp): `box p = a.pointer()`
        // declares a box, and every capsule of a box is called through p.
        const satelliteSuitLayout *layout = nullptr;
        if (const UserDefinedHandle *object = value.as_user_defined())
            layout = *object != nullptr ? (*object)->layout.get() : nullptr;
        else if (const ObjectPointer *pointer = value.as_pointer())
            layout = pointer->layout.get();
        if (layout == nullptr) {
            why = "it holds " + std::string(value.kind_name());
            return false;
        }
        // AN OBJECT OF A SPACESUIT THAT EXTENDS IT FITS TOO: it is one of those as well.
        if (!layout->is_a(shape.suit)) {
            why = std::string(value.is_pointer() ? "it holds a pointer at an object" : "it holds an object") +
                  " of the spacesuit " + layout->shown;
            return false;
        }
        return true;
    }

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

    // AN INFINITY NAME HOLDS AN INFINITY, AND NOTHING LESS (the author's H of 2026-10-11, reversing
    // SATELLITE_INFINITY.md Q24, which let it take a plain number): "= 5 is an error, the correct syntax
    // should be a minimum of infinity". A name that must hold a number at one time and an infinity at
    // another says so: satellite.container.multiple<satellite.variable.number, satellite.variable.infinity>.
    if (wanted == satelliteObject::infinity && value.kind() != wanted) {
        why = "it holds " + std::string(value.kind_name()) + kAMinimumOfInfinity;
        return false;
    }

    if (value.kind() != wanted) {
        // JUST WHAT IT HOLDS. Every caller has already said what the name was
        // declared as, so naming the wanted type again reads as
        // "declared a string, and this name takes a string" -- which tells a
        // person nothing and reads like the machine arguing with itself.
        // `multiple` is the exception and says its list, because the declaration
        // alone does not name the types it accepts.
        why = "it holds " + std::string(value.kind_name());
        // A FRACTION WHERE A NUMBER OR A FLOAT GOES -- every a / b of two numbers is one since 2026-10-12 --
        // is told what to write, in the author's words (his 9b): "declare a fraction, or write
        // satellite.convert.number(a / b)".
        if (value.is_fraction() && (wanted == satelliteObject::number || wanted == satelliteObject::floating))
            why += wanted == satelliteObject::number
                       ? " -- declare a fraction, or write satellite.convert.number(...) for the whole number "
                         "nearest it"
                       : " -- declare a fraction, or write satellite.convert.float(...) for the float it is worth";
        return false;
    }

    if (shape.parameters.empty())
        return true;                             // `list` with no <> is a list of anything

    // A LIST'S ITEMS. EVERY one of them, because a parameter that is checked for
    // the first item only is a parameter that says nothing.
    if (wanted == satelliteObject::list) {
        const satelliteList *held = value.as_list()->get();
        if (held == nullptr) return true;
        // FOUND TO FIT THIS SHAPE ALREADY, AND NOT CHANGED SINCE (ERRORS4 033; satelliteList::fits): its items
        // were checked on the way in, and a long one is not walked again at every name it is handed to.
        const std::uint64_t remembered = held->items.size() >= kRememberedFrom ? number_of_shape(shape) : 0;
        if (remembered != 0 && held->fits.load(std::memory_order_relaxed) == remembered)
            return true;
        for (std::size_t at = 0; at < held->items.size(); ++at) {
            std::string inner;
            if (!value_fits(shape.parameters[0], held->items[at], inner)) {
                why = "item " + std::to_string(at + 1) + " of it does not fit: " + inner;
                return false;
            }
        }
        if (remembered != 0)
            held->fits.store(remembered, std::memory_order_relaxed);
        return true;
    }

    // AN INDEX'S KEYS AND VALUES, both, for the same reason.
    if (wanted == satelliteObject::index) {
        const satelliteIndex *held = value.as_index()->get();
        if (held == nullptr) return true;
        for (std::size_t at = 0; at < held->entries.size(); ++at) {
            std::string inner;
            if (!value_fits(shape.parameters[0], held->entries[at].key, inner)) {
                why = "a key of it does not fit: " + inner;
                return false;
            }
            if (shape.parameters.size() > 1 &&
                !value_fits(shape.parameters[1], held->entries[at].value, inner)) {
                why = "a value of it does not fit: " + inner;
                return false;
            }
        }
        return true;
    }

    return true;
}

namespace {

// EVERY TYPE OF A multiple THAT IS OF THE VALUE'S KIND, through multiples of multiples,
// counted; the first is kept. A multiple with no types among them is every type, so of any.
void of_its_kind(const TypeShape &shape, const satelliteObject &value, const TypeShape *&first, int &count)
{
    for (const TypeShape &arm : shape.parameters) {
        const bool a_multiple = arm.word == word::code_of(1, 4, 6);
        if (a_multiple && !arm.parameters.empty()) {
            of_its_kind(arm, value, first, count);
            continue;
        }
        const bool fits = a_multiple || (arm.is_a_suit() ? value.answers_as_an_object()
                                                          : kind_of_type_word(arm.word) == value.kind());
        if (!fits) continue;
        if (count++ == 0) first = &arm;
    }
}

// A TYPE A KEY CAN BE (ERRORS4 028): one of the five key_name_of files a key under (satellite_index.hpp), or a
// multiple of nothing but those -- the run's own rule, asked of the declared type instead of a value.
bool a_key_type(const TypeShape &key)
{
    if (key.word == word::code_of(1, 4, 6)) {
        for (const TypeShape &arm : key.parameters)
            if (!a_key_type(arm)) return false;
        return !key.parameters.empty();
    }
    if (key.is_a_suit()) return false;
    const satelliteObject::Kind kind = kind_of_type_word(key.word);
    return kind == satelliteObject::number || kind == satelliteObject::string || kind == satelliteObject::boolean ||
           kind == satelliteObject::binary || kind == satelliteObject::percentage;
}

} // namespace

const TypeShape &arm_holding(const TypeShape &shape, const satelliteObject &value)
{
    if (shape.word != word::code_of(1, 4, 6) || shape.parameters.empty())
        return shape;
    const TypeShape *first = nullptr;
    int count = 0;
    of_its_kind(shape, value, first, count);
    return count == 1 ? *first : shape;          // none, or two of its kind: not chosen (the header)
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
    // A SPACESUIT'S NAME IS A TYPE (2026-09-22): `run_log`, `tagged_report.run_log`,
    // wherever a type is written -- a capsule's parameter, between a list's < and >. It is read as written and resolved by whoever holds the scope
    // table; here there is none. A spacesuit takes nothing between < and >.
    if (word == token::name_token) {
        dotted_names_at(row, at, out.suit_names);
        out.word = word::code_of(1, 10);
        return true;
    }
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
    // A MAP WHOSE KEY TYPE COULD NEVER BE A KEY is refused where it is declared, in the run's sentence (ERRORS4 028):
    // map<list<number>, number> m was accepted, satellite.access offered m[key] = 1, and every fill was S301 at run.
    if (is_an_index_word(out.word) && !a_key_type(out.parameters.front())) {
        why = std::string(word::spelling_of(out.word)) + "'s key is " + shape_written(out.parameters.front()) +
              ", and a key must be a number, a string, a bool, a binary or a percentage -- something that cannot "
              "change after it is filed under";
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
