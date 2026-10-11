#!/usr/bin/env python3
# satellite/satellite_variable_infinity/infinity_oracle.py -- the infinity, power,
# sat, asat ... worked out EXACTLY, before any C++ exists (SATELLITE_INFINITY.md).
#
# Every computed table and figure in SATELLITE_INFINITY.md is this file's output,
# pasted verbatim, so the spec can be re-run instead of re-read. When the C++ lands
# (INF-2 onward) it is checked against this the way check_numbers.py checks
# satellite_number against Python's int: the value column must match, and a row
# that reads ERROR must be refused.
#
# WHAT IT MODELS: arithmetic where at least one side is in the infinity family.
# Two plain numbers are 004's own arithmetic -- whole-number `/`, percentages
# rounded half away from zero -- and are NOT modelled here; `4 / 3` is 1 in satl.
#
# THE ENGINE IS EXACT. A value is a list of TERMS, largest first: (exponent,
# count). The unit of a term is infinity to that exponent, and the exponent is
# itself a value, so the definition is recursive:
#
#     5                      [(0, 5)]                       5, a plain number
#     infinity               [(1, 1)]                       (infinity)
#     infinity - 500         [(1, 1), (0, -500)]            (infinity, -500): the author's
#                                                           REGISTER is every term after the first
#     infinity.power_of(2)   [(2, 1)]                       (infinity^2)
#     infinity.power_of(inf) [(infinity, 1)]                (infinity-1), a power
#
# This is Conway's arithmetic of surreal numbers written in Cantor's normal form
# (sums of count x infinity^exponent, commutative), not Cantor's ordinal arithmetic:
# here 1 + infinity is `(infinity, +1)`, and infinity - 5 exists.
#
# The width (arguments.infinity, 128) never enters an answer. It bounds how many
# decimal places a count may have, and it is how many nines `.nines()` shows.
#
# The normal form was written by the design panel of 2026-09-18 (its soundness
# lens); the type read from exponent height is the panel judge's; the rung names
# `infinity-1`, `infinity-2` are the author's (message six), and one number in one set
# of parentheses with every attached number signed is his (message ten). Showing the
# exponent between the named rungs (`(infinity^3)`) is this file's proposal (Q5).
#
#     python3 satellite/satellite_variable_infinity/infinity_oracle.py

import sys
from decimal import Decimal, getcontext
from fractions import Fraction as F
from functools import cmp_to_key

sys.setrecursionlimit(100000)  # Python's guard, not satellite's: the C++ keeps its own stack

PLACES = 128                   # arguments.infinity: the most decimal places a count may have
SHOWN = 32                     # arguments.infinity_display


class SatError(Exception):
    """What satellite answers with an ERROR, never a guess."""


# ---------------------------------------------------------------------------
# THE NORMAL FORM: a tuple of (exponent, count), largest exponent first, no two
# equal exponents, no zero count.
# ---------------------------------------------------------------------------
ZERO = ()
def num(c):
    c = F(c)
    return () if c == 0 else ((ZERO, c),)
ONE = num(1)
INF = ((ONE, F(1)),)

def sign(a): return 0 if not a else (1 if a[0][1] > 0 else -1)
def neg(a): return tuple((e, -c) for e, c in a)
def cmp(a, b):
    """THE ORDER: the sign of the leading term of a - b. A larger unit wins only when
    its count is positive, so -1 infinity is below every number."""
    return sign(add(a, neg(b)))

def add(a, b):
    """THE ONE RULE (the author): "either add the number or flip the sign of the
    number THEN add the number". Like units add their counts; unlike ones are
    attached, sorted largest first. Every operation below ends here."""
    acc = {}
    for e, c in a + b:
        acc[e] = acc.get(e, F(0)) + c
    terms = [(e, c) for e, c in acc.items() if c != 0]
    terms.sort(key=cmp_to_key(lambda x, y: -cmp(x[0], y[0])))
    return tuple(terms)

def sub(a, b): return add(a, neg(b))

def mul(a, b):
    """Every term by every term: counts multiply, exponents add, and the one rule
    adds the pieces. infinity * infinity is infinity^2."""
    out = ()
    for ea, ca in a:
        for eb, cb in b:
            out = add(out, ((add(ea, eb), ca * cb),))
    return out

