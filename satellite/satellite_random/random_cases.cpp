// satellite/satellite_random/random_cases.cpp -- satellite.random with no interpreter
// around it. check.sh runs it (`build/random_cases`) and counts its ok lines.
//
// WHAT IT HOLDS, in order:
//   1. the vendored pcg32 and pcg32_k16384 against upstream's OWN expected output
//      (test-high/expected/*.out), so PCG_BITCOUNT_T = unsigned changed nothing;
//   2. wide_unsigned<2> against unsigned __int128, every operator, 20,000 operand pairs;
//   3. wide_unsigned<8>: products agree with __int128 in their low 128 bits, the shifts
//      undo each other, the cells' inverse multiplier is one, and the doubling unxorshift
//      answers what pcg's own recursive one answers;
//   4. pcg512_k16384: the same seed and stream repeat through a table tick, other seeds
//      and streams differ, nothing repeats in a row, every bit position is balanced, the
//      bytes are even;
//   5. the kernel seeds each thread's generator on its own, and a seeded source repeats;
//   6. the sampler, with a seeded source: dice, a two-limb bound, a range below zero, a
//      step, digit counts of 0, 20 and 1000, and a range of three past 10^30;
//   7. the spin: each grade's window held, with real draws thrown away, and the floor --
//      a throwaway that outlasts the whole window is thrown away exactly once;
//   8. what a draw costs on this machine -- printed, never asserted. The author measures
//      speed himself: `build/random_cases | grep note`.
//
// Written 2026-10-02.

#include "pcg_512.hpp"
#include "random_draw.hpp"
#include "random_source.hpp"
#include "random_spin.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

using namespace satellite004;

namespace {

// seeded(12345)'s first limb -- set from the build's own note line once, then held.
constexpr unsigned long long int kPinnedFirstLimb = 0x81278c2271a7a249ull;

int cases = 0, failed = 0;

void expect(const std::string &what, bool ok)
{
    ++cases;
    if (!ok)
        ++failed;
    std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what.c_str());
}

template <class... Args>
std::string said(const char *shape, Args... args)
{
    char text[512];
    std::snprintf(text, sizeof text, shape, args...);
    return text;
}

using u128 = unsigned __int128;
u128 to128(const wide_unsigned<2> &w) { return (static_cast<u128>(w.limb[1]) << 64) | w.limb[0]; }
wide_unsigned<2> from128(u128 v)
{
    wide_unsigned<2> w;
    w.limb[0] = static_cast<unsigned long long int>(v);
    w.limb[1] = static_cast<unsigned long long int>(v >> 64);
    return w;
}

satellite_number power_of(unsigned long long int base, unsigned long long int exponent)
{
    satellite_number out;
    satellite_number::power(satellite_number(base), satellite_number(exponent), out);
    return out;
}

