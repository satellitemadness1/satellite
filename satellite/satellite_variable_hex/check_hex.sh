# satellite/satellite_variable_hex/check_hex.sh -- check.sh's rows for satellite.variable.hex, SOURCED
# by check.sh (never run alone): `expect`, `$interpreter` and build/ are check.sh's.
#
# ONE FILE PER TYPE (2026-09-22), so the four types the author asked for at once
# could each be given their rows without four builders editing check.sh's own lines.
# A fixture program for these rows goes in tests/hex_*.satl.

# satellite.variable.hex (the author, 2026-09-22): "similar thing for hex, which has an
# x in front of it, with hex numbers only for it 0 - 9 A - F". Shown as written, as
# wide as written; case is not part of the value, so xff shows as xFF (003 DESIGN 8.5).
expect "satellite.variable.hex my_number = x1F, and x00FF, xff and -x1F shown as a hex" \
       "x1F|x00FF|xFF|-x1F" \
       "$("$interpreter" tests/hex.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/hex.satl > /dev/null 2>&1; expect "tests/hex.satl runs" 0 $?

# BY WORTH, ANSWERING A NUMBER, as every x literal did when it was a number and as a
# binary does -- and `b1100 + xFF` is still the 267 tests/arithmetic.satl pins. Two
# hexes are equal only as wide as each other: x00FF is not xFF.
"$interpreter" tests/hex_by_worth.satl > build/hex.out 2>/dev/null; expect "tests/hex_by_worth.satl runs" 0 $?
expect "x1F + 1 is 32, x1F == 31, x00FF == xFF is false, x00ff == x00FF, x10 * x10, b1100 + xFF" \
       "32|true|false|true|256|267" "$(sed -n '1,6p' build/hex.out | tr '\n' '|' | sed 's/|$//')"
expect "a hex in a satellite.variable.number name keeps what it is worth: x1F is 31, and 31 + 1" \
       "31|32" "$(sed -n '7,8p' build/hex.out | tr '\n' '|' | sed 's/|$//')"

# THE METHODS: worth, text as shown, four bits to the digit, its own digits as wide as
# written, the width in bits (003 DESIGN 8.5: "width() COUNTS BITS"), reversed.
expect "x00FF .number .string .binary .hex .width .reverse(), and (-x1F).hex" \
       "255|x00FF|0000000011111111|00FF|16|xFF00|-1F" \
       "$("$interpreter" tests/hex_methods.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"

# THE x IS REQUIRED (the author, of the b: "just make the prefix b mandatory and move
# on"), before anything runs, on the declaration and on every later `name = ...`.
"$interpreter" tests/hex_without_x.satl > build/hex_x.out 2>&1; expect "a hex with no x: 1F" 27 $?
expect "... says ERROR: expected x1F, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected x1F$' build/hex_x.out)|$(grep -x before build/hex_x.out)"
"$interpreter" tests/hex_assigned_without_x.satl > build/hex_x.out 2>&1; expect "a hex given FF later" 27 $?
expect "... says ERROR: expected xFF, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected xFF$' build/hex_x.out)|$(grep -x before build/hex_x.out)"
"$interpreter" tests/hex_digits_not_hex.satl > build/hex_x.out 2>&1; expect "a hex given x1G" 27 $?
expect "... is told x1G is not hex, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: x1G is not hex' build/hex_x.out)|$(grep -x before build/hex_x.out)"
"$interpreter" tests/hex_method_not_a_hex.satl > build/hex_x.out 2>&1; expect "my_number.append on a hex name" 27 $?
expect "... is told what a hex has, before anything runs" "1|" \
       "$(grep -c 'satl(check).*my_number.append -- a hex has .number, .string,' build/hex_x.out)|$(grep -x before build/hex_x.out)"

# A SUM ON A HEX IS A NUMBER, and a hex name refuses it rather than invent a width.
"$interpreter" tests/hex_sum_in_a_hex.satl > build/hex_x.out 2>&1; expect "a hex name given my_number + x01" 27 $?
expect "... says it was given the number 32, and both ways out" 1 \
       "$(grep -c 'given the number 32 -- a hex name holds only a hex, and arithmetic on a hex answers a number, .*write the hex it should be: x20' build/hex_x.out)"

# MIXING THE NEW TYPES IS THE AUTHOR'S "GRAND FINALE": said as not built yet.
"$interpreter" tests/hex_meets_percentage.satl > build/hex_x.out 2>&1; expect "x1F + 50%" 14 $?
expect "... says a percentage meeting a hex is not built yet" 1 \
       "$(grep -c 'a percentage meeting a hex is' build/hex_x.out)"

# THIS RUN'S SECOND SPELLINGS (words/aliases.tsv): each declares the first spelling's type.
"$interpreter" tests/hex_second_spellings.satl > build/hex.out 2>/dev/null; expect "tests/hex_second_spellings.satl runs" 0 $?
expect "satellite.variable.hexadecimal h = x1F displays x1F" "x1F" "$(sed -n '1p' build/hex.out)"
expect "satellite.variable.bin b = b1010 displays b1010" "b1010" "$(sed -n '2p' build/hex.out)"
expect "satellite.variable.percent p = 50% displays 50%" "50%" "$(sed -n '3p' build/hex.out)"