def is_finite(a): return all(e == ZERO for e, _ in a)
def finite_val(a): return a[0][1] if a else F(0)

def div(a, b):
    if not b:
        raise SatError("division by zero")
    if len(b) == 1:                                   # one term: exponents subtract
        eb, cb = b[0]
        out = tuple((sub(e, eb), c / cb) for e, c in a)
        if any(sign(e) < 0 for e, _ in out):
            raise SatError("the answer has a part infinitely small; no type holds it before INF-8")
        return add(out, ())
    if not all(is_finite(e) for e, _ in a + b):       # several terms: long division, finite exponents only
        raise SatError("dividing by a sum with an infinite exponent: no ending is known")
    q, r = (), a
    while r and cmp(r[0][0], b[0][0]) >= 0:
        t = ((sub(r[0][0], b[0][0]), r[0][1] / b[0][1]),)
        q = add(q, t)
        r = sub(r, mul(t, b))
    if r:
        raise SatError("does not divide exactly: the answer is an endless series of infinitely small parts")
    return q

def power_of(a, e):
    if is_finite(a) and is_finite(e):
        raise SatError("two plain numbers: 004's own arithmetic, not modelled here")
    if is_finite(e):
        x = finite_val(e)
        if x.denominator == 1 and x >= 0:             # a whole exponent: repeated multiply
            k, out, base = int(x), ONE, a
            while k:
                if k & 1:
                    out = mul(out, base)
                base = mul(base, base)
                k >>= 1
            return out
    if is_finite(a):                                  # a plain number to an infinite power
        x = finite_val(a)
        if x == 0 or x == 1:
            return num(x)
        raise SatError(f"{number_text(x)} to an infinite power has no type")
    if len(a) == 1:
        ea, ca = a[0]
        if ca != 1:
            raise SatError(f"a count other than 1 ({number_text(ca)}) raised to an infinite or non-whole power is not built")
        ne = mul(ea, e)                               # (infinity^x)^y is infinity^(x*y); checked() judges ne
        return ((ne, F(1)),) if ne else ONE
    raise SatError("a sum raised to an infinite or non-whole power is an endless series")


# ---------------------------------------------------------------------------
# SATELLITE'S RULE ON TOP: every count, the plain term's included, is a decimal
# that ENDS, within arguments.infinity places. Cut instead, (infinity / 3) * 3 -
# infinity would be -10^-128 infinities: below -10^1000, an error of infinite size.
# ---------------------------------------------------------------------------
def places(c):
    d, k2, k5 = F(c).denominator, 0, 0
    while d % 2 == 0:
        d //= 2; k2 += 1
    while d % 5 == 0:
        d //= 5; k5 += 1
    return None if d != 1 else max(k2, k5)

def checked(v):
    """Every term, and every term of every exponent, all the way down. Until INF-8
    (message nine) an exponent below 1 other than 0 is refused: it would be an
    infinitely small number (negative) or one between the numbers and 1 infinity."""
    for e, c in v:
        p = places(c)
        if p is None:
            raise SatError(f"a count of {c.numerator}/{c.denominator} never ends; cut, it would be wrong by an infinite amount")
        if p > PLACES:
            raise SatError(f"a count needs {p} places, more than arguments.infinity ({PLACES})")
        if e != ZERO:
            if sign(e) < 0:
                raise SatError("a negative exponent is infinitely small; no type holds it before INF-8")
            if cmp(e, ONE) < 0:
                raise SatError(f"an exponent of {display(e)} lies between the numbers and one infinity; no type holds it before INF-8")
            checked(e)
    return v

def t_add(a, b): return checked(add(a, b))
def t_sub(a, b): return checked(sub(a, b))
def t_mul(a, b): return checked(mul(a, b))
def t_div(a, b): return checked(div(a, b))
def t_pow(a, b): return checked(power_of(a, b))
def percent(p): return num(F(p) / 100)               # 004's % is percent-OF: 10 - 50% prints 5
def satellite_infinity(x=None):                       # message eight: satellite.infinity(x) is infinity ** x
    return INF if x is None else t_pow(INF, x)


