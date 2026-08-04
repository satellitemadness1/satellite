// Division, modulo, powers, shifts and decimal parsing on satellite_number's
// big form.
//
// The important test here is not any single hand-picked case -- it is the
// randomized identity a == q*b + r.  Algorithm D's correction path (step D6)
// fires roughly once in 2^64 divisions on random input, so no list of small
// examples will ever reach it.  Thousands of wide random divisions, each
// checked by multiplying the answer back out, is what actually exercises it.
#include <cstdio>
#include <string>
#include <vector>

#include "satellite/container.hpp"

using namespace satellite;

static int checks = 0, failures = 0;

static void check(bool cond, const std::string& what) {
    ++checks;
    if (!cond) {
        ++failures;
        printf("  FAIL: %s\n", what.c_str());
    }
}

// Build a BigInt from decimal text.  Uses from_string, which is itself under
// test -- so every use of it is preceded by a round-trip check of the same text.
static BigInt big_of(const std::string& s) {
    bool ok = false;
    Container c = big::from_string(s, &ok);
    BigInt b;
    if (c.type == Type::Big) {
        b = *c.as_big();
    } else if (c.type == Type::Int) {
        b.neg = c.i < 0;
        uint64_t mag = c.i < 0 ? (uint64_t)(-(c.i + 1)) + 1 : (uint64_t)c.i;
        if (mag) b.limbs.push_back(mag);
    }
    c.release();
    return b;
}

static std::string str_of(const Container& c) {
    return c.to_string();
}

// ---------------------------------------------------------------------------
static void test_from_string() {
    printf("from_string\n");
    bool ok = false;

    Container c = big::from_string("0", &ok);
    check(ok && str_of(c) == "0", "\"0\" parses to 0");
    c.release();

    c = big::from_string("42", &ok);
    check(ok && str_of(c) == "42" && c.type == Type::Int, "small values demote to Int");
    c.release();

    // 10^19 is exactly the chunk boundary -- one digit past a full limb of
    // decimal, so it catches an off-by-one in the folding loop.
    c = big::from_string("10000000000000000000", &ok);
    check(ok && str_of(c) == "10000000000000000000", "10^19 round-trips");
    c.release();

    const std::string big40 = "1234567890123456789012345678901234567890";
    c = big::from_string(big40, &ok);
    check(ok && str_of(c) == big40, "40-digit value round-trips");
    c.release();

    c = big::from_string("-" + big40, &ok);
    check(ok && str_of(c) == "-" + big40, "negative 40-digit round-trips");
    c.release();

    c = big::from_string("1_000_000", &ok);
    check(ok && str_of(c) == "1000000", "underscores are separators, as the lexer spells them");
    c.release();

    // The int64 boundary: the value one past what strtoll can hold is exactly
    // the case that sent the lexer down this path in the first place.
    c = big::from_string("9223372036854775808", &ok);
    check(ok && str_of(c) == "9223372036854775808", "INT64_MAX+1 parses");
    c.release();

    c = big::from_string("-9223372036854775808", &ok);
    check(ok && str_of(c) == "-9223372036854775808" && c.type == Type::Int,
          "INT64_MIN still fits an Int");
    c.release();

    for (const char* bad : {"", "-", "+", "12a", "1.5", " 12"}) {
        c = big::from_string(bad, &ok);
        check(!ok, std::string("rejects \"") + bad + "\"");
        c.release();
    }
}

