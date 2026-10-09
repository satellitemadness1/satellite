# builds parts/04_string.satl from string_loss.satl: the fixed test's 100 operations and every one of its checks, word
# for word, on the text of the value handed in; the first chain's checks the fixed test leaves open kept
from chainlib import read, write, sub, main_of, as_capsule, line_index, guard, break_checks, fail

src = read('string_loss.satl')
body = as_capsule(main_of(src), 'string_loss')
body = sub(body, '    satellite.variable.string s = "547311173"\n',
'''    satellite.variable.string s = given.string                 // the chain: the value handed in, as its text
    satellite.variable.string t0 = given.string                // the text every pass must end at
''')
# the end of a pass: the test stopped its loop with pass = 10000 when the string did not end at "547311173"
body = sub(body, '''        satellite.statement.if (s != "547311173")
        {
            pass = 10000
        }
''', '''        satellite.statement.if (s != t0)
        {
            satellite.statement.break
        }
''')
body = sub(body, '''    satellite.statement.if (s == "547311173")
    {
        satellite.console.display(547311173)
    }
    satellite.statement.else
    {
        satellite.console.display(s)
    }
    satellite.return(satellite)
}
''', '''    // HANDS ON the string read back as a number -- or, when a check failed, the value it was given times 2
    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == "")
    {
        satellite.statement.if (s == t0)
        {
            out = s.number
        }
    }
    satellite.return(out)
}
''')
lines = body.split('\n')
loop = line_index(lines, '// 50 out')
end = line_index(lines, '// every pass ends at 547311173 and fails no check; the first that does not stops here, keeping its string')
# every check inside the pass also stops the passes at once: a later .find or .substring of a text a loss took away
# would otherwise stop the run with a refusal (S420, S411) before the pass could be answered wrong
lines, n = break_checks(lines, loop, end, 'wrong = wrong + "')
if n != 13:
    fail('%d check blocks in the pass, not 13' % n)
# THE FIRST CHAIN'S CHECKS, kept where the fixed test still leaves them open:
# (1) .find refuses a text that is not there (S420), so each text a check or a cut finds is looked for first
# (2) F10 and F05 are undone by a .replace that takes EVERY copy, and no check guards them: a second U+19C40 or e-acute
#     -- one a loss wrote in place of the letter beside it -- would go with the first, unseen. The first chain checked
#     that exactly one copy went; here it is the fixed test's own form of that check (F20, F17): the text must stand
#     once, as the very end.
finds = [
    ('// CHECK: the undo below cuts the string at "1/3", so it must stand once, as the very end', '"1/3"', 'F21'),
    ('// CHECK: the undo below takes every copy of "x1F", so it must stand once, as the very end', '"x1F"', 'F20'),
    ('// CHECK: the undo below cuts the string at "b1010", so it must stand once, as the very end', '"b1010"', 'F19'),
    ('// CHECK: the undo below takes every copy of "1.5", so it must stand once, as the very end', '"1.5"', 'F17'),
    ('// CHECK: the undo below cuts the string at "\U0001f600", so it must stand once, as the very end', '"\U0001f600"', 'F08'),
    ('// CHECK: the undo below cuts the string at "ω", so it must stand once, as the very end', '"ω"', 'F06'),
]
for anchor, text, name in finds:
    i = line_index(lines, anchor)
    lines[i:i] = guard('!s.contains(' + text + ')', '.find refuses a text that is not there (S420), so it is looked for first',
                       'wrong = wrong + " ' + name + '"')
for undo, text, name, what in [
    ('s = s.replace("\U00019c40", "")  // undoes F10', '"\U00019c40"', 'F10', 'U+19C40'),
    ('s = s.replace("\xe9", "")  // undoes F05', '"\xe9"', 'F05', '\xe9'),
]:
    i = line_index(lines, undo)
    lines[i:i] = (['        // CHECK (the first chain\'s, kept): the undo below takes every copy of ' + what + ', so it must stand once, as',
                   '        // the very end'] +
                  guard('!s.contains(' + text + ')', '', 'wrong = wrong + " ' + name + '"') +
                  guard('s.find(' + text + ') + 1 != s.size', '', 'wrong = wrong + " ' + name + '"'))
body = '\n'.join(lines)
head = '''// 4. THE STRING (string_loss.satl, fixed 2026-10-06). The value goes in as its text, "547311173", and each pass
// runs the test's 100 operations -- 50 that carry it away to 1,411 characters (.hex .bin .number, every kind of number
// joined in front and behind, case changes, accents, Greek, Cyrillic, Deseret, emoji, CJK, escapes, .replace
// .split/.join .reverse(), rotations, answers written in, colours, whitespace, every character split apart) and 50
// that undo them in reverse order -- with all of its fixes: the string compared with its literal before F15 and after
// F31 and again once F32 is undone, the middle 1,411 characters, .trim's whitespace, U+2005F last, the colour codes
// cut by position and what they carry, and each cut at a .find and each take-every-copy undo finding its text once,
// as the very end.
// THE CHAIN CHANGES: the string starts as the text of the value handed in, and every pass must end at that text
// (t0); the passes come from `passes` (the test stopped them with pass = 10000; here they break); a failed check --
// which the test names in `wrong` -- becomes the loss flag, and stops the passes at once, since a later .find or
// .substring of a text a loss took away would otherwise stop the run with a refusal before the pass ended. And two
// checks of the first chain stay, which the fixed test leaves open: .find refuses a text that is not there (S420), so
// each text a check finds is looked for first; and the take-every-copy undos of F10 and F05 are checked as the test
// checks F20 and F17 -- the text must stand once, as the very end -- so a second copy cannot go unseen with the first.
// HANDS ON: the string read back as a number -- or, when a check failed or a pass did not come back, the value it
// was given times 2.
'''
write('04_string.satl', head + body)
