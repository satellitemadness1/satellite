# time_test/satellite.library/python/empty.py -- the Python equivalent of programs/empty.satl:
# the loop alone, 200,000 turns. A line-for-line translation.
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.


def main():
    counter = 0
    while counter < 200000:

        counter = counter + 1
    print(counter)
    return


print()
main()