# ---------------------------------------------------------------------------
# THE TYPE IS HOW HIGH THE EXPONENT TOWER GOES. Rank 0 infinity, 1 power, 2 sat,
# 3 asat ... read from the LEADING term; rank k is shown `infinity-k`, and the
# canonical infinity-k is a tower of k+1 infinities.
# ---------------------------------------------------------------------------
def rank(a):
    if not a or a[0][0] == ZERO:
        return None                                   # a plain number
    e = a[0][0]
    c = cmp(e, ONE)
    if c == 0:
        return 0
    if c < 0:
        raise SatError("an exponent between 0 and 1 has no type")
    r = rank(e)
    return 1 if r is None else r + 1

def unit(k):
    """The canonical infinity-k, which satellite.<name>() makes. unit(0) is infinity."""
    u = INF
    for _ in range(k):
        u = power_of(INF, u)
    return u

def letters(k):                                       # bijective base 26: 1 a .. 26 z, 27 aa
    s = ''
    while k > 0:
        k, r = divmod(k - 1, 26)
        s = chr(97 + r) + s
    return s

def name(r): return ['infinity', 'power', 'sat'][r] if r < 3 else letters(r - 2) + 'sat'

def rank_of_name(n):
    if n in ('infinity', 'power', 'sat'):
        return ['infinity', 'power', 'sat'].index(n)
    k = 0
    for ch in n[:-3]:
        k = k * 26 + ord(ch) - 96
    return k + 2

def path(r):
    """The author's layout; three letters and more nest one folder a letter (derived)."""
    n = name(r)
    if r == 0:
        return '/infinity/infinity.satl'
    p = n[:-3]
    if r < 3 or len(p) == 1:
        return f'/infinity/{n}/{n}_object.satl'
    return '/infinity/' + '/'.join(p[:-1]) + f'/{n}_object.satl'


# ---------------------------------------------------------------------------
# DISPLAY, the author's (message ten): one number is one set of parentheses -- he
# likens it to how a python list is displayed -- `(infinity, -500000000000000)`,
# `(infinity.infinity, +90440393845)`.
# The first term shows a sign only when it is negative; every attached term shows its
# own `+` or `-`; a count of 1 is not printed. An exponent that is itself in the family
# is shown as its own parentheses ("it is still just adding sets of parentheses"). A
# plain number -- what is left when an infinity goes away -- shows bare. A level's
# dash touches its word (`infinity-1`), and an attached term's sign follows a comma,
# so the two never meet.
# ---------------------------------------------------------------------------
def number_text(c):
    c = F(c)
    p = places(c)
    if p is None:
        raise SatError("a number that never ends is never shown cut")
    if p == 0:
        return str(c.numerator)
    s = f"{abs(c.numerator) * 10 ** p // c.denominator:0{p + 1}d}"
    return ('-' if c < 0 else '') + s[:-p] + '.' + s[-p:]

def unit_text(e):
    if cmp(e, ONE) == 0:
        return 'infinity'
    r = rank(e)
    if r is not None and cmp(e, unit(r)) == 0:
        return f'infinity-{r + 1}'
    return 'infinity^' + (number_text(finite_val(e)) if is_finite(e) else display(e))

def term_text(e, c, first):
    size = abs(c)
    if e == ZERO:
        body = number_text(size)
    else:
        body = unit_text(e) if size == 1 else f'{number_text(size)} {unit_text(e)}'
    return ('-' if c < 0 else '') + body if first else ('-' if c < 0 else '+') + body

def display(a):
    if not a:
        return '0'
    if is_finite(a):
        return number_text(finite_val(a))            # a plain number shows bare
    return '(' + ', '.join(term_text(e, c, k == 0) for k, (e, c) in enumerate(a)) + ')'


# ---------------------------------------------------------------------------
# THE INFINITY COUNTER AND ITS WARNING (message ten). The counter moves "whenever the
# number changes" (the author, answering Q39): a calculation whose answer comes back
# into the object with the same type adds 1; reading the object does not. At arguments.infinity.counter (999,999,999) satl prints the
# warning and sets the counter to 0. When the object is "destroyed and turned into a
# different class object", the counter is destroyed with it.
# ---------------------------------------------------------------------------
WIDTH = 80

