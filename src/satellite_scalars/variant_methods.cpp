// The four `satellite.variable.variant` methods, `1 6 14 1` through
// `1 6 14 4` -- PLAN M12. See satellite_scalars/handlers.hpp for what installs
// and methods_internal.hpp for the shape every row shares.
//
// A VARIANT IS THE `Value` ITSELF, AND THAT IS THE WHOLE REPRESENTATION.
// DESIGN §8's table carried a dash in the variant's representation column until
// this milestone; §8.7 now says what fills it: M9's discriminated union IS the
// representation, with no arm of its own, no handle and no bytes. A slot
// declared `variant` holds whatever `Value` it holds, and the declared type
// does here the one thing a declared type does anywhere in this language --
// it numbers the selectors. These four are what it numbers.
//
// THESE ARE THE ONLY ROWS IN THE TABLE WITH NO RECEIVER CHECK, and that is
// DESIGN §8.7's answer made executable rather than an omission. "Nothing" is a
// STATE EVERY TYPE HAS -- reading one of PLAN §8's M12 blocker -- so any value
// at all, the nothing state included, is a legal thing for a variant slot to be
// holding, and a receiver that cannot be of a wrong type has nothing for a
// string_at-shaped helper to refuse. The one refusal in this file is `held`'s,
// and it is about ABSENCE, not type: S0714, DESIGN §6.4 qualification 3's
// second non-dispatchable state, asked for by name at the line it happened.
//
// THE WORDS ARE THE ARMS, AND THE LIST GROWS WITH THE VARIANT ITSELF. `holding`
// answers one of five words today -- "nothing", "bool", "number", "string",
// "satellite" -- because value.hpp's variant has five arms today; the day a
// later milestone appends an arm (the file handle at M19, the float behind its
// handle at M15), word_of() below is the ONE place the vocabulary widens, and
// `holds` loosens with it. That is errors.def's block-note argument run
// forwards: `holds("float")` refuses today and can loosen into an answer the
// day a variant can actually hold one, while a false answered today would be a
// sentence the language could never take back.
//
// AND `holds` REFUSES A WORD OFF THE LIST RATHER THAN ANSWERING false, which
// is DESIGN §1.1 at its cheapest possible site. `box.holds("strng")` answering
// false forever is a condition no program can ever satisfy wearing a working
// test's clothes; the refusal names the five words and the caret names the
// line. `box.holding() == "strng"` still answers false -- equality is not a
// question about the vocabulary -- and `holds` existing is what makes writing
// that unnecessary.

#include "satellite_scalars/methods_internal.hpp"

#include "error_reporter/report.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::scalars {

namespace {

// The word for what a value is, ONE ARM EACH, in value.hpp's arm order. The
// answers a program compares against, so they are the READING of each type --
// the last path segment, the way display prints a bool as "true" and the
// runtime as "satellite" -- and not error_reporter's sentence fragments:
// type_name() says "the satellite runtime" because its callers already name a
// wanted type, and a `holding` answer stands alone.
const char *word_of(const Value &value)
{
    if (value.is_nothing())
        return "nothing";
    if (value.is_bool())
        return "bool";
    if (value.is_number())
        return "number";
    if (value.is_string())
        return "string";
    return "satellite";
}

// The legal words of `holds(x)`, which are exactly the words word_of() can
// answer -- one vocabulary, held in one place, per the file note.
bool a_word_an_arm_answers(const std::string &word)
{
    return word == "nothing" || word == "bool" || word == "number" ||
           word == "string" || word == "satellite";
}

const char *kWantedWord =
    "a word the variant could be holding -- \"nothing\", \"bool\", "
    "\"number\", \"string\" or \"satellite\"";

// --- the four, in numbering order -------------------------------------------

// `holding` `1 6 14 1` -- what are you holding, as the word. Never refuses:
// the nothing state is an ANSWER here, which is the whole reason M12 exists in
// front of M14's `typed()` and M19's `read_line`.
bool variant_holding(eval::Machine &, const Value *a, uint32_t, Value *answer)
{
    *answer = Value::string(encode(word_of(a[0])));
    return true;
}

// `holds(x)` `1 6 14 2` -- the same question as yes or no.
bool variant_holds(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Value &asked_for = a[1];
    const Str *text = std::get_if<Str>(&asked_for);
    if (text == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), std::string(m.text_of(m.here())), kWantedWord,
            type_name(asked_for)));
        return false;
    }
    const std::string word = *text ? decode(**text) : std::string();
    if (!a_word_an_arm_answers(word)) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), std::string(m.text_of(m.here())), kWantedWord,
            "\"" + word + "\""));
        return false;
    }
    *answer = Value::boolean(word == word_of(a[0]));
    return true;
}

// `held` `1 6 14 3` -- the value itself, or S0714 by name. Plain assignment
// out of a variant already copies whatever it holds, nothing included, because
// nothing is a state every slot may be in; `held` exists for the program that
// means "there is something in here, and stop me at this line if not". The
// refusal is the loosenable half of that sentence and the answer is the other.
bool variant_held(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    if (a[0].is_nothing()) {
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), std::string(m.text_of(m.here()))));
        return false;
    }
    *answer = a[0];
    return true;
}

// `clear` `1 6 14 4` -- the receiver becomes nothing, the same word and the
// same one-value-two-destinations contract as the string's `1 6 1 15`: the
// answer IS the receiver's new value and the op writes it back to the slot.
// Clearing a variant already holding nothing answers nothing, not a refusal --
// the method promises a state, not a transition.
bool variant_clear(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::nothing();
    return true;
}

} // namespace

void install_variant_methods()
{
    using words::NodeId;
    eval::Handlers &table = eval::Handlers::table();

    // Receiver bound and counted, exactly as the string and number halves do
    // it. The one mutating row is `clear`, and §6.4's storage-slot rule is the
    // op's to enforce, not this install's to remember.
    struct Row {
        NodeId path;
        eval::HandlerFn fn;
        uint32_t arity;
        bool mutates;
    };
    static constexpr Row rows[] = {
        {NodeId::VARIABLE_VARIANT_HOLDING, variant_holding, 1, false},
        {NodeId::VARIABLE_VARIANT_HOLDS,   variant_holds,   2, false},
        {NodeId::VARIABLE_VARIANT_HELD,    variant_held,    1, false},
        {NodeId::VARIABLE_VARIANT_CLEAR,   variant_clear,   1, true},
    };
    for (const Row &row : rows)
        table.install(static_cast<words::PathId>(row.path),
                      {row.fn, true, row.arity, "M12", row.mutates});
}

} // namespace satellite::scalars
