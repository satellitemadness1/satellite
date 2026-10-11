# builds parts/00_helpers.satl: the chain's own two capsules for the nine digits, and lost(held, must), which the
# map, list and multiple tests each define -- the same text in all three, checked here -- and which the chain keeps
# once, here, since one program cannot define one capsule three times
from chainlib import read, write, fail

blocks = []
for name in ('map_loss.satl', 'list_loss.satl', 'multiple_loss.satl'):
    src = read(name)
    a = src.index('// lost(held, must)')
    b = src.index('satellite.capsule satellite.main()')
    blocks.append(src[a:b].rstrip() + '\n')
if len(set(blocks)) != 1:
    fail('lost() is not the same text in the map, list and multiple tests: give each part its own')

nine = '''// THE NINE DIGITS (the chain's own). The map, the list, the file and the object hold the value as its nine digits,
// {5, 4, 7, 3, 1, 1, 1, 7, 3} for 547311173. The last eight are single digits; the first holds everything above
// them (and the sign), so every whole number has nine, and the nine always make the number again.
satellite.capsule nine_digits(satellite.variable.number value)
{
    satellite.container.list digits = {}
    satellite.variable.number rest = value
    satellite.variable.number d = 0
    satellite.statement.for(satellite.variable.number i = 0; i < 8; i + 1)
    {
        d = (rest % 10 + 10) % 10
        digits.insert(1, d)
        rest = (rest - d) / 10
    }
    digits.insert(1, rest)
    satellite.return(digits)
}

// the number nine digits make: the first times 10 ^ 8, and so on down
satellite.capsule from_nine_digits(satellite.container.list digits)
{
    satellite.variable.number value = 0
    satellite.statement.for(satellite.variable.number i = 1; i < 10; i + 1)
    {
        value = value * 10 + digits[i]
    }
    satellite.return(value)
}

// FROM THE MAP, LIST AND MULTIPLE TESTS, word for word (each of the three defines it):
'''
write('00_helpers.satl', nine + blocks[0])
