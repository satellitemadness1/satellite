// satellite/satellite_object/satellite_object.cpp -- the methods the author
// asked to be bolted onto the two classes. The header says why it is a god class
// on purpose; this file is the part that would be worth objecting to if it did
// any work, and it does none.
//
// EVERY METHOD HERE IS A SWITCH AND A CALL. The switch is on the PAIR of tags as
// one integer (`pair_of`), and each case calls the one function in the .hpp
// named after that pair. So this file says WHICH, and a file named
// number_and_string_add.hpp says WHAT. Adding satellite_float means adding arms
// to the enum, files named float_and_number_add.hpp and so on, and cases here --
// and touching no arithmetic that already works.
//
// THE CASE LABELS READ LIKE THE FILENAMES, which is the whole reason the author's
// naming convention is worth keeping: `case pair_of(number, string)` sits
// directly above a call to `number_and_string_add`, so a reader never has to
// guess which file a pair went to.

#include "../satellite_variable_thread/satellite_thread.hpp"
#include "satellite_object.hpp"
#include "fast_paths.hpp"
#include "object_color.hpp"
#include "object_float.hpp"
#include "object_fraction.hpp"
#include "object_hexadecimal.hpp"
// The list arm's items, which satellite_object.hpp cannot name: a list holds
// objects, so its definition has to come after this class is complete.
#include "satellite_list.hpp"
#include "satellite_index.hpp"

#include "bool_and_bool_compare.hpp"
#include "bool_to_string.hpp"
#include "bytecode_and_bytecode_join.hpp"
#include "number_and_number_add.hpp"
#include "number_and_number_compare.hpp"
#include "number_and_number_divide.hpp"
#include "number_and_number_modulus.hpp"
#include "number_and_number_multiply.hpp"
#include "number_and_number_power.hpp"
#include "number_and_number_subtract.hpp"
#include "number_and_string_add.hpp"
#include "number_to_binary.hpp"
#include "number_to_hexadecimal.hpp"
#include "number_to_string.hpp"
#include "string_and_number_add.hpp"
#include "string_and_string_add.hpp"
#include "string_and_string_compare.hpp"
#include "string_and_string_subtract.hpp"
#include "string_to_number.hpp"

