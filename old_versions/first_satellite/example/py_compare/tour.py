import math
import sys
import time


def show(v):
    if v is True:
        return "true"
    if v is False:
        return "false"
    if isinstance(v, list):
        return "[" + ", ".join(show(x) for x in v) + "]"
    return str(v)


def display(v):
    print(show(v))


def satellite_slice(v, lo, hi):
    n = len(v)
    if lo < 0:
        lo += n
    if hi < 0:
        hi += n
    lo = max(0, min(lo, n))
    hi = max(0, min(hi, n))
    if hi < lo:
        hi = lo
    return v[lo:hi]


total = 0


class item:
    def __init__(self, name, amount):
        self.label = name
        self.count = amount

    def name(self):
        return self.label

    def quantity(self):
        return self.count

    def restock(self, amount):
        self.count = self.count + amount

    def describe(self):
        return self.label + " x" + show(self.count)


class crate(item):
    def __init__(self, name, amount):
        super().__init__(name, amount)
        self.per_box = 12

    def boxes(self):
        return math.floor(self.count / self.per_box)

    def describe(self):
        return self.label + " x" + show(self.count) + " in " + show(self.boxes()) + " boxes"


def fact(n):
    if n <= 1:
        return 1

    return n * fact(n - 1)


def is_even(n):
    if n == 0:
        return True

    return is_odd(n - 1)


def is_odd(n):
    if n == 0:
        return False

    return is_even(n - 1)


def tally(n):
    global total
    total = total + n

    return total


def report(thing):
    return thing.describe()


def main(argz):
    a = 17
    b = 5

    display(a + b)
    display(a - b)
    display(a * b)
    display(a / b)
    display(a % b)
    display(-a)
    display(a + b)
    display(a - b)
    display(a * b)
    display(a / b)
    display(a % b)
    display(math.floor(a / b))
    display(math.ceil(a / b))
    display(round(a / b))
    display(abs(-a))
    display(fact(18))

    display(a > b)
    display(a >= b)
    display(a < b)
    display(a <= b)
    display(a == 17)
    display(a != b)

    yes = True
    no = False

    display(not yes)
    display(yes and no)
    display(yes or no)
    display(not no)

    greeting = "hello, satellite"

    display(greeting)
    display(len(greeting))
    display("sat" in greeting)
    display(greeting.startswith("hello"))
    display(greeting.endswith("lite"))
    display(greeting + "!")
    display(greeting + "!")
    display(greeting[0])
    display(greeting[-1])
    display(satellite_slice(greeting, 7, len(greeting)))
    display(satellite_slice(greeting, 0, 5))
    display(satellite_slice(greeting, 7, 10))

    primes = []

    primes.append(2)
    primes.append(3)
    primes.append(5)
    primes.append(7)
    primes.append(11)

    display(primes)
    display(len(primes))
    display(primes[0])
    display(primes[-1])
    display(7 in primes)
    display(4 in primes)
    display(primes[2])
    display(primes[-1])
    display(satellite_slice(primes, 1, 4))
    display(satellite_slice(primes, 0, 2))
    display(satellite_slice(primes, 3, len(primes)))
    display(satellite_slice(primes, 2, 100))

    grid = []

    grid.append(satellite_slice(primes, 0, 2))
    grid.append(satellite_slice(primes, 3, len(primes)))

    display(grid)

    i = 0

    while i < 3:
        display(i)

        i = i + 1

    for j in range(0, 3):
        display(j * j)

    if a > b:
        display("a wins")
    elif a == b:
        display("tie")
    else:
        display("b wins")

    display(is_even(10))
    display(is_odd(10))
    display(tally(4))
    display(tally(6))

    bolt = item("bolt", 40)

    display(bolt.name())
    display(bolt.quantity())
    display(bolt.describe())

    bolt.restock(8)

    display(bolt.describe())

    alias = bolt

    alias.restock(2)

    display(bolt.describe())
    display(bolt is alias)

    washer = crate("washer", 100)

    display(washer.boxes())
    display(washer.describe())
    display(report(bolt))
    display(report(washer))

    started = time.time_ns()
    ended = time.time_ns()

    display(ended - started >= 0)
    display(len(argz) >= 1)

    return 0


sys.exit(main(sys.argv))
