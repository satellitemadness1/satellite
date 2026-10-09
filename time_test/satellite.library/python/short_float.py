# time_test/satellite.library/python/short_float.py -- the Python equivalent of
# programs/short_float.satl: f = f + 0.5, 200,000 turns.
# A satellite float is a decimal.Decimal (the steps of 0.5 are exact, but the semantics are
# satl's); Python has no decimal literal, so the 0.5 in the loop is made once before it, as satl
# makes its literal once when it reads the program. Otherwise line for line.
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.

from decimal import ROUND_HALF_UP, Decimal, getcontext

# A SATELLITE FLOAT: 128 decimal places, rounded half away from zero where an operation does not
# end; 400 digits of working precision so a sum is exact before any rounding.
getcontext().prec = 400
getcontext().rounding = ROUND_HALF_UP
SHOWN = Decimal("1E-32")
HALF = Decimal("0.5")


def display(value):
    # satl shows a float rounded half away from zero to 32 places, trailing zeros removed.
    shown = format(value.quantize(SHOWN, rounding=ROUND_HALF_UP), "f")
    if "." in shown:
        shown = shown.rstrip("0").rstrip(".")
    print(shown)


def main():
    f = Decimal("0.5")
    counter = 0
    while counter < 200000:
        f = f + HALF
        counter = counter + 1
    display(f)
    return


print()
main()