namespace satellite004 {
namespace {

using NumberPair = signed long long int (*)(const satellite_number &, const satellite_number &, satellite_number &);

// The six arithmetic methods differ only by which pair function they call and
// which sign they name in a refusal, so the shape is written once.
signed long long int run_number_pair(const satelliteObject &left, const satelliteObject &right,
                                     NumberPair operation, const char *sign,
                                     satelliteObject &out, std::string &why)
{
    satellite_number answer;
    const signed long long int code = operation(*left.as_number(), *right.as_number(), answer);
    if (code != success) {
        why = std::string("the ") + sign + " of " + left.as_number()->to_text() + " and " +
              right.as_number()->to_text() + " is " +
              (code == division_by_zero ? "a division by zero"
                                        : "not a whole number, and there is no satellite_float yet");
        return code;
    }
    out = satelliteObject::of_number(std::move(answer));
    return success;
}

// A REFUSAL NAMES BOTH KINDS, because that is what a person fixes. DESIGN 1.1:
// nothing is converted, so a pair with no scenario stops rather than guessing.
signed long long int refuse_pair(const satelliteObject &left, const satelliteObject &right,
                                 const char *sign, std::string &why)
{
    why = std::string(sign) + " was given " + left.kind_name() + " and " + right.kind_name() +
          ", and there is no scenario for that pair";
    return types_do_not_meet;
}

// The pairs that refuse by converting nothing say so in the conversion's own
// words, since "no scenario" would be untrue -- the scenario exists and is a
// conversion the program has to write out loud.
signed long long int refuse_conversion(const satelliteObject &left, const satelliteObject &right,
                                       std::string &why)
{
    why = std::string("+ was given ") + left.kind_name() + " and " + right.kind_name() +
          ", and satellite converts nothing on its own -- write the conversion "
          "(satellite.variable.number.to_string) out loud";
    return types_do_not_meet;
}

// A BINARY IN ARITHMETIC AND IN AN ORDERING IS READ BY WHAT ITS BITS ARE WORTH,
// and an arithmetic answer is a number. That is exactly what `b1100 + xFF` and
// `counter < b0011` did before satellite.variable.binary was a type of its own --
// the literal was a number in base 2 then, and check.sh pins the 267 -- so making
// it a type changed what a binary DISPLAYS and nothing it computes. Only against a
// number or another binary: a binary met by a string is refused as the pair it
// really is, naming both kinds.
//
// THIS DEPARTS FROM RULINGS THE AUTHOR HAS ALREADY MADE, on purpose and for now.
// 003 DESIGN 8.5 records them as DECIDED AND UNBUILT (2026-09-09): `+` on two bit
// runs ADDS and answers a bit run, THE WIDTH GROWS TO FIT and never wraps, and
// with mixed types THE LEFT OPERAND'S TYPE WINS -- so `b1100 + xFF` would be
// `b100001011`, not 267. The same section says why they were not built then: the
// author, "we are doing just too much at one time", and binary arithmetic decided
// in the margin of another milestone "is how it comes out inconsistent". When
// they are built, read_by_worth's callers wrap the answer back up as a binary of
// the left operand's type; nothing else here moves.
//
// ONE WART, NAMED: `b0010 == 2` and `b10 == 2` are both true while `b0010 == b10`
// is false (width), because a binary meets a number by worth and a binary by
// bits and width. 8.5 says different arms never compare EQUAL, which would end it
// -- and would also refuse what `while(counter < b0011)` did before this type.
bool read_by_worth(const satelliteObject &left, const satelliteObject &right)
{
    const bool left_binary = left.is_binary();
    const bool right_binary = right.is_binary();
    return (left_binary || right_binary) && (left_binary || left.is_number()) &&
           (right_binary || right.is_number());
}

satelliteObject worth_of(const satelliteObject &value)
{
    const satellite_binary_number *held = value.as_binary();
    return held != nullptr ? satelliteObject::of_number(held->bits) : value;
}

// THE INFINITY'S ARITHMETIC IS DESIGNED AND NOT BUILT (SATELLITE_INFINITY.md), so an
// operator that meets one says that, and which milestone builds it -- not "there is
// no scenario for that pair", which would be untrue: the scenario is written down to
// the last row of the oracle. Asked BEFORE the percentage's branch, which would
// otherwise answer `inf * 50%` with a pair it knows nothing about.
//
// ONLY FOR THE PAIRS THE SPEC BUILDS: an infinity with an infinity, a number, a
// percentage, or a binary read by its worth. `"a" + inf` is no milestone's -- it is a
// string meeting a kind it does not convert -- and is refused as that pair, below.
bool refuse_infinity_arithmetic(const satelliteObject &left, const satelliteObject &right, const char *sign,
                                const char *builds_it, std::string &why)
{
    const auto of_the_math = [](const satelliteObject &value) {
        return value.is_infinity() || value.is_number() || value.is_percentage() || value.is_binary();
    };
    if ((!left.is_infinity() && !right.is_infinity()) || !of_the_math(left) || !of_the_math(right))
        return false;
    why = std::string(sign) + " was given " + left.kind_name() + " and " + right.kind_name() +
          ", and an infinity's " + sign + " is not built yet (SATELLITE_INFINITY.md, " + builds_it + ")";
    return true;
}

// THE FOUR TYPES OF 2026-09-22 ANSWER EVERY PAIR THEY ARE IN, EACH FROM ITS OWN
// FILE (object_float.hpp says why). Asked after the two-number fast path and the
// infinity's refusal, and before the percentage and the binary, so `x1F + 1` and
// `2.5 * 50%` reach the hex's and the float's own files and not a pair written
// before either type existed. THE ORDER BELOW DECIDES WHICH FILE HEARS A PAIR OF
// TWO OF THEM -- the fraction first, since a fraction is made of floats.
bool answered_by_its_own_file(char sign, const satelliteObject &left, const satelliteObject &right,
                              satelliteObject &out, std::string &why, signed long long int &code)
{
    if (left.is_fraction() || right.is_fraction())
        code = fraction_operation(sign, left, right, out, why);
    else if (left.is_color() || right.is_color())
        code = color_operation(sign, left, right, out, why);
    else if (left.is_hexadecimal() || right.is_hexadecimal())
        code = hexadecimal_operation(sign, left, right, out, why);
    else if (left.is_float() || right.is_float())
        code = float_operation(sign, left, right, out, why);
    else
        return false;
    return true;
}

bool compared_by_its_own_file(const satelliteObject &left, const satelliteObject &right, int &order,
                              std::string &why, signed long long int &code)
{
    if (left.is_fraction() || right.is_fraction())
        code = fraction_compare(left, right, order, why);
    else if (left.is_color() || right.is_color())
        code = color_compare(left, right, order, why);
    else if (left.is_hexadecimal() || right.is_hexadecimal())
        code = hexadecimal_compare(left, right, order, why);
    else if (left.is_float() || right.is_float())
        code = float_compare(left, right, order, why);
    else
        return false;
    return true;
}

} // namespace

// A LITERAL ARRIVES AS UTF-8 BYTES and becomes a satellite_string here -- the
// one door between the lexer's counted payload and the language's own string.
// Strict: a bad sequence answers string_error (4) and `out` is untouched, so a
// program never holds a string standing for bytes that could not be read.
signed long long int satelliteObject::of_utf8(const std::string &utf8, satelliteObject &out,
                                             std::size_t &bad_offset)
{
    satellite_string held;
    const signed long long int code = satellite_string::from_utf8(utf8, held, bad_offset);
    if (code != success)
        return code;
    out = satelliteObject::of_string(std::move(held));
    return success;
}

// THE WAY BACK OUT, for the one place a value leaves as bytes: a numbered
// library's `text` scenario takes a std::string (number_row.hpp).
std::string satelliteObject::text_utf8() const
{
    const satellite_string *held = as_string();
    return held == nullptr ? std::string() : held->to_utf8();
}

// Arm by arm; the header says why this is not std::variant's own. Two values of
// different kinds are never the same value -- nothing is converted to find out
// (DESIGN 1.1), so `4` and `"4"` are simply not equal.
bool operator==(const satelliteObject &l, const satelliteObject &r)
{
    if (l.kind() != r.kind())
        return false;
    switch (l.kind()) {
    case satelliteObject::nothing: return true;
    case satelliteObject::boolean: return *l.as_bool() == *r.as_bool();
    case satelliteObject::number: return number_and_number_compare(*l.as_number(), *r.as_number()) == 0;
    case satelliteObject::string: return string_and_string_compare(*l.as_string(), *r.as_string()) == 0;
    case satelliteObject::bytecode: return *l.as_bytecode() == *r.as_bytecode();
    case satelliteObject::capsule: return *l.as_capsule() == *r.as_capsule();
    // IDENTITY, NOT CONTENTS: the same object, or not the same object.
    case satelliteObject::user_defined: return *l.as_user_defined() == *r.as_user_defined();
    // BITS AND WIDTH: `b0010` is not `b10` (satellite_binary_number.hpp).
    case satelliteObject::binary: return *l.as_binary() == *r.as_binary();
    case satelliteObject::percentage: return *l.as_percentage() == *r.as_percentage();
    // IDENTITY, as for an object: the same open file, or not.
    case satelliteObject::file: return l.as_file() == r.as_file();
    // TWO LISTS ARE EQUAL WHEN THEY HOLD EQUAL ITEMS. This arm said "the same
    // list, or not the same list" until red note 8 was closed, and the note it
    // carried explained why: while nothing had decided whether `b = a` shares or
    // copies, a deep compare would have been an answer to a question still open.
    //
    // THAT QUESTION IS ANSWERED NOW -- a list is a VALUE, `b = a` copies
    // (satellite_list.hpp) -- and identity became the wrong answer the moment it
    // was. `{1, 2} == {1, 2}` was refused and `g.contains({1, 2})` answered
    // false on a list that plainly held it, because the copy-on-write clone the
    // language makes on every write is a different address by design.
    case satelliteObject::list: {
        const satelliteList *left = l.as_list()->get();
        const satelliteList *right = r.as_list()->get();
        if (left == right) return true;                  // the same list, or both empty
        const std::size_t how_many = left == nullptr ? 0 : left->items.size();
        if (how_many != (right == nullptr ? 0 : right->items.size())) return false;
        for (std::size_t at = 0; at < how_many; ++at)
            if (!(left->items[at] == right->items[at])) return false;
        return true;
    }
    // AN INDEX IS EQUAL WHEN IT HOLDS THE SAME KEYS AND THE SAME VALUES, AND
    // THE ORDER DOES NOT COUNT. That is python's rule for a dict and it is the
    // right one: insertion order is what the index REMEMBERS, not what it IS, so
    // two indexes filled with the same pairs in different orders are the same
    // answer to every question a program can ask of them -- while still printing
    // differently, exactly as python's do.
    case satelliteObject::index: {
        const satelliteIndex *left = l.as_index()->get();
        const satelliteIndex *right = r.as_index()->get();
        if (left == right) return true;
        const std::size_t how_many = left == nullptr ? 0 : left->entries.size();
        if (how_many != (right == nullptr ? 0 : right->entries.size())) return false;
        for (std::size_t at = 0; at < how_many; ++at) {
            std::string key_name;
            if (!key_name_of(left->entries[at].first, key_name)) return false;
            const satelliteObject *theirs = value_at(*right, key_name);
            if (theirs == nullptr || !(*theirs == left->entries[at].second)) return false;
        }
        return true;
    }
    // BY VALUE: two infinities are equal when the order says so, and one number
    // has one term list, so that is the same as holding the same terms.
    case satelliteObject::infinity:
        return satellite_infinity::compare(l.as_infinity(), r.as_infinity()) == 0;
    // TWO WINDOWS ARE EQUAL WHEN THEY ARE THE SAME WINDOW, and never when they
    // merely look alike. A window is a thing on a screen: two windows with the
    // same title and size are two windows, and a program comparing them is
    // asking which one it has, not what they have on them.
    case satelliteObject::window:
        return l.as_window() == r.as_window();
    // AND TWO THREADS THE SAME WAY: the same thread, not two that run the same capsule.
    case satelliteObject::thread:
        return l.as_thread() == r.as_thread();
    // THE FOUR OF 2026-09-22 SAY FOR THEMSELVES what makes two of them the same.
    case satelliteObject::floating: return float_same(*l.as_float(), *r.as_float());
    case satelliteObject::hexadecimal: return hexadecimal_same(*l.as_hexadecimal(), *r.as_hexadecimal());
    case satelliteObject::color: return color_same(*l.as_color(), *r.as_color());
    case satelliteObject::fraction: return fraction_same(*l.as_fraction(), *r.as_fraction());
    case satelliteObject::how_many_kinds: break;
    }
    return false;
}

const char *satelliteObject::kind_name() const
{
    switch (kind()) {
    case boolean: return "a bool";
    case number: return "a number";
    case string: return "a string";
    case bytecode: return "bytecode";
    case capsule: return "a capsule";
    case user_defined: return "an object";
    case binary: return "a binary";
    case percentage: return "a percentage";
    case file: return "a file";
    case list: return "a list";
    case index: return "an index";
    case infinity: return "an infinity";
    case window: return "a window";
    case floating: return "a float";
    case hexadecimal: return "a hex";
    case color: return "a color";
    case fraction: return "a fraction";
    case thread: return "a thread";
    case nothing: break;
    case how_many_kinds: break;
    }
    return "nothing";
}

// ---------------------------------------------------------------------------
// `+` -- the one operator with more than one pair, which is 003 DESIGN 6.6's
// ruling that joining and adding are the same shape.
// ---------------------------------------------------------------------------
signed long long int satelliteObject::add(const satelliteObject &other, satelliteObject &out,
                                          std::string &why) const
{
    // THE FAST PATH FIRST, which is the author's own design order: "we build all
    // the fast paths ... THEN we at last build the slower all encompassing" one.
    // Two numbers is the commonest thing this language does, and it was reaching
    // its fast path THIRD -- behind a percentage test and a by-worth test that
    // are both false whenever both sides are numbers. Hoisting is behaviour-
    // identical for exactly that reason: read_by_worth() needs one side BINARY,
    // and is_percentage() needs one side a percentage.
    if (pair_of(kind(), other.kind()) == pair_of(number, number))
        return run_number_pair(*this, other, number_and_number_add, "+", out, why);
    if (refuse_infinity_arithmetic(*this, other, "+", "INF-3", why))
        return not_built_yet;
    if (signed long long int code = success; answered_by_its_own_file('+', *this, other, out, why, code))
        return code;
    if (is_percentage() || other.is_percentage())
        return percentage_operation('+', worth_of(*this), worth_of(other), out, why);
    if (read_by_worth(*this, other))
        return worth_of(*this).add(worth_of(other), out, why);

    switch (pair_of(kind(), other.kind())) {
    case pair_of(string, string): {
        satellite_string answer;
        const signed long long int code = string_and_string_add(*as_string(), *other.as_string(), answer);
        if (code != success) { why = "the two strings could not be joined"; return code; }
        out = satelliteObject::of_string(std::move(answer));
        return success;
    }

    // BOTH ORDERS, BOTH REFUSING, and each through its own file -- see
    // number_and_string_add.hpp for why the file exists while it refuses.
    case pair_of(number, string): {
        satellite_string answer;
        const signed long long int code = number_and_string_add(*as_number(), *other.as_string(), answer);
        if (code != success)
            return refuse_conversion(*this, other, why);
        out = satelliteObject::of_string(std::move(answer));
        return success;
    }
    case pair_of(string, number): {
        satellite_string answer;
        const signed long long int code = string_and_number_add(*as_string(), *other.as_number(), answer);
        if (code != success)
            return refuse_conversion(*this, other, why);
        out = satelliteObject::of_string(std::move(answer));
        return success;
    }

    // THE ARM THAT ONLY EXISTS BECAUSE BYTECODE IS IN THE VARIANT.
    case pair_of(bytecode, bytecode): {
        satellite_bytecode answer;
        const signed long long int code = bytecode_and_bytecode_join(*as_bytecode(), *other.as_bytecode(), answer);
        if (code != success) { why = "the two runs of codes could not be joined"; return code; }
        out = satelliteObject::of_bytecode(std::move(answer));
        return success;
    }

    default:
        return refuse_pair(*this, other, "+", why);
    }
}

// ---------------------------------------------------------------------------
// The five that are numbers only -- today, and `-`, which also takes a string
// from a string. Each keeps its own switch rather than sharing one, so that
// adding a float pair to `*` does not touch `-`.
// ---------------------------------------------------------------------------
signed long long int satelliteObject::subtract(const satelliteObject &other, satelliteObject &out,
                                               std::string &why) const
{
    // THE FAST PATH FIRST, which is the author's own design order: "we build all
    // the fast paths ... THEN we at last build the slower all encompassing" one.
    // Two numbers is the commonest thing this language does, and it was reaching
    // its fast path THIRD -- behind a percentage test and a by-worth test that
    // are both false whenever both sides are numbers. Hoisting is behaviour-
    // identical for exactly that reason: read_by_worth() needs one side BINARY,
    // and is_percentage() needs one side a percentage.
    if (pair_of(kind(), other.kind()) == pair_of(number, number))
        return run_number_pair(*this, other, number_and_number_subtract, "-", out, why);
    if (refuse_infinity_arithmetic(*this, other, "-", "INF-3", why))
        return not_built_yet;
    if (signed long long int code = success; answered_by_its_own_file('-', *this, other, out, why, code))
        return code;
    if (is_percentage() || other.is_percentage())
        return percentage_operation('-', worth_of(*this), worth_of(other), out, why);
    if (read_by_worth(*this, other))
        return worth_of(*this).subtract(worth_of(other), out, why);
    // THE FIRST OCCURRENCE OF THE RIGHT STRING IS TAKEN AWAY (the author, 2026-09-17:
    // "minus takes away the smallest string", "first occurrence").
    if (pair_of(kind(), other.kind()) == pair_of(string, string)) {
        satellite_string answer;
        const signed long long int code = string_and_string_subtract(*as_string(), *other.as_string(), answer);
        if (code != success) { why = "the string could not be taken away"; return code; }
        out = satelliteObject::of_string(std::move(answer));
        return success;
    }
    return refuse_pair(*this, other, "-", why);
}

signed long long int satelliteObject::multiply(const satelliteObject &other, satelliteObject &out,
                                               std::string &why) const
{
    // THE FAST PATH FIRST, which is the author's own design order: "we build all
    // the fast paths ... THEN we at last build the slower all encompassing" one.
    // Two numbers is the commonest thing this language does, and it was reaching
    // its fast path THIRD -- behind a percentage test and a by-worth test that
    // are both false whenever both sides are numbers. Hoisting is behaviour-
    // identical for exactly that reason: read_by_worth() needs one side BINARY,
    // and is_percentage() needs one side a percentage.
    if (pair_of(kind(), other.kind()) == pair_of(number, number))
        return run_number_pair(*this, other, number_and_number_multiply, "*", out, why);
    if (refuse_infinity_arithmetic(*this, other, "*", "INF-3 and INF-4", why))
        return not_built_yet;
    if (signed long long int code = success; answered_by_its_own_file('*', *this, other, out, why, code))
        return code;
    if (is_percentage() || other.is_percentage())
        return percentage_operation('*', worth_of(*this), worth_of(other), out, why);
    if (read_by_worth(*this, other))
        return worth_of(*this).multiply(worth_of(other), out, why);
    return refuse_pair(*this, other, "*", why);
}

signed long long int satelliteObject::divide(const satelliteObject &other, satelliteObject &out,
                                             std::string &why) const
{
    // THE FAST PATH FIRST, which is the author's own design order: "we build all
    // the fast paths ... THEN we at last build the slower all encompassing" one.
    // Two numbers is the commonest thing this language does, and it was reaching
    // its fast path THIRD -- behind a percentage test and a by-worth test that
    // are both false whenever both sides are numbers. Hoisting is behaviour-
    // identical for exactly that reason: read_by_worth() needs one side BINARY,
    // and is_percentage() needs one side a percentage.
    if (pair_of(kind(), other.kind()) == pair_of(number, number))
        return run_number_pair(*this, other, number_and_number_divide, "/", out, why);
    if (refuse_infinity_arithmetic(*this, other, "/", "INF-3 and INF-4", why))
        return not_built_yet;
    if (signed long long int code = success; answered_by_its_own_file('/', *this, other, out, why, code))
        return code;
    if (is_percentage() || other.is_percentage())
        return percentage_operation('/', worth_of(*this), worth_of(other), out, why);
    if (read_by_worth(*this, other))
        return worth_of(*this).divide(worth_of(other), out, why);
    return refuse_pair(*this, other, "/", why);
}

signed long long int satelliteObject::modulus(const satelliteObject &other, satelliteObject &out,
                                              std::string &why) const
{
    // THE FAST PATH FIRST, which is the author's own design order: "we build all
    // the fast paths ... THEN we at last build the slower all encompassing" one.
    // Two numbers is the commonest thing this language does, and it was reaching
    // its fast path THIRD -- behind a percentage test and a by-worth test that
    // are both false whenever both sides are numbers. Hoisting is behaviour-
    // identical for exactly that reason: read_by_worth() needs one side BINARY,
    // and is_percentage() needs one side a percentage.
    if (pair_of(kind(), other.kind()) == pair_of(number, number))
        return run_number_pair(*this, other, number_and_number_modulus, "%", out, why);
    if (signed long long int code = success; answered_by_its_own_file('%', *this, other, out, why, code))
        return code;
    if (is_percentage() || other.is_percentage())
        return percentage_operation('%', worth_of(*this), worth_of(other), out, why);
    if (read_by_worth(*this, other))
        return worth_of(*this).modulus(worth_of(other), out, why);
    return refuse_pair(*this, other, "%", why);
}

signed long long int satelliteObject::power(const satelliteObject &other, satelliteObject &out,
                                            std::string &why) const
{
    // THE FAST PATH FIRST, which is the author's own design order: "we build all
    // the fast paths ... THEN we at last build the slower all encompassing" one.
    // Two numbers is the commonest thing this language does, and it was reaching
    // its fast path THIRD -- behind a percentage test and a by-worth test that
    // are both false whenever both sides are numbers. Hoisting is behaviour-
    // identical for exactly that reason: read_by_worth() needs one side BINARY,
    // and is_percentage() needs one side a percentage.
    if (pair_of(kind(), other.kind()) == pair_of(number, number)) {
        const signed long long int code = run_number_pair(*this, other, number_and_number_power, "^", out, why);
        // A NUMBER TO A NEGATIVE POWER IS A FLOAT (SATELLITE_INFINITY.md FLT-2 and
        // Q13, 2026-09-22): `2 ^ -1` is 0.5, an answer that was refused for want of
        // one. Asked only on that refusal, so every whole answer keeps its path.
        if (code == answer_is_not_whole)
            return float_operation('^', *this, other, out, why);
        return code;
    }
    if (refuse_infinity_arithmetic(*this, other, "^", "INF-4 and INF-5", why))
        return not_built_yet;
    if (signed long long int code = success; answered_by_its_own_file('^', *this, other, out, why, code))
        return code;
    if (is_percentage() || other.is_percentage())
        return percentage_operation('^', worth_of(*this), worth_of(other), out, why);
    if (read_by_worth(*this, other))
        return worth_of(*this).power(worth_of(other), out, why);
    return refuse_pair(*this, other, "^", why);
}

// ---------------------------------------------------------------------------
// One ordering, six spellings read it (expression.cpp decides which may ask).
// ---------------------------------------------------------------------------
signed long long int satelliteObject::compare(const satelliteObject &other, int &order,
                                              std::string &why) const
{
    if (signed long long int code = success; compared_by_its_own_file(*this, other, order, why, code))
        return code;
    if (is_percentage() || other.is_percentage())
        return percentage_compare(*this, other, order, why);
    // AN INFINITY IS ORDERED AGAINST AN INFINITY AND AGAINST A NUMBER, and a binary
    // meets one by worth, as it meets a number (SATELLITE_INFINITY.md Part 3: the sign
    // of the first term of a - b). So `satellite.infinity() > 10 ^ 100` is true, and
    // `while (count < inf)` runs (Q27). Anything else it meets is refused below.
    if (is_infinity() || other.is_infinity()) {
        const auto worth = [](const satelliteObject &value) -> const satellite_number * {
            if (const satellite_binary_number *bits = value.as_binary()) return &bits->bits;
            return value.as_number();
        };
        if (is_infinity() && other.is_infinity()) {
            order = satellite_infinity::compare(as_infinity(), other.as_infinity());
            return success;
        }
        if (is_infinity() && worth(other) != nullptr) {
            order = satellite_infinity::compare(as_infinity(), *worth(other));
            return success;
        }
        if (worth(*this) != nullptr) {                     // and the other is the infinity
            order = -satellite_infinity::compare(other.as_infinity(), *worth(*this));
            return success;
        }
        return refuse_pair(*this, other, "a comparison", why);
    }
    // A binary against a number, by worth (read_by_worth says why). Two binaries
    // keep their own case below, where the width counts.
    if (read_by_worth(*this, other) && !(is_binary() && other.is_binary()))
        return worth_of(*this).compare(worth_of(other), order, why);

    switch (pair_of(kind(), other.kind())) {
    case pair_of(number, number):
        order = number_and_number_compare(*as_number(), *other.as_number());
        return success;
    case pair_of(string, string):
        order = string_and_string_compare(*as_string(), *other.as_string());
        return success;
    case pair_of(boolean, boolean):
        order = bool_and_bool_compare(*as_bool(), *other.as_bool());
        return success;
    // BY WORTH, AND THEN BY WIDTH, so `==` on two binaries is bits AND width
    // (003 DESIGN 8.5: `b0010 == b10` is false). `<` orders by worth, and only
    // two binaries of equal worth are ordered by width -- `b10 < b0010` -- because
    // one integer answers all six comparisons and it cannot be 0 for two values
    // that are not equal.
    case pair_of(binary, binary): {
        const satellite_binary_number &left = *as_binary();
        const satellite_binary_number &right = *other.as_binary();
        order = number_and_number_compare(left.bits, right.bits);
        if (order == 0 && left.width != right.width)
            order = left.width < right.width ? -1 : 1;
        return success;
    }
    default:
        return refuse_pair(*this, other, "a comparison", why);
    }
}

// ---------------------------------------------------------------------------
// THE CONVERSIONS. Explicit, never automatic, each through its own file.
// ---------------------------------------------------------------------------
signed long long int satelliteObject::to_string(satellite_string &out, std::string &why) const
{
    switch (kind()) {
    case boolean: return bool_to_string(*as_bool(), out);
    case number: return number_to_string(*as_number(), out);
    case string: out = *as_string(); return success;
    case percentage: {                     // "50%": exactly what display prints
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8(as_percentage()->written(), out, bad_offset);
    }
    // The b and the digits, exactly what display prints (003 DESIGN 8.5).
    case binary: {
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8(as_binary()->written(), out, bad_offset);
    }
    case capsule: {
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8(as_capsule()->name, out, bad_offset);
    }
    case nothing: {
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8("nothing", out, bad_offset);
    }
    // BYTECODE AS TEXT IS THE CONVERTERS' JOB, not this one's: PLAN M1.5-M3.6
    // decides whether a stored program is a token transcription that is re-lexed
    // or a 16-bit code stream, and printing it before that ruling would be
    // choosing the answer by accident (PROGRESS 6, still open).
    case bytecode:
        why = "bytecode has no text yet -- the converters (PLAN M1.5-M3.6) decide what it reads as";
        return not_built_yet;
    // AN OBJECT PRINTS ITSELF THROUGH A CAPSULE ITS SPACESUIT DECLARES, and that
    // capsule is the user's to write. Inventing a spelling here would give every
    // spacesuit one the author never chose.
    case user_defined:
        why = "an object has no to_string capsule, and satellite does not invent one";
        return not_built_yet;
    // A FILE'S TEXT IS read_all, and its name is path -- two different strings, so
    // `.string` choosing one of them would be a guess.
    case file:
        why = "a file has two strings, its text (read_all) and its name (path) -- write the one you mean";
        return types_do_not_meet;
    // A LIST READS BACK AS WHAT WAS TYPED: {1, "two", {3}}.
    //
    // THIS IS THE ONE PLACE A SPELLING IS CHOSEN RATHER THAN REFUSED, and the
    // reason it is not the same call the bytecode and spacesuit arms make. Those
    // two refuse because satellite has no syntax for them, so any text would be
    // invented. A list HAS syntax -- the author wrote it the same day -- so
    // echoing the literal is not a guess, it is the one spelling already decided.
    //
    // STRINGS INSIDE A LIST KEEP THEIR QUOTES, though `display("x")` prints x
    // without them. Inside a list they are what tells `{1}` from `{"1"}`, and a
    // person reading output that cannot tell those apart is reading output they
    // cannot trust.
    case list: {
        std::string written = "{";
        const satelliteList *held = as_list()->get();
        if (held != nullptr) {
            for (std::size_t at = 0; at < held->items.size(); ++at) {
                if (at != 0) written += ", ";
                const satelliteObject &item = held->items[at];
                satellite_string one;
                const signed long long int made = item.to_string(one, why);
                if (made != success)
                    return made;          // why already says which item and how
                if (item.is_string()) written += "\"" + one.to_utf8() + "\"";
                else written += one.to_utf8();
            }
        }
        written += "}";
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8(written, out, bad_offset);
    }
    // AN INDEX READS BACK AS ITS PAIRS, IN THE ORDER THEY WERE PUT IN -- which
    // is the whole point of it being a dict rather than a sorted map, so the
    // spelling has to show that order rather than tidy it away.
    case index: {
        std::string written = "{";
        const satelliteIndex *held = as_index()->get();
        if (held != nullptr) {
            for (std::size_t at = 0; at < held->entries.size(); ++at) {
                if (at != 0) written += ", ";
                const satelliteObject &key = held->entries[at].first;
                const satelliteObject &value = held->entries[at].second;
                satellite_string one;
                if (key.to_string(one, why) != success) return types_do_not_meet;
                written += key.is_string() ? "\"" + one.to_utf8() + "\"" : one.to_utf8();
                written += ": ";
                if (value.to_string(one, why) != success) return types_do_not_meet;
                written += value.is_string() ? "\"" + one.to_utf8() + "\"" : one.to_utf8();
            }
        }
        written += "}";
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8(written, out, bad_offset);
    }
    // AN INFINITY READS AS ITS ONE SET OF PARENTHESES, exactly what display prints:
    // (infinity), (-infinity), and one day (infinity, -500).
    case infinity: {
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8(satellite_infinity::display(as_infinity()), out, bad_offset);
    }
    // A WINDOW READS AS WHAT IT IS AND WHAT IS ON IT, because the one thing a
    // person displays a window for is to see WHICH window they have. Every other
    // piece reads as its own name and its words -- (button "ok"), (label "hello")
    // -- and the name comes from kPieceNames, so a widget added since this was
    // written displays as itself rather than as whatever the last `else` said.
    case window: {
        const satellite_window *which = as_window();
        std::string written;
        if (which == nullptr)
            written = "(no window)";
        else if (which->piece != satellite_window::window)
            // A PIECE WITH NOTHING TO SAY READS AS JUST ITSELF -- (switch), not
            // (switch ""). A switch has no words at all and an empty text box
            // has none yet, and a pair of empty quotes for both says less than
            // leaving them off.
            written = which->text.empty()
                          ? "(" + std::string(which->piece_shown()) + ")"
                          : "(" + std::string(which->piece_shown()) + " \"" + which->text + "\")";
        else
            written = std::string(which->on_the_screen ? "(window \"" : "(closed window \"") +
                      which->title + "\")";
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8(written, out, bad_offset);
    }
    case floating: return float_to_string(*as_float(), out, why);
    case hexadecimal: return hexadecimal_to_string(*as_hexadecimal(), out, why);
    case color: return color_to_string(*as_color(), out, why);
    case fraction: return fraction_to_string(*as_fraction(), out, why);
    // A THREAD READS AS WHICH CAPSULE AND HOW FAR IT HAS GOT: (thread work, running).
    case thread: {
        const satellite_thread *which = as_thread();
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8(which != nullptr ? which->shown() : std::string("(no thread)"), out,
                                           bad_offset);
    }
    case how_many_kinds: break;
    }
    why = "there is nothing here to make a string of";
    return types_do_not_meet;
}

signed long long int satelliteObject::to_number(satellite_number &out, std::string &why) const
{
    switch (kind()) {
    case number: out = *as_number(); return success;
    case binary: out = as_binary()->bits; return success;   // what the bits are worth; the width does not survive
    case string: {
        const signed long long int code = string_to_number(*as_string(), out);
        if (code != success)
            why = "that string is not a whole number this can read";
        return code;
    }
    // AN INFINITY IS LARGER THAN EVERY NUMBER, so there is no number to make of one
    // -- and cutting it to some very large number would be the wrong answer that does
    // not say so (SATELLITE_INFINITY.md INF-2).
    case infinity:
        why = "an infinity is larger than every number, so no number can be made out of it";
        return types_do_not_meet;
    case floating: return float_to_number(*as_float(), out, why);
    case hexadecimal: return hexadecimal_to_number(*as_hexadecimal(), out, why);
    case color: return color_to_number(*as_color(), out, why);
    case fraction: return fraction_to_number(*as_fraction(), out, why);
    default: break;
    }
    why = std::string("a number cannot be made out of ") + kind_name();
    return types_do_not_meet;
}

signed long long int satelliteObject::to_binary(satellite_string &out, std::string &why) const
{
    if (is_number())
        return number_to_binary(*as_number(), out);
    if (is_binary()) {                     // the digits as written: the width is kept
        std::size_t bad_offset = 0;
        return satellite_string::from_utf8(as_binary()->digits(), out, bad_offset);
    }
    if (is_float()) return float_to_binary(*as_float(), out, why);
    if (is_hexadecimal()) return hexadecimal_to_binary(*as_hexadecimal(), out, why);
    if (is_color()) return color_to_binary(*as_color(), out, why);
    if (is_fraction()) return fraction_to_binary(*as_fraction(), out, why);
    why = std::string("base 2 text cannot be made out of ") + kind_name();
    return types_do_not_meet;
}

signed long long int satelliteObject::to_hexadecimal(satellite_string &out, std::string &why) const
{
    if (is_number())
        return number_to_hexadecimal(*as_number(), out);
    if (is_binary())
        return number_to_hexadecimal(as_binary()->bits, out);
    if (is_float()) return float_to_hexadecimal(*as_float(), out, why);
    if (is_hexadecimal()) return hexadecimal_to_hexadecimal(*as_hexadecimal(), out, why);
    if (is_color()) return color_to_hexadecimal(*as_color(), out, why);
    if (is_fraction()) return fraction_to_hexadecimal(*as_fraction(), out, why);
    why = std::string("base 16 text cannot be made out of ") + kind_name();
    return types_do_not_meet;
}

} // namespace satellite004
