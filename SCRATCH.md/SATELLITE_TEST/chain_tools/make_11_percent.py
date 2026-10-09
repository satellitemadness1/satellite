# builds parts/11_percent.satl from percent_loss.satl: the fixed test's 100 operations and its 30 guards and the keyed
# check, word for word, on the value handed in -- made a percentage digit by digit; the first chain's per-pass check
from chainlib import read, write, sub, main_of, as_capsule

src = read('percent_loss.satl')
body = as_capsule(main_of(src), 'percent_loss')
# a number is never given to a percentage name (S301), so x is built from the value's digits with percentage
# arithmetic alone, as the first chain built it -- ten times itself (x * 1000%) plus the next digit's percentage
body = sub(body, '    satellite.variable.percentage start = 547311173%\n    satellite.variable.percentage x = 547311173%\n',
'''    // the chain: the value handed in, as a percentage -- a number is never given to a percentage name (S301), so it
    // is built from its digits with percentage arithmetic alone: ten times itself (x * 1000%) plus the next digit
    satellite.container.map digit = {"0": 0%, "1": 1%, "2": 2%, "3": 3%, "4": 4%, "5": 5%, "6": 6%, "7": 7%, "8": 8%, "9": 9%}
    satellite.variable.string text = given.string
    satellite.variable.percentage x = 0%
    satellite.statement.for(satellite.variable.number i = 1; i < text.size + 1; i + 1)
    {
        satellite.statement.if (text[i] != "-")
        {
            x = x * 1000% + digit[text[i]]
        }
    }
    satellite.statement.if (given < 0)
    {
        x = -x
    }
    satellite.variable.percentage start = x
''')
# THE FIRST CHAIN'S PER-PASS CHECK, kept: the fixed test reads `wrong` and compares x only after the last pass
body = sub(body, '''        x = x * 4100%
    }
''', '''        x = x * 4100%
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
''', '''    // HANDS ON 100 * x, the number x is the percentage of -- or, when a check failed, the value it was given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == 0)
    {
        satellite.statement.if (x == start)
        {
            out = 100 * x
        }
    }
    satellite.return(out)
}
''')
head = '''// 11. THE PERCENTAGE (percent_loss.satl, fixed 2026-10-06). x starts as the value handed in, as a percentage:
// 547311173%. Each pass runs the test's 100 operations, one a line: + and - of percentages, a percentage OF a
// percentage (*), a share by a number (/) -- a literal, a number below zero, a binary, a number name, and numbers
// made of a number and a percentage -- its sign turned over, and trips through a list, a map, a map keyed by a
// percentage and both spellings' names: 50 forward, then their 50 inverses in reverse order. A percentage keeps 32
// digits after the point exactly. All of the fixes are here: the 30 operations that can round a loss away are each
// multiplied back and compared with x before them, and what keyed[12.5%] adds is checked against 87.5%.
// THE CHAIN CHANGES: a number is never given to a percentage name (S301), so x is built from the digits of the value
// handed in with percentage arithmetic alone, as the first chain built it, and start is that; the passes come from
// `passes`; a failed check becomes the loss flag; and the FIRST CHAIN'S PER-PASS CHECK stays -- the fixed test
// compares x only after the last pass. The first chain's 28 whole-number checks are replaced by the fixed test's 30.
// HANDS ON: 100 * x, the number x is the percentage of -- or, when a check failed, the value it was given times 2.
'''
write('11_percent.satl', head + body)