// ---------------------------------------------------------------------------
static void test_divmod_basics() {
    printf("divmod -- basics and sign rules\n");
    Container q, r;

    check(!big::divmod(big_of("100"), big_of("0"), &q, &r), "division by zero returns false");

    check(big::divmod(big_of("100"), big_of("7"), &q, &r), "100/7 succeeds");
    check(str_of(q) == "14" && str_of(r) == "2", "100/7 == 14 rem 2");
    q.release(); r.release();

    check(big::divmod(big_of("7"), big_of("100"), &q, &r), "7/100 succeeds");
    check(str_of(q) == "0" && str_of(r) == "7", "divisor larger: quotient 0, remainder intact");
    q.release(); r.release();

    check(big::divmod(big_of("100"), big_of("100"), &q, &r), "equal operands");
    check(str_of(q) == "1" && str_of(r) == "0", "100/100 == 1 rem 0");
    q.release(); r.release();

    // Truncated division: toward zero, remainder follows the dividend.  These
    // four are the whole sign contract.
    struct { const char* a; const char* b; const char* q; const char* r; } signs[] = {
        {"7", "2", "3", "1"}, {"-7", "2", "-3", "-1"},
        {"7", "-2", "-3", "1"}, {"-7", "-2", "3", "-1"},
    };
    for (auto& s : signs) {
        big::divmod(big_of(s.a), big_of(s.b), &q, &r);
        check(str_of(q) == s.q && str_of(r) == s.r,
              std::string(s.a) + "/" + s.b + " == " + s.q + " rem " + s.r);
        q.release(); r.release();
    }

    // A remainder of zero must not carry the dividend's sign as "-0".
    big::divmod(big_of("-6"), big_of("3"), &q, &r);
    check(str_of(q) == "-2" && str_of(r) == "0", "-6/3 leaves remainder 0, not -0");
    q.release(); r.release();

    // Only one half wanted.
    check(big::divmod(big_of("100"), big_of("7"), &q, nullptr), "quotient only");
    check(str_of(q) == "14", "quotient correct with null remainder");
    q.release();
    check(big::divmod(big_of("100"), big_of("7"), nullptr, &r), "remainder only");
    check(str_of(r) == "2", "remainder correct with null quotient");
    r.release();
}

// ---------------------------------------------------------------------------
static void test_divmod_wide() {
    printf("divmod -- multi-limb\n");
    Container q, r;

    // A 39-digit dividend by a 20-digit divisor: several limbs each way, so
    // this is the first case that runs the real estimation loop rather than
    // short division.
    const std::string a = "170141183460469231731687303715884105727";  // 2^127 - 1
    const std::string b = "18446744073709551616";                     // 2^64
    big::divmod(big_of(a), big_of(b), &q, &r);
    check(str_of(q) == "9223372036854775807", "(2^127-1)/2^64 == 2^63-1");
    check(str_of(r) == "18446744073709551615", "remainder == 2^64-1");
    q.release(); r.release();

    // Exact division of a product must leave nothing behind.
    const std::string p1 = "340282366920938463463374607431768211507";
    const std::string p2 = "170141183460469231731687303715884105757";
    BigInt x = big_of(p1), y = big_of(p2);
    Container prod = big::mul(x, y);
    BigInt pb = *prod.as_big();
    big::divmod(pb, y, &q, &r);
    check(str_of(q) == p1 && str_of(r) == "0", "product / one factor == the other, exactly");
    q.release(); r.release(); prod.release();
}

// ---------------------------------------------------------------------------
// The real test: a == q*b + r must hold for every division, with |r| < |b| and
// r's sign matching a.  Random wide operands, checked by multiplying back.
static void test_divmod_identity() {
    printf("divmod -- randomized a == q*b + r identity\n");

    uint64_t seed = 0x9E3779B97F4A7C15ull;
    auto next = [&seed]() {
        seed = seed * 6364136223846793005ull + 1442695040888963407ull;
        return seed;
    };
    auto make = [&next](size_t limbs, bool neg) {
        BigInt v;
        for (size_t i = 0; i < limbs; ++i) v.limbs.push_back(next());
        while (!v.limbs.empty() && v.limbs.back() == 0) v.limbs.pop_back();
        v.neg = neg && !v.limbs.empty();
        return v;
    };

    int bad = 0;
    for (int trial = 0; trial < 4000; ++trial) {
        const size_t alimbs = 1 + (next() % 8);
        const size_t blimbs = 1 + (next() % 4);
        BigInt a = make(alimbs, (next() & 1) != 0);
        BigInt b = make(blimbs, (next() & 1) != 0);
        if (b.limbs.empty()) continue;

        Container q, r;
        if (!big::divmod(a, b, &q, &r)) { ++bad; continue; }

        // Rebuild a from the answer: q*b + r.
        BigInt qb, rb;
        if (q.type == Type::Big) qb = *q.as_big();
        else { uint64_t m = q.i < 0 ? (uint64_t)(-(q.i + 1)) + 1 : (uint64_t)q.i;
               qb.neg = q.i < 0; if (m) qb.limbs.push_back(m); }
        if (r.type == Type::Big) rb = *r.as_big();
        else { uint64_t m = r.i < 0 ? (uint64_t)(-(r.i + 1)) + 1 : (uint64_t)r.i;
               rb.neg = r.i < 0; if (m) rb.limbs.push_back(m); }

        Container qb_prod = big::mul(qb, b);
        BigInt qbp;
        if (qb_prod.type == Type::Big) qbp = *qb_prod.as_big();
        else { uint64_t m = qb_prod.i < 0 ? (uint64_t)(-(qb_prod.i + 1)) + 1
                                          : (uint64_t)qb_prod.i;
               qbp.neg = qb_prod.i < 0; if (m) qbp.limbs.push_back(m); }
        Container sum = big::add(qbp, rb);

        Container a_c = big::normalize(new BigInt(a));
        if (str_of(sum) != str_of(a_c)) ++bad;

        // |r| < |b|, and r never disagrees in sign with a.
        if (big::cmp_mag(rb, b) >= 0) ++bad;
        if (!rb.limbs.empty() && rb.neg != a.neg) ++bad;

        q.release(); r.release(); qb_prod.release(); sum.release(); a_c.release();
    }
    check(bad == 0, "4000 random divisions all satisfy a == q*b + r, |r| < |b|");
    if (bad) printf("    (%d of 4000 failed)\n", bad);
}