double milliseconds_since(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

// ---- 1. upstream's expected output ---------------------------------------------------

void upstream_reference()
{
    // vendor/pcg-cpp/pcg-cpp-0.98/test-high/expected/check-pcg32.out, "Round 1: 32bit:".
    const unsigned want_pcg32[6] = {0xa15c02b7u, 0x7b47f409u, 0xba1d3330u, 0x83d2f293u, 0xbfa4784bu, 0xcbed606eu};
    // .../check-pcg32_k16384.out, the same line.
    const unsigned want_k16384[6] = {0xe7cee806u, 0x5f20781cu, 0xfe49a493u, 0x18715647u, 0x4624a50au, 0x7d1c9694u};
    pcg32 plain(42u, 54u);
    auto extended = std::make_unique<pcg32_k16384>(42u, 54u);
    bool plain_right = true, extended_right = true;
    for (int i = 0; i < 6; ++i) {
        plain_right = plain_right && plain() == want_pcg32[i];
        extended_right = extended_right && (*extended)() == want_k16384[i];
    }
    expect("pcg32(42, 54) answers upstream's expected output, round 1 (test-high/expected/check-pcg32.out)", plain_right);
    expect("pcg32_k16384(42, 54) answers upstream's expected output, round 1 (check-pcg32_k16384.out)", extended_right);
}

// ---- 2. wide_unsigned<2> against unsigned __int128 ----------------------------------------

void wide_against_int128()
{
    pcg32 rng(7u);
    const auto limb = [&] { return (static_cast<unsigned long long int>(rng()) << 32) | rng(); };
    bool add = true, sub = true, mul = true, shl = true, shr = true, bits = true, cmp = true, neg = true, mixed = true,
         wide_shift = true;
    for (int i = 0; i < 20000; ++i) {
        u128 a = (static_cast<u128>(limb()) << 64) | limb(), b = (static_cast<u128>(limb()) << 64) | limb();
        if (i % 7 == 0) b = rng() & 0xffu;          // small
        if (i % 11 == 0) a = 0;                      // zero
        if (i % 13 == 0) a = ~static_cast<u128>(0);  // all ones
        if (i % 17 == 0) b = a;                      // equal
        const wide_unsigned<2> A = from128(a), B = from128(b);
        add = add && to128(A + B) == static_cast<u128>(a + b);
        sub = sub && to128(A - B) == static_cast<u128>(a - b);
        mul = mul && to128(A * B) == static_cast<u128>(a * b);
        const unsigned s = rng() % 128u;
        shl = shl && to128(A << s) == static_cast<u128>(a << s);
        shr = shr && to128(A >> s) == static_cast<u128>(a >> s);
        wide_shift = wide_shift && to128(A << 128u) == 0 && to128(A >> 200) == 0 && to128(A << 0) == a;
        bits = bits && to128(A & B) == (a & b) && to128(A | B) == (a | b) && to128(A ^ B) == (a ^ b) &&
               to128(~A) == static_cast<u128>(~a);
        neg = neg && to128(-A) == static_cast<u128>(0 - a);
        cmp = cmp && (A < B) == (a < b) && (A == B) == (a == b) && (A <= B) == (a <= b) && (A > B) == (a > b) &&
              (A >= B) == (a >= b) && (A != B) == (a != b);
        const unsigned long long int k = limb();
        mixed = mixed && to128(A + k) == static_cast<u128>(a + k) && to128(A - 1) == static_cast<u128>(a - 1) &&
                to128(A * k) == static_cast<u128>(a * k) && to128(A & k) == (a & k) && to128(A | 1) == (a | 1) &&
                (A == k) == (a == k) && to128(k - A) == static_cast<u128>(k - a) &&
                static_cast<unsigned long long int>(A) == static_cast<unsigned long long int>(a) &&
                (A < k) == (a < k);
    }
    expect("wide_unsigned<2> + - * agree with unsigned __int128 over 20,000 operand pairs", add && sub && mul);
    expect("... and << >> by 0 to 127, and 0 at or past the width", shl && shr && wide_shift);
    expect("... and & | ^ ~, unary minus, and the six comparisons", bits && neg && cmp);
    expect("... and with an ordinary integer on either side, which is the wide form and not the low limb", mixed);
}

// ---- 3. wide_unsigned<8> and the doubling unxorshift ----------------------------------------

void wide_512()
{
    pcg32 rng(9u);
    const auto random512 = [&] {
        uint512 w;
        for (unsigned long long int &l : w.limb)
            l = (static_cast<unsigned long long int>(rng()) << 32) | rng();
        return w;
    };
    bool low128 = true, shifts = true, unx = true, roundtrip = true, inverse = true;
    for (int i = 0; i < 3000; ++i) {
        const uint512 a = random512(), b = random512();
        low128 = low128 && to128(wide_unsigned<2>(a * b)) ==
                               static_cast<u128>(to128(wide_unsigned<2>(a)) * to128(wide_unsigned<2>(b)));
        const unsigned s = rng() % 512u;
        shifts = shifts && ((a << s) >> s) == (a & (~uint512() >> s)) && ((a >> s) << s) == (a & (~uint512() << s));
        const unsigned shift = 1 + rng() % 511u;
        const uint512 shifted = a ^ (a >> shift);
        unx = unx && satellite004::unxorshift(shifted, 512u, shift) == pcg_extras::unxorshift(shifted, 512u, shift);
        roundtrip = roundtrip && satellite004::unxorshift(shifted, 512u, shift) == a;
    }
    inverse = (kCellMixMultiplier * kCellMixUnmultiplier) == uint512(1) &&
              (kCellMixUnmultiplier * kCellMixMultiplier) == uint512(1);
    expect("wide_unsigned<8>: a product's low 128 bits are the low limbs' __int128 product", low128);
    expect("... and a shift up then down keeps exactly the bits that fit, either way round", shifts);
    expect("the cells' inverse multiplier, worked out by the compiler, multiplies back to 1", inverse);
    expect("the doubling unxorshift answers what pcg's own recursive unxorshift answers, shifts 1 to 511", unx);
    expect("... and undoes x ^= x >> s exactly", roundtrip);
}

// ---- 4. pcg512_k16384 ---------------------------------------------------------------

void generator_512()
{
    auto a = std::make_unique<pcg512_k16384>(uint1024(42u), uint1024(54u));
    auto b = std::make_unique<pcg512_k16384>(uint1024(42u), uint1024(54u));
    auto other_seed = std::make_unique<pcg512_k16384>(uint1024(43u), uint1024(54u));
    auto other_stream = std::make_unique<pcg512_k16384>(uint1024(42u), uint1024(55u));
    bool same = true, moving = true, differ_seed = false, differ_stream = false;
    uint512 last = (*a)();
    (*b)();
    for (int i = 0; i < 70000; ++i) {
        const uint512 x = (*a)(), y = (*b)();
        same = same && x == y;
        moving = moving && x != last;
        last = x;
        if (i < 100) {
            differ_seed = differ_seed || x != (*other_seed)();
            differ_stream = differ_stream || x != (*other_stream)();
        }
    }
    expect("pcg512_k16384: the same seed and stream give the same 70,001 outputs, through a table tick at 65,536", same);
    expect("... and no two in a row are alike", moving);
    expect("... and another seed, or another stream, gives other outputs", differ_seed && differ_stream);

    unsigned counts[512] = {}, bytes[256] = {};
    const int draws = 20000;
    for (int n = 0; n < draws; ++n) {
        const uint512 v = (*a)();
        for (unsigned i = 0; i < 8; ++i) {
            for (unsigned bit = 0; bit < 64; ++bit)
                counts[i * 64 + bit] += (v.limb[i] >> bit) & 1u;
            for (unsigned k = 0; k < 8; ++k)
                ++bytes[(v.limb[i] >> (8 * k)) & 0xffu];
        }
    }
    unsigned low = draws, high = 0;
    for (unsigned i = 0; i < 512; ++i) {
        if (counts[i] < low) low = counts[i];
        if (counts[i] > high) high = counts[i];
    }
    expect(said("... every one of its 512 bit positions is set between 48%% and 52%% of 20,000 draws (%u to %u)", low, high),
           low >= 9600 && high <= 10400);
    double chi = 0;
    for (unsigned v = 0; v < 256; ++v) {
        const double away = static_cast<double>(bytes[v]) - 5000.0;
        chi += away * away / 5000.0;
    }
    expect(said("... and its 1,280,000 bytes take every value evenly: chi-square %.0f over 255 degrees, under 400", chi),
           chi < 400);
    const RandomGeneratorFacts facts = random_generator_facts();
    expect(said("the generator is %s: %u bits a call, %zu bytes of state (period 2^%zu; pcg's period_pow2() says 2^%zu and "
                "overcounts, pcg_512.hpp)",
                facts.generator, facts.output_bits, facts.state_bytes, facts.period_pow2, pcg512_k16384::period_pow2()),
           std::strcmp(facts.generator, "pcg512_k16384") == 0 && facts.output_bits == 512 &&
               facts.state_bytes == sizeof(pcg512_k16384) && facts.period_pow2 < pcg512_k16384::period_pow2());
}

// ---- 5. seeding ---------------------------------------------------------------------

void seeding()
{
    const auto made = std::chrono::steady_clock::now();
    const unsigned long long int mine = random_source().next_limb();
    const double ms = milliseconds_since(made);
    unsigned long long int theirs = mine;
    std::thread([&] { theirs = random_source().next_limb(); }).join();
    expect(said("the generator is seeded from the kernel a thread at a time: two threads' first limbs differ "
                "(this thread's made in %.2f ms)", ms),
           mine != theirs);
    expect("... and a thread keeps its generator between calls", random_source().next_limb() != mine);
    auto once = seeded_random_source(12345), twice = seeded_random_source(12345), other = seeded_random_source(12346);
    bool same = true, differ = false;
    for (int i = 0; i < 1000; ++i) {
        const unsigned long long int x = once->next_limb();
        same = same && x == twice->next_limb();
        differ = differ || x != other->next_limb();
    }
    expect("seeded from 12345 twice gives the same 1000 limbs; from 12346, other limbs", same && differ);
    // PINNED: the first limb of seed 12345, as this build makes it. A seeded sequence is built
    // from its seed in a defined order and must be the same on every compiler and machine
    // (random_source.cpp), so this number may never change without this line saying so.
    const unsigned long long int first = seeded_random_source(12345)->next_limb();
    std::printf("note  seeded(12345)'s first limb is 0x%016llx\n", first);
    expect("the first limb of seed 12345 is the pinned one", first == kPinnedFirstLimb);
}

// ---- 6. the sampler -------------------------------------------------------------------

void sampler()
{
    auto source = seeded_random_source(1);
    LimbSource &limbs = *source;

    unsigned faces[6] = {};
    bool inside = true;
    for (int i = 0; i < 600000; ++i) {
        const satellite_number r = random_below(limbs, satellite_number(6ull));
        if (r.fits_one_limb() && r.limb(0) < 6)
            ++faces[r.limb(0)];
        else
            inside = false;
    }
    bool even = true;
    for (unsigned f = 0; f < 6; ++f)
        even = even && faces[f] >= 98000 && faces[f] <= 102000;
    expect(said("600,000 draws below 6 land on each face 98,000 to 102,000 times (%u %u %u %u %u %u)", faces[0], faces[1],
                faces[2], faces[3], faces[4], faces[5]),
           inside && even);

    bool one = true;
    for (int i = 0; i < 100; ++i)
        one = one && random_below(limbs, satellite_number(1ull)).is_zero();
    expect("below 1 is always 0", one);

    const satellite_number two_to_65 = power_of(2, 65);
    unsigned two_limbs = 0;
    bool under = true;
    for (int i = 0; i < 10000; ++i) {
        const satellite_number r = random_below(limbs, two_to_65);
        under = under && r < two_to_65;
        two_limbs += r.limb_count() == 2 ? 1 : 0;
    }
    expect(said("below 2^65 stays below it, and reaches past one limb about half the time (%u of 10,000)", two_limbs),
           under && two_limbs >= 4500 && two_limbs <= 5500);

    unsigned eleven[11] = {};
    bool bounded = true;
    const DrawShape minus_five_to_five = range_shape(satellite_number::from_signed(-5), satellite_number(5ull));
    for (int i = 0; i < 110000; ++i) {
        const satellite_number shifted = draw(limbs, minus_five_to_five) + satellite_number(5ull);
        if (shifted.fits_one_limb() && !shifted.negative() && shifted.limb(0) <= 10)
            ++eleven[shifted.limb(0)];
        else
            bounded = false;
    }
    bool all_seen = true;
    for (unsigned v = 0; v < 11; ++v)
        all_seen = all_seen && eleven[v] >= 8800 && eleven[v] <= 11200;
    expect("between -5 and 5, both ends in, every value 8,800 to 11,200 times of 110,000", bounded && all_seen);

    bool on_step = true, every_place = true;
    unsigned places[21] = {};
    const DrawShape by_five = stepped_shape(satellite_number(0ull), satellite_number(100ull), satellite_number(5ull));
    for (int i = 0; i < 21000; ++i) {
        const satellite_number r = draw(limbs, by_five);
        if (r.fits_one_limb() && !r.negative() && r.limb(0) <= 100 && r.limb(0) % 5 == 0)
            ++places[r.limb(0) / 5];
        else
            on_step = false;
    }
    for (unsigned p = 0; p < 21; ++p)
        every_place = every_place && places[p] > 0;
    bool floors = true;
    const DrawShape by_seven = stepped_shape(satellite_number(0ull), satellite_number(100ull), satellite_number(7ull));
    for (int i = 0; i < 1000; ++i) {
        const satellite_number r = draw(limbs, by_seven);
        floors = floors && r.fits_one_limb() && r.limb(0) <= 98 && r.limb(0) % 7 == 0;
    }
    expect("0 to 100 by 5 lands on each of the 21 places; by 7 it never passes 98", on_step && every_place && floors);

    const satellite_number ten_to_20 = power_of(10, 20), ten_to_19 = power_of(10, 19);
    const DrawShape twenty = digits_shape(satellite_number(20ull));
    unsigned full_width = 0;
    bool below = true;
    for (int i = 0; i < 100000; ++i) {
        const satellite_number r = draw(limbs, twenty);
        below = below && r < ten_to_20 && !r.negative();
        full_width += r >= ten_to_19 ? 1 : 0;
    }
    expect(said("20 digits is below 10^20 every time, and 20 digits wide about nine times in ten (%u of 100,000)", full_width),
           below && full_width >= 88000 && full_width <= 92000);
    expect("0 digits is 0", draw(limbs, digits_shape(satellite_number())).is_zero());
    const satellite_number ten_to_1000 = power_of(10, 1000);
    const DrawShape thousand_digits = digits_shape(satellite_number(1000ull));
    bool thousand = true;
    for (int i = 0; i < 20; ++i) {
        const satellite_number r = draw(limbs, thousand_digits);
        thousand = thousand && r < ten_to_1000 && r.limb_count() >= 50;
    }
    expect("1000 digits is below 10^1000 and about 52 limbs wide", thousand);

    const satellite_number ten_to_30 = power_of(10, 30);
    const DrawShape three_past = range_shape(ten_to_30, ten_to_30 + satellite_number(2ull));
    bool seen[3] = {false, false, false}, three = true;
    for (int i = 0; i < 300; ++i) {
        const satellite_number offset = draw(limbs, three_past) - ten_to_30;
        if (offset.fits_one_limb() && !offset.negative() && offset.limb(0) <= 2)
            seen[offset.limb(0)] = true;
        else
            three = false;
    }
    expect("between 10^30 and 10^30 + 2 answers all three", three && seen[0] && seen[1] && seen[2]);
}

// ---- 7. the spin ----------------------------------------------------------------------

struct Throwaway {
    LimbSource *limbs;
    const DrawShape *shape;
};

void throw_one_away(void *at)
{
    const Throwaway &work = *static_cast<const Throwaway *>(at);
    (void)draw(*work.limbs, *work.shape);
}

void sleep_past_the_window(void *)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
}

