#!/usr/bin/env python3
"""check_random_output.py <file> -- holds tests/random.satl's output inside its bounds.

Each line is "<grade> <shape> <n>" or "<word> <n>"; every n must be inside the bounds the
program asked for and on its step, every tag must appear exactly once, and the two
20-digit draws must differ. Prints yes, or the first line
that is wrong. check.sh runs it."""
import sys

WANT = {'fast digits': 1, 'fast range': 1, 'fast step': 1, 'normal digits': 1, 'normal range': 1,
        'normal step': 1, 'ultra digits': 1, 'ultra range': 1, 'ultra step': 1, 'one': 1, 'zero': 1, 'big': 1}


def inside(tag, n):
    return {'fast digits': 0 <= n < 10 ** 20,
            'fast range': 1 <= n <= 6,
            'fast step': 0 <= n <= 100 and n % 5 == 0,
            'normal digits': 0 <= n < 10 ** 20,
            'normal range': -5 <= n <= 5,
            'normal step': 10 <= n <= 50 and n % 10 == 0,
            'ultra digits': 0 <= n < 10 ** 160,
            'ultra range': 1 <= n <= 10 ** 21,
            'ultra step': -100 <= n <= 100 and (n + 100) % 25 == 0,
            'one': n == 7,
            'zero': n == 0,
            'big': n in (10 ** 40, 10 ** 40 + 1)}.get(tag, False)


def main():
    seen, values, bad = {}, {}, []
    with open(sys.argv[1]) as handle:
        lines = handle.read().split('\n')
    for line in lines:
        if not line:
            continue
        try:
            tag, text = line.rsplit(' ', 1)
            n = int(text)
        except ValueError:
            bad.append(line)
            continue
        seen[tag] = seen.get(tag, 0) + 1
        values.setdefault(tag, set()).add(n)
        if not inside(tag, n):
            bad.append(line)
    if seen != WANT:
        bad.append('counts %r' % sorted(seen.items()))
    if values.get('fast digits') == values.get('normal digits'):
        bad.append('fast(20) and normal(20) drew the same number')
    print('yes' if not bad else bad[0])


main()
