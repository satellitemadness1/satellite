# builds parts/06_infinity.satl from infinity_loss.satl: the fixed test's 100 operations and every one of its checks,
# word for word, on the value handed in; the first chain's guard before the reverse kept
from chainlib import read, write, sub, main_of, as_capsule, line_index, line_starting, guard, break_checks, fail

src = read('infinity_loss.satl')
body = as_capsule(main_of(src), 'infinity_loss')
body = sub(body, '    satellite.variable.number start = 547311173\n    satellite.variable.infinity x = 547311173\n',
           '    satellite.variable.number start = given                     // the chain: the value handed in\n    satellite.variable.infinity x = given\n')
body = sub(body, '''    // 547311173, x's own text -- or the first pass that went wrong, and what x, up and held were then
    satellite.statement.if (seen == "")
    {
        seen = x.string
    }
    satellite.console.display(seen)
    satellite.return(satellite)
}
''', '''    // HANDS ON x's own text read back as a number -- or, when a check failed, the value it was given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == 0)
    {
        out = x.string.number
    }
    satellite.return(out)
}
''')
lines = body.split('\n')
loop = line_index(lines, '// FORWARD: 50 operations')
end = line_index(lines, '// THE PASS ENDS WHERE IT BEGAN -- or the run stops at the first pass that does not, and says so')
# every check inside the pass also stops the passes at once (the end-of-pass checks already do)
lines, n = break_checks(lines, loop, end)
if n != 5:
    fail('%d check blocks inside the pass, not 5' % n)
# THE FIRST CHAIN'S GUARD, kept: x.string.reverse().number cannot read a number below zero -- its text turned round
# ends in its minus (S120) -- and x is never below zero there on the right path, so a loss that sends it below zero
# is counted and ends the passes with the loss flag, instead of stopping the run. The fixed test still leaves it open.
i = line_starting(lines, 'x = x.string.reverse().number + (x % 10 - 8) * far')
lines[i:i] = guard('x < 0', 'its text turned round would end in its minus, which .number cannot read (S120); x is never below zero here')
body = '\n'.join(lines)
head = '''// 6. THE INFINITY (infinity_loss.satl, fixed 2026-10-06). x, an infinity name, holds the value handed in -- a plain
// number, as the help allows -- and up and held are infinity names too: up is satellite.infinity(), held starts at
// 0. Each pass runs the test's 100 operations, 50 forward and the same 50 undone, last first, each undo spelled
// differently -- + - * / .add(), the minus in its spellings, .reverse(), % to split x, x handed into held and an
// infinity put in its place, and 28 steps chosen by the ORDER of an infinity, each an if / else -- with all of its
// fixes: x, up and held checked as text at the halfway point, the two results that were never read checked, every
// division keeping its remainder in the same statement, each reverse guarded against a dropped end zero, and every
// pass checked, up and held as text too.
// THE CHAIN CHANGES: x and start are the value handed in; the passes come from `passes`; a failed check becomes the
// loss flag, and a check inside the pass (the five before its end) stops the passes at once, as the end-of-pass ones
// already did; and the FIRST CHAIN'S GUARD stays, which the fixed test leaves open: x.string.reverse().number cannot
// read a number below zero (S120), and x is never below zero there, so a loss that sends it below zero is counted.
// HANDS ON: x's own text read back as a number -- or, when a check failed, the value it was given times 2.
'''
write('06_infinity.satl', head + body)
