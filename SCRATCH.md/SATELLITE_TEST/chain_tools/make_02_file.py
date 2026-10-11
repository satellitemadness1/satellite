# builds parts/02_file.satl from file_loss.satl (the reviewer found no hole in it, so it is unchanged since the first
# chain): its 100 file operations, made from the value handed in, with the first chain's two checks kept
from chainlib import read, write, sub, main_of, as_capsule

src = read('file_loss.satl')
body = as_capsule(main_of(src), 'file_loss')
# the 14 lines, and every text the operations look for or write that spells 547311173, made from the value handed in
body = sub(body, '{\n    satellite.variable.string name = "file_loss_547311173.se"\n',
'''{
    satellite.variable.number wrong = 0
    satellite.variable.string t = given.string
    satellite.variable.string bits = "b" + given.binary
    satellite.variable.string hx_line = "x" + given.hex
    satellite.container.list digits = nine_digits(given)
    satellite.variable.string name = "file_loss_547311173.se"
''')
body = sub(body, '''    f.append("547311173")
    f.append("b100000100111110100111001000101")
    f.append("x209F4E45")
    f.append("547311173.0")
''', '''    f.append(t)
    f.append(bits)
    f.append(hx_line)
    f.append(t + ".0")
''')
body = sub(body, '''    f.append("5")
    f.append("4")
    f.append("7")
    f.append("3")
    f.append("1")
    f.append("1")
    f.append("1")
    f.append("7")
    f.append("3")
''', '''    satellite.statement.for(satellite.variable.number i = 1; i < 10; i + 1)
    {
        f.append(digits[i].string)
    }
''')
body = sub(body, '    satellite.variable.string seen = ""\n\n', '\n')
for old, new in [
    ('f.append("547311173 is satellite")', 'f.append(t + " is satellite")'),
    ('f.replace("x209F4E45", "x209F4E45 is hex")', 'f.replace(hx_line, hx_line + " is hex")'),
    ('f.replace("547311173.0", "547311173.5")', 'f.replace(t + ".0", t + ".5")'),
    ('f.replace("b100000100111110100111001000101", "binary 547311173")', 'f.replace(bits, "binary " + t)'),
    ('f.insert(3, "547311173 547311173 547311173")', 'f.insert(3, t + " " + t + " " + t)'),
    ('f.replace("binary 547311173", "binary 547311173 (30 bits)")', 'f.replace("binary " + t, "binary " + t + " (30 bits)")'),
    ('f.insert(7, "547311173.0 is a float")', 'f.insert(7, t + ".0 is a float")'),
    ('f.replace("SATELLITE", "satellite 547311173")', 'f.replace("SATELLITE", "satellite " + t)'),
    ('f.insert(1, "547311173")', 'f.insert(1, t)'),
    ('f.remove("547311173")\n', 'f.remove(t)\n'),
    ('f.replace("satellite 547311173", "SATELLITE")', 'f.replace("satellite " + t, "SATELLITE")'),
    ('f.remove("547311173.0 is a float")', 'f.remove(t + ".0 is a float")'),
    ('f.replace("binary 547311173 (30 bits)", "binary 547311173")', 'f.replace("binary " + t + " (30 bits)", "binary " + t)'),
    ('f.remove("x209F4E45")', 'f.remove(hx_line)'),
    ('f.append("x209F4E45")', 'f.append(hx_line)'),
    ('f.remove("547311173 547311173 547311173")', 'f.remove(t + " " + t + " " + t)'),
    ('f.replace("binary 547311173", "b100000100111110100111001000101")', 'f.replace("binary " + t, bits)'),
    ('f.replace("547311173.5", "547311173.0")', 'f.replace(t + ".5", t + ".0")'),
    ('f.replace("x209F4E45 is hex", "x209F4E45")', 'f.replace(hx_line + " is hex", hx_line)'),
    ('f.remove("547311173 is satellite")', 'f.remove(t + " is satellite")'),
]:
    body = sub(body, old, new)
# THE FIRST CHAIN'S TWO CHECKS, kept: the fixed test is the same test, so both holes are still open in it
body = sub(body, '        f.truncate(14)\n', '''        satellite.statement.if (f.size != 16)
        {
            // the chain: truncate(14) keeps 14 lines -- the two it cuts were read above, and there must be no third
            wrong = wrong + 1
        }
        f.truncate(14)
''')
body = sub(body, '        m = f[dot]\n', '''        satellite.statement.if (dot == 0)
        {
            // the chain: the float line's ".5" is gone -- a value whose digits the replaces above reached first (never
            // 547311173's), or a lost line -- so it is a loss, and f[0] is never read
            wrong = wrong + 1
            satellite.statement.break
        }
        m = f[dot]
''')
body = sub(body, '''        // THE PASS ENDS WHERE IT BEGAN -- or the run stops at the first pass that does not, and says so
        satellite.statement.if (f.read_all != begun)
        {
            satellite.statement.if (seen == "")
            {
                seen = "pass " + pass.string + ":"
                satellite.statement.for(satellite.variable.number i = 1; i < f.size + 1; i + 1)
                {
                    seen = seen + " | " + f[i]
                }
            }
            satellite.statement.break
        }
''', '''        // THE PASS ENDS WHERE IT BEGAN -- or the passes stop at the first that does not
        satellite.statement.if (f.read_all != begun)
        {
            wrong = wrong + 1
        }
        satellite.statement.if (wrong > 0)
        {
            satellite.statement.break
        }
''')
body = sub(body, '''    satellite.statement.if (seen == "")
    {
        satellite.statement.if (disk.read_all == begun)
        {
            seen = disk.first
        }
        satellite.statement.else
        {
            seen = "on the disk:"
            satellite.statement.for(satellite.variable.number i = 1; i < disk.size + 1; i + 1)
            {
                seen = seen + " | " + disk[i]
            }
        }
    }
''', '''    satellite.variable.number out = given * 2
    satellite.statement.if (wrong == 0)
    {
        satellite.statement.if (disk.read_all == begun)
        {
            out = disk.first.number
        }
    }
''')
body = sub(body, '''    gone.join()

    satellite.console.display(seen)
    satellite.return(satellite)
}
''', '''    gone.join()
    satellite.return(out)
}
''')
head = '''// 2. THE FILE (file_loss.satl; the reviewer found no hole in it, and it is unchanged). The value is written as a file
// of 14 lines, file_loss_547311173.se beside this program: its text, its binary, its hex, its float, its name, and its
// nine digits one a line. Each pass runs 100 different file operations on it -- 59 forward, the file closed and
// opened (written to the disk and read back) at the middle, 41 back -- and every pass ends by comparing the whole
// file with the 14 lines it began as. At the end the file is read back from the disk, and removed.
// THE CHAIN CHANGES: the 14 lines are made from the value handed in, and so is every text an operation looks for or
// writes that spelled 547311173 ("x209F4E45" is "x" + its hex, "547311173.0" its text + ".0", and so on), so the
// test runs on whatever value it is given; the passes come from `passes`; and a pass that did not come back becomes
// the loss flag. Two checks of the first chain stay, since the test is the same: truncate(14), the one operation that
// cuts lines it never read, must cut exactly the two it read (the file holds 16 lines there); and a float line whose
// ".5" is gone is a loss that stops the passes, rather than a read of line 0.
// HANDS ON: the first line of the file, read back from the disk after the last pass -- or, when a pass did not come
// back, the value it was given times 2. The file is removed before the test ends (satellite.system.delete is not
// built, so bash removes it, by the path satl gives).
'''
write('02_file.satl', head + body)
