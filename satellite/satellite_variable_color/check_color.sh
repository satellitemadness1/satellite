# satellite/satellite_variable_color/check_color.sh -- check.sh's rows for satellite.variable.color, SOURCED
# by check.sh (never run alone): `expect`, `$interpreter` and build/ are check.sh's.
#
# ONE FILE PER TYPE (2026-09-22), so the four types the author asked for at once
# could each be given their rows without four builders editing check.sh's own lines.
# A fixture program for these rows goes in tests/color_*.satl.
#
# A REFUSAL'S SENTENCE IS MATCHED WHOLE. The report wraps it at the screen's width,
# so its lines are joined with a blank before the match -- a row that matched only
# the first wrapped line would pass with the end of the sentence wrong.
color_said() { tr '\n' ' ' < build/color.out | grep -c "$1"; }

# satellite.variable.color (the author, 2026-09-22): "satellite.variable.color my_color =
# x000000 or just 000000 without the x, but a variable.color is always width 6
# hexadecimal number". Shown with its x, upper case; colour is the same word.
expect "satellite.variable.color = 000000, x000000, ff00aa and 00ff00, and satellite.variable.colour = x0000FF" \
       "x000000|x000000|xFF00AA|x00FF00|x0000FF" \
       "$("$interpreter" tests/color.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/color.satl > /dev/null 2>&1; expect "tests/color.satl runs" 0 $?

# ON A LATER LINE the lexer does not know the name is a colour, and six digits still
# read as six digits -- unless a variable has that name, which is then the variable.
expect "a colour given 123456, abcdef, x00ff00, the variable facade, ff00aa, 5 and 000000, 75 on later lines" \
       "x123456|xABCDEF|x00FF00|x0000FF|xFF00AA, 5|x000000, 75" \
       "$("$interpreter" tests/color_later_lines.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"

# TRANSPARENCY, HIS TWO WAYS: "my_color.transparency(0 - 99) and my_color = x000000,
# 0-99 for transparency". Displayed in his own spelling; a solid colour has no ", 0".
"$interpreter" tests/color_transparency.satl > build/color.out 2>/dev/null
expect "tests/color_transparency.satl runs" 0 $?
expect "= x000000, 50 displays x000000, 50, and .transparency(20) makes it x000000, 20" \
       "x000000, 50|x000000, 20" "$(sed -n '1,2p' build/color.out | tr '\n' '|' | sed 's/|$//')"
expect ".transparency and .transparency() read it back: 20" "20|20" \
       "$(sed -n '3,4p' build/color.out | tr '\n' '|' | sed 's/|$//')"
expect "= x00ff00, 99 on a later line, and .transparency(0) leaves the ', 0' off" "x00FF00, 99|x00FF00" \
       "$(sed -n '5,6p' build/color.out | tr '\n' '|' | sed 's/|$//')"

# ITS CONVERSIONS, AND SAMENESS: the digits AND the transparency.
expect "x00FF00 .number .hex .binary .string, then == and != with and without a transparency" \
       "65280|00FF00|000000001111111100000000|x00FF00|true|false|true" \
       "$("$interpreter" tests/color_methods.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"

# SIX DIGITS, before anything runs: five and seven are refused rather than widened.
"$interpreter" tests/color_five_digits.satl > build/color.out 2>&1; expect "a colour declared 00ff0" 27 $?
expect "... says 00ff0 is 5 hex digits and a color is six, before anything runs" "1|" \
       "$(color_said 'satl(check): in satellite.main, ERROR: 00ff0 is 5 hex digits -- a color is exactly six hex digits, like x00FF00 ')|$(grep -x before build/color.out)"
"$interpreter" tests/color_seven_digits.satl > build/color.out 2>&1; expect "a colour given x00ff000 later" 27 $?
expect "... says 00ff000 is 7 hex digits, before anything runs" "1|" \
       "$(color_said 'satl(check): in satellite.main, ERROR: 00ff000 is 7 hex digits -- a color is exactly six hex digits, like x00FF00 ')|$(grep -x before build/color.out)"
