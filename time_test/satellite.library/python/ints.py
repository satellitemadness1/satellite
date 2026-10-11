# time_test/satellite.library/python/ints.py -- the Python equivalent of programs/ints.satl:
# n = n + 1, 200,000 turns. A line-for-line translation; a satellite number is a Python int.
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.


def main():
    n = 0
    counter = 0
    while counter < 200000:
        n = n + 1
        counter = counter + 1
    print(n)
    return


print()
main()
