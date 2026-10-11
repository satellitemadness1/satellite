# builds parts/09_fraction.satl from fraction_loss.satl: the fixed test's 100 operations, h's 18 moves beside them and
# every check, word for word, on the value handed in
from chainlib import read, write, sub, main_of, as_capsule, line_index, break_checks, fail

src = read('fraction_loss.satl')
body = as_capsule(main_of(src), 'fraction_loss')
body = sub(body, '    satellite.variable.fraction start = 547311173/1\n    satellite.variable.fraction f = 547311173/1\n',
           '    satellite.variable.fraction start = given                   // the chain: the value handed in, over 1\n'
           '    satellite.variable.fraction f = given\n')
body = sub(body, '''    satellite.statement.if (seen == "")
    {
        satellite.statement.if (f == start)
        {
            satellite.statement.if (f.string == start.string)
            {
                satellite.console.display(547311173)
            }
            satellite.statement.else
            {
                satellite.console.display(f)
            }
        }
        satellite.statement.else
        {
            satellite.console.display(f)
        }
    }
    satellite.statement.else
    {
        satellite.console.display(seen)
    }
    satellite.return(satellite)
}
''', '''    // HANDS ON f's numerator -- or, when a check failed, the value it was given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == 0)
    {
        satellite.statement.if (f == start)
        {
            satellite.statement.if (f.string == start.string)
            {
                out = f.numerator
            }
        }
    }
    satellite.return(out)
}
''')
lines = body.split('\n')
loop = line_index(lines, '// FORWARD: 50 operations take it far from where it began')
end = line_index(lines, '// THE PASS ENDS WHERE IT BEGAN -- or the run stops at the first pass that does not, and says so')
# every check inside the pass also stops the passes at once: a numerator with a loss in it meets the percentage
# shares on the way back, which refuse a share that is not whole (S402) before the pass could end
lines, n = break_checks(lines, loop, end)
if n != 16:
    fail('%d check blocks inside the pass, not 16' % n)
body = '\n'.join(lines)
head = '''// 9. THE FRACTION (fraction_loss.satl, fixed 2026-10-06). f starts as the value handed in, over 1 -- a number given to
// a fraction name becomes that number over 1 -- and each pass runs the test's 100 operations, one a line: its sign
// turned over, its numerator read, worked on as a whole number (+ - *, and * by a percentage to share it back out
// exactly) and given back as n/1, its .denominator read, its .string read back as a number, trips through a list, a
// map, another fraction name and two number names: 50 forward, then their 50 inverses in reverse order. Fraction
// arithmetic itself is not built (1/3 + 1/3 is S210). All of the fixes are here: each whole-number division checked
// right after it runs, the denominator checked after every move and folded into every reading, h = 547311173/7 making
// 18 moves beside f (a fixture of the test's own, not the chain's value), both checked at the halfway point against
// exact math, and every pass checked.
// THE CHAIN CHANGES: f and start are the value handed in; the passes come from `passes`; a failed check becomes the
// loss flag, and a check inside the pass stops the passes at once, since a numerator with a loss in it would meet a
// percentage share on the way back that refuses to round (S402) before the pass ended. The first chain's own fixes
// for this test (the four divisions, every pass checked) are the fixed test's own now, in its form.
// HANDS ON: f's numerator -- or, when a check failed, the value it was given times 2.
'''
write('09_fraction.satl', head + body)
