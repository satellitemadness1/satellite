# time_test/satellite.library/python/arithmetic.py -- the Python equivalent of
# programs/arithmetic.satl: small whole numbers, a few operators a turn, 1,000,000 turns.
# A line-for-line translation; a satellite number is a Python int (neither has a ceiling).
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.


def main():
    total = 0
    counter = 0
    while counter < 1000000:
        total = total + counter * 3 % 7
        counter = counter + 1
    print(total)
    return


print()
main()
