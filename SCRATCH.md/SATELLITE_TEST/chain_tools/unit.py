# unit.py [passes] [part...] -- each part of the chain ALONE, through probe.sh, one run at a time, on 547311173 and on
# values it is not written for: what an earlier test can hand it after a loss (twice, four times 547311173), and the
# first chain's eight others. A part must hand on 547311173 for 547311173, and never 547311173 for anything else.
# Writes runs/unit/matrix.txt.
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PROBE = os.path.join(HERE, '..', 'probe.sh')
PARTS = [('01_map', 'map_loss'), ('02_file', 'file_loss'), ('03_list', 'list_loss'), ('04_string', 'string_loss'),
         ('05_multiple', 'multiple_loss'), ('06_infinity', 'infinity_loss'), ('07_object', 'object_loss'),
         ('08_number', 'number_loss'), ('09_fraction', 'fraction_loss'), ('10_float', 'float_loss'),
         ('11_percent', 'percent_loss'), ('12_binary', 'binary_loss'), ('13_hex_operations', 'hex_operations_loss'),
         ('14_hex', 'hex_loss'), ('15_color', 'color_loss'), ('16_bool', 'bool_loss')]
INPUTS = ['547311173', '1094622346', '2189244692', '547311174', '547311172', '547311173999', '10 ^ 30',
          '10 ^ 1000 + 547311173', '-547311173', '0']
passes = int(sys.argv[1]) if len(sys.argv) > 1 else 3
only = set(sys.argv[2:])


def text(path):
    with open(path, encoding='utf-8') as f:
        return f.read()


head = 'satellite.include(satellite)\narguments.float.decimal(128)\narguments.float.whole(4096)\narguments.infinity_display(128)\n\n'
helpers = text(os.path.join(HERE, 'parts', '00_helpers.satl'))
rows = []
for part, cap in PARTS:
    if only and part not in only:
        continue
    body = text(os.path.join(HERE, 'parts', part + '.satl'))
    folder = os.path.join(HERE, 'runs', 'unit', part)
    os.makedirs(folder, exist_ok=True)
    for k, value in enumerate(INPUTS):
        name = 'u%02d.satl' % k
        prog = (head + helpers + '\n' + body + '\nsatellite.capsule satellite.main()\n{\n'
                '    satellite.variable.number passes = %d\n    satellite.variable.number n = %s\n'
                '    satellite.console.display(%s(n, passes))\n    satellite.return(satellite)\n}\n' % (passes, value, cap))
        with open(os.path.join(folder, name), 'w', encoding='utf-8') as f:
            f.write(prog)
        t0 = time.time()
        with open(os.path.join(folder, name[:-5] + '.out'), 'w') as out, open(os.path.join(folder, name[:-5] + '.err'), 'w') as err:
            rc = subprocess.call([PROBE, name], cwd=folder, stdout=out, stderr=err)
        secs = time.time() - t0
        got = text(os.path.join(folder, name[:-5] + '.out')).strip()
        report = ''
        if rc != 0:
            lines = [l for l in text(os.path.join(folder, name[:-5] + '.err')).split('\n') if l[:1] == 'S' and l[1:4].isdigit()]
            report = lines[0] if lines else ''
        left = [x for x in os.listdir(folder) if x.endswith('.se')]
        if value == '547311173':
            verdict = 'ok' if (rc == 0 and got == '547311173') else 'WRONG'
        elif got == '547311173':
            verdict = 'GAVE BACK 547311173'
        elif rc != 0:
            verdict = 'stopped'
        else:
            verdict = 'ok'
        rows.append('%-14s %-22s exit=%-3d %5.2fs %-20s out=%s %s%s' % (
            cap, value, rc, secs, verdict, got[:70] + ('...(%d chars)' % len(got) if len(got) > 70 else ''), report,
            ' LEFT ' + ' '.join(left) if left else ''))
        print(rows[-1], flush=True)
with open(os.path.join(HERE, 'runs', 'unit', 'matrix.txt'), 'w', encoding='utf-8') as f:
    f.write('passes = %d\n' % passes + '\n'.join(rows) + '\n')
