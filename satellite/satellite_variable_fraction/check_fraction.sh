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
# shows 1/3, never 1.0/3.0; 2/4 stays 2/4 but == 1/2; a spaced 4 / 3 is the fraction
# 4/3 too (2026-10-12: "what about always a fraction?").
# The last is two fractions that both round to 0.0 at arguments.float.decimal's 128
# places and are not the same: compared exactly, by cross-multiplying.
expect "1/3 is a fraction shown 1/3; 2/4 stays 2/4 and == 1/2; 1/3 < 1/2; 4 / 3 is the fraction 4/3" \
       "1/3|2/4|true|true|false|1.5/2|1/3|-1/3|1/3|4/3|false|true|true" \
       "$(fraction_lines "$interpreter" tests/fraction_literal.satl)"

# A NAME: .satellite.numerator and .satellite.denominator (written .satellite. since
# 2026-10-11, NM-8) are the whole numbers when they are whole (so f.satellite.numerator + 1
# is 3) and the float when not (1.5); 3 given to a fraction name is
# 3/1; a list holding 2/4 contains 1/2, because sameness is by value too.
expect "a fraction name: .satellite.numerator, .denominator, a number given to it is over 1, and its sign is the numerator's" \
       "2/4|2|4|3|true|1.5|3/1|5/7|-5/7|-5|true" \
       "$(fraction_lines "$interpreter" tests/fraction_store.satl)"

# THE REFUSALS, each a sentence naming what was written.
"$interpreter" tests/fraction_zero.satl > build/fraction_zero.out 2>&1
code=$?
expect "a fraction's bottom number cannot be 0, refused at the literal that writes it" "22|1|1" \
       "$code|$(grep -cx before build/fraction_zero.out)|$(tr '\n' ' ' < build/fraction_zero.out | grep -c "1/0: a fraction's bottom number cannot be 0")"
"$interpreter" tests/fraction_spaced.satl > build/fraction_spaced.out 2>&1
code=$?
expect "a spaced slash given to a fraction name is the fraction (2026-10-12): 1 / 3 holds 1/3" "0|1|1" \
       "$code|$(grep -cx before build/fraction_spaced.out)|$(grep -cx 1/3 build/fraction_spaced.out)"
# A NUMBER NAME GIVEN ONE IS REFUSED before anything runs (the author's 9b, 2026-10-12).
"$interpreter" tests/fraction_in_a_number.satl > build/fraction_in_a_number.out 2>&1
code=$?
expect "a number name given 1 / 3 is refused before anything runs: declare a fraction, or write satellite.convert.number" "27|0|1" \
       "$code|$(grep -cx before build/fraction_in_a_number.out)|$(tr '\n' ' ' < build/fraction_in_a_number.out | grep -c 'n was declared satellite.variable.number, and 1 / 3 is a fraction -- declare a fraction, or write satellite.convert.number(1 / 3)')"
# SATELLITE.CONVERT (the author, 2026-10-12). .number is the whole number nearest -- 7 / 2 is 4, -7 / 2
# is -4, 2.4999 is 2 -- with S021 on the screen once a line, and the run goes on; 4 / 2 is 2 and says
# nothing. .whole cuts toward zero and says nothing (his B: what / answered before, so a program keeps
# its answers); .float is the float. The loop rounds three times on one line: one S021, then its count.
"$interpreter" tests/convert_number.satl > build/convert_number.out 2> build/convert_number.err
code=$?
expect "satellite.convert.number rounds (.5 and up is one more, half away from zero), .whole cuts, .float is the float" \
       "0|4|-4|1|2|3|2|3|-3|2|3.5|0|1|1|2|2|3" "$code|$(tr '\n' '|' < build/convert_number.out | sed 's/|$//')"
expect "... S021 once for each line that rounded, never for 4 / 2 or .whole, and the loop's count names its line" "6|0|1" \
       "$(grep -c 'S021 NUMBER_ROUNDED: satellite.convert.number(' build/convert_number.err)|$(grep -c 'convert.number(4 / 2)\|convert.whole' build/convert_number.err)|$(grep -c 'S021 at .*convert_number.satl:21.* happened 3 times in all; it was reported once' build/convert_number.err)"
# + AND - OVER THE LOWEST COMMON DENOMINATOR, NEVER REDUCED (the author's C, 2026-10-12): 7/2 + 1/3 is
# 23/6, 1/4 + 1/4 is 2/4, a number is itself over 1 (7/2 + 1 is 9/2), a float top stays a float (1.5/2 +
# 1/3 is 6.5/6), and a fraction name holds the answer. The last line rounds 23/6 to 4, with its one S021.
"$interpreter" tests/fraction_add.satl > build/fraction_add.out 2> build/fraction_add.err
code=$?
expect "a fraction + or - a fraction or a number is a fraction over the lowest common denominator, never reduced" \
       "0|23/6|19/6|2/4|9/2|3/4|8/12|0/2|-1/6|6.5/6|23/6|4/3|7/6|4" "$code|$(tr '\n' '|' < build/fraction_add.out | sed 's/|$//')"
expect "... and satellite.convert.number of one is the whole number nearest it, with S021 once" "1" \
       "$(grep -c 'S021 NUMBER_ROUNDED: satellite.convert.number(' build/fraction_add.err)"
# SO A NUMBER NAME GIVEN 1 / 2 + 1 / 2 IS REFUSED before anything runs, as a number name given 1 / 2 is.
"$interpreter" tests/fraction_added_in_a_number.satl > build/fraction_added_in_a_number.out 2>&1
code=$?
expect "a number name given 1 / 2 + 1 / 2 is refused before anything runs -- a fraction + a fraction is a fraction" "27|0|1" \
       "$code|$(grep -cx before build/fraction_added_in_a_number.out)|$(tr '\n' ' ' < build/fraction_added_in_a_number.out | grep -cF 'n was declared satellite.variable.number, and 1 / 2 + 1 / 2 is a fraction -- declare a fraction, or write satellite.convert.number(1 / 2 + 1 / 2)')"
# * / % ^ ON A FRACTION ARE NOT BUILT YET: refused by name where they are reached, and before anything
# runs when a fraction name is given one.
"$interpreter" tests/fraction_arithmetic.satl > build/fraction_arithmetic.out 2>&1
code=$?
expect "a fraction under * / % ^ is refused by name where it is reached -- not built yet (+ and - are)" "14|1|1" \
       "$code|$(grep -cx before build/fraction_arithmetic.out)|$(tr '\n' ' ' < build/fraction_arithmetic.out | grep -cF 'the * of 1/3 and 3 is not built yet')"
"$interpreter" tests/fraction_times_in_a_name.satl > build/fraction_times_in_a_name.out 2>&1
code=$?
expect "... and a fraction name given 1/3 * 3 is refused before anything runs" "14|0|1" \
       "$code|$(grep -cx before build/fraction_times_in_a_name.out)|$(tr '\n' ' ' < build/fraction_times_in_a_name.out | grep -cF '1/3 is a fraction, and the * of a fraction is not built yet (+ and - are)')"
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
