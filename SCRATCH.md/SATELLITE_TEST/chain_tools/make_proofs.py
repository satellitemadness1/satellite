# make_proofs.py -- copies of loss_chain.satl with a loss injected inside ONE test's part, each in a folder of its own:
#   proofs/canfail/<test>/loss_chain.satl   ONE line changed in that test (an operand), the can-fail proofs
#   proofs/inj/<name>/loss_chain.satl       the reviewer's injection <name>, ported: the same change he made to the
#                                           original test, made to the chain's copy of that line
# A change must find its line exactly once inside its test's part, or nothing is written. Writes proofs/made.txt.
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(HERE, 'loss_chain.satl'), encoding='utf-8') as f:
    SRC = f.read().split('\n')

# each test's part runs from its header line to the next one (the last to main's own comment)
HEADS = ['// 1. THE MAP', '// 2. THE FILE', '// 3. THE LIST', '// 4. THE STRING', '// 5. THE MULTIPLE', '// 6. THE INFINITY',
         '// 7. THE OBJECT', '// 8. THE NUMBER', '// 9. THE FRACTION', '// 10. THE FLOAT', '// 11. THE PERCENTAGE',
         '// 12. THE BINARY', '// 13. THE HEX ', '// 14. THE HEX WALK', '// 15. THE COLOUR', '// 16. THE BOOL',
         '// THE CHAIN: 547311173 in']
NAMES = ['map', 'file', 'list', 'string', 'multiple', 'infinity', 'object', 'number', 'fraction', 'float', 'percent',
         'binary', 'hex_operations', 'hex', 'color', 'bool']
starts = []
for h in HEADS:
    hits = [i for i, l in enumerate(SRC) if l.startswith(h)]
    assert len(hits) == 1, (h, hits)
    starts.append(hits[0])
SPAN = {NAMES[k]: (starts[k], starts[k + 1]) for k in range(16)}


def find(lines, test, code):
    """the one line in test's part whose code is `code` (the whole line, or followed by a space and a comment)"""
    a, b = SPAN[test]
    code = code.strip()
    hits = [i for i in range(a, b) if lines[i].strip() == code or lines[i].strip().startswith(code + ' ')]
    if len(hits) != 1:
        raise SystemExit('%s: %r is there %d times' % (test, code, len(hits)))
    return hits[0]


def apply(test, edits):
    """edits: ('replace', old code, new code) changes that code on its line, keeping the indent and any comment;
    ('after', code, new line) puts a new line after that one, at its indent. Answers the text and what changed."""
    lines = list(SRC)
    said = []
    inserted = 0
    for kind, code, new in edits:
        i = find(lines, test, code)
        indent = lines[i][:len(lines[i]) - len(lines[i].lstrip())]
        if kind == 'replace':
            old_line = lines[i]
            lines[i] = indent + new + lines[i].strip()[len(code.strip()):]
            said.append('line %d: %s  ->  %s' % (i + 1 - inserted, old_line.strip()[:110], lines[i].strip()[:110]))
        else:
            lines[i + 1:i + 1] = [indent + new]
            said.append('after line %d (%s): + %s' % (i + 1 - inserted, lines[i].strip()[:80], new))
            inserted += 1
    return '\n'.join(lines), said


def write(folder, text):
    os.makedirs(folder, exist_ok=True)
    with open(os.path.join(folder, 'loss_chain.satl'), 'w', encoding='utf-8') as f:
        f.write(text)