def infinity_warning(name):
    rule = '-' * WIDTH
    def centered(s):
        return ' ' * max(0, (WIDTH - len(s)) // 2) + s
    return '\n'.join(['', rule, centered('SATELLITE INFINITY WARNING'), rule, '',
                      centered(f'OBJECT: "{name}"'), centered('WILL NEVER REACH INFINITY'), '', rule, '']) + '\n'

def type_of(v):
    return 'number' if is_finite(v) else rank(v)

def rung_of(v):
    """Q42's other reading: the type changes only at a NAMED rung (infinity-k)."""
    if is_finite(v):
        return 'number'
    k = 0
    while cmp(v, unit(k + 1)) >= 0:
        k += 1
    return k

class Slot:
    """One named variable holding an infinity-family value, and its counter."""
    def __init__(self, name, value, counter_row, kind=type_of):
        self.name, self.value, self.counter, self.row, self.kind = name, value, 0, counter_row, kind

    def calculate(self, answer, into_self=True):
        """A calculation with this object. into_self: the answer is written back into it,
        which is when its number changes; otherwise it was only read, and nothing counts."""
        if not into_self:
            return 'read, not changed: no count'
        if self.kind(answer) != self.kind(self.value):
            self.value, self.counter = answer, 0     # destroyed: a new object, a new counter
            return 'destroyed, counter 0' if not is_finite(answer) else 'destroyed; a plain number has no counter'
        self.value = answer
        if is_finite(self.value):
            return 'a plain number: no counter'
        self.counter += 1
        if self.counter >= self.row:
            self.counter = 0
            return 'WARNING, counter 0'
        return f'counter {self.counter}'

def nines(count, width=PLACES):
    """.nines(): the count in nines, (C): |count| - 10^-w with the sign in front, where w
    is the nines width -- widened past the count's own last digit when the count has
    as many places, so the nines always come after it. Shown to min(w, SHOWN) places,
    cut. Derived for the view; never used in an answer."""
    w = max(width, places(count) + 1)
    shown = min(w, SHOWN)
    v = abs(F(count)) - F(1, 10 ** w)
    whole = v.numerator // v.denominator
    frac = v - whole
    return ('-' if count < 0 else '') + f'{whole}.' + str(frac.numerator * 10 ** shown // frac.denominator).rjust(shown, '0')


# ---------------------------------------------------------------------------
# THE TABLES IN SATELLITE_INFINITY.md
# ---------------------------------------------------------------------------
def row(label, thunk):
    try:
        v = thunk()
        r = rank(v)
        print(f'    {label:40s} {display(v):36s} {"number" if r is None else name(r)}')
    except SatError as refusal:
        print(f'    {label:40s} ERROR: {refusal}')

def truth(label, b): print(f'    {label:40s} {"true" if b else "false"}')

def tables():
    inf, p, s = INF, power_of(INF, INF), unit(2)
    print('THE INFINITY')
    row('satellite.infinity()', lambda: inf)
    row('inf + inf', lambda: t_add(inf, inf))
    row('inf * 50%', lambda: t_mul(inf, percent(50)))
    row('inf - 50%', lambda: t_sub(inf, t_mul(inf, percent(50))))
    row('inf - 500000000000000000', lambda: t_sub(inf, num(500000000000000000)))
    row('inf + 7', lambda: t_add(inf, num(7)))
    row('(inf - 5) * 8', lambda: t_mul(t_sub(inf, num(5)), num(8)))
    row('(inf + 6) / 4', lambda: t_div(t_add(inf, num(6)), num(4)))
    row('inf * -1', lambda: t_mul(inf, num(-1)))
    row('inf + inf - inf', lambda: t_sub(t_add(inf, inf), inf))
    x = inf
    for _ in range(1000):
        x = t_sub(x, ONE)
    row('x = x - 1, a thousand times', lambda: x)
    truth('(inf - 1) - 1 == inf - 2', cmp(sub(sub(inf, ONE), ONE), sub(inf, num(2))) == 0)
    truth('inf > 10 ^ 100', cmp(inf, num(10 ** 100)) > 0)
    truth('inf - 10 ^ 200 > 10 ^ 300', cmp(sub(inf, num(10 ** 200)), num(10 ** 300)) > 0)
    truth('inf + inf > inf + 10 ^ 300', cmp(add(inf, inf), add(inf, num(10 ** 300))) > 0)
    truth('inf * -1 < 5', cmp(neg(inf), num(5)) < 0)
    truth('inf * -1 < 0 - 10 ^ 300', cmp(neg(inf), num(-10 ** 300)) < 0)

    print('\nDIVIDING')
    row('inf / 4', lambda: t_div(inf, num(4)))
    row('inf / 3', lambda: t_div(inf, num(3)))
    row('(inf * 3 + 1) / 3', lambda: t_div(t_add(t_mul(inf, num(3)), ONE), num(3)))
    row('inf / 10 ^ 200', lambda: t_div(inf, num(10 ** 200)))
    row('(inf * inf - 1) / (inf - 1)', lambda: t_div(t_sub(t_mul(inf, inf), ONE), t_sub(inf, ONE)))
    row('5 / inf', lambda: t_div(num(5), inf))

    print('\nWHEN THE INFINITY GOES AWAY')
    row('inf - inf', lambda: t_sub(inf, inf))
    row('(inf + 5) - inf', lambda: t_sub(t_add(inf, num(5)), inf))
    row('inf / inf', lambda: t_div(inf, inf))
    row('inf * 0', lambda: t_mul(inf, num(0)))
    row('(inf + 6) / 4 - inf / 4', lambda: t_sub(t_div(t_add(inf, num(6)), num(4)), t_div(inf, num(4))))

    print('\nA ROUNDED PERCENTAGE GOING IN (satl prints 100% / 3 as 33.33333333333333333333333333333333%)')
    third = F(round(F(100, 3) * 10 ** 32), 10 ** 32) / 100
    row('inf * (100% / 3)', lambda: t_mul(inf, num(third)))
    row('inf * (100% / 3) * 3 - inf', lambda: t_sub(t_mul(t_mul(inf, num(third)), num(3)), inf))

    print('\nTHE POWER (infinity-1)')
    row('inf * inf', lambda: t_mul(inf, inf))
    row('inf * inf * inf', lambda: t_mul(t_mul(inf, inf), inf))
    row('inf.power_of(2) - inf', lambda: t_sub(t_pow(inf, num(2)), inf))
    row('inf.power_of(100000)', lambda: t_pow(inf, num(100000)))
    row('inf.power_of(inf)', lambda: p)
    row('satellite.infinity(satellite.infinity())', lambda: satellite_infinity(satellite_infinity()))
    row('satellite.infinity(2)', lambda: satellite_infinity(num(2)))
    row('p + p', lambda: t_add(p, p))
    row('p * 8', lambda: t_mul(p, num(8)))
    row('p - inf', lambda: t_sub(p, inf))
    row('p - inf + 5', lambda: t_add(t_sub(p, inf), num(5)))
    row('p * p', lambda: t_mul(p, p))
    row('p / inf', lambda: t_div(p, inf))
    row('inf.power_of(inf + 1)', lambda: t_pow(inf, add(inf, ONE)))
    row('(inf * inf).power_of(inf)', lambda: t_pow(t_mul(inf, inf), inf))
    row('(inf + 5).power_of(inf)', lambda: t_pow(add(inf, num(5)), inf))
    row('(inf + inf).power_of(inf)', lambda: t_pow(add(inf, inf), inf))
    row('inf.power_of(-1)', lambda: t_pow(inf, num(-1)))
    row('inf.power_of(50%)', lambda: t_pow(inf, percent(50)))
    row('(inf * inf).power_of(50%)', lambda: t_pow(t_mul(inf, inf), percent(50)))
    row('inf.power_of(150%)', lambda: t_pow(inf, percent(150)))
    row('inf.power_of(150%) / inf', lambda: t_div(t_pow(inf, percent(150)), inf))
    row('2.power_of(inf)', lambda: t_pow(num(2), inf))
    truth('p > inf * 10 ^ 100', cmp(p, mul(inf, num(10 ** 100))) > 0)
    truth('inf * inf * inf < p', cmp(mul(mul(inf, inf), inf), p) < 0)
    truth('1000 * p < p * p', cmp(mul(p, num(1000)), mul(p, p)) < 0)
    truth('p - inf < p + 5', cmp(sub(p, inf), add(p, num(5))) < 0)
    truth('p * -1 < inf', cmp(neg(p), inf) < 0)
    y = inf
    for _ in range(99):
        y = t_mul(y, y)
    row('my_inf = my_inf * my_inf, 99 times', lambda: y)
    truth('... and that is still < p', cmp(y, p) < 0)

    print('\nSAT AND ABOVE')
    row('inf ** inf ** inf  (right to left)', lambda: t_pow(inf, t_pow(inf, inf)))
    row('(inf ** inf) ** inf', lambda: t_pow(p, inf))
    row('satellite.sat()', lambda: s)
    row('p.power_of(p)', lambda: t_pow(p, p))
    row('inf ** inf ** inf ** inf', lambda: t_pow(inf, t_pow(inf, t_pow(inf, inf))))
    row('satellite.asat()', lambda: unit(3))
    row('satellite.sat() + satellite.power()', lambda: t_add(s, p))
    row('satellite.sat() - 1', lambda: t_sub(s, ONE))
    row('satellite.sat().power_of(inf)', lambda: t_pow(s, inf))
    row('satellite.sat().power_of(satellite.sat())', lambda: t_pow(s, s))
    z = inf
    for _ in range(300):
        z = t_pow(inf, z)
    row('x = inf.power_of(x), 300 passes from inf', lambda: z)
    big = mul(power_of(inf, mul(inf, num(10 ** 6))), num(10 ** 6))
    truth('satellite.sat() > 10 ^ 6 * inf.power_of(inf * 10 ^ 6)', cmp(s, big) > 0)
    truth('satellite.zsat() < satellite.aasat()', cmp(unit(28), unit(29)) < 0)

    print('\nA COUNT IN NINES, FOR .nines() (128 nines, shown cut to 32)')
    for c in (1, 2, F(1, 2), 8, -1):
        print(f'    count {number_text(c):4s} {nines(c)}')
    a_two = 2 * (1 - F(1, 10 ** 10))                  # (A): the nines ARE the count, and 2 infinities add them
    print(f'    count 2, .resize(10):     (C) {nines(2, width=10)}   (A) {number_text(a_two)}')
    print(f'    count 10^-20, .resize(10): {nines(F(1, 10 ** 20), width=10)}   (the nines follow the last digit)')

    print('\nTHE COUNTER: arguments.infinity.counter set to 3, my_inf = my_inf * my_inf')
    slot = Slot('my_inf', inf, 3)
    for k in range(1, 9):
        said = slot.calculate(t_mul(slot.value, slot.value))
        print(f'    pass {k}   {name(type_of(slot.value)):8s} {display(slot.value):32s} {said}')
    other = Slot('my_inf', inf, 3, kind=rung_of)
    warned = [k for k in range(1, 9) if other.calculate(t_mul(other.value, other.value)).startswith('WARNING')]
    print(f'    the other reading (Q42), the type changes only at a named rung: warnings at passes {warned}')
    slot = Slot('x', inf, 3)
    steps = [('y = x + 1', lambda v: t_add(v, ONE), False)] + \
            [('x = x * 2', lambda v: t_mul(v, num(2)), True)] * 2 + \
            [('x = x * x', lambda v: t_mul(v, v), True)] + \
            [('x = x * 2', lambda v: t_mul(v, num(2)), True)] * 3 + \
            [('x = x - x', lambda v: t_sub(v, v), True), ('x = x + 1', lambda v: t_add(v, ONE), True)]
    for what, answer, into_self in steps:
        said = slot.calculate(answer(slot.value), into_self)
        kind = type_of(slot.value)
        print(f'    {what:18s} x is {"number" if kind == "number" else name(kind):8s} {said}')

    print('\nTHE WARNING (80 columns; one empty line before and after)')
    for line in infinity_warning('my_inf').split('\n')[:-1]:
        print('    ' + line if line else '')

    print('\nWHAT THEY ARE FOR: which grows faster, with n put in as infinity')
    n = inf
    slow = t_add(t_mul(num(1000), t_mul(n, n)), n)
    fast = t_mul(num(F(1, 1000)), t_mul(t_mul(n, n), n))
    row('1000 * n * n + n', lambda: slow)
    row('0.001 * n * n * n', lambda: fast)
    truth('the second is larger in the end', cmp(fast, slow) > 0)
    row('(n * n + 1) / (n * n - n)', lambda: t_div(t_add(t_mul(n, n), ONE), t_sub(t_mul(n, n), n)))

    print('\nNAMES AND FOLDERS')
    for r in (0, 1, 2, 3, 4, 5, 28, 29, 30, 31, 32, 54, 55, 704, 705):
        assert rank_of_name(name(r)) == r
        shown = 'infinity' if r == 0 else f'infinity-{r}'
        print(f'    {shown:13s} satellite.variable.{name(r):8s} satellite.{name(r) + "()":10s} {path(r)}')
    print(f'    rank 10^40 is {name(10 ** 40)}; satellite.unsat is rank {rank_of_name("unsat")}')


def checks():
    """The figures the spec quotes in its prose, each computed here."""
    inf, p = INF, power_of(INF, INF)
    print('\nCHECKS')

    # 1. The first idea of message five: the infinity as 128 nines, a number converted to that width.
    for k in (17, 127, 128, 200):
        same = (10 ** 128 - 1) - 10 ** k
        print(f'    inf - 10^{k:<4d} as 128 nines: {"NEGATIVE" if same < 0 else "positive"} and finite;'
              f'  with the register: above 10^1000 = {cmp(sub(inf, num(10 ** k)), num(10 ** 1000)) > 0}')

    # 2. A count cut at 128 places is wrong by an infinite amount.
    cut_third = F(int('3' * PLACES), 10 ** PLACES)
    v = sub(mul(((ONE, cut_third),), num(3)), inf)
    print(f'    1/3 cut at 128 places: (inf / 3) * 3 - inf = -10^-128 infinities, below -10^1000: {cmp(v, num(-10 ** 1000)) < 0}')

    # 3. Reading (A) -- the nines ARE the count -- drifts: inf * 50% * 2 is not inf.
    one_a = 1 - F(1, 10 ** PLACES)
    half_a = F(int(one_a / 2 * 10 ** PLACES), 10 ** PLACES)          # cut to 128 places
    print(f'    under (A), inf * 50% * 2 == inf: {half_a * 2 == one_a};  under (C): {F(1, 2) * 2 == 1}')

    # 4. Keeping +5 attached through a promotion gives a wrong number: (x+5)^x / x^x -> e^5.
    getcontext().prec = 40
    x = Decimal(10) ** 6
    print(f'    (x + 5)^x / x^x at x = 10^6: {((x + 5) / x) ** x:.4f}   (e^5 = {Decimal(5).exp():.4f})')

    # 5. Reading "a sat is a longer LIST": a four-high tower of 3s against (3^3)^(3^3).
    print(f'    3^3^3^3 = 3^{3 ** 27}, and (3^3)^(3^3) = 27^27 = 3^{3 * 27}: the four-high "power" is larger')

    # 6. Every sat beats every power (and so on up), over values built from the tables' pieces.
    s, a = unit(2), unit(3)
    seeds = [inf, mul(inf, inf), power_of(inf, num(5)), p, power_of(p, inf), mul(p, p), s, power_of(p, p), a]
    pool = set()
    for v0 in seeds:
        for extra in (ONE, num(10 ** 100), inf, mul(inf, inf), p, s):
            pool.update((v0, mul(v0, num(8)), add(v0, extra), sub(v0, extra), div(v0, num(4))))
    pool = [w for w in pool if sign(w) > 0 and rank(w) is not None]
    pairs = [(u, w) for u in pool for w in pool if rank(u) > rank(w)]
    wrong = sum(1 for u, w in pairs if cmp(u, w) <= 0)
    print(f'    {len(pool)} positive values; {len(pairs)} pairs where one has the higher type; the higher is larger in all but {wrong}')

    # 7. The a-run naming reading (after azsat, aaasat) at rank 10^40.
    print(f'    an a-run name at rank 10^40 would be about {(10 ** 40 - 2) // 26:.2e} letters; bijective base 26: {len(name(10 ** 40))}')


if __name__ == '__main__':
    tables()
    checks()
