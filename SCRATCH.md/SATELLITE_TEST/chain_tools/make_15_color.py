# builds parts/15_color.satl from color_loss.satl (the reviewer found no hole in it, so it is unchanged since the
# first chain): worth, the colour as a number, starts at the value handed in; paint starts at its own written start,
# since no number can be put into a colour (S301); every pass checks paint came back (the first chain's check)
from chainlib import read, write, sub, main_of, as_capsule

body = as_capsule(main_of(read('color_loss.satl')), 'color_loss')
body = sub(body, '    satellite.variable.number worth = paint.transparency * x1000000 + paint.number      // 547311173\n',
           '    satellite.variable.number wrong = 0\n'
           '    satellite.variable.number worth = given      // the chain: the value handed in (the test makes it from paint, 547311173)\n')
body = sub(body, '''        paint.transparency(paint.transparency - 5)
        worth = worth - paint.number
    }
''', '''        paint.transparency(paint.transparency - 5)
        worth = worth - paint.number
        // THE PASS ENDS WHERE IT BEGAN -- or the passes stop here (the chain's, kept from the first chain)
        satellite.statement.if (paint != start)
        {
            wrong = wrong + 1
        }
        satellite.statement.if (wrong > 0)
        {
            satellite.statement.break
        }
    }
''')
body = sub(body, '''    satellite.statement.if (paint != start)
    {
        satellite.console.display(paint)
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
''', '''    // HANDS ON worth -- times 2 when paint did not come back; a worth that came back wrong is handed on wrong
    satellite.variable.number out = worth
    satellite.statement.if (wrong > 0)
    {
        out = worth * 2
    }
    satellite.return(out)
}
''')
head = '''// 15. THE COLOUR (color_loss.satl; the reviewer found no hole in it, and it is unchanged). A colour takes its digits
// only from a six-digit hex, and no number can be put into one (S301), so paint, the colour, starts at its own
// written start, x9F4E45, 32 -- 547311173 is x209F4E45, and x20 is 32, its transparency -- and worth, the colour as a
// number, carries the value. The colour goes through its transparency changed both ways, c.transparency(n) and
// c = c, n (plus, minus and 99 - t, with numbers, binaries and hexes; down to 0 and up to 99), its digits turned round
// through the hex paint_hex and put back, and moves between colour names. worth takes in every reading -- .number,
// .binary, .transparency -- and gives it back. 50 operations out and 50 back, in the reverse order. Every / keeps
// its remainder.
// THE CHAIN CHANGES: worth starts at the value handed in (the test makes it from paint); the passes come from
// `passes`; and the FIRST CHAIN'S PER-PASS CHECK stays -- every pass ends by comparing paint with where it began (its
// digits and its transparency), and the passes stop at the first that did not come back; the test compared it only
// after the last pass.
// HANDS ON: worth -- times 2 when paint did not come back; a worth that came back wrong is handed on wrong.
'''
write('15_color.satl', head + body)
