// number_test -- the proof that src/satellite_number/ does what DESIGN §8.1
// specifies. See tests/number_test/number_test.hpp for the harness and for what
// is being proved.
//
// EVERY ASSERTION COMPARES TEXT, and that is a decision rather than a
// convenience. Number::operator== is compare(), and compare() is one of the
// things these sections are about -- so a suite that asserted `a == b` would be
// checking compare() against itself, and a compare() that answered "equal" to
// everything would pass every one of them. to_string() is the one function that
// turns a value into something a person can read, so it is what the assertions
// read. section_text is where to_string itself is checked, against literals.

#include "number_test.hpp"

#include <cstdio>
#include <string>

namespace number_test {

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

satellite::Number of(const std::string &text)
{
    satellite::Number out;
    if (!satellite::Number::parse(text, out))
        check(false, "the fixture `" + text + "` does not parse -- every "
                     "assertion below it is about zero");
    return out;
}

std::string text_of(const satellite::Number &value)
{
    return value.to_string();
}

} // namespace number_test

int main()
{
    number_test::section_limbs();
    number_test::section_sign();
    number_test::section_arithmetic();
    number_test::section_rounding();
    number_test::section_exact();
    number_test::section_text();
    number_test::section_draw();

    if (number_test::failures != 0) {
        printf("number_test: %d failed\n", number_test::failures);
        return 1;
    }
    printf("number_test: ok\n");
    return 0;
}