void spinning()
{
    auto source = seeded_random_source(5);
    const DrawShape die = range_shape(satellite_number(1ull), satellite_number(6ull));
    Throwaway work{source.get(), &die};
    for (int t = 0; t < 3; ++t) {
        const RandomTier tier = static_cast<RandomTier>(t);
        const SpinWindow window = spin_window(tier);
        const auto start = std::chrono::steady_clock::now();
        const long long int thrown = spin(tier, *source, &throw_one_away, &work);
        const double ms = milliseconds_since(start);
        expect(said("%s spins inside the author's %lld to %lld ms: %.0f ms, %lld draws of 1 to 6 thrown away",
                    random_tier_name(tier), window.least_ms, window.most_ms, ms, thrown),
               ms >= static_cast<double>(window.least_ms) && ms <= static_cast<double>(window.most_ms) + 100 && thrown >= 1);
    }
    const auto start = std::chrono::steady_clock::now();
    const long long int once = spin(RandomTier::fast, *source, &sleep_past_the_window, nullptr);
    const double ms = milliseconds_since(start);
    expect(said("a throwaway that outlasts fast's whole window is thrown away exactly once (%lld, %.0f ms)", once, ms),
           once == 1 && ms >= 1100 && ms < 1400);
}

// ---- 8. what it costs, printed ---------------------------------------------------------

