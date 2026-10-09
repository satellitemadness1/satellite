# builds parts/01_map.satl from map_loss.satl: the fixed test's 100 operations and every one of its checks, word for
# word, on the nine digits of the value handed in; a caught loss hands on the value times 2
from chainlib import read, write, sub, main_of, as_capsule

src = read('map_loss.satl')
body = as_capsule(main_of(src), 'map_loss')
body = sub(body,
'''    satellite.container.map x = {"s": 5, "sa": 4, "sat": 7, "sate": 3, "satel": 1, "satell": 1, "satelli": 1, "satellit": 7, "satellite": 3}
    satellite.container.map start = {"s": 5, "sa": 4, "sat": 7, "sate": 3, "satel": 1, "satell": 1, "satelli": 1, "satellit": 7, "satellite": 3}
''',
'''    satellite.container.list d = nine_digits(given)                                                                      // the chain: the value handed in, as its nine digits
    satellite.container.map x = {"s": d[1], "sa": d[2], "sat": d[3], "sate": d[4], "satel": d[5], "satell": d[6], "satelli": d[7], "satellit": d[8], "satellite": d[9]}
    satellite.container.map start = x
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
'''    // HANDS ON the nine values put back into the number -- or, when a check failed, the value given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == 0)
    {
        out = from_nine_digits(x.values)
    }
    satellite.return(out)
}
''')
head = '''// 1. THE MAP (map_loss.satl, fixed 2026-10-06), FIRST: its operations are written around the order 547311173's
// digits sort into (1 1 1 3 3 4 5 7 7), and the first test always meets the value untouched. The value goes in as
// its nine digits, each filed under the word up to the letter it looks like -- {"s": 5, "sa": 4, ... "satellite": 3}
// for 547311173 -- and each pass runs the test's 100 operations, 1-50 away and 51-100 back, with all of its fixes:
// every whole-number / keeps its remainder (30, 58, 77, 84, 85, 90, 91), 92 takes 547311168 away, the 23 lost()
// checks before the operations that cannot see a loss, and every pass checked -- the same entries in the same order.
// THE CHAIN CHANGES ONLY: the map starts from the nine digits of the value handed in, not from its own literal; the
// passes come from `passes`; and what it would have printed instead of 547311173 -- {"pass", ..., "wrong", ...} --
// becomes the loss flag. Its own first chain's fixes are all in the fixed test now, so none is added here.
// HANDS ON: the nine values put back together into the number -- or, when a check failed, the value it was given
// times 2 (an even number, which 547311173 never is).
'''
write('01_map.satl', head + body)