# 2. CAN-FAIL: one operand changed inside each test
CANFAIL = [
    ('map', 'x["number"] = x["number"] - 14', 'x["number"] = x["number"] - 15'),
    ('file', 'f.insert(8, "7")', 'f.insert(9, "7")'),
    ('list', 'x[2] = x[2] + 547311180', 'x[2] = x[2] + 547311181'),
    ('string', 's = s + "⦃" + 547311173', 's = s + "⦃" + 547311174'),
    ('multiple', 'x = x - 452688827', 'x = x - 452688826'),
    ('infinity', 'x = x - 99', 'x = x - 98'),
    ('object', 'v.add(1)', 'v.add(2)'),
    ('number', 'n = n - 1', 'n = n - 2'),
    ('fraction', 'f = f.numerator * 0.1% + (f.denominator - 1) * far', 'f = f.numerator * 0.2% + (f.denominator - 1) * far'),
    ('float', 'x = x + 0.' + '0' * 127 + '7', 'x = x + 0.' + '0' * 127 + '8'),
    ('percent', 'x = x + 0.' + '0' * 31 + '7%', 'x = x + 0.' + '0' * 31 + '8%'),
    ('binary', 'worth = worth + -b0110', 'worth = worth + -b0111'),
    ('hex_operations', 'worth = worth - x3 * x5 * x7', 'worth = worth - x3 * x5 * x6'),
    ('hex', 'walk = walk + 1', 'walk = walk + 2'),
    ('color', 'worth = worth - paint.number * x5', 'worth = worth - paint.number * x6'),
    ('bool', 's01 = s01 != s29', 's01 = s01 != s28'),
]