void timing()
{
    auto source = seeded_random_source(3);
    unsigned long long int sink = 0;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 4000000; ++i)
        sink ^= source->next_limb();
    const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / 4e6;
    std::printf("note  %.2f ns a 64-bit limb (%llx)\n", ns, sink & 0xf);
    auto ultra = std::make_unique<pcg512_k16384>(uint1024(3u));
    uint512 wide_sink;
    const auto wide_start = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000000; ++i)
        wide_sink ^= (*ultra)();
    const double wide_ns =
        std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - wide_start).count() / 1e6;
    std::printf("note  %.1f ns a 512-bit output of pcg512_k16384, table ticks included (%llx)\n", wide_ns,
                wide_sink.limb[0] & 0xf);
    const DrawShape die = range_shape(satellite_number(1ull), satellite_number(6ull));
    const auto die_start = std::chrono::steady_clock::now();
    for (int i = 0; i < 4000000; ++i)
        sink ^= draw(*source, die).limb(0);
    const double die_ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - die_start).count() / 4e6;
    std::printf("note  %.1f ns a draw of 1 to 6, the whole sampler (%llx)\n", die_ns, sink & 0xf);
}

} // namespace

int main()
{
    upstream_reference();
    wide_against_int128();
    wide_512();
    generator_512();
    seeding();
    sampler();
    spinning();
    timing();
    std::printf("%d cases, %d failed\n", cases, failed);
    return failed == 0 ? 0 : 1;
}
