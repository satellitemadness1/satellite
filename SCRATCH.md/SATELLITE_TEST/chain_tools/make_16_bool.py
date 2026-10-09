# builds parts/16_bool.satl from bool_loss.satl (the reviewer found no hole in it, so it is unchanged since the first
# chain): its 100 operations a pass on the low 30 bits of the value handed in; what 30 bits cannot hold is carried
# around them
from chainlib import read, write, sub

src = read('bool_loss.satl')
spelled = src[src.index('// the number 30 bools spell, the first one the top bit'):src.index('satellite.capsule satellite.main()')]
main = src[src.index('satellite.capsule satellite.main()'):]
bits = main[main.index('    // THE 30 BITS OF 547311173, from the top'):main.index('    satellite.variable.string seen = ""')]
loop = main[main.index('    satellite.statement.for(satellite.variable.number pass = 0; pass < 10000; pass + 1)'):
            main.index('    // 547311173, PUT BACK TOGETHER FROM THE 30 BOOLS')]
loop = sub(loop, '; pass < 10000; pass + 1)', '; pass < passes; pass + 1)')
loop = sub(loop, '''            satellite.statement.if (seen == "")
            {
                seen = "pass " + pass.string + ": " + spelled({s29, s28, s27, s26, s25, s24, s23, s22, s21, s20, s19, s18, s17, s16, s15, s14, s13, s12, s11, s10, s09, s08, s07, s06, s05, s04, s03, s02, s01, s00}).string
            }
            satellite.statement.break''', '''            wrong = wrong + 1
            satellite.statement.break''')
head = '''// 16. THE BOOL (bool_loss.satl; the reviewer found no hole in it, and it is unchanged), LAST: the number put back
// together from 30 bools. A bool cannot hold a number, so this test holds its 30 bits: s29 ... s00 are the bits of
// the value handed in, each made from the number itself -- 547311173 is b100000100111110100111001000101, all of it in
// 30 bits. Each pass runs 100 DIFFERENT bool operations, one a line: 50 forward, then the same 50 undone in reverse
// order, each undo spelled differently. Every line puts a function of OTHER bools into one bool with ! (not), != (xor)
// or == (xnor), so every line undoes itself; and since a line that undoes itself also undoes its own mistake on the
// next pass, every pass ends by comparing all 30 bools with where they began, and the passes stop at the first that
// did not come back.
// THE CHAIN CHANGES: the 30 bits are the low 30 bits of the value handed in, and what 30 bits cannot hold (nothing,
// for 547311173) is carried around them as a number and put back above them at the end; the passes come from
// `passes`; a pass that did not come back becomes the loss flag.
// HANDS ON: the number the 30 bools spell, with what they could not hold put back above them -- times 2 when a pass
// did not come back.
'''
cap = '''satellite.capsule bool_loss(satellite.variable.number given, satellite.variable.number passes)
{
    satellite.variable.number wrong = 0
    satellite.variable.number base = 1073741824                      // the chain: 2 ^ 30
    satellite.variable.number start = (given % base + base) % base     // the chain: the low 30 bits of the value handed in
    satellite.variable.number above = (given - start) / base           // the chain: what 30 bits cannot hold, 0 for 547311173

''' + bits.rstrip() + '\n\n' + loop.rstrip() + '''

    // HANDS ON the number the 30 bools spell, with what they could not hold put back above them -- times 2 when a
    // pass did not come back
    satellite.variable.number out = above * base + spelled({s29, s28, s27, s26, s25, s24, s23, s22, s21, s20, s19, s18, s17, s16, s15, s14, s13, s12, s11, s10, s09, s08, s07, s06, s05, s04, s03, s02, s01, s00})
    satellite.statement.if (wrong > 0)
    {
        out = out * 2
    }
    satellite.return(out)
}
'''
write('16_bool.satl', head + spelled.rstrip() + '\n\n' + cap)
