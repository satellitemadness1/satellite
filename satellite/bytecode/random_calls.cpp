// satellite/bytecode/random_calls.cpp -- satellite.random's words. random_calls.hpp says
// what each shape draws and what is refused.

#include "random_calls.hpp"

#include "word_codes.hpp"
#include "../satellite_random/random_draw.hpp"
#include "../satellite_random/random_spin.hpp"

namespace satellite004 {

namespace {

using token::Code;

// THE EIGHTEEN ROWS ARE ONE RUN OF CODES: words.tsv lists `1 7` through `1 7 16` one after
// another, so a word's code is its row and the run is a range. A new word under
// satellite.random would be appended at the END of words.tsv (a code is a row, never
// renumbered) and would need naming here; the static_asserts say so the day one is added.
constexpr Code kRandom = word::fixed_code<1, 7>;
constexpr Code kSeededBare = word::fixed_code<1, 7, 16>;
static_assert(kSeededBare - kRandom == 17, "satellite.random's eighteen rows are one run of codes");
static_assert(word::fixed_code<1, 7, 1> == kRandom + 2 && word::fixed_code<1, 7, 12> == kRandom + 13,
              "1 7 n is kRandom + 1 + n");

enum class Shape { word, bare, digits, range, stepped, seeded };

struct RandomWord {
    RandomTier tier;
    Shape shape;
    std::size_t takes;
};

// 1 7 n, by n -- the author's table: 1-3 the bare grades, 4-6 fast's digits, range and
// step, 7-9 normal's, 10-12 ultra's, 13-16 seeded. THE COUNT OF NUMBERS DECIDES THE SHAPE
// (the author, 2026-10-02: "if the user enters two numbers, then it's a min and a max, and
// if they enter 1 number it's the number of digits"): the lexer picks the row by the call's
// own commas (bytecode_registry.cpp), so one word with one number IS the digits row and the
// same word with two IS the range row, whatever the numbers are -- literals or names.
RandomWord random_word_of(Code code)
{
    const unsigned index = static_cast<unsigned>(code - kRandom);
    if (index < 2)
        return {RandomTier::fast, Shape::word, 0};
    const unsigned n = index - 1;
    if (n <= 3)
        return {static_cast<RandomTier>(n - 1), Shape::bare, 0};
    if (n <= 12) {
        const unsigned k = n - 4;
        return {static_cast<RandomTier>(k / 3), static_cast<Shape>(static_cast<int>(Shape::digits) + k % 3), k % 3 + 1};
    }
    return {RandomTier::fast, Shape::seeded, 0};
}

std::string grade_spelled(RandomTier tier)
{
    return std::string("satellite.random.") + random_tier_name(tier);
}

const char *shape_spelled(Shape shape)
{
    return shape == Shape::digits ? "(digits)" : shape == Shape::range ? "(min, max)" : "(min, max, step)";
}

// What the spin throws away: one draw of the very shape the answer will have.
struct Throwaway {
    LimbSource *limbs;
    const DrawShape *shape;
};

void throw_one_away(void *at)
{
    const Throwaway &work = *static_cast<const Throwaway *>(at);
    (void)draw(*work.limbs, *work.shape);
}

} // namespace

bool is_random_word(Code code)
{
    return code >= kRandom && code <= kSeededBare;
}

signed long long int random_word_refused(Code code, std::size_t given, std::string &why)
{
    if (!is_random_word(code))
        return success;
    const RandomWord word = random_word_of(code);
    const std::string grade = grade_spelled(word.tier);
    switch (word.shape) {
    case Shape::word:
        why = "satellite.random is not a call on its own -- pick a grade and call that: "
              "satellite.random.fast(1, 6), satellite.random.normal(20) or satellite.random.ultra(0, 100, 5)";
        return satl_line_not_understood;
    case Shape::seeded:
        why = "satellite.random.seeded is not built yet -- its shape is the author's to settle "
              "(SCRATCH.md/RANDOM.md); satellite.random.fast, normal and ultra draw";
        return not_built_yet;
    case Shape::bare:
        // 003's ruling (2026-09-04), in its help line's words.
        why = grade + "() draws nothing -- a random number with no width and no bounds is not a question with "
              "an answer. Give it one number for a count of digits, two for a min and a max, or three for a "
              "min, a max and a step: " + grade + "(20), " + grade + "(1, 6) or " + grade + "(0, 100, 5)";
        return random_needs_a_shape;
    default:
        break;
    }
    if (given != word.takes) {
        why = grade + " takes one number (a count of digits), two (a min and a max) or three (a min, a max and "
              "a step), and was given " + std::to_string(given);
        return satl_line_not_understood;
    }
    return success;
}

Value call_random_word(Code code, const std::vector<Value> &arguments, ExpressionContext &context)
{
    std::string why;
    if (const signed long long int refused = random_word_refused(code, arguments.size(), why); refused != success) {
        context.refuse(refused, why);
        return Value();
    }
    const RandomWord word = random_word_of(code);
    const std::string spelled = grade_spelled(word.tier) + shape_spelled(word.shape);

    // EVERY ARGUMENT A WHOLE NUMBER. A float or a fraction is refused and told why in the
    // S431 sentence; a binary or a hex holds a whole number and is told the word for it.
    const satellite_number *given[3] = {nullptr, nullptr, nullptr};
    for (std::size_t i = 0; i < arguments.size() && i < 3; ++i) {
        given[i] = arguments[i].as_number();
        if (given[i] == nullptr) {
            const bool holds_one = arguments[i].is_binary() || arguments[i].is_hexadecimal();
            context.refuse(random_wants_whole_numbers,
                           spelled + " takes whole numbers, and was given " + arguments[i].kind_name() +
                               (holds_one ? " -- its .number is the whole number it holds" : ""));
            return Value();
        }
    }

    // THE SHAPE, WORKED OUT ONCE, after every check it needs (random_draw.hpp).
    DrawShape shape;
    switch (word.shape) {
    case Shape::digits: {
        const satellite_number &digits = *given[0];
        if (digits.negative()) {
            context.refuse(random_wants_whole_numbers,
                           spelled + " -- a count of digits is 0 or more, and was given " + digits.to_text());
            return Value();
        }
        shape = digits_shape(digits);
        break;
    }
    case Shape::range: {
        const satellite_number &least = *given[0], &most = *given[1];
        if (most < least) {
            context.refuse(random_range_empty, spelled + " -- min " + least.to_text() + " is above max " +
                                                   most.to_text() + ", so there is nothing between them to draw");
            return Value();
        }
        shape = range_shape(least, most);
        break;
    }
    case Shape::stepped: {
        const satellite_number &least = *given[0], &most = *given[1], &step = *given[2];
        if (step.negative() || step.is_zero()) {
            context.refuse(random_step_not_a_step, spelled + " -- a step is 1 or more, and was given " + step.to_text());
            return Value();
        }
        if (most < least) {
            context.refuse(random_range_empty, spelled + " -- min " + least.to_text() + " is above max " +
                                                   most.to_text() + ", so there is nothing between them to draw");
            return Value();
        }
        // THE STEP LANDS ON MAX OR THE CALL IS REFUSED, naming the last value it reaches
        // (003's rule, 2026-09-04): `fast(0, 100, 7)` says it may answer 100 and never would.
        satellite_number places, remainder;
        satellite_number::divide(most - least, step, places, remainder);
        if (!remainder.is_zero()) {
            context.refuse(random_step_misses, spelled + " -- counting from " + least.to_text() + " a step of " +
                                                   step.to_text() + " reaches " + (most - remainder).to_text() +
                                                   " and then passes " + most.to_text() +
                                                   "; make max a value the step lands on, or change the step");
            return Value();
        }
        shape = stepped_shape(least, most, step);
        break;
    }
    default:
        context.refuse(error, spelled + " reached no shape, which is a fault in satellite");
        return Value();
    }

    // THE SPIN, AFTER EVERY CHECK (random_spin.hpp): the author's windows, draws of this very
    // shape thrown away for the span drawn inside them -- then the answer, the same shape.
    LimbSource &limbs = random_source();
    Throwaway work{&limbs, &shape};
    spin(word.tier, limbs, &throw_one_away, &work);
    return Value::of_number(draw(limbs, shape));
}

} // namespace satellite004
