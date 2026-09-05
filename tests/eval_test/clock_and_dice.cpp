// M13's rows: the three tiers by four shapes, the two time rows, and the two
// sources of nondeterminism behaving under PLAN M13's done-when clauses.
//
// TWO LAYERS TESTED TWO WAYS, tiers.hpp's own split. The distribution claims
// -- both ends reachable, the short answers happening, the step set closed --
// run a HUNDRED draws through draw_* with the same splitmix32 stub
// tests/number_test/draw.cpp drives the sampler with, so they cost
// microseconds and are deterministic under a fixed seed. The language claims
// -- the alias carrying a real call, the refusals with their codes, the tier
// floors, the pacing -- run through build() and the real rows, spins
// included, which is why this section is the slow one in the suite: THREE
// SECONDS OF THAT IS THE ULTRA FLOOR BEHAVING, clause 3's floor-only rule --
// a ceiling is a machine's to miss under load, so no ceiling is asserted.
//
// THE SIGNAL HALF OF sleep IS NOT HERE, for interrupted.cpp's reason run the
// other way round: sleep wakes on EINTR, EINTR needs a real signal, and a
// fixture that forked and killed would be testing the kernel's delivery. The
// real-terminal transcript is in MILESTONES/M13.md.
//
// THE TABLE IS PROCESS-WIDE, SO THIS SECTION PUTS IT BACK EMPTY -- the same
// contract every install-the-real-rows section keeps.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"
#include "evaluator/dispatch.hpp"
#include "satellite_random/handlers.hpp"
#include "satellite_random/tiers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_time/handlers.hpp"

#include <chrono>
#include <string>

namespace eval_test {

namespace {

using satellite::Number;

// number_test/draw.cpp's stub, character for character -- the seam's proof
// that the tier tests below are about the shapes and not about PCG.
class SplitMix : public satellite::Bits32 {
public:
    explicit SplitMix(unsigned seed) : state_(seed) {}

