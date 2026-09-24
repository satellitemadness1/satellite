# time_test/satellite.library/python/long_multiply.py -- the Python equivalent of
# programs/long_multiply.satl: m = third * third, floats of 128 places, 200,000 turns.
# A satellite float is a decimal.Decimal: the division and every multiplication are rounded to
# 128 places half away from zero, as satl rounds them. Otherwise line for line.
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.

from decimal import ROUND_HALF_UP, Decimal, getcontext

# A SATELLITE FLOAT: 128 decimal places, rounded half away from zero where an operation does not
# end; 400 digits of working precision so a product is exact before it is rounded.
getcontext().prec = 400
getcontext().rounding = ROUND_HALF_UP
PLACES = Decimal("1E-128")
SHOWN = Decimal("1E-32")


def display(value):
    # satl shows a float rounded half away from zero to 32 places, trailing zeros removed.
    shown = format(value.quantize(SHOWN, rounding=ROUND_HALF_UP), "f")
    if "." in shown:
        shown = shown.rstrip("0").rstrip(".")
    print(shown)


def main():
    third = Decimal(1)
    third = (third / 3).quantize(PLACES, rounding=ROUND_HALF_UP)
    m = Decimal("0.0")
    counter = 0
    while counter < 200000:
        m = (third * third).quantize(PLACES, rounding=ROUND_HALF_UP)
        counter = counter + 1
    display(m)
    return


print()
main()
