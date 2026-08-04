// Division and modulo across Int, Big and Real.
//
// These reach big::divmod, which is exercised much harder by test_bignum (4,000
// random divisions checking a == q*b + r).  What is tested HERE is the wiring:
// that the dispatch tables send each pair of types to the right implementation,
// that the answer keeps the same semantics as a value grows past int64, and
// that the operations which fault on the hardware are caught before they do.
#include "satellite/container.hpp"

#include <cstdio>
#include <limits>
#include <string>

using namespace satellite;

static int failures = 0;
static int checks   = 0;

static void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; printf("  FAIL  %s\n", what.c_str()); }
}

static void eq(const Container& got, const std::string& want,
               const std::string& what) {
    ++checks;
    if (got.to_string() != want) {
        ++failures;
        printf("  FAIL  %s\n        want [%s]\n        got  [%s]\n",
               what.c_str(), want.c_str(), got.to_string().c_str());
    }
}

static Container I(int64_t v) { return Container::integer(v); }
static Container R(double v)  { return Container::real(v); }
static Container B(const char* digits) {
    bool ok = false;
    return big::from_string(digits, &ok);
}

int main() {
    init_tables();
    printf("division and modulo tests\n");

    // ---- truncated semantics ----------------------------------------------
    // Quotient toward zero, remainder takes the sign of the DIVIDEND.  This is
    // the C++ rule; the alternative (floored, as in Python) would answer -4 and
    // 1 for the two below.
    eq(div(I(7), I(2)),   "3",  "7 / 2");
    eq(mod(I(7), I(2)),   "1",  "7 % 2");
    eq(div(I(-7), I(2)),  "-3", "-7 / 2 truncates toward zero");
    eq(mod(I(-7), I(2)),  "-1", "-7 % 2 follows the dividend");
    eq(div(I(7), I(-2)),  "-3", "7 / -2");
    eq(mod(I(7), I(-2)),  "1",  "7 % -2 follows the dividend, not the divisor");
    eq(div(I(-7), I(-2)), "3",  "-7 / -2");
    eq(mod(I(-7), I(-2)), "-1", "-7 % -2");

    // ---- the identity that has to hold -------------------------------------
    for (int64_t a = -20; a <= 20; ++a)
        for (int64_t b = -7; b <= 7; ++b) {
            if (b == 0) continue;
            Container q = div(I(a), I(b)), r = mod(I(a), I(b));
            check(q.type == Type::Int && r.type == Type::Int &&
                      q.i * b + r.i == a,
                  "a == q*b + r for " + std::to_string(a) + "," +
                      std::to_string(b));
        }

    // ---- division by zero is nil, not a crash ------------------------------
    // There is no integer meaning "undefined", and returning 0 would let the
    // mistake propagate silently through everything downstream.
    check(div(I(7), I(0)).type == Type::Nil, "7 / 0 is nil");
    check(mod(I(7), I(0)).type == Type::Nil, "7 % 0 is nil");
    check(div(R(7), I(0)).type == Type::Nil, "7.0 / 0 is nil, not inf");
    check(mod(R(7), I(0)).type == Type::Nil, "7.0 % 0 is nil, not nan");
    check(div(B("100000000000000000000"), I(0)).type == Type::Nil,
          "big / 0 is nil");
    check(mod(B("100000000000000000000"), I(0)).type == Type::Nil,
          "big % 0 is nil");

    // ---- the two that fault on x86 -----------------------------------------
    // |INT64_MIN| is one past INT64_MAX, so the quotient has no int64 form.
    // idiv does not return a wrong answer for these -- it raises SIGFPE and
    // takes the process down, so reaching this line at all is the test.
    {
        const int64_t kMin = std::numeric_limits<int64_t>::min();
        Container q = div(I(kMin), I(-1));
        eq(q, "9223372036854775808", "INT64_MIN / -1 promotes to big");
        check(q.type == Type::Big, "and is a Big, not a wrapped Int");
        eq(mod(I(kMin), I(-1)), "0", "INT64_MIN % -1 is 0 without faulting");
    }

    // ---- big integers ------------------------------------------------------
    // Before div_table had Big entries these all answered nil.
    eq(div(B("170141183460469231731687303715884105727"), I(1000000007)),
       "170141182269480955845320612798", "2^127-1 / 1000000007");
    eq(mod(B("170141183460469231731687303715884105727"), I(1000000007)),
       "639816141", "2^127-1 % 1000000007");
    eq(div(B("100000000000000000000"), B("10000000000")), "10000000000",
       "big / big");
    eq(mod(B("100000000000000000000"), B("3")), "1", "big % small");

    // A quotient that fits in an int64 comes back as an Int, not a one-limb
    // Big -- otherwise every later comparison against a literal would fail.
    {
        Container q = div(B("100000000000000000000"), B("100000000000000000000"));
        eq(q, "1", "a big quotient that fits demotes to Int");
        check(q.type == Type::Int, "and carries the Int tag");
    }

    // Sign rules survive the promotion, which is the whole point of matching
    // big::divmod's semantics to div_int's.
    eq(mod(B("-170141183460469231731687303715884105727"), I(1000000007)),
       "-639816141", "a negative big remainder follows the dividend");

    // ---- reals -------------------------------------------------------------
    eq(div(R(7.5), I(2)),  "3.750000", "7.5 / 2 is real division");
    eq(mod(R(7.5), I(2)),  "1.500000", "7.5 % 2 uses fmod");
    eq(mod(R(-7.5), I(2)), "-1.500000",
       "-7.5 % 2 truncates like the integer case, it does not round to nearest");

    // A Real anywhere in the pair wins over Big -- the table order in
    // init_tables() depends on it, and getting it backwards would silently
    // truncate this to an integer.
    {
        Container r = div(B("100000000000000000000"), R(2.0));
        check(r.type == Type::Real, "big / real is real division, not truncated");
        r = mod(B("100000000000000000000"), R(3.0));
        check(r.type == Type::Real, "big % real is real too");
    }

    // ---- string division is untouched --------------------------------------
    eq(div(Container::str("a,b,c"), Container::str(",")), "[a, b, c]",
       "\"a,b,c\" / \",\" still splits");
    check(mod(Container::str("a"), Container::str("b")).type == Type::Nil,
          "but strings have no modulo");

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
