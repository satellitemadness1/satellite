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

    // ---- the satellite character map --------------------------------------
    // a satellite character is a CODE, not a Unicode codepoint
    check(charmap::from_ascii('a') == 1,  "'a' is code 1");
    check(charmap::from_ascii('z') == 26, "'z' is code 26");
    check(charmap::from_ascii('A') == 27, "'A' is code 27");
    check(charmap::from_ascii('Z') == 52, "'Z' is code 52");
    check(charmap::from_ascii('0') == 53, "'0' is code 53");
    check(charmap::from_ascii('1') == 54, "'1' is code 54");
    check(charmap::from_ascii('9') == 62, "'9' is code 62");
    check(charmap::from_ascii('!') == 63, "'!' starts the symbols at 63");
    check(charmap::from_ascii(')') == 72, "')' closes !@#$%^&*() at 72");
    check(charmap::from_ascii('-') == 73, "'-' is code 73");
    check(charmap::from_ascii('+') == 76, "'+' closes -_=+ at 76");
    check(charmap::from_ascii('[') == 77, "'[' is code 77");
    check(charmap::from_ascii('|') == 82, "'|' closes [{]}\\| at 82");
    check(charmap::from_ascii(';') == 83, "';' is code 83");
    check(charmap::from_ascii(',') == 87, "',' closes ;:'\", at 87");
    check(charmap::from_ascii('<') == 88, "'<' is code 88");
    check(charmap::from_ascii('?') == 92, "'?' closes <.>/? at 92");
    check(charmap::from_ascii('~') == 94, "'~' is the last of your table at 94");
    check(charmap::from_ascii(' ') == 95, "space is code 95 (added -- confirm)");

    // every TEXT code round trips
    {
        bool all = true;
        for (char32_t code = 1; code < charmap::TEXT_COUNT; ++code)
            if (charmap::from_ascii(charmap::to_ascii(code)) != code) all = false;
        check(all, "every text code round trips ascii -> code -> ascii");
    }

    // ---- satellite math tokens --------------------------------------------
    // the whole point: an operator is NOT the ascii character that looks like it
    check(charmap::from_ascii('-') == 73, "a hyphen in text is code 73");
    check(charmap::OP_MINUS == 99, "satellite SUBTRACTION is code 99");
    check(charmap::from_ascii('-') != charmap::OP_MINUS,
          "hyphen and minus can never be confused");
    check(charmap::from_ascii('*') != charmap::OP_TIMES, "asterisk is not multiply");
    check(charmap::from_ascii('/') != charmap::OP_DIVIDE, "slash is not divide");
    check(charmap::from_ascii('+') != charmap::OP_PLUS, "plus sign is not addition");

    // from_ascii must never invent an operator code
    {
        bool clean = true;
        for (int ch = 0; ch < 256; ++ch)
            if (charmap::is_operator(charmap::from_ascii((char)ch))) clean = false;
        check(clean, "no ascii character ever maps to an operator code");
    }

    check(charmap::is_operator(charmap::OP_PLUS),  "OP_PLUS is an operator");
    check(charmap::is_operator(charmap::OP_GE),    "OP_GE is an operator");
    check(!charmap::is_operator(73),               "code 73 is text, not an operator");
    check(!charmap::is_operator(charmap::SPACE),   "space is text, not an operator");
    check(std::string(charmap::op_text(charmap::OP_LE)) == "<=",
          "multi-character operators render correctly");

    // an expression stored AS A STRING, tokens and text side by side
    {
        auto* p = new SatString();
        p->append_text("cost");
        char32_t minus = charmap::OP_MINUS;
        p->append(&minus, 1);
        p->append_text("2");
        Container expr;
        expr.type = Type::Str;
        expr.obj  = p;

        check(p->len == 6, "6 characters: c o s t MINUS 2");
        check(p->at(4) == charmap::OP_MINUS, "position 4 holds the math token");
        eq(expr, "cost-2", "tokenized expression renders readably");

        // and a literal hyphen at the same position is a different character
        Container plain = Container::str("cost-2");
        check(plain.as_str()->at(4) == 73, "the literal version holds the hyphen");
        check(plain.as_str()->at(4) != p->at(4),
              "same rendering, different characters -- unambiguous");
    }

    // strings hold codes, and indexing is exact
    Container word = Container::str("aA0");
    check(word.as_str()->len == 3, "three characters");
    check(word.as_str()->at(0) == 1,  "at(0) is code 1 for 'a'");
    check(word.as_str()->at(1) == 27, "at(1) is code 27 for 'A'");
    check(word.as_str()->at(2) == 53, "at(2) is code 53 for '0'");
    eq(word, "aA0", "codes render back to text");

    // space works, which is why 95 had to exist
    eq(add(Container::str("hello"), Container::str(" world")), "hello world",
       "a space survives the round trip");

    // anything outside the map becomes void
    Container off = Container::str("a\xc3\xa9" "b");  // 'é' in UTF-8; split so 'b'
                                                   // is not eaten by the hex escape
    check(off.as_str()->at(1) == charmap::VOID, "unmapped input becomes void (0)");
    check(off.as_str()->at(2) == charmap::VOID, "both bytes of it become void");
    eq(off, "a??b", "void characters render visibly as '?'");

    // small strings must not allocate
    check(hello.as_str()->inline_stored(), "short string stays inline (no malloc)");
    check(Container::str("0123456789abcdef").as_str()->inline_stored(),
          "16 characters still fits inline");
    check(!Container::str("0123456789abcdefg").as_str()->inline_stored(),
          "17 characters moves to the heap");
    Container big_s = Container::str(std::string(500, 'x'));
    check(!big_s.as_str()->inline_stored(), "long string moves to the heap");
    check(big_s.as_str()->len == 500, "long string keeps its length");

    // ---- string operators --------------------------------------------------
    eq(mul(Container::str("ab"), Container::integer(3)), "ababab", "\"ab\" * 3 repeats");
    eq(mul(Container::integer(2), Container::str("xy")), "xyxy", "2 * \"xy\" also repeats");
    eq(mul(Container::str("ab"), Container::integer(0)), "", "* 0 gives the empty string");
    eq(div(Container::str("a,b,c"), Container::str(",")), "[a, b, c]", "\"a,b,c\" / \",\" splits");
    eq(div(Container::str("a"), Container::str(",")), "[a]", "no separator gives one piece");
    eq(div(Container::str("abc"), Container::str("")), "[a, b, c]",
       "empty separator splits into characters");

    // ---- comparison and sort order -----------------------------------------
    check(equals(Container::str("abc"), Container::str("abc")).b, "equal strings");
    check(!equals(Container::str("abc"), Container::str("abd")).b, "unequal strings");
    check(equals(Container::integer(5), Container::real(5.0)).b, "5 == 5.0 across types");
    check(equals(Container::integer(2), Container::str("2")).b == false,
          "an int is not equal to its text");

    // raw code order would give "zebra" < "Apple" (z=26, A=27); the collation
    // key restores what a user actually expects from a sorted list
    check(less(Container::str("Apple"), Container::str("zebra")).b,
          "\"Apple\" sorts before \"zebra\" (collation, not raw code)");
    check(less(Container::str("9"), Container::str("a")).b,
          "digits sort before letters");
    check(less(Container::str("abc"), Container::str("abcd")).b,
          "a prefix sorts before the longer string");
    check(less(Container::integer(2), Container::integer(10)).b, "2 < 10 numerically");

    // ---- search ------------------------------------------------------------
    {
        Container hay = Container::str("one,two,one");
        SatString one; one.append_text("one");
        SatString comma; comma.append_text(",");
        check(hay.as_str()->find(one) == 0,       "find returns the first index");
        check(hay.as_str()->find_last(one) == 8,  "find_last returns the last index");
        check(hay.as_str()->count(one) == 2,      "count finds both occurrences");
        check(hay.as_str()->count(comma) == 2,    "count works on separators");
        check(hay.as_str()->contains(one),        "contains");
        check(hay.as_str()->starts_with(one),     "starts_with");
        check(hay.as_str()->ends_with(one),       "ends_with");
    }

    // ---- transforms --------------------------------------------------------
    eq(str_upper(Container::str("Hello, World!")), "HELLO, WORLD!", "upper");
    eq(str_lower(Container::str("Hello, World!")), "hello, world!", "lower");
    eq(str_trim(Container::str("  padded  ")), "padded", "trim");
    eq(str_trim(Container::str("   ")), "", "trim of all spaces is empty");
    eq(str_reverse(Container::str("satellite")), "etilletas", "reverse");
    eq(str_replace_all(Container::str("a-b-c"), Container::str("-"), Container::str("+")),
       "a+b+c", "replace_all");
    eq(str_replace_all(Container::str("aaa"), Container::str("a"), Container::str("aa")),
       "aaaaaa", "replace_all does not rescan what it wrote");
    eq(str_slice(Container::str("satellite"), 0, 3), "sat", "slice from the front");
    eq(str_slice(Container::str("satellite"), -4, 4), "lite", "negative start counts from the end");
    eq(str_slice(Container::str("abc"), 1, 99), "bc", "slice clamps to the end");
    eq(str_join(div(Container::str("a,b,c"), Container::str(",")), Container::str("-")),
       "a-b-c", "split then join round trips");

    {
        Container s = Container::str("x-y-z");
        SatString dash; dash.append_text("-");
        s.as_str()->remove_all(dash);
        eq(s, "xyz", "remove_all strips every occurrence");
    }

    // ---- text to number ----------------------------------------------------
    eq(str_to_number(Container::str("42")), "42", "\"42\" parses");
    eq(str_to_number(Container::str("-42")), "-42", "negative parses");
    eq(str_to_number(Container::str("  7")), "7", "leading space is skipped");
    check(str_to_number(Container::str("3.5")).type == Type::Real, "\"3.5\" is a Real");
    check(str_to_number(Container::str("abc")).type == Type::Nil, "non-numeric text is nil");
    eq(str_to_number(Container::str("99999999999999999999")), "99999999999999999999",
       "a number too big for int64 parses straight into Big");

    // ---- charmap arithmetic is what makes those cheap ----------------------
    check(charmap::to_upper(charmap::from_ascii('a')) == charmap::from_ascii('A'),
          "to_upper is c + 26, no lookup table");
    check(charmap::to_lower(charmap::from_ascii('Z')) == charmap::from_ascii('z'),
          "to_lower is c - 26");
    check(charmap::to_upper(charmap::from_ascii('5')) == charmap::from_ascii('5'),
          "case conversion leaves non-letters alone");
    check(charmap::digit_value(charmap::from_ascii('7')) == 7,
          "digit value is code - 53, not a parse");
    check(charmap::digit_value(charmap::from_ascii('a')) == -1, "letters have no digit value");
    check(charmap::is_letter(charmap::from_ascii('q')), "is_letter");
    check(charmap::is_digit(charmap::from_ascii('0')), "is_digit");
    check(charmap::is_space(charmap::SPACE), "is_space");
    check(!charmap::is_letter(charmap::OP_MINUS), "a math token is not a letter");

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
