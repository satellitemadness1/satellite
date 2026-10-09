# make_demos.py -- shows that each kept first-chain check closes a hole the FIXED test still leaves open: the same one
# line injected into the fixed test alone (fixed16/) and into the chain, at the same place in the operations.
#   demos/<name>/fixed/<test>_loss.satl   the fixed test with the line
#   demos/<name>/chain/loss_chain.satl    the chain with the line
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
FIXED = os.path.join(HERE, '..', 'fixed16')
with open(os.path.join(HERE, 'loss_chain.satl'), encoding='utf-8') as f:
    CHAIN = f.read()

DEMOS = [
    # a second U+19C40 at the end, just before F10's undo takes every copy: the fixed test's undo takes both
    ('string_f10', 'string_loss.satl', 's = s - "\U00010428"  // undoes F11', 's = s + "\U00019c40"'),
    # a second e-acute at the end, just before F05's undo takes every copy
    ('string_f05', 'string_loss.satl', 's = s.substring(1, s.find("ω"))  // undoes F06', 's = s + "\xe9"'),
    # 41 puts in its list's largest, not its smallest: 60's remove(0) finds no 0 and is refused (S420)
    ('list_60', 'list_loss.satl', None, None),
    # x below zero just before the reverse on the way back: its text turned round cannot be read (S120)
    ('infinity_rev', 'infinity_loss.satl', 'x = x / -1 + x % -1 * far', 'x = 0 - x'),
]


def inject_after(text, code, new, where):
    lines = text.split('\n')
    hits = [i for i, l in enumerate(lines) if l.strip() == code or l.strip().startswith(code + ' ')]
    if len(hits) != 1:
        sys.exit('%s: %r is there %d times' % (where, code, len(hits)))
    i = hits[0]
    indent = lines[i][:len(lines[i]) - len(lines[i].lstrip())]
    lines[i + 1:i + 1] = [indent + new]
    return '\n'.join(lines), i + 1


def replace_code(text, code, new, where):
    lines = text.split('\n')
    hits = [i for i, l in enumerate(lines) if l.strip() == code or l.strip().startswith(code + ' ')]
    if len(hits) != 1:
        sys.exit('%s: %r is there %d times' % (where, code, len(hits)))
    i = hits[0]
    lines[i] = lines[i].replace(code, new, 1)
    return '\n'.join(lines), i


for name, test, after, new in DEMOS:
    with open(os.path.join(FIXED, test), encoding='utf-8') as f:
        fixed = f.read()
    if name == 'list_60':
        a, ia = replace_code(fixed, 'x.append(x[13].min)', 'x.append(x[13].max)', 'fixed ' + test)
        b, ib = replace_code(CHAIN, 'x.append(x[13].min)', 'x.append(x[13].max)', 'chain')
        what = 'x.append(x[13].min) -> x.append(x[13].max)'
    else:
        a, ia = inject_after(fixed, after, new, 'fixed ' + test)
        b, ib = inject_after(CHAIN, after, new, 'chain')
        what = 'after "%s": + %s' % (after, new)
    for sub, fname, text in (('fixed', test, a), ('chain', 'loss_chain.satl', b)):
        d = os.path.join(HERE, 'demos', name, sub)
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, fname), 'w', encoding='utf-8') as f:
            f.write(text)
    print('%-13s fixed line %d, chain line %d: %s' % (name, ia + 1, ib + 1, what))
