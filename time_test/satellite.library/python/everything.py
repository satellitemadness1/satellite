# time_test/satellite.library/python/everything.py -- the Python equivalent of
# programs/everything.satl: every kind of work each turn -- small and large numbers, small and
# large strings, small and large lists, small and large capsules, small and large spacesuits --
# 40,000 turns. A spacesuit is a class (protected members carry a leading underscore; a field's
# default is set before the constructor's body, as satl does) and a capsule is a function.
# NOT LINE FOR LINE, three places:
#   - satl counts list items from 1, so items[counter % items.size + 1] is
#     items[counter % len(items)] here;
#   - station's fuel can reach -1, and satl's % keeps the sign of the left side (-1 % 1000 is -1;
#     Python's is 999), so that one line uses satl_remainder; every other % here only ever sees
#     values of 0 or more, where the two agree;
#   - CPython joins "satl" + "-" + "004" into one constant, and "churn" + "/" into "churn/", when
#     it compiles this file, so those joins cost Python nothing a turn where satl makes them.
# Python keeps these ASCII strings at one byte a character where satl uses 16-bit characters.
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.


def satl_remainder(left, right):
    # satl's %: the remainder takes the sign of the left side (it truncates toward zero).
    remainder = abs(left) % abs(right)
    return -remainder if left < 0 else remainder


# A SMALL SPACESUIT: one field, one capsule. A new one is made every turn.
class point:
    def __init__(self, x_input):
        # satellite.protected
        self._x = 0
        # satellite.constructor
        self._x = x_input

    # satellite.public
    def call_x(self):
        return self._x


# A LARGE SPACESUIT: fourteen fields -- numbers, strings, a list -- a constructor, a
# protected capsule and five public ones. One lives the whole run and is worked every
# turn; another is built new every turn.
class station:
    def __init__(self, name_input):
        # satellite.protected
        self._name = "station"
        self._log = ""
        self._status = "idle"
        self._crew = 0
        self._fuel = 1000
        self._oxygen = 1000
        self._power = 500
        self._heading = 0
        self._speed = 0
        self._docked = 0
        self._alarms = 0
        self._turns = 0
        self._checksum = 7
        self._readings = []
        # satellite.constructor
        self._name = name_input
        self._crew = 3
        self._status = "ready"

    def _settle(self):
        self._fuel = satl_remainder(self._fuel, 1000) + 1
        self._oxygen = self._oxygen % 1000 + 1
        self._power = self._power % 500 + 1

    # satellite.public
    def call_tick(self, n):
        self._turns = self._turns + 1
        self._fuel = self._fuel - n % 3
        self._oxygen = self._oxygen - self._crew % 2
        self._power = self._power + n % 5
        self._heading = (self._heading + n) % 360
        self._speed = self._speed + 1
        self._checksum = (self._checksum * 31 + n) % 1000003
        self._settle()

    def call_record(self, reading):
        self._readings.append(reading)
        if reading % 97 == 0:
            self._alarms = self._alarms + 1
            self._status = "alarm"

    def call_dock(self):
        self._docked = self._docked + 1
        self._log = self._name + " docked"

    def call_checksum(self):
        return self._checksum + self._alarms + self._docked

    def call_readings(self):
        return len(self._readings)


# A SMALL CAPSULE: one line.
def twice(n):
    return n * 2


# A LARGE CAPSULE: about twenty-five statements of mixed work -- numbers, strings, a list
# of its own -- every call.
def churn(n, text):
    a = n % 1000
    b = a * a + 17
    c = b % 13 + a % 7
    d = (a + b + c) % 101
    label = text + "/" + "churn"
    again = "churn" + "/" + text
    matches = 0
    if label == again:
        matches = matches + 1
    if label == text + "/churn":
        matches = matches + 2
    local = [a, b, c, d]
    local.append(a + d)
    local.append(b % 9)
    total = sum(local)
    biggest = max(local)
    e = total % 1000 + biggest % 100
    f = e * 3 + matches
    g = f % 17 + d
    h = g + c * 2
    i = h % 11
    j = i + a % 5
    return j + len(local)


def main():
    # THE LARGE THINGS, made once before the loop: a 978-digit number (3 to the 2048th)
    # and two 30,208-character strings, built apart so comparing them compares every
    # character rather than finding one string twice.
    modulus = 3
    k = 0
    while k < 11:
        modulus = modulus * modulus
        k = k + 1
    big = modulus - 12345
    large = "the satellite programming language, raced against itself. "
    large_copy = "the satellite programming language, raced against itself. "
    k = 0
    while k < 9:
        large = large + large
        large_copy = large_copy + large_copy
        k = k + 1

    items = []
    tiny = [0, 0, 0]
    word = ""
    joined = ""
    small = 0
    same = 0
    counter = 0
    hq = station("hq")
    while counter < 40000:
        # SMALL NUMBERS
        small = small + counter * 3 % 7
        # LARGE NUMBERS: a 978-digit multiply and remainder
        big = (big * 1000003 + counter) % modulus
        # SMALL STRINGS
        word = "satl" + "-" + "004"
        if word == "satl-004":
            same = same + 1
        # LARGE STRINGS: 30,208 characters compared, and copied into a join
        if large == large_copy:
            same = same + 1
        joined = large + word
        # SMALL CONTAINERS
        tiny = [counter, small, counter % 11]
        small = small + sum(tiny) % 5
        # THE LARGE CONTAINER: grows by one a turn, and an item read back from it
        items.append(counter * 7919 % 1000003)
        small = small + items[counter % len(items)] % 3
        # A SMALL CAPSULE AND A LARGE ONE
        small = small + twice(counter) % 3
        churned = churn(counter, word)
        small = small + churned % 5
        # A SMALL SPACESUIT, new every turn
        p = point(counter)
        small = small + p.call_x() % 2
        # A LARGE SPACESUIT: the one that lives the whole run, worked, and one built new
        hq.call_tick(counter)
        hq.call_record(counter * 13 % 1009)
        visitor = station("visitor")
        visitor.call_dock()
        visitor.call_tick(counter)
        small = small + visitor.call_checksum() % 3
        counter = counter + 1

    print(small)
    print(same)
    print(big % 1000000007)
    print(sum(items))
    print(sorted(items)[0])
    print(hq.call_checksum())
    print(hq.call_readings())
    return


print()
main()
