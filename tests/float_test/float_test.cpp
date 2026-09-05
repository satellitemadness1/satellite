// float_test -- the proof that src/satellite_float/ does what DESIGN §8.6
// specifies. See tests/float_test/float_test.hpp for the harness and for what
// is being proved.

#include "float_test.hpp"

#include <cstdio>

namespace float_test {

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

satellite::Number number_of(const std::string &text)
{
    satellite::Number out;
    if (!satellite::Number::parse(text, out))
        check(false, "the fixture `" + text + "` does not parse -- every "
                     "assertion below it is about zero");
    return out;
}

satellite::Float of(const std::string &text)
{
    return satellite::Float::from_number(number_of(text));
}

std::string text_of(const satellite::Float &value)
{
    return value.to_string();
}

} // namespace float_test

int main()
{
    float_test::section_representation();
    float_test::section_arithmetic();
    float_test::section_power();

    if (float_test::failures != 0) {
        printf("float_test: %d failed\n", float_test::failures);
        return 1;
    }
    printf("float_test: ok\n");
    return 0;
}