// ---------------------------------------------------------------------------
static void test_pow_and_shifts() {
    printf("pow and shifts\n");

    Container c = big::pow_u64(big_of("2"), 0);
    check(str_of(c) == "1", "anything ** 0 == 1");
    c.release();

    c = big::pow_u64(big_of("2"), 10);
    check(str_of(c) == "1024", "2 ** 10");
    c.release();

    c = big::pow_u64(big_of("2"), 127);
    check(str_of(c) == "170141183460469231731687303715884105728", "2 ** 127");
    c.release();

    c = big::pow_u64(big_of("-2"), 3);
    check(str_of(c) == "-8", "negative base, odd exponent stays negative");
    c.release();

    c = big::pow_u64(big_of("-2"), 4);
    check(str_of(c) == "16", "negative base, even exponent turns positive");
    c.release();

    c = big::pow_u64(big_of("0"), 5);
    check(str_of(c) == "0", "0 ** 5 == 0");
    c.release();

    // Refusing is the contract, not crashing or swapping to death.
    c = big::pow_u64(big_of("2"), 10000000000ull);
    check(c.type == Type::Nil, "an absurd exponent returns nil rather than exhausting memory");
    c.release();

    c = big::shl(big_of("1"), 128);
    check(str_of(c) == "340282366920938463463374607431768211456", "1 << 128 == 2^128");
    c.release();

    c = big::shl(big_of("1"), 0);
    check(str_of(c) == "1", "shift by 0 is identity");
    c.release();

    c = big::shr(big_of("340282366920938463463374607431768211456"), 128);
    check(str_of(c) == "1", "2^128 >> 128 == 1");
    c.release();

    c = big::shr(big_of("100"), 1000);
    check(str_of(c) == "0", "shifting past the top gives 0, not garbage");
    c.release();

    // A shift that is not a multiple of 64 crosses limb boundaries, which is
    // where an unguarded `>> (64 - s)` would be undefined behaviour.
    c = big::shl(big_of("1"), 65);
    check(str_of(c) == "36893488147419103232", "1 << 65 crosses a limb boundary");
    c.release();

    c = big::shr(big_of("36893488147419103232"), 65);
    check(str_of(c) == "1", "and shifts back");
    c.release();

    // shr is a magnitude shift: truncation toward zero, not a floor shift.
    c = big::shr(big_of("-9"), 1);
    check(str_of(c) == "-4", "-9 >> 1 truncates toward zero to -4");
    c.release();
}

// ---------------------------------------------------------------------------
int main() {
    printf("test_bignum\n");
    test_from_string();
    test_divmod_basics();
    test_divmod_wide();
    test_divmod_identity();
    test_pow_and_shifts();
    printf("test_bignum: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