# 3. THE REVIEWER'S INJECTIONS (review_injections/inj_*.satl), each the same change made to the chain's line
REV = [
    ('inj_bin_rev', 'binary', [('replace', a, b) for a, b in [
        ('turned = bits.reverse()', 'turned = bits'), ('wide_turned = wide.reverse()', 'wide_turned = wide'),
        ('turned = (-bits).reverse()', 'turned = (-bits)'), ('wide_turned = (-wide).reverse()', 'wide_turned = (-wide)'),
        ('worth = worth + turned.reverse()', 'worth = worth + turned'), ('bits = -held.reverse()', 'bits = -held'),
        ('wide = -wide_held.reverse()', 'wide = -wide_held'), ('wide_held = -wide.reverse()', 'wide_held = -wide'),
        ('held = -bits.reverse()', 'held = -bits'), ('worth = worth - turned.reverse()', 'worth = worth - turned'),
        ('wide = -wide_turned.reverse()', 'wide = -wide_turned'), ('bits = -turned.reverse()', 'bits = -turned'),
        ('wide = wide_turned.reverse()', 'wide = wide_turned'), ('bits = turned.reverse()', 'bits = turned')]]),
    ('inj_float_ctrl', 'float', [('after', 'x = x ^ 2 / x', 'x = x + 0.000000000000000000000000001')]),
    ('inj_float_fold', 'float', [('after', 'x = x * 1.1', 'x = x - 3.75')]),
    ('inj_float_small', 'float', [('after', 'x = x ^ 2 / x', 'x = x + 0.0000000000000000000000000000000000000001')]),
    ('inj_float_string', 'float', [('replace', 'x = 1.75 + x.string', 'x = 1.75 + (x - x % 1).string')]),
    ('inj_float_turn', 'float', [('after', 'x = 1.75 + x.string', 'x = x + 10 ^ 148')]),
    ('inj_frac_den', 'fraction', [('replace', 'f = -f', 'f = -547311173/7')]),
    ('inj_frac_div_m', 'fraction', [('after', 'f = f.numerator - 547311173547311173 + (f.denominator - 1) * far', 'f = f.numerator - 1')]),
    ('inj_frac_div_p', 'fraction', [('after', 'f = f.numerator - 547311173547311173 + (f.denominator - 1) * far', 'f = f.numerator + 1')]),
    ('inj_hex_rev', 'hex_operations', [('replace', a, b) for a, b in [
        ('hx_turned = hx.reverse()', 'hx_turned = hx'), ('hx_wide_turned = hx_wide.reverse()', 'hx_wide_turned = hx_wide'),
        ('hx_turned = (-hx).reverse()', 'hx_turned = (-hx)'), ('hx_wide_turned = (-hx_wide).reverse()', 'hx_wide_turned = (-hx_wide)'),
        ('worth = worth + hx_turned.reverse()', 'worth = worth + hx_turned'), ('hx = -hx_held.reverse()', 'hx = -hx_held'),
        ('hx_wide = -hx_wide_held.reverse()', 'hx_wide = -hx_wide_held'), ('hx_wide_held = -hx_wide.reverse()', 'hx_wide_held = -hx_wide'),
        ('hx_held = -hx.reverse()', 'hx_held = -hx'), ('worth = worth - hx_turned.reverse()', 'worth = worth - hx_turned'),
        ('hx_wide = -hx_wide_turned.reverse()', 'hx_wide = -hx_wide_turned'), ('hx = -hx_turned.reverse()', 'hx = -hx_turned'),
        ('hx_wide = hx_wide_turned.reverse()', 'hx_wide = hx_wide_turned'), ('hx = hx_turned.reverse()', 'hx = hx_turned')]]),
    ('inj_inf_sign', 'infinity', [('replace', a, b) for a, b in [
        ('up = -up', 'up = up'), ('up = -(up)', 'up = (up)'), ('x = -up', 'x = up'), ('x = -(x)', 'x = (x)'),
        ('up = - up', 'up = up'), ('x = (-x)', 'x = (x)'),
        ('satellite.statement.if (-up < b100000100111110100111001000101)', 'satellite.statement.if (up < b100000100111110100111001000101)'),
        ('satellite.statement.if (up == -satellite.infinity())', 'satellite.statement.if (up == satellite.infinity())'),
        ('satellite.statement.if (-up > x)', 'satellite.statement.if (up > x)'),
        ('satellite.statement.if (x < -up)', 'satellite.statement.if (x < up)'),
        ('satellite.statement.if (-satellite.infinity() == up)', 'satellite.statement.if (satellite.infinity() == up)'),
        ('satellite.statement.if (b100000100111110100111001000101 > -up)', 'satellite.statement.if (b100000100111110100111001000101 > up)'),
        ('x = -((x))', 'x = ((x))'), ('up = -(-(-up))', 'up = up'), ('x = -(-(-x))', 'x = x'), ('up = (-up)', 'up = (up)'),
        ('up = -((up))', 'up = ((up))')]]),
    ('inj_list_div', 'list', [('after', 'x[8] = x[8] * 1000000000000000000000000000000', 'x[8] = x[8] + 1')]),
    ('inj_list_mod', 'list', [('replace', 'x[4] = x[4] + 547311172000', 'x[4] = x[4] + 0')]),
    ('inj_map_bool', 'map', [('after', 'x[satellite.bool.true] = 547311173.5', 'x[satellite.bool.true] = 1')]),
    ('inj_map_div', 'map', [('after', 'x["satellite"] = x["satellite"] * 547311173', 'x["satellite"] = x["satellite"] + 1')]),
    ('inj_map_mod', 'map', [('replace', 'x["s"] = x["s"] + 547311168', 'x["s"] = x["s"] + 0')]),
    ('inj_mult_digit2', 'multiple', [('after', 'x = 0 + x', 'x = x - 3')]),
    ('inj_mult_digit', 'multiple', [('after', 'x = 0 + x', 'x = x + 3')]),
    ('inj_mult_div', 'multiple', [('after', 'x = x * 547311173', 'x = x + 1')]),
    ('inj_num_prefix', 'number', [('replace', 'n = 0 + ("547" + n.str)', 'n = 0 + ("" + n.str)')]),
    ('inj_obj_negate', 'object', [('replace', 'value = -value', 'value = value')]),
    ('inj_pct_big', 'percent', [('after', 'x = x * 100000000%', 'x = x + 0.01%')]),
    ('inj_pct_ctrl', 'percent', [('after', 'x = x * 100000000%', 'x = x + 0.1%')]),
    ('inj_pct_ulp', 'percent', [('after', 'x = x + 0.' + '0' * 31 + '1%', 'x = x + 0.' + '0' * 31 + '1%')]),
    ('inj_str_trim', 'string', [('after', 's = s.trim', 's = s - "\U0002005f"')]),
]

made = []
for test, old, new in CANFAIL:
    text, said = apply(test, [('replace', old, new)])
    write(os.path.join(HERE, 'proofs', 'canfail', test), text)
    made.append('canfail %-15s %s' % (test, said[0]))
for name, test, edits in REV:
    text, said = apply(test, edits)
    write(os.path.join(HERE, 'proofs', 'inj', name), text)
    made.append('%-16s %-15s %s' % (name, test, ('\n' + ' ' * 33).join(said)))
with open(os.path.join(HERE, 'proofs', 'made.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(made) + '\n')
print('\n'.join(made))
print(len(CANFAIL), 'can-fail copies,', len(REV), 'injections')
