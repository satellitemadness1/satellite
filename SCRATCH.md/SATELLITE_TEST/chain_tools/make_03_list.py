# builds parts/03_list.satl from list_loss.satl: the fixed test's 100 operations and every one of its checks, word for
# word, on the nine digits of the value handed in; the first chain's look-before-you-remove guards kept
from chainlib import read, write, sub, main_of, as_capsule, line_starting, guard

src = read('list_loss.satl')
body = as_capsule(main_of(src), 'list_loss')
body = sub(body,
'''    satellite.container.list x = {5, 4, 7, 3, 1, 1, 1, 7, 3}
    satellite.container.list start = {5, 4, 7, 3, 1, 1, 1, 7, 3}
''',
'''    satellite.container.list x = nine_digits(given)            // the chain: the value handed in, as its nine digits
    satellite.container.list start = x
''')
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
'''    // HANDS ON the nine digits put back into the number -- or, when a check failed, the value given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == 0)
    {
        out = from_nine_digits(x)
    }
    satellite.return(out)
}
''')
# THE FIRST CHAIN'S GUARDS, kept: remove refuses an item that is not there (S420), an index of 0 is refused (S413),
# so each item an undo takes out by its value is looked for first -- a missing one is a loss that ends the passes and
# hands on the loss flag, a number, instead of stopping the whole chain. The fixed test still leaves these open.
# (anchor line -- the guard goes right before it --, the condition that is the loss)
guards = [
    ('x.remove({})', '!x.contains({})'),
    ('wrong = wrong + lost(x[x.search(20)], 20)', 'x.search(20) == 0'),
    ('x.remove(satellite.bool.false)', '!x.contains(satellite.bool.false)'),
    ('x.remove(satellite.bool.true)', '!x.contains(satellite.bool.true)'),
    ('x.remove("0-1-1-1-10-3-3-4-5-7")', '!x.contains("0-1-1-1-10-3-3-4-5-7")'),
    ('x.remove(0)', 'lost(x.last, 0) == 1'),
    ('x.remove(547311173)', '!x.contains(547311173)'),
    ('x.remove(35)', '!x.contains(35)'),
    ('x.remove({"satellite": 547311173})', '!x.contains({"satellite": 547311173})'),
    ('x[13].remove(0)', '!x[13].contains(0)'),
    ('x.remove({7, 7, 5, 4, 3, 3, 1, 1, 1})', '!x.contains({7, 7, 5, 4, 3, 3, 1, 1, 1})'),
    ('x[1].remove("satellite")', '!x[1].contains("satellite")'),
    ('wrong = wrong + lost(x[x.search(547311173.0)], 547311173.0)', 'x.search(547311173.0) == 0'),
    ('x.remove("547311173")', '!x.contains("547311173")'),
    ('x.remove_at(x.index_of(547311173))', 'x.index_of(547311173) == 0'),
]
why = {
    'lost(x.last, 0) == 1': 'the 0 that 41 put in is the last item: read first, so remove(0) cannot refuse or take another',
    'x.search(20) == 0': 'search answers 0 when nothing holds it, and x[0] is refused',
    'x.search(547311173.0) == 0': 'search answers 0 when nothing holds it, and x[0] is refused',
    'x.index_of(547311173) == 0': 'index_of answers 0 when it is not there, and remove_at(0) is refused',
}
lines = body.split('\n')
for anchor, cond in guards:
    i = line_starting(lines, anchor)
    lines[i:i] = guard(cond, why.get(cond, 'removed by its value, and remove refuses an item that is not there'))
body = '\n'.join(lines)
head = '''// 3. THE LIST (list_loss.satl, fixed 2026-10-06). The value goes in as the list of its nine digits -- {5, 4, 7, 3,
// 1, 1, 1, 7, 3} for 547311173 -- and each pass runs the test's 100 operations, 1-50 away and 51-100 back, with all
// of its fixes: every whole-number / keeps its remainder (80, 90, 91, 96, 98), 95 takes 547311172000 away, the 20
// lost() checks before the operations that cannot see a loss, and every pass checked against where it began.
// THE CHAIN CHANGES: the list starts from the nine digits of the value handed in; the passes come from `passes`; what
// the test would have printed instead of 547311173 becomes the loss flag; and the FIRST CHAIN'S GUARDS stay, since
// the fixed test still leaves them open: remove(x) refuses an item that is not there (S420) and an index of 0 is
// refused (S413), so each item an undo takes out by its value is looked for first (15 guards, marked "chain:"), and
// one that is missing is a loss that ends the passes with a number instead of stopping the run. The first chain's
// other list fixes (the remainders, 95) are the fixed test's own now; its 60 is a guard here (the 0 must be the last
// item) and 60 itself stays the test's remove(0).
// HANDS ON: the nine digits put back together into the number -- or, when a check failed, the value it was given
// times 2.
'''
write('03_list.satl', head + body)
