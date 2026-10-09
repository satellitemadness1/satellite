# builds parts/07_object.satl from object_loss.satl: its spacesuits (the vault now made from the value handed in) and
# the fixed test's 100 calls a pass, with every check -- the middle too
from chainlib import read, write, sub

src = read('object_loss.satl')
suits = src[src.index('// AN OBJECT INSIDE THE OBJECT'):src.index('satellite.capsule satellite.main()')]
suits = sub(suits, '''        satellite.variable.number value = 547311173
        satellite.variable.string text = "547311173"
        satellite.container.list<satellite.variable.number> digits = {5, 4, 7, 3, 1, 1, 1, 7, 3}''',
'''        satellite.variable.number value = 0
        satellite.variable.string text = ""
        satellite.container.list<satellite.variable.number> digits = {}''')
suits = sub(suits, '''    satellite.constructor()
    {
        pocket made
        inner = made
    }''', '''    // the chain: the vault is made of the value handed in -- the number, its text and its nine digits
    satellite.constructor(satellite.variable.number start)
    {
        pocket made
        inner = made
        value = start
        text = start.string
        digits = nine_digits(start)
    }''')
main = src[src.index('satellite.capsule satellite.main()'):]
body = main[main.index('    big_vault v\n'):main.index("    // 547311173, the object's own number -- or what went wrong\n")]
body = sub(body, '    big_vault v\n', '    big_vault v(given)\n')
body = sub(body, '; pass < 10000; pass + 1)', '; pass < passes; pass + 1)')
head = '''// 7. THE OBJECT (object_loss.satl, fixed 2026-10-06). A big_vault -- a spacesuit that extends vault -- made of the
// value handed in: the number, its text, the list of its nine digits, an object of its own (a pocket, holding 0),
// and big_vault's own field, extra, at 0. Each pass makes the test's 100 calls of its public capsules -- 50 forward,
// then the same 50 undone, last first -- through FOUR names for the one object (v; same; as_vault, a vault name
// holding it, so its calls run big_vault's capsules where big_vault replaces vault's; and p, its .pointer()), with
// all of its checks: a division counts a remainder it would drop, a pop counts a digit that is not the one pushed,
// the fix -- every field checked against its known middle after the 50 calls out -- and every pass ending where it
// began; at the end the .reference() taken at the start must still be the start, the object must equal it field for
// field, and the pointer must still point.
// THE CHAIN CHANGES: the vault is made from the value handed in (its constructor takes it) instead of from 547311173
// written in its fields; the passes come from `passes`; and what the test would have printed instead of 547311173
// becomes the loss flag. The first chain had no fixes of its own for this test.
// HANDS ON: the object's own number, read back with get() -- or, when anything went wrong, the value it was given
// times 2.
'''
capsule = ('satellite.capsule object_loss(satellite.variable.number given, satellite.variable.number passes)\n{\n' + body +
           '''    // HANDS ON the object's own number -- or, when anything went wrong, the value it was given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (seen == "")
    {
        out = v.get()
    }
    satellite.return(out)
}
''')
write('07_object.satl', head + suits.rstrip() + '\n\n' + capsule)
