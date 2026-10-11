# time_test/satellite.library/python/lists.py -- the Python equivalent of programs/lists.satl:
# 300,000 appends, then the whole list summed, its largest found, and sorted.
# A line-for-line translation: items.sum is sum(items), items.max is max(items), and
# items.sort().first is sorted(items)[0] -- a sorted copy, then its first item, as satl does.
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.


def main():
    items = []
    counter = 0
    while counter < 300000:
        items.append(counter * 7919 % 1000003)
        counter = counter + 1
    print(sum(items))
    print(max(items))
    print(sorted(items)[0])
    return


print()
main()
