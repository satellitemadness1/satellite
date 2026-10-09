# satellite/satellite_variable_float/check_float.sh -- check.sh's rows for satellite.variable.float, SOURCED
# by check.sh (never run alone): `expect`, `$interpreter` and build/ are check.sh's.
#
# ONE FILE PER TYPE (2026-09-22), so the four types the author asked for at once
# could each be given their rows without four builders editing check.sh's own lines.
# A fixture program for these rows goes in tests/float_*.satl.
#
# PYTHON'S decimal MODULE IS THE AUTHORITY for every digit below, as it is for the
# percentage's: ROUND_HALF_UP is half away from zero (SATELLITE_INFINITY.md Q17b),
# a float holds arguments.float.decimal places (128) and shows
# arguments.infinity_display (32) of them (Q43).

# A line of output per display, joined with | so a row reads on one line.
float_lines() { "$@" 2>/dev/null | tr '\n' '|' | sed 's/|$//'; }

# THE LITERAL -- FLT-1's done-when: 12.34 displays 12.34, and 12.05 == 12.5 is false.
# 12.50 is 12.5 and 2.0 is 2.0: a zero at the end of the fraction goes, but a float
# always keeps one place, so it never displays as the number it is not.
expect "a float literal displays as written, less the zero at its end (FLT-1: 12.34, and 12.05 == 12.5 is false)" \
       "12.34|12.05|false|12.5|true|2.0|1.25|-12.5|-2.5|0.0|12.34|3" "$(float_lines "$interpreter" tests/float_literal.satl)"

# A PLAIN NUMBER EITHER SIDE is read as the float it is. 4 / 3 is still 1 (Q13):
# two plain numbers never become a float, and 4.0 / 3 is one. 2 ^ -1 is 0.5 (FLT-2),
# and a power a billion deep that rounds to 0.0 says so without building the number.
expect "a float meets a plain number either side; 4 / 3 is still 1, 4.0 / 3 is the float, 2 ^ -1 is 0.5" \
       "3.5|3.5|9.5|-9.5|3.0|4.5|1|1.33333333333333333333333333333333|0.75|true|true|true|0.5|0.0|0.0" \
       "$(float_lines "$interpreter" tests/float_mixed.satl)"

expect "a plain number given to a float name becomes a float, in the declaration and after it" \
       "3.0|7.5|-7.0" "$(float_lines "$interpreter" tests/float_store.satl)"

# 2.0 / 3 AT <places>: what it shows, and -- times 10 ^ 128 -- every place it holds.
float_divide_wanted() {
    python3 -c "
from decimal import Decimal as D, getcontext, ROUND_HALF_UP
getcontext().prec = 400
held = (D(2) / D(3)).quantize(D(1).scaleb(-$1), rounding=ROUND_HALF_UP)
print(format(held.quantize(D(1).scaleb(-min($1, 32)), rounding=ROUND_HALF_UP), 'f').rstrip('0'))
print(format(held.scaleb(128), 'f').split('.')[0] + '.0')" | tr '\n' '|' | sed 's/|$//'
}
expect "2.0 / 3 holds 128 places ending ...667 (FLT-2, Q17b) and shows 32" \
       "$(float_divide_wanted 128)" "$(float_lines "$interpreter" tests/float_divide.satl)"

# THE ROWS, SET FOR ONE MACHINE in config.ini by their names without `arguments.`,
# each in a HOME of its own so the suite's config.ini is never touched.
float_home() {   # float_home <name> <config.ini line>
    mkdir -p "build/float_home_$1/.satl"
    printf '%s\n' "$2" > "build/float_home_$1/.satl/config.ini"
    printf '%s' "$PWD/build/float_home_$1"
}
expect "arguments.float.whole and arguments.float.decimal are rows, 4096 and 128 by default" 2 \
       "$("$interpreter" --debug tests/float_divide.satl 2>&1 | grep -cE '^\[satellite\] arguments\.float\.(whole = 4096|decimal = 128) ')"
expect "... and float.decimal = 10 in config.ini holds 2.0 / 3 to 10 places" \
       "$(float_divide_wanted 10)" "$(float_lines env HOME="$(float_home ten 'float.decimal = 10')" "$interpreter" tests/float_divide.satl)"
expect "... and past float.whole = 3 digits the low digits round to zeros, half away from zero" \
       "123000.0|124000.0|0.12345678901234" \
       "$(float_lines env HOME="$(float_home three 'float.whole = 3')" "$interpreter" tests/float_whole.satl)"
HOME="$(float_home bad 'float.decimal = ten')" "$interpreter" tests/float_divide.satl > build/float_bad_row.out 2>&1
code=$?
expect "... and a config.ini precision that is not a count is refused by name, never replaced by the default" "20|1" \
       "$code|$(tr '\n' ' ' < build/float_bad_row.out | grep -c 'float.decimal = ten is not a precision')"

