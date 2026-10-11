# builds parts/10_float.satl from float_loss.satl: the fixed test's 100 operations and its 24 guards and three %
# checks, word for word, on the value handed in; the first chain's per-pass check kept
from chainlib import read, write, sub, main_of, as_capsule

src = read('float_loss.satl')
# the three settings under its satellite.include are the chain's head now (the same three lines, for the whole program)
sub(src, 'satellite.include(satellite)\narguments.float.decimal(128)\narguments.float.whole(4096)\narguments.infinity_display(128)\n', '')
body = as_capsule(main_of(src), 'float_loss')
body = sub(body, '    satellite.variable.float start = 547311173\n    satellite.variable.float x = 547311173.0\n',
           '    satellite.variable.float start = given                      // the chain: the value handed in, as a float\n'
           '    satellite.variable.float x = given\n')
# THE FIRST CHAIN'S PER-PASS CHECK, kept: the fixed test reads `wrong` and compares x only after the last pass
body = sub(body, '''        x = x / 10 ^ -100
    }
''', '''        x = x / 10 ^ -100
        // THE PASS ENDS WHERE IT BEGAN -- or the passes stop here (the chain's, kept from the first chain)
        satellite.statement.if (x != start)
        {
            wrong = wrong + 1
        }
        satellite.statement.if (wrong > 0)
        {
            satellite.statement.break
        }
    }
''')
body = sub(body, '''    satellite.statement.if (wrong > 0)
    {
        satellite.console.display("lost: " + wrong + " checks failed, and x ended at " + x)
    }
    satellite.statement.else satellite.statement.if (x == start)
    {
        satellite.console.display(547311173)
    }
    satellite.statement.else
    {
        satellite.console.display(x)
    }
    satellite.return(satellite)
}
''', '''    // HANDS ON x as a whole number -- or, when a check failed, the value it was given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == 0)
    {
        satellite.statement.if (x == start)
        {
            out = x.number
        }
    }
    satellite.return(out)
}
''')
head = '''// 10. THE FLOAT (float_loss.satl, fixed 2026-10-06). x starts as the value handed in, as a float -- a whole number
// given to a float name becomes one -- and each pass runs the test's 100 operations: 50 that carry it far away (down
// to 0.000...0547311173, out to the 128th place, through a run of 9s, across zero five times, up to 1000 digits
// before the point), then the 50 that undo them, in reverse order: + - * / % and ^ between floats, a float and a
// whole number either way round and a float and text, x squared and cubed and divided by itself, x's whole part, and
// x through its own text. Every operation is exact by the float's own rules, which the lines under
// satellite.include at the top of this program hold whatever config.ini says. All of the fixes are here: the 24
// operations that can round a loss away are each multiplied back and compared with x before them, and what each of
// the three % lines adds is checked against the literal its undo takes away.
// THE CHAIN CHANGES: x and start are the value handed in; the passes come from `passes`; a failed check becomes the
// loss flag; and the FIRST CHAIN'S PER-PASS CHECK stays -- the fixed test compares x only after the last pass, so a
// pass that did not come back is caught here, at its end, before the next pass can undo it. The first chain's 22
// whole-number checks are replaced by the fixed test's 24 guards.
// HANDS ON: x as a whole number -- or, when a check failed, the value it was given times 2.
'''
write('10_float.satl', head + body)
