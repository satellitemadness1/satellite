# builds parts/08_number.satl from number_loss.satl: the fixed test's 100 operations and every one of its checks, word
# for word, on the value handed in
from chainlib import read, write, sub, main_of, as_capsule, line_index, break_checks, fail

src = read('number_loss.satl')
body = as_capsule(main_of(src), 'number_loss')
body = sub(body, '    satellite.variable.number start = 547311173\n',
           '    satellite.variable.number start = given                     // the chain: the value handed in\n')
body = sub(body, '    satellite.variable.number n = 547311173\n', '    satellite.variable.number n = given\n')
body = sub(body, '''    // 547311173, n's own text -- or the first pass that went wrong, and what n was then
    satellite.statement.if (seen == "")
    {
        seen = n.string
    }
    satellite.console.display(seen)
    satellite.return(satellite)
}
''', '''    // HANDS ON n -- or, when a check failed, the value it was given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == 0)
    {
        out = n
    }
    satellite.return(out)
}
''')
lines = body.split('\n')
loop = line_index(lines, 'satellite.statement.for(satellite.variable.number pass = 0; pass < passes; pass + 1)')
end = line_index(lines, '// THE PASS ENDS WHERE IT BEGAN -- or the run stops at the first pass that does not, and says so')
# the halfway check also stops the passes at once: n with a loss in it meets the percentage shares on the way back,
# which refuse a share that is not whole (S402) before the pass could end
lines, n = break_checks(lines, loop, end)
if n != 1:
    fail('%d check blocks inside the pass, not 1' % n)
body = '\n'.join(lines)
head = '''// 8. THE NUMBER (number_loss.satl, fixed 2026-10-06). n starts at the value handed in and each pass runs the test's
// 100 operations: 1-50 take it far away -- through the 64-bit and 128-bit edges, below zero and back, out to 656
// digits -- with + - * / % and ^, decimal, hex, binary and text operands, .add(), .reverse(), its own text, and
// percentages; 47 and 49 also force the two rare corrections inside satl's own long division; 51-100 undo them in
// reverse order. Every division keeps its remainder, and all of the fixes are here: 55 checks the 547 it takes out
// stood in front of exactly 350 digits, both reverses guard against a dropped end zero, the four rotations against a
// wrap, n is checked as text at the halfway point against its 656 digits, and every pass must end where it began.
// THE CHAIN CHANGES: n and start are the value handed in; the passes come from `passes`; a failed check becomes the
// loss flag, and the halfway check stops the passes at once, since n with a loss in it would meet a percentage share
// on the way back that refuses to round (S402) before the pass ended. The first chain had no fixes of its own here.
// HANDS ON: n -- or, when a check failed, the value it was given times 2.
'''
write('08_number.satl', head + body)
