# builds parts/05_multiple.satl from multiple_loss.satl: the fixed test's 100 operations and every one of its checks,
# word for word, on the value handed in; the first chain's look-before-you-remove guards kept
from chainlib import read, write, sub, main_of, as_capsule, line_starting, guard

src = read('multiple_loss.satl')
body = as_capsule(main_of(src), 'multiple_loss')
body = sub(body,
'satellite.container.list, satellite.container.map> x = 547311173\n    satellite.variable.number start = 547311173\n',
'satellite.container.list, satellite.container.map> x = given      // the chain: the value handed in\n    satellite.variable.number start = given\n')
body = sub(body,
'''    satellite.statement.if (wrong == 0)
    {
        satellite.console.display(547311173)
    }
    satellite.statement.else
    {
        satellite.console.display({"pass", failed, "wrong", wrong, x})
    }
    satellite.return(satellite)
}
''',
'''    // HANDS ON x, the number it holds -- or, when a check failed, the value it was given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == 0)
    {
        out = x
    }
    satellite.return(out)
}
''')
# THE FIRST CHAIN'S GUARDS, kept: what an undo takes out by its value must be there -- remove refuses an item that is
# not there, an index of 0 is refused, a missing key cannot be read -- so a missing one is a loss that ends the passes
# with the loss flag instead of stopping the run. The fixed test still leaves these open.
guards = [
    ('x.remove_at(x.index_of(x.size - 1))', 'x.index_of(x.size - 1) == 0', 'index_of answers 0 when it is not there, and remove_at(0) is refused'),
    ('x.remove("satellite")', '!x.contains("satellite")', 'removed by its value, and remove refuses an item that is not there'),
    ('wrong = wrong + lost(x[x.size]["s"], 5)', '!x[x.size].has("s")', 'the key 33 filed must be there, or reading it is refused'),
    ('x.remove({"satellite": 547311173})', '!x.contains({"satellite": 547311173})', 'removed by its value, and remove refuses an item that is not there'),
    ('x.remove(547311173.0)', '!x.contains(547311173.0)', 'removed by its value, and remove refuses an item that is not there'),
    ('wrong = wrong + lost(x[x.search(547311173)], 547311173)', 'x.search(547311173) == 0', 'search answers 0 when nothing holds it, and x[0] is refused'),
]
lines = body.split('\n')
for anchor, cond, why in guards:
    i = line_starting(lines, anchor)
    lines[i:i] = guard(cond, why)
body = '\n'.join(lines)
head = '''// 5. THE MULTIPLE (multiple_loss.satl, fixed 2026-10-06). x, a satellite.container.multiple of a number, a float, a
// string, a list and a map, starts as the value handed in, the NUMBER, and each pass runs the test's 100 operations:
// 1-50 carry it through every one of its five types -- the number past two 64-bit words with its digits turned round,
// into text and back, a float, that float as text with its minus and point spelled out, a list of its characters,
// that list inside a map beside a copy changed on its own, five levels down -- and 51-100 undo them, the last first.
// All of its fixes are here: every whole-number / keeps its remainder (57, 70, 92, 96, 97, 99), the text 89 cuts is
// checked first, the 26 lost() checks before the operations that cannot see a loss, and every pass must end at the
// NUMBER it began as.
// THE CHAIN CHANGES: x starts as the value handed in, and so does start; the passes come from `passes`; what the test
// would have printed instead of 547311173 becomes the loss flag; and the FIRST CHAIN'S GUARDS stay, which the fixed
// test still leaves open: what an undo takes out by its value must be there -- remove refuses an item that is not
// there, index_of and search answer 0 and x[0] is refused, a missing key cannot be read -- so each is looked for
// first (6 guards, marked "chain:"), and a missing one is a loss that ends the passes with a number. The first
// chain's other fixes for this test (the remainders, 89's 0, the markers read before they go) are the fixed test's own.
// HANDS ON: x, the number it holds -- or, when a check failed, the value it was given times 2.
'''
write('05_multiple.satl', head + body)
