# time_test/satellite.library/python/strings.py -- the Python equivalent of
# programs/strings.satl: a string joined to itself and compared, 300,000 turns.
# A line-for-line translation. Python keeps these ASCII strings at one byte a character where
# satl uses 16-bit characters.
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.


def main():
    piece = "the satellite programming language races itself, compiled twice over."
    joined = ""
    same = 0
    counter = 0
    while counter < 300000:
        joined = piece + piece + piece + piece
        if joined == piece + piece + piece + piece:
            same = same + 1
        counter = counter + 1
    print(same)
    return


print()
main()
