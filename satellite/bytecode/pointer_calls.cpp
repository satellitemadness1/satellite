// satellite/bytecode/pointer_calls.cpp -- the header says what every object answers.

#include "pointer_calls.hpp"

#include "word_codes.hpp"
#include "../satellite_object/satellite_spacesuit.hpp"

namespace satellite004 {

std::string object_is_gone_because(const std::string &written, const std::string &receiver)
{
    return written + " -- " + receiver + " points at an object that is gone: nothing else held it, and a "
                                         "pointer does not keep its object living";
}

// ---------------------------------------------------------------------------
// AN OBJECT'S FIVE WORDS RUN BY THEIR NUMBERS (the author, 2026-10-11: "we have to give everything a
// number and then make it run those numbers"). Each is satellite.spacesuit.<it>(), 1 10 n -- the
// spacesuit is what an object is an object of, and satellite.class its other spelling -- whose code is
// a file under satellite-numbers/words/.
// ---------------------------------------------------------------------------

// EVERY NUMBER BELOW IS IN words/words.tsv, OR THIS FILE DOES NOT COMPILE: a fixed_code of 0 would
// quietly leave that word with no number, and `.satellite.` before it refused.
template <int... Last>
inline constexpr bool every_object_word_numbered = ((word::fixed_code<1, 10, Last> != 0) && ...);
static_assert(every_object_word_numbered<1, 2, 3, 4, 5>, "an object's word's number is missing from words/words.tsv");

token::Code object_number_of(token::Code method)
{
    switch (method) {
    case token::pointer_token: return word::fixed_code<1, 10, 1>;
    case token::reference_token: return word::fixed_code<1, 10, 2>;
    case token::ok_token: return word::fixed_code<1, 10, 3>;
    case token::lock_token: return word::fixed_code<1, 10, 4>;
    case token::unlock_token: return word::fixed_code<1, 10, 5>;
    default: return 0;
    }
}

Value run_object_number(token::Code method, token::Code number, const std::vector<std::bitset<16>> &row,
                        std::size_t &at, const Value &object, const std::string &receiver, std::size_t dot,
                        ExpressionContext &context)
{
    const auto code_here = [&row](std::size_t k) {
        return k < row.size() ? static_cast<token::Code>(row[k].to_ulong()) : token::Code(0);
    };
    // THE CALL AS WRITTEN, spelled only when a refusal says it: a call that is right costs no text.
    const auto written = [&receiver, method]() {
        return receiver + ".satellite." + token::method_name_of(method);
    };
    // EACH TAKES NOTHING, IN ITS BRACKETS -- said before anything else, as it always was, and with
    // nothing inside them worked out.
    if (code_here(at) != token::left_parenthesis_token || code_here(at + 1) != token::right_parenthesis_token) {
        context.refuse(satl_line_not_understood, written() + "() takes nothing, in its brackets", dot);
        return Value();
    }
    at += 2;

    // THE NUMBER RUNS: table[code], one subtraction and one load (function_table.hpp), and the
    // function its file under satellite-numbers/words/ put in the row at start-up. A refusal it says
    // points at the period, as every refusal of these words always has.
    const NumberRow *its_row = context.functions.method(number);
    if (its_row == nullptr || its_row->scenarios.method_call == nullptr) {
        context.refuse(not_built_yet, written() + "() is not built for an object yet", dot);
        return Value();
    }
    const auto run = [&]() {
        Value answer = its_row->scenarios.method_call(object, nullptr, nullptr, {}, true, receiver, context);
        if (context.code != success && !context.placed) {
            context.refused_at = dot;
            context.placed = true;
        }
        return answer;
    };

    // ASKED OF A POINTER BEFORE ITS OBJECT IS HELD, because a pointer whose object is gone still
    // answers: ok() is false, pointer() is the pointer, and reference() says the object is gone. The
    // pointer is asked about first -- one compare, and almost every call is on an object.
    if (object.is_pointer() && every_object_answers(method))
        return run();

    // THE OBJECT IS HELD HERE, BY A HANDLE OF ITS OWN, for the whole call -- a pointer's object too,
    // so it cannot go while .satellite.lock() turns its switch.
    const UserDefinedHandle held = object_behind(object);
    if (held == nullptr || held->layout == nullptr) {
        if (object.is_pointer()) {
            context.refuse(object_is_gone, object_is_gone_because(written(), receiver), dot);
            return Value();
        }
        context.refuse(satl_line_not_understood, receiver + " holds no object yet, so it has no " +
                                                     token::method_name_of(method) +
                                                     " -- a field of a spacesuit's type starts empty until something "
                                                     "gives it one", dot);
        return Value();
    }
    return run();
}

} // namespace satellite004