# EVERY OPERATOR, AGAINST PYTHON. The program is written from the same list the
# answers are worked out from, so the two cannot drift apart.
python3 - build/float_python.satl > build/float_python.wanted <<'FLOAT_PY'
import sys
from decimal import Decimal as D, getcontext, ROUND_HALF_UP
getcontext().prec = 3000
pairs = [("1.5", "+", "2.25"), ("0.1", "+", "0.2"), ("99.999", "+", "0.001"), ("10", "-", "0.5"),
         ("2.5", "-", "10"), ("1.05", "-", "1.05"), ("-2.5", "*", "4"), ("1.25", "*", "1.25"),
         ("0.1", "*", "0.1"), ("123456789.123456789", "*", "987654321.987654321"), ("2.0", "/", "3"),
         ("1", "/", "3.0"), ("-7.0", "/", "3"), ("22.0", "/", "7"), ("1.0", "/", "4"), ("1", "/", "8.0"),
         ("-0.5", "/", "-0.25"), ("0.000001", "/", "3"), ("100", "/", "7.0"), ("5.5", "%", "2"),
         ("-7.5", "%", "2"), ("7.5", "%", "-2"), ("1.5", "^", "3"), ("-1.5", "^", "3"), ("0.5", "^", "10"),
         ("2.0", "^", "0"), ("2", "^", "-1"), ("2.0", "^", "-3"), ("3", "^", "-2"), ("0.5", "^", "400"),
         ("0.5", "^", "1000"), ("2", "^", "-1000"), ("1.1", "^", "-50")]
def rounded(v, places):
    return v.quantize(D(1).scaleb(-places), rounding=ROUND_HALF_UP) if v.as_tuple().exponent < -places else v
def shown(v):
    s = format(rounded(rounded(v, 128), 32), 'f')
    s = (s if '.' in s else s + '.0').rstrip('0')
    s = s + '0' if s.endswith('.') else s
    return s[1:] if s.startswith('-') and D(s) == 0 else s
work = {'+': lambda x, y: x + y, '-': lambda x, y: x - y, '*': lambda x, y: x * y,
        '/': lambda x, y: x / y, '%': lambda x, y: x % y, '^': lambda x, y: x ** int(y)}
for a, op, b in pairs:
    print(shown(work[op](D(a), D(b))))
with open(sys.argv[1], 'w') as program:
    program.write("satellite.include(satellite)\n\nsatellite.capsule satellite.main()\n{\n")
    for a, op, b in pairs:
        program.write(f"    satellite.console.display({a} {op} {b})\n")
    program.write("    satellite.return(satellite)\n}\n")
FLOAT_PY
expect "+ - * / % ^ on floats and numbers agree with Python's decimal, digit for digit ($(wc -l < build/float_python.wanted) answers)" \
       "$(tr '\n' '|' < build/float_python.wanted | sed 's/|$//')" "$(float_lines "$interpreter" build/float_python.satl)"

# THE REFUSALS, each a sentence naming what was written.
"$interpreter" tests/float_refused.satl > build/float_refused.out 2>&1
code=$?
expect "12.5's .number is refused where it is reached, naming the value -- satellite rounds nothing on its own" "24|1|1" \
       "$code|$(grep -cx before build/float_refused.out)|$(tr '\n' ' ' < build/float_refused.out | grep -c '12.5 is not a whole number, so there is no number to make of it')"
"$interpreter" tests/float_method_check.satl > build/float_method_check.out 2>&1
code=$?
expect "a method a float does not have is refused by name before anything runs" "14|0|1" \
       "$code|$(grep -cx before build/float_method_check.out)|$(tr '\n' ' ' < build/float_method_check.out | grep -c 'f.find is not built for satellite.variable.float yet')"
"$interpreter" tests/float_meets_percentage.satl > build/float_percentage.out 2>&1
code=$?
expect "a float meeting a percentage waits for the new types to be mixed together, and says so" "14|1" \
       "$code|$(tr '\n' ' ' < build/float_percentage.out | grep -c 'a float meeting a percentage is not built yet -- the new types are mixed together later')"
"$interpreter" tests/float_into_number.satl > build/float_into_number.out 2>&1
code=$?
# A LITERAL IS JUDGED BEFORE THE RUN since 2026-09-26 (ERRORS2 #8), in a full report, which
# wraps the sentence -- so it is read with its lines joined, as the rows above read theirs.
expect "a float given to a number name is refused: it holds a float" "27|1" \
       "$code|$(tr '\n' ' ' < build/float_into_number.out | grep -c 'n was declared satellite.variable.number, and it holds a float')"
