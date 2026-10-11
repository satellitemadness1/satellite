#!/usr/bin/env python3
# satellite/satellite_variable_infinity/check_infinity.py -- holds satellite_infinity
# (build/infinity_cases, infinity_cases.cpp) to infinity_oracle.py, the way
# check_numbers.py holds satellite_number to Python's int.
#
# SATELLITE_INFINITY.md: "When the C++ lands it is checked against the oracle ... the
# value column must match, and an ERROR row must be refused." INF-2 builds no
# arithmetic, so there is no ERROR row to refuse yet: what INF-2 builds is the DISPLAY
# and the ORDER, and those are checked here over
#
#   - EVERY VALUE THE SPEC'S TABLES PRINT -- the oracle's own tables(), run with its
#     row() and truth() swapped for ones that keep the value instead of printing it;
#   - the pool the oracle's CHECKS use to show every sat beats every power, with each
#     value's negative beside it, and plain numbers at the edges of a limb;
#   - canonical infinity-k made by k nestings, and a chain 100,000 exponents deep that
#     every walk -- compare, display, and the destructor -- must survive.
#
# Every value is written to the harness as the oracle's own normal form (see the
# format in infinity_cases.cpp), so what the C++ holds is exactly what the oracle holds.
#
#     make build/infinity_cases && python3 satellite/satellite_variable_infinity/check_infinity.py [harness]

import contextlib
import functools
import io
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import infinity_oracle as O  # noqa: E402 -- the path above is how it is found

# THE ORACLE'S ORDER, REMEMBERED. cmp() is a pure function of two tuples, and the
# oracle's add() sorts through it, so the same exponents are compared again and again:
# 94 s of this script's 95 were cmp. Remembering its answers changes none of them --
# add() looks the name up at call time, so it reaches this one -- and makes it seconds.
O.cmp = functools.lru_cache(maxsize=None)(O.cmp)

HARNESS = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', '..', 'build', 'infinity_cases')
DEEP = 100000                    # SATELLITE_INFINITY.md INF-5: "the C++ at 100,000"


@functools.lru_cache(maxsize=None)       # every value is written once, however many pairs it is in
def written(v):
    return '[' + ''.join(f'({written(e)} {O.number_text(c)})' for e, c in v) + ']'


def table_values():
    """Every value a table row of the spec prints, in the order it prints them."""
    kept = []

    def row(label, thunk):
        try:
            kept.append((label, thunk()))
        except O.SatError:
            pass                 # an ERROR row: INF-3 onward refuses it, INF-2 has nothing to refuse

    O.row, O.truth = row, (lambda label, holds: None)
    with contextlib.redirect_stdout(io.StringIO()):
        O.tables()
    return kept


def pool_values():
    """The oracle's CHECKS pool (its soundness check 6), both signs, and plain numbers."""
    inf, p = O.INF, O.power_of(O.INF, O.INF)
    s, a = O.unit(2), O.unit(3)
    seeds = [inf, O.mul(inf, inf), O.power_of(inf, O.num(5)), p, O.power_of(p, inf), O.mul(p, p), s,
             O.power_of(p, p), a]
    pool = []
    for v0 in seeds:
        for extra in (O.ONE, O.num(10 ** 100), inf, O.mul(inf, inf), p, s):
            pool += [v0, O.mul(v0, O.num(8)), O.add(v0, extra), O.sub(v0, extra), O.div(v0, O.num(4))]
    # INF-8'S SHAPES, written straight in normal form: nothing makes them until INF-8,
    # but the order is built whole at INF-2, and comparing against a plain number has a
    # path of its own for a first exponent below 0 -- which no table value reaches.
    minus_one, half = O.neg(O.ONE), O.num(O.F(1, 2))
    pool += [((minus_one, O.F(5)),), ((O.ZERO, O.F(5)), (minus_one, O.F(3))),
             ((O.ZERO, O.F(5)), (minus_one, O.F(-3))), ((O.ONE, O.F(1)), (minus_one, O.F(1))),
             ((O.neg(O.INF), O.F(1)),), ((half, O.F(2)),), ((O.ONE, O.F(1)), (half, O.F(-7)), (O.ZERO, O.F(2)))]
    pool += [O.neg(v) for v in pool]
    pool += [O.num(n) for n in (0, 1, -1, 5, 2 ** 64 - 1, 2 ** 64, -(2 ** 64), 10 ** 300, -10 ** 300)]
    unique, seen = [], set()
    for v in pool:
        if v not in seen:
            seen.add(v)
            unique.append(v)
    return unique


def main():
    cases = []                   # (the line sent, what the oracle says comes back)
    tables = table_values()
    pool = pool_values()
    everything = [v for _, v in tables] + pool

    for v in everything:
        cases.append((f'show {written(v)}', O.display(v)))
        cases.append((f'negate {written(v)}', O.display(O.neg(v))))
    # THE ORDER: every pair in the pool, and every table value against the pool.
    for v in pool:
        for w in pool:
            cases.append((f'compare {written(v)} {written(w)}', str(O.cmp(v, w))))
    for _, v in tables:
        for w in pool:
            cases.append((f'compare {written(v)} {written(w)}', str(O.cmp(v, w))))
            cases.append((f'compare {written(w)} {written(v)}', str(O.cmp(w, v))))
    # A PLAIN NUMBER ON THE OTHER SIDE, as `satellite.infinity() > 10 ^ 100` asks it.
    for v in everything:
        for n in (0, 1, -1, 5, 10 ** 100, 2 ** 64, -(2 ** 64) + 1, 10 ** 300, -10 ** 300):
            cases.append((f'number {written(v)} {n}', str(O.cmp(v, O.num(n)))))
    # THE CANONICAL RUNGS, made by nesting, and one chain deep enough to need the stack.
    for k in (0, 1, 2, 3, 28, 29, 300, 1000):
        cases.append((f'unit {k}', O.display(O.unit(k)) if k <= 300 else f'(infinity-{k})'))
    cases.append((f'deep {DEEP}', f'ok {DEEP}'))

    ran = subprocess.run([HARNESS], input=''.join(line + '\n' for line, _ in cases),
                         capture_output=True, text=True)
    answers = ran.stdout.split('\n')
    wrong = 0
    for at, (line, wanted) in enumerate(cases):
        got = answers[at] if at < len(answers) else '(no answer)'
        if got != wanted:
            wrong += 1
            if wrong <= 20:
                shown = line if len(line) < 300 else line[:300] + ' ...'
                print(f'FAIL {shown}\n     wanted {wanted}\n     got    {got}')
    if ran.returncode != 0 and wrong == 0:
        wrong += 1
        print(f'FAIL the harness exited {ran.returncode}: {ran.stderr.strip()[:500]}')
    print(f'{len(cases)} cases: {len(tables)} table values and {len(pool)} pool values shown, '
          f'every pair ordered, infinity-k to 1000 and a chain {DEEP} deep -- '
          + ('every answer the oracle\'s' if wrong == 0 else f'{wrong} WRONG'))
    return 0 if wrong == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
