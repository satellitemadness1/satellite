# time_test/satellite.library/python/big_numbers.py -- the Python equivalent of
# programs/big_numbers.satl: x = x * 3, 150,000 turns, growing to about 71,500 digits.
# A line-for-line translation; a Python int has no ceiling, like a satellite number.
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.


def main():
    x = 1
    counter = 0
    while counter < 150000:
        x = x * 3
        counter = counter + 1
    print(x % 1000000007)
    return


print()
main()
