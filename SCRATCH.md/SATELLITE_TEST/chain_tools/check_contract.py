# check_contract.py <loss_chain.satl> -- the contract test_programs.hpp sets for satellite.test's programs, checked
# the way satl and make_test_programs.py read it: the FIRST line in the file that is `    satellite.variable.number
# passes = <N>` must be the first line inside satellite.main (test_run.cpp rewrites the first one it finds), and the
# text must never hold )satl" (it ends the raw string it is embedded in)
import hashlib, re, sys
path = sys.argv[1]
with open(path, encoding='utf-8') as f:
    text = f.read()
ok = True
line = '\n    satellite.variable.number passes = '
at = text.find(line)
main = text.find('\nsatellite.capsule satellite.main()\n{\n')
if at < 0 or main < 0 or at != main + len('\nsatellite.capsule satellite.main()\n{') or text.count('\nsatellite.capsule satellite.main()\n') != 1:
    print('CONTRACT: the first passes line is not the first line of the one satellite.main'); ok = False
m = re.match(r'(\d+)\n', text[at + len(line):])
if not m:
    print('CONTRACT: the passes line has no whole number'); ok = False
if ')satl"' in text:
    print('CONTRACT: the text holds )satl"'); ok = False
for word in ('satellite.console.input', 'satellite.window', 'satellite.network'):
    if word in text:
        print('CONTRACT: the text uses ' + word); ok = False
displays = text.count('satellite.console.display(')
if displays != 1:
    print('CONTRACT: %d displays, not the one in main' % displays); ok = False
print('%s: %d lines, md5 %s, passes = %s, contract %s' % (path, text.count('\n'), hashlib.md5(text.encode()).hexdigest(),
      m.group(1) if m else '?', 'kept' if ok else 'BROKEN'))
sys.exit(0 if ok else 1)
