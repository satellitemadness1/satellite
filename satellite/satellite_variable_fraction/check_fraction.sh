# satellite/satellite_variable_fraction/check_fraction.sh -- check.sh's rows for satellite.variable.fraction, SOURCED
# by check.sh (never run alone): `expect`, `$interpreter` and build/ are check.sh's.
#
# ONE FILE PER TYPE (2026-09-22), so the four types the author asked for at once
# could each be given their rows without four builders editing check.sh's own lines.
# A fixture program for these rows goes in tests/fraction_*.satl.
#
# (the author, 2026-09-22) "let's just start with satellite.variable.fraction
# my_number = satellite_number1/satellite_number2 simply two numbers that are tied
# together, and when the user declares them, we will automatically make them floats
# with decimal points, but display them as just the whole number".

# A line of output per display, joined with | so a row reads on one line.
fraction_lines() { "$@" 2>/dev/null | tr '\n' '|' | sed 's/|$//'; }

# THE LITERAL: a touching slash between two numbers (the author, 2026-09-16). 1/3
# shows 1/3, never 1.0/3.0; 2/4 stays 2/4 but == 1/2; a spaced 4 / 3 is still 1.
# The last is two fractions that both round to 0.0 at arguments.float.decimal's 128
# places and are not the same: compared exactly, by cross-multiplying.
expect "1/3 is a fraction shown 1/3; 2/4 stays 2/4 and == 1/2; 1/3 < 1/2; 4 / 3 is still 1" \
       "1/3|2/4|true|true|false|1.5/2|1/3|-1/3|1/3|1|false|true|true" \
       "$(fraction_lines "$interpreter" tests/fraction_literal.satl)"

# A NAME: .numerator and .denominator are the whole numbers when they are whole (so
# f.numerator + 1 is 3) and the float when not (1.5); 3 given to a fraction name is
# 3/1; a list holding 2/4 contains 1/2, because sameness is by value too.
expect "a fraction name: .numerator, .denominator, a number given to it is over 1, and its sign is the numerator's" \
       "2/4|2|4|3|true|1.5|3/1|5/7|-5/7|-5|true" \
       "$(fraction_lines "$interpreter" tests/fraction_store.satl)"

# THE REFUSALS, each a sentence naming what was written.
"$interpreter" tests/fraction_zero.satl > build/fraction_zero.out 2>&1
code=$?
expect "a fraction's bottom number cannot be 0, refused at the literal that writes it" "22|1|1" \
       "$code|$(grep -cx before build/fraction_zero.out)|$(tr '\n' ' ' < build/fraction_zero.out | grep -c "1/0: a fraction's bottom number cannot be 0")"
"$interpreter" tests/fraction_spaced.satl > build/fraction_spaced.out 2>&1
code=$?
expect "a spaced slash given to a fraction name is refused before anything runs: it is whole-number division" "27|0|1" \
       "$code|$(grep -cx before build/fraction_spaced.out)|$(tr '\n' ' ' < build/fraction_spaced.out | grep -c '1 / 3 is whole-number division -- a fraction is written with a touching slash: 1/3')"
"$interpreter" tests/fraction_arithmetic.satl > build/fraction_arithmetic.out 2>&1
code=$?
expect "fraction arithmetic is refused by name where it is reached -- not built yet" "14|1|1" \
       "$code|$(grep -cx before build/fraction_arithmetic.out)|$(tr '\n' ' ' < build/fraction_arithmetic.out | grep -c 'the + of 1/3 and 1/3 is fraction arithmetic, and fraction arithmetic is not built yet')"
"$interpreter" tests/fraction_method_check.satl > build/fraction_method_check.out 2>&1
code=$?
expect "a method a fraction does not have is refused by name before anything runs" "14|0|1" \
       "$code|$(grep -cx before build/fraction_method_check.out)|$(tr '\n' ' ' < build/fraction_method_check.out | grep -c 'f.number is not built for satellite.variable.fraction yet')"
"$interpreter" tests/fraction_meets_float.satl > build/fraction_float.out 2>&1
code=$?
expect "a fraction meeting a float waits for the new types to be mixed together, and says so" "14|1" \
       "$code|$(tr '\n' ' ' < build/fraction_float.out | grep -c 'a fraction meeting a float is not built yet -- the new types are mixed together later')"

# A TOUCHING SLASH THAT IS STILL NOT A FRACTION. The refusal "the fraction type is
# not built yet" stood for every touching slash between digits until 2026-09-22 and
# had no row; now that 1/3 is a fraction, what is left of it is a slash after a name,
# a hex or a binary that ends in a digit -- n1/2 -- and this row keeps it asserted.
"$interpreter" tests/fraction_not_a_fraction.satl > build/fraction_not_a_fraction.out 2>&1
code=$?
expect "a touching slash after a name (n1/2) is still refused, as a fraction of a name that is not built yet" "14|1|1" \
       "$code|$(grep -cx before build/fraction_not_a_fraction.out)|$(tr '\n' ' ' < build/fraction_not_a_fraction.out | grep -c 'a touching / makes a fraction only between two numbers written out, like 1/3')"
