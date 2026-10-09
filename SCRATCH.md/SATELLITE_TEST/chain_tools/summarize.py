# summarize.py <proofs folder> -- one line a proof: what changed (proofs/made.txt), the passes it ran at, the exit
# code, the seconds, and the last line it printed, with that line read as a multiple of 547311173 where it is one
import os
import re
import sys

base = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), 'proofs')
rows = []
for kind in ('canfail', 'inj'):
    for name in sorted(os.listdir(os.path.join(base, kind))):
        d = os.path.join(base, kind, name)
        with open(os.path.join(d, 'loss_chain.satl'), encoding='utf-8') as f:
            m = re.search(r'\n    satellite\.variable\.number passes = (\d+)\n', f.read())
        passes = m.group(1) if m else '?'
        try:
            with open(os.path.join(d, 'times.txt'), encoding='utf-8') as f:
                t = f.read().strip().split('\n')[-1]
        except OSError:
            rows.append('%-8s %-16s passes=%s NOT RUN' % (kind, name, passes))
            continue
        ex = re.search(r'exit=(\d+)', t).group(1)
        secs = re.search(r'seconds=([\d.]+)', t).group(1)
        with open(os.path.join(d, 'loss_chain.out'), encoding='utf-8') as f:
            out = f.read()
        lines = out.split('\n')
        last = lines[-2] if out.endswith('\n') and len(lines) > 1 else (lines[-1] if lines else '')
        note = ''
        if re.fullmatch(r'-?\d+', last or ''):
            v = int(last)
            if v != 0 and v % 547311173 == 0:
                q = v // 547311173
                k = q.bit_length() - 1
                note = ' = 547311173 x %d%s' % (q, ' (2^%d)' % k if q > 0 and q == 1 << k else '')
            elif v % 2 == 0:
                note = ' (even)'
        err = re.search(r'err=\[(.*)\]$', t).group(1)
        left = re.search(r'leftover=\[(.*?)\]', t).group(1)
        rows.append('%-8s %-16s passes=%s exit=%s %7ss lines=%d last=[%s]%s%s%s' % (
            kind, name, passes, ex, secs, out.count('\n'), last[:100] + ('...(%d digits)' % len(last) if len(last) > 100 else ''),
            note, ' err=' + err[:120] if err else '', ' LEFT ' + left if left.strip() else ''))
print('\n'.join(rows))
