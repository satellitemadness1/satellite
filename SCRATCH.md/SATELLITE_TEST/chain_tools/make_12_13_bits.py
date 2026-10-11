# builds parts/12_binary.satl and parts/13_hex_operations.satl from binary_loss.satl and hex_operations_loss.satl: the
# fixed tests' 100 operations and their middle checks, word for word; worth, the number each carries beside its
# typed values, starts at the value handed in -- no number can be put into a binary or a hex (S301)
from chainlib import read, write, sub, main_of, as_capsule


def build(srcfile, part, capname, typed, wide, start_worth, head):
    body = as_capsule(main_of(read(srcfile)), capname)
    body = sub(body, '    satellite.variable.number worth = ' + start_worth + '\n',
               '    satellite.variable.number worth = given      // the chain: the value handed in (the test starts it at '
               + start_worth + ', 547311173)\n')
    old_end = '''    satellite.statement.if (seen != "")
    {
        satellite.console.display(seen)
    }
    satellite.statement.else satellite.statement.if (TYPED != start)
    {
        satellite.console.display(TYPED)
    }
    satellite.statement.else satellite.statement.if (WIDE != start_wide)
    {
        satellite.console.display(WIDE)
    }
    satellite.statement.else satellite.statement.if (worth != 547311173)
    {
        satellite.console.display(worth)
    }
    satellite.statement.else
    {
        satellite.console.display(547311173)
    }
    satellite.return(satellite)
}
'''.replace('TYPED', typed).replace('WIDE', wide)
    new_end = '''    // HANDS ON worth -- or, when the middle, TYPED or WIDE did not come back, the value it was given times 2; a worth
    // that came back wrong without a check seeing it is handed on as it is, wrong (the test would have printed it)
    satellite.variable.number out = worth
    satellite.statement.if (seen != "")
    {
        out = given * 2
    }
    satellite.statement.else satellite.statement.if (TYPED != start)
    {
        out = given * 2
    }
    satellite.statement.else satellite.statement.if (WIDE != start_wide)
    {
        out = given * 2
    }
    satellite.return(out)
}
'''.replace('TYPED', typed).replace('WIDE', wide)
    body = sub(body, old_end, new_end)
    write(part, head + body)


build('binary_loss.satl', '12_binary.satl', 'binary_loss', 'bits', 'wide', 'bits',
'''// 12. THE BINARY (binary_loss.satl, fixed 2026-10-06). A binary's arithmetic answers a number, and no number can be
// put into a binary (S301), so the value is held as the test holds it, three ways: bits (a binary) and wide (the
// same bits 32 digits wide, so lost leading zeros would show) start at their own written start,
// b100000100111110100111001000101 and b00100000100111110100111001000101; and worth, the number, starts at the value
// handed in. bits and wide go through .reverse(), the minus sign and moves between binary names; worth goes through +
// - * / % ^ .add .number .binary with every operand a binary (or a binary meeting a hex), past 64 bits, below zero and
// with leading zeros, bits and wide added in and taken out again: 50 operations out and 50 back, in the reverse
// order. Every / keeps its remainder. The fix is here: after the 50 out, each of the seven values must be its known
// middle, worked out by hand -- or the passes stop there -- so a pass that came back wrong stops at the next middle.
// THE CHAIN CHANGES: worth starts at the value handed in, not at bits; the passes come from `passes`; a middle that
// was not the middle, or bits or wide not back at the end, becomes the loss flag -- the value handed in times 2, not
// worth times 2 as in the first chain: the middle check stops the passes half way through one, where worth is no
// value the chain carries, and it fires on every value but 547311173. The first chain's per-pass check of bits and
// wide is not added: the middle of the next pass and the checks after the last one see the same.
// HANDS ON: worth -- or, when the middle, bits or wide did not come back, the value it was given times 2 (an even
// number, which 547311173 never is); a worth that came back wrong without a check seeing it is handed on wrong.
''')
build('hex_operations_loss.satl', '13_hex_operations.satl', 'hex_operations_loss', 'hx', 'hx_wide', 'hx',
'''// 13. THE HEX (hex_operations_loss.satl, fixed 2026-10-06). A hex's arithmetic answers a number, and a hex name holds
// only a hex (S301), so the value is held as the test holds it, three ways: hx (a hex) and hx_wide (the same number
// 12 digits wide, x0000209F4E45, so lost leading zeros would show) start at their own written start, x209F4E45; and
// worth, the number, starts at the value handed in. hx and hx_wide go through .reverse(), the minus sign and moves
// between hex names; worth goes through + - * / % ^ .add .number .binary .width with every operand a hex (or a hex
// meeting a binary), past 64 bits, below zero, with leading zeros and in lower case, hx and hx_wide added in and
// taken out again: 50 operations out and 50 back, in the reverse order. Every / keeps its remainder. The fix is here:
// after the 50 out, each of the seven values must be its known middle -- or the passes stop there.
// THE CHAIN CHANGES: worth starts at the value handed in, not at hx; the passes come from `passes`; a middle that was
// not the middle, or hx or hx_wide not back at the end, becomes the loss flag -- the value handed in times 2, not
// worth times 2 as in the first chain: the middle check stops the passes half way through one, where worth is no
// value the chain carries. The first chain's per-pass check of hx and hx_wide is not added: the middle of the next
// pass and the checks after the last one see the same.
// HANDS ON: worth -- or, when the middle, hx or hx_wide did not come back, the value it was given times 2; a worth
// that came back wrong without a check seeing it is handed on wrong.
''')
