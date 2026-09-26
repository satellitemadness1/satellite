#!/usr/bin/env python3
"""check_help_examples.py <satl> -- every program in satellite.help/*/help_text.txt, run through satl.

Run from the repository root. Windows and keyboard input are never run (a window would open on
the author's desktop whatever DISPLAY says); each program runs with no display, SATL_NO_WINDOW=1,
an empty XDG_RUNTIME_DIR and 20 seconds. Expected failures on 2026-09-25: the arguments example
(it wants two words after it), satellite.history (refused on purpose, its comment says so),
satellite.include and satellite.library (they name files that are not there), and the window
examples (NO_DISPLAY, which is the point of running them with none).

A program starts at a line `satellite.include(satellite)` and runs to the last line of code
before prose begins again (a line that is not blank, not indented, and not satellite code or
a brace). A block that follows a line ending "refused today:" or containing "refused" is
expected to exit non-zero; any other must exit 0. Prints one line per program."""
import glob, os, re, subprocess, sys, tempfile
satl = os.path.abspath(sys.argv[1])
bad = 0
for path in sorted(glob.glob('satellite.help/*/help_text.txt')) + ['satellite.help/help.txt']:
    lines = open(path).read().split('\n')
    i = 0
    while i < len(lines):
        if lines[i].strip() != 'satellite.include(satellite)':
            i += 1
            continue
        label = ''
        for back in range(i - 1, max(i - 4, -1), -1):
            if lines[back].strip():
                label = lines[back].strip()
                break
        start, j, last_code = i, i, i
        while j < len(lines):
            l = lines[j]
            if l.strip() and not (l.startswith((' ', '\t', '{', '}', 'satellite.', '//')) or l.strip() in ('{', '}')):
                break
            if l.strip():
                last_code = j
            j += 1
        body = '\n'.join(lines[start:last_code + 1]) + '\n'
        expect_fail = 'refused' in label.lower()
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, 'example.satl')
            with open(p, 'w') as f:
                f.write(body)
            env = {k: v for k, v in os.environ.items() if k not in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'GDK_BACKEND')}
            empty = tempfile.mkdtemp(); os.chmod(empty, 0o700)
            env.update(SATL_NO_WINDOW='1', XDG_RUNTIME_DIR=empty)
            try:
                r = subprocess.run([satl, 'example.satl'], cwd=d, capture_output=True, text=True, timeout=20,
                                   env=env, stdin=subprocess.DEVNULL)
            except subprocess.TimeoutExpired:
                print(f'TIMEOUT {path}:{start + 1}')
                bad += 1
                i = last_code + 1
                continue
        ok = (r.returncode != 0) if expect_fail else (r.returncode == 0)
        if not ok:
            bad += 1
            reason = [l for l in r.stderr.split('\n') if l.startswith(('satl(', '[satellite]', 'S'))][:2]
            print(f'FAIL {path}:{start + 1} ({"expects refusal" if expect_fail else "expects exit 0"}, got {r.returncode}): {" / ".join(reason)[:220]}')
        else:
            print(f'ok   {path}:{start + 1}')
        i = last_code + 1
print(f'{bad} failing')
