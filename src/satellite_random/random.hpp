#pragma once

// satellite.random -- the three tiers, the spin, and the 32-bit seam.
//
// DESIGN §11 is the specification. Three tiers that differ in NOTHING a program
// can see except how long they take, and §11.1 is the refusal that travels with
// them: PCG makes no cryptographic claim, its state is recoverable from its
// output, and no tier here is described as secure. `ultra` is slower and better
// distributed. It is not a CSPRNG.
//
// THIS LANDS AHEAD OF ITS MILESTONE, like M1.5 did. `satellite.random` is in
// PLAN §8's "Later" pile, which is not a milestone -- SCRATCH.md/MILESTONE.md
// counts its 16 paths among the unscheduled. What is here is the half that can
// be built and tested before `satellite_number` is ported: the generator, the
// seam, and the spin policy. The half that cannot is drawing an N-digit number,
// which needs the arbitrary-precision half (M8).

#include <cstdint>
#include <memory>

namespace satellite {

// The 32-bit seam, and it is deliberately two lines wide.
//
// Uniform over the WHOLE 32-bit range. A generator that cannot promise that is
// not one the sampler's uniformity argument holds for -- the arbitrary-precision
// side does rejection sampling per limb and that argument is what it rests on.
//
// The first satellite proved this seam is generator-agnostic by driving it with
// a home-grown splitmix32 stub in its tests. Keeping it means the PCG dependency
// is replaceable without touching anything above.
class Bits32 {
public:
    virtual ~Bits32();
    virtual unsigned next() = 0;
};

enum class Tier { fast, normal, ultra };

// The throwaway window, in milliseconds, inclusive at both ends. DESIGN §11's
// table, and the ONLY thing that separates the three tiers.
struct Window {
    long long minimum_ms;
    long long maximum_ms;
};

Window window_for(Tier tier);

// The largest number anyone may ask for, in decimal digits.
//
// CHECKED BEFORE ANY BIT IS DRAWN, which is the whole point of having it here
// rather than inside the draw. A request the sampler will refuse -- a fractional
// size, or one wider than this -- must not spend three seconds spinning first.
// The first satellite settled on this number and said why: one draw at the
// ceiling is about 11k limbs and 44 KB that a program then has to do something
// with. The arithmetic underneath is exact at any size, so the limit is about
// what a caller can use rather than what the generator can compute.
inline constexpr long long MAX_RANDOM_DIGITS = 100000;

// A PCG-backed source of 32-bit words.
//
// pcg32_k16384 -- ext_setseq_xsh_rr_64_32<14,16,true>, a 16384-word extension
// array. Seeded from std::random_device through pcg-cpp's seed_seq_from, which
// fills the whole table: about half a million bits of kernel entropy, and about
// 1.6 ms to do it. See pcg/README.md for why this exact type and what it costs.
//
// The pcg type is not named in this header on purpose, so that nothing above
// this line includes an Apache-2.0 header. pcg/README.md §"The licence problem"
// is why that is worth one pointer indirection.
class Source : public Bits32 {
public:
    Source();
    explicit Source(std::uint64_t seed);
    ~Source() override;

    Source(const Source &) = delete;
    Source &operator=(const Source &) = delete;

    unsigned next() override;

private:
    struct State;
    std::unique_ptr<State> state_;
};

// Spin: throw work away for a while, then let the caller have the next one.
//
// `draw_and_discard` must generate a number of EXACTLY the size the caller is
// about to ask for and throw it away. DESIGN §11: the discarded work is the same
// work as the answer, which is what makes the window mean anything. Discarding
// raw 32-bit words instead would make the spin cheaper than the draw it is
// supposed to be hiding.
//
// AT LEAST ONE, ALWAYS. The window is a MINIMUM and not a budget: a do/while,
// so a size whose single draw takes longer than the whole window still costs
// exactly one thrown-away number, and a window that has already expired on entry
// still costs one. Without that floor a 50 ms window on a large enough request
// would throw nothing away at all and the tier would be a lie.
//
// Returns how many numbers were discarded, which is never zero. The caller is
// expected to record it -- PLAN §9 says measure on this machine rather than
// quote, and this count is the measurement.
long long spin(Tier tier, Bits32 &bits, void (*draw_and_discard)(Bits32 &, void *),
               void *context);

} // namespace satellite