    unsigned next() override
    {
        state_ += 0x9e3779b9u;
        unsigned z = state_;
        z = (z ^ (z >> 16)) * 0x21f0aaadu;
        z = (z ^ (z >> 15)) * 0x735a2d97u;
        return z ^ (z >> 15);
    }

private:
    unsigned state_;
};

std::string capsule(const std::string &body)
{
    return "satellite.capsule it()\n{\n" + body + "}\n";
}

std::string answers(const std::string &body)
{
    Run run;
    build(capsule(body), run);
    if (!run.built)
        return "<did not compile>";
    return answer_of(run, "it", {});
}

bool refused_with(const std::string &body, satellite::errors::Code code)
{
    Run run;
    build(capsule(body), run);
    if (!run.built)
        return false;
    call(run, "it", {});
    return ran_into(code);
}

long long milliseconds_to_answer(const std::string &body)
{
    const auto before = std::chrono::steady_clock::now();
    answers(body);
    const auto after = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(after - before)
        .count();
}

// PLAN M13 done-when clause 1, through the seam: uniform over [0, 10^40)
// means about one draw in ten renders shorter than 40 digits, and the test
// asserts the short answers HAPPEN -- one that merely tolerated them "is not
// the test §11 asked for". Deterministic under the fixed seed.
void digit_draws_run_short()
{
    SplitMix bits(20260904);
    int shorter = 0;
    for (int i = 0; i < 100; i++) {
        Number drawn;
        check(satellite::draw_digits(bits, 40, drawn),
              "a 40-digit draw answers");
        if (drawn.to_string().size() < 40)
            shorter++;
    }
    check(shorter >= 1, "of a hundred 40-digit draws, some render shorter -- "
                        "a leading zero is not printed and uniform means one "
                        "draw in ten has one");
    check(shorter <= 40, "and not most of them -- the short answers are the "
                         "tenth, not the rule");
}

// Clause 2's both-ends proof, and clause 11's step set, through the seam.
void both_ends_and_the_step_set()
{
    SplitMix bits(42);

    bool saw_low = false;
    bool saw_high = false;
    for (int i = 0; i < 100; i++) {
        Number drawn;
        check(satellite::draw_range(bits, Number(1), Number(2), drawn),
              ".range(1, 2) answers");
        if (drawn == Number(1))
            saw_low = true;
        if (drawn == Number(2))
            saw_high = true;
    }
    check(saw_low && saw_high,
          ".range is inclusive at BOTH ends -- a hundred coin flips see both "
          "faces, or the +1 is missing");

    bool seen[4] = {false, false, false, false};
    for (int i = 0; i < 100; i++) {
        Number drawn;
        check(satellite::draw_step(bits, Number(1), Number(10), Number(3),
                                   drawn),
              "the step shape answers");
        const Number offset = Number::sub(drawn, Number(1));
        bool member = false;
        for (int k = 0; k < 4; k++) {
            if (drawn == Number(1 + 3 * k)) {
                seen[k] = true;
                member = true;
            }
        }
        check(member, "fast(1, 10, 3) answers 1, 4, 7 or 10 and nothing else "
                      "-- got " +
                          drawn.to_string() + ", offset " + offset.to_string());
    }
    check(seen[0] && seen[1] && seen[2] && seen[3],
          "and all four members appear across a hundred draws");
}

// The language surface: the rows installed, the alias carrying a real call,
// and every refusal answering its S09xx row. `.range(7, 7)` is the one range
// call whose answer is checkable, and it is also clause 2's degenerate end.
void the_rows_answer()
{
    using satellite::errors::Code;

    check(answers("    satellite.return("
                  "satellite.random.fast.range(7, 7))\n") == "7",
          ".range(7, 7) is 7 -- M2's alias rewrite carries a real call");
    check(answers("    satellite.return("
                  "satellite.random.fast(0))\n") == "0",
          "fast(0) is uniform over [0, 10^0), which is exactly 0");

    check(refused_with("    satellite.return(satellite.random.fast())\n",
                       Code::RANDOM_NEEDS_A_SHAPE),
          "the zero-argument call is a refusal by design, the author's text");
    check(refused_with("    satellite.return(satellite.random.ultra)\n",
                       Code::RANDOM_NEEDS_A_SHAPE),
          "and the bare spelling folds to the same number and the same text");
    check(refused_with("    satellite.return("
                       "satellite.random.fast(\"x\"))\n",
                       Code::RANDOM_NOT_A_DIGIT_COUNT),
          "fast(\"x\") wants a whole number of digits");
    check(refused_with("    satellite.return(satellite.random.fast(1.5))\n",
                       Code::RANDOM_NOT_A_DIGIT_COUNT),
          "fast(1.5) shares fast(\"x\")'s text -- clause 5's first pair");
    check(refused_with("    satellite.return(satellite.random.fast(0 - 1))\n",
                       Code::RANDOM_DIGITS_OUT_OF_RANGE),
          "fast(0 - 1) is out of the drawable range");
    check(refused_with("    satellite.return("
                       "satellite.random.fast(100001))\n",
                       Code::RANDOM_DIGITS_OUT_OF_RANGE),
          "fast(100001) shares its text -- clause 5's second pair");
    check(refused_with("    satellite.return("
                       "satellite.random.fast(1, \"x\"))\n",
                       Code::RANDOM_WANTS_NUMBERS),
          ".range with a string wants two number arguments");
    check(refused_with("    satellite.return("
                       "satellite.random.fast(1, 6.5))\n",
                       Code::RANDOM_WANTS_WHOLE_NUMBERS),
          ".range with a fraction wants whole numbers");
    check(refused_with("    satellite.return("
                       "satellite.random.fast.range(10, 1))\n",
                       Code::RANDOM_RANGE_EMPTY),
          ".range(10, 1) is empty and says so");
    check(refused_with("    satellite.return("
                       "satellite.random.fast(1, 10, 0))\n",
                       Code::RANDOM_STEP_NOT_A_STEP),
          "a step of 0 steps nowhere");
    check(refused_with("    satellite.return("
                       "satellite.random.fast(1, 10, 4))\n",
                       Code::RANDOM_STEP_MISSES),
          "fast(1, 10, 4) is refused -- the step lands on 9 and never 10");

    check(refused_with("    satellite.time.sleep(\"x\")\n",
                       Code::EVAL_WRONG_TYPE),
          "sleep of a string is a type sentence, S0713's");
    check(refused_with("    satellite.time.sleep(0 - 1)\n",
                       Code::TIME_NOT_A_LENGTH),
          "sleep of -1 is not a length of time");
}

// Clause 6: two asks, two values -- the one-line proof the representation is
// an integer. And what an instant can DO at M13: be displayed (ISO-8601, the
// 'Z' a fact about the type), be held by a typed name or a variant, and give
// `holding` its sixth word.
void the_clock_answers()
{
    check(answers("    satellite.return("
                  "satellite.time.now == satellite.time.now)\n") == "false",
          "now twice is two different values -- 61 bits of epoch against 21 ns "
          "of granularity");

    const std::string shown =
        answers("    satellite.return(satellite.time.now)\n");
    check(shown.size() == 30 && shown.find('T') == 10 &&
              shown.back() == 'Z' && shown.find('.') == 19,
          "an instant displays as ISO-8601 with all nine fractional digits -- "
          "got `" + shown + "`");

    check(answers("    satellite.variable.variant box\n"
                  "    box = satellite.time.now\n"
                  "    satellite.return(box.holding())\n") == "time",
          "a variant holds an instant and `holding` answers its sixth word");
    check(answers("    satellite.variable.time t = satellite.time.now\n"
                  "    satellite.variable.time u = t\n"
                  "    satellite.return(t == u)\n") == "true",
          "a copied instant is the same instant");
}

// Clause 3, floor only, and clause 7's pacing. The window's minimum is the
// one number a loaded build box cannot take away.
void the_floors_hold()
{
    const long long fast =
        milliseconds_to_answer("    satellite.return(satellite.random.fast(2))\n");
    check(fast >= 50, "fast spins at least its 50 ms floor -- took " +
                          std::to_string(fast) + " ms");

    const long long normal = milliseconds_to_answer(
        "    satellite.return(satellite.random.normal(2))\n");
    check(normal >= 500, "normal spins at least its 500 ms floor -- took " +
                             std::to_string(normal) + " ms");

    const long long ultra = milliseconds_to_answer(
        "    satellite.return(satellite.random.ultra(40))\n");
    check(ultra >= 2000, "ultra spins at least its 2000 ms floor -- took " +
                             std::to_string(ultra) + " ms");

    const long long paced = milliseconds_to_answer(
        "    satellite.variable.number i = 0\n"
        "    satellite.statement.while (i < 3)\n"
        "    {\n"
        "        satellite.time.sleep(0.05)\n"
        "        i = i + 1\n"
        "    }\n"
        "    satellite.return(i)\n");
    check(paced >= 150, "three sleep(0.05) pace at least 150 ms -- took " +
                            std::to_string(paced) + " ms, the unit is seconds");
}

} // namespace

void section_clock_and_dice()
{
    satellite::random::install_handlers();
    satellite::time::install_handlers();
    satellite::scalars::install_handlers();

    digit_draws_run_short();
    both_ends_and_the_step_set();
    the_rows_answer();
    the_clock_answers();
    the_floors_hold();

    satellite::eval::Handlers::table().clear();
}

} // namespace eval_test
