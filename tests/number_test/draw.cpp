// The uniform draw at arbitrary precision -- and satellite_random's first
// consumer.
//
// LAYOUT.md HAS CALLED satellite_random "THE ONE MODULE IN THE TREE WITH NO
// CONSUMER" SINCE IT LANDED, and 040-sources.mk says why: the half that could
// not be built at M2 was "drawing an N-digit number, which needs the
// arbitrary-precision half (M8)". This section is what closes that. It links
// both modules, which is the only place in the tree they meet.
//
// THE SKEW IS RE-MEASURED HERE RATHER THAN QUOTED. random.cpp records the first
// satellite's figures for what a bare `%` does -- 1398410 / 904307 / 697283
// against a uniform 998574 / 1001152 / 1000274, a 2:1 skew -- and PLAN §9's
// rule is to measure on this machine rather than repeat a number. So the
// comparison is run below against a generator whose output is known exactly,
// which is what makes the result a fact about the sampler rather than about a
// random seed.

#include "number_test.hpp"

#include "satellite_random/random.hpp"

#include <string>
#include <vector>

namespace number_test {

using satellite::Number;

namespace {

// splitmix32, which is the shape of driver random.hpp names: "the first
// satellite proved this seam is generator-agnostic by driving it with a
// home-grown splitmix32 stub in its tests. Keeping it means the PCG dependency
// is replaceable without touching anything above."
//
// SO THE UNIFORMITY MEASURED BELOW IS THE SAMPLER'S AND NOT PCG's, which is the
// point of testing through the seam rather than through the generator.
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

} // namespace

void section_draw()
{
    // --- what the sampler refuses, before drawing a bit ---------------------
    //
    // random.hpp's own note: a request the sampler will refuse must not spend
    // three seconds spinning first, so the checks are on the BOUND and happen
    // before any draw.
    Counting bits(7);
    Number out;
    check(!Number::random_below(of("0"), bits, out),
          "[0, 0) has nothing in it and is refused");
    check(!Number::random_below(of("-5"), bits, out),
          "a negative bound is refused");
    check(!Number::random_below(of("2.5"), bits, out),
          "a fractional bound is refused -- a draw is an integer");
    check(!Number::random_below(of("1e2000000000"), bits, out),
          "a bound of 2 billion digits is refused on its COUNT, before "
          "mul_pow10 is asked to build it");

    // MAX_RANDOM_DIGITS is satellite_random's and is not repeated in
    // satellite_number -- bignum_bigint.hpp's header note is why the v1 copy
    // did not come across. This is the assertion that they are one number: a
    // second copy that drifted would let one of these two through.
    check(satellite::MAX_RANDOM_DIGITS == 100000,
          "the ceiling is 100000 and lives in satellite_random/random.hpp");
    check(!Number::random_below(of("1e100001"), bits, out),
          "a bound wider than the ceiling is refused");
    check(Number::random_below(of("1e100000"), bits, out),
          "and 10^100000 is not -- the ceiling is on the DRAW, and every value "
          "under 10^100000 is at most 100000 digits, which is the +1 in "
          "random.cpp");

    // --- a draw is in range, and is a positive integer -----------------------
    for (int i = 0; i < 200; i++) {
        Counting source(static_cast<unsigned>(i * 7919));
        Number drawn;
        check(Number::random_below(of("1000"), source, drawn),
              "a draw below 1000 succeeds");
        check(Number::compare(drawn, of("0")) >= 0, "a draw is not negative");
        check(Number::compare(drawn, of("1000")) < 0, "a draw is below its bound");
        check(drawn.is_integer(), "a draw is a whole number");
        check(drawn.positive(),
              "a draw is positive, zero included -- random.cpp sets the flag "
              "rather than deriving it");
    }

    // A bound past what a 32-bit generator can express on its own, which is the
    // whole reason this code is in satellite_number rather than beside the
    // generator.
    Counting wide(1);
    Number big_draw;
    check(Number::random_below(
              of("100000000000000000000000000000000000000000"), wide, big_draw),
          "a 41-digit bound draws -- a generator's own bounded draw caps at "
          "2^32 and this is what the limb-aligned sampler is for");
    check(Number::compare(big_draw,
                          of("100000000000000000000000000000000000000000")) < 0,
          "and the answer is under it");

    // --- the skew, measured on this tree ------------------------------------
    //
    // WHAT THE REJECTION ACTUALLY PREVENTS, and it is one level in from where a
    // reader expects. random.cpp's draw_below() rejects because `%` on a raw
    // 32-bit word is biased whenever the bound does not divide 2^32 -- but for
    // a bound of 3 that is one extra preimage out of 4.29 billion and no test
    // could see it. The visible skew is the SECOND rejection, the one in
    // BigInt::random_below: the top limb is drawn over [0, top+1), and folding
    // that back with `% bound` instead of redrawing is what gives the 2:1.
    //
    // MEASURED HERE, 2026-08-31, over 3,000,000 draws with a splitmix32 stub --
    // the same shape of driver the first satellite used to prove the Bits32
    // seam is generator-agnostic, and not PCG, so the figures are about the
    // sampler rather than about a generator.
    constexpr unsigned kBound = 3;
    constexpr int kDraws = 3000000;

    long long fair[kBound] = {0, 0, 0};
    {
        SplitMix source(0x9e3779b9u);
        Number drawn;
        for (int i = 0; i < kDraws; i++) {
            if (!Number::random_below(of("3"), source, drawn)) {
                check(false, "a draw below 3 must succeed");
                break;
            }
            long long which = 0;
            drawn.to_integer(which);
            fair[which]++;
        }
    }

    // The same draw with the rejection replaced by a fold. `top + 1` is 4 for a
    // bound of 3, so values 0..3 become 0,1,2,0 and residue 0 gets twice the
    // share of the other two.
    long long folded[kBound] = {0, 0, 0};
    {
        SplitMix source(0x9e3779b9u);
        for (int i = 0; i < kDraws; i++)
            folded[(source.next() % (kBound + 1)) % kBound]++;
    }

    const long long expected = kDraws / kBound;
    for (unsigned i = 0; i < kBound; i++) {
        const long long off = fair[i] > expected ? fair[i] - expected
                                                 : expected - fair[i];
        check(off * 100 < expected,
              "bucket " + std::to_string(i) + " of the rejection sampler is "
              "within 1% of even -- " + std::to_string(fair[i]) + " against " +
                  std::to_string(expected));
    }

    // And the fold is not, by a factor of about two. The assertion is the
    // SHAPE -- one bucket far above the others -- rather than a figure, because
    // a figure would be an assertion about this stub's stream.
    check(folded[0] > folded[1] + folded[1] / 2 &&
              folded[0] > folded[2] + folded[2] / 2,
          "folding with `%` instead of redrawing skews the low residue by "
          "about 2:1 -- " + std::to_string(folded[0]) + " / " +
              std::to_string(folded[1]) + " / " + std::to_string(folded[2]) +
              ", which is the failure the compare-and-redraw in "
              "BigInt::random_below exists to stop");
}

} // namespace number_test