"$interpreter" tests/color_split_digits.satl > build/color.out 2>&1; expect "a colour given 00ff00 later" 27 $?
expect "... says write x00ff00, before anything runs" "1|" \
       "$(color_said 'satl(check): in satellite.main, ERROR: write x00ff00 -- on a line after the one that declares a color, digits that start with a number and hold a letter need their x ')|$(grep -x before build/color.out)"
"$interpreter" tests/color_digits_or_variable.satl > build/color.out 2>&1; expect "satellite.variable.color d = facade, where facade is a variable" 27 $?
expect "... is told it reads as digits here, and both ways out, before anything runs" "1|" \
       "$(color_said 'satl(check): in satellite.main, ERROR: facade is read here as hex digits, and it is also a variable -- on the line that declares a color, a run of hex digits is the color.s digits. For the variable, declare the color first and give it facade on the next line; for the digits, write xFACADE ')|$(grep -x before build/color.out)"
"$interpreter" tests/color_sign.satl > build/color.out 2>&1; expect "a colour declared -x00FF00" 27 $?
expect "... says a color has no sign, before anything runs" "1|" \
       "$(color_said 'satl(check): in satellite.main, ERROR: a color has no sign, and this one has a minus in front of it -- x000000 is black, and there is no color below it ')|$(grep -x before build/color.out)"

# A TRANSPARENCY IS 0 TO 99: after the comma it is judged before anything runs, and
# given to .transparency(...) it is judged where it runs.
"$interpreter" tests/color_transparency_100.satl > build/color.out 2>&1; expect "= x000000, 100" 27 $?
expect "... says a transparency is 0 to 99, before anything runs" "1|" \
       "$(color_said 'satl(check): in satellite.main, ERROR: a transparency is a whole number from 0 (solid) to 99 (almost clear), and this is 100 ')|$(grep -x before build/color.out)"
"$interpreter" tests/color_transparency_method_100.satl > build/color.out 2>&1; expect "my_color.transparency(100)" 27 $?
expect "... says a transparency is 0 to 99, after before" "1|before" \
       "$(color_said 'satl(run): in my_color, my_color.transparency(...) -- a transparency is a whole number from 0 (solid) to 99 (almost clear), and it was given 100 ')|$(grep -x before build/color.out)"

# WHAT A COLOUR HAS NOT GOT: a method, before anything runs; an order and arithmetic
# where they run, each by name.
"$interpreter" tests/color_method_not_a_color.satl > build/color.out 2>&1; expect "my_color.append on a colour name" 27 $?
expect "... is told what a color has, before anything runs" "1|" \
       "$(color_said 'satl(check): in satellite.main, my_color.append -- a color has .transparency, .number, .string, .hex and .binary ')|$(grep -x before build/color.out)"
"$interpreter" tests/color_no_order.satl > build/color.out 2>&1; expect "red < blue on two colours" 27 $?
expect "... says only == and != compare two colors" 1 \
       "$(color_said 'satl(run): < was given two colors, and only == and != compare those -- there is no order between two colors ')"
"$interpreter" tests/color_arithmetic.satl > build/color.out 2>&1; expect "red + 1" 27 $?
expect "... says a color has no arithmetic, and where its number is" 1 \
       "$(color_said 'satl(run): + was given a color and a number -- a color has no arithmetic: it is six hex digits and a transparency. Its .number is what the digits are worth, to do sums on ')"

# MIXING THE NEW TYPES IS THE AUTHOR'S "GRAND FINALE": said as not built yet.
"$interpreter" tests/color_meets_hex.satl > build/color.out 2>&1; expect "green == x00FF00" 14 $?
expect "... says a hex meeting a color is not built yet" 1 \
       "$(color_said 'satl(run): a comparison was given a color and a hex -- a hex meeting a color is not built yet: the new types are mixed together later ')"
