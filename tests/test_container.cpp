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

static void eq(const Container& c, const std::string& want, const std::string& what) {
    ++checks;
    std::string got = c.to_string();
    if (got != want) {
        ++failures;
        printf("  FAIL  %s\n        want %s\n        got  %s\n",
               what.c_str(), want.c_str(), got.c_str());
    }
}

int main() {
    init_tables();

    printf("satellite_container tests\n");

    // ---- integers ---------------------------------------------------------
    eq(add(Container::integer(2), Container::integer(3)),  "5", "2 + 3");
    eq(sub(Container::integer(10), Container::integer(4)), "6", "10 - 4");
    eq(mul(Container::integer(6), Container::integer(7)), "42", "6 * 7");
    eq(sub(Container::integer(3), Container::integer(10)), "-7", "3 - 10 goes negative");
    check(add(Container::integer(2), Container::integer(3)).type == Type::Int,
          "small ints stay Int, never promote");

    // ---- overflow promotes to Big, and only then --------------------------
    const int64_t MAXI = std::numeric_limits<int64_t>::max();
    const int64_t MINI = std::numeric_limits<int64_t>::min();

    Container ovf = add(Container::integer(MAXI), Container::integer(1));
    check(ovf.type == Type::Big, "int64 max + 1 promotes to Big");
    eq(ovf, "9223372036854775808", "int64 max + 1 value");

    Container und = sub(Container::integer(MINI), Container::integer(1));
    check(und.type == Type::Big, "int64 min - 1 promotes to Big");
    eq(und, "-9223372036854775809", "int64 min - 1 value");

    // ---- Big demotes back to Int when it fits again -----------------------
    Container back = sub(ovf, Container::integer(1));
    check(back.type == Type::Int, "Big demotes back to Int when it fits");
    eq(back, "9223372036854775807", "demoted value is int64 max");

    Container zero = add(ovf, sub(Container::integer(0), ovf));
    check(zero.type == Type::Int, "big + (-big) demotes to Int 0");
    eq(zero, "0", "big cancels to zero");

    // INT64_MIN round trip is the classic off-by-one trap
    Container minrt = add(Container::big_from_i64(MINI), Container::integer(0));
    check(minrt.type == Type::Int, "INT64_MIN survives Big round trip as Int");
    eq(minrt, "-9223372036854775808", "INT64_MIN value preserved");

    // ---- big arithmetic against known values ------------------------------
    Container p32 = Container::integer(4294967296LL);          // 2^32
    eq(mul(p32, p32), "18446744073709551616", "2^32 * 2^32 = 2^64");

    Container fact = Container::integer(1);
    for (int k = 2; k <= 30; ++k) fact = mul(fact, Container::integer(k));
    eq(fact, "265252859812191058636308480000000", "30! computed by promotion");

    Container neg = mul(fact, Container::integer(-1));
    eq(neg, "-265252859812191058636308480000000", "30! negated");
    eq(add(fact, neg), "0", "30! + -30! = 0");
    eq(sub(neg, fact), "-530505719624382117272616960000000", "-30! - 30!");

    // ---- reals ------------------------------------------------------------
    check(add(Container::real(1.5), Container::integer(2)).type == Type::Real,
          "real + int is Real");
    check(mul(Container::real(2.0), Container::real(4.0)).d == 8.0, "2.0 * 4.0 = 8.0");

    // ---- strings ----------------------------------------------------------
    Container hello = Container::str("hello");
    Container world = Container::str(" world");
    eq(add(hello, world), "hello world", "string + string");
    eq(add(Container::str("n: "), Container::integer(5)), "n: 5", "string + int");
    eq(add(Container::integer(5), Container::str(" apples")), "5 apples", "int + string");

    // the satellite '-' operator: removes the first occurrence
    eq(sub(Container::str("my_string"), Container::str("str")), "my_ing",
       "\"my_string\" - \"str\"");
    eq(sub(Container::str("my_str"), Container::str("str")), "my_",
       "\"my_str\" - \"str\" (your example)");
    eq(sub(Container::str("aXbXc"), Container::str("X")), "abXc",
       "only the FIRST occurrence is removed");
    eq(sub(Container::str("abc"), Container::str("zz")), "abc",
       "missing substring leaves the string alone");

    // small strings must not allocate
    check(hello.as_str()->inline_stored(), "short string stays inline (no malloc)");
    Container big_s = Container::str(std::string(500, 'x'));
    check(!big_s.as_str()->inline_stored(), "long string moves to the heap");
    check(big_s.as_str()->len == 500, "long string keeps its length");

    // ---- lists ------------------------------------------------------------
    Container l1 = Container::list();
    l1.as_list()->items.push_back(Container::integer(1));
    l1.as_list()->items.push_back(Container::str("two"));
    Container l2 = Container::list();
    l2.as_list()->items.push_back(Container::integer(3));
    eq(add(l1, l2), "[1, two, 3]", "list + list");

    // ---- refcounting ------------------------------------------------------
    {
        Container a = Container::str("shared");
        check(a.as_str()->rc.load() == 1, "fresh string has rc 1");
        {
            Container b = a;
            check(a.as_str()->rc.load() == 2, "copy bumps rc to 2");
            Container c;
            c = b;
            check(a.as_str()->rc.load() == 3, "assignment bumps rc to 3");
        }
        check(a.as_str()->rc.load() == 1, "scope exit drops rc back to 1");

        Container m = std::move(a);
        check(m.as_str()->rc.load() == 1, "move does not bump rc");
        check(a.type == Type::Nil, "moved-from container is nil");

        m = m;   // self-assignment must not free
        check(m.as_str()->rc.load() == 1 && m.to_string() == "shared",
              "self-assignment is safe");
    }

    // ---- unsupported combinations return nil rather than crashing ---------
    check(mul(Container::str("a"), Container::list()).type == Type::Nil,
          "string * list is nil, not a crash");
    check(sub(Container::list(), Container::integer(1)).type == Type::Nil,
          "list - int is nil");

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
