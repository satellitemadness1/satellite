# chainlib.py -- what every make_NN_*_part.py shares: read a loss test, edit it only by CHECKED replacements, write
# its part. A replacement whose text is not there exactly as often as expected stops the build, so a loss test that
# changed under a generator is never edited blind -- the generator says which text it no longer finds.
#
# The loss tests are read from $LOSS_TESTS when it is set (build.sh's first argument), else from ../fixed16.
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.environ.get('LOSS_TESTS') or os.path.join(HERE, '..', 'fixed16')
PARTS = os.path.join(HERE, 'parts')


def fail(why):
    sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + why + '\n')
    sys.exit(1)


def read(name):
    with open(os.path.join(SRC, name), encoding='utf-8') as f:
        return f.read()


def write(part, text):
    os.makedirs(PARTS, exist_ok=True)
    with open(os.path.join(PARTS, part), 'w', encoding='utf-8') as f:
        f.write(text)
    print('wrote parts/%s (%d lines)' % (part, text.count('\n')))


def sub(text, old, new, count=1):
    """old -> new, and old must be there exactly `count` times."""
    found = text.count(old)
    if found != count:
        fail('%r is there %d times, not %d' % (old[:150], found, count))
    return text.replace(old, new)


def main_of(text):
    """the text from `satellite.capsule satellite.main()` to the end."""
    marker = 'satellite.capsule satellite.main()\n'
    if text.count(marker) != 1:
        fail('no single satellite.main()')
    return text[text.index(marker):]


def as_capsule(main_text, name):
    """satellite.main() becomes the chain's capsule `name(given, passes)`, and its loop runs `passes` times."""
    out = sub(main_text, 'satellite.capsule satellite.main()\n',
              'satellite.capsule ' + name + '(satellite.variable.number given, satellite.variable.number passes)\n')
    return sub(out, '; pass < 10000; pass + 1)', '; pass < passes; pass + 1)')


def line_index(lines, exact, lo=0, hi=None):
    """the one line (stripped) equal to `exact`, between lo and hi."""
    hi = len(lines) if hi is None else hi
    hits = [i for i in range(lo, hi) if lines[i].strip() == exact.strip()]
    if len(hits) != 1:
        fail('the line %r is there %d times, not once' % (exact[:150], len(hits)))
    return hits[0]


def line_starting(lines, start, lo=0, hi=None):
    """the one line whose code is `start`: the whole line, or followed by a space (a trailing comment)."""
    hi = len(lines) if hi is None else hi
    code = start.strip()
    hits = [i for i in range(lo, hi) if lines[i].strip() == code or lines[i].strip().startswith(code + ' ')]
    if len(hits) != 1:
        fail('a line starting %r is there %d times, not once' % (start[:150], len(hits)))
    return hits[0]


def guard(cond, why, wrong_line='wrong = wrong + 1', indent='        '):
    """a check the chain adds, under a `// chain:` line saying why: when `cond` holds, the loss is counted and the
    passes stop at once."""
    out = [indent + '// chain: ' + why] if why else []
    return out + [indent + 'satellite.statement.if (' + cond + ')',
                  indent + '{',
                  indent + '    ' + wrong_line,
                  indent + '    satellite.statement.break',
                  indent + '}']


def break_checks(lines, lo, hi, wrong_start='wrong = wrong + '):
    """every check block between lines lo and hi -- an if whose body is the one line `wrong = wrong + ...` --
    also stops the passes at once (the chain's: a loss a check has caught is handed on as the loss flag before a
    later line can stop the run with a refusal). Answers the new lines and how many blocks it changed."""
    out = []
    changed = 0
    for i, l in enumerate(lines):
        out.append(l)
        if lo <= i < hi and l.strip().startswith(wrong_start) and lines[i - 1].strip() == '{' \
                and lines[i + 1].strip() == '}' and lines[i - 2].strip().startswith('satellite.statement.if ('):
            out.append(l[:len(l) - len(l.lstrip())] + 'satellite.statement.break')
            changed += 1
    return out, changed
