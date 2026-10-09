# builds parts/14_hex.satl from hex_loss.satl, the author's hex walk (the reviewer found no hole in it, so it is
# unchanged since the first chain): the walk carries the value handed in, 50 x passes steps each way
from chainlib import read, write, sub, main_of

body = main_of(read('hex_loss.satl'))
body = sub(body, 'satellite.capsule satellite.main()\n',
           'satellite.capsule hex_loss(satellite.variable.number given, satellite.variable.number passes)\n')
body = sub(body, '    satellite.variable.number walk = my_hex\n',
           '    satellite.variable.number steps = 50 * passes                  // the chain: 500,000 at 10,000 passes\n'
           '    satellite.variable.number walk = given + my_hex                // the chain: the value handed in, plus the hex\n')
body = sub(body, '    satellite.statement.while(step < 500000)\n', '    satellite.statement.while(step < steps)\n', 2)
body = sub(body, '''    satellite.statement.if (walk == my_hex)
    {
        satellite.console.display(547311173)
    }
    satellite.statement.else
    {
        satellite.console.display(walk)
    }

    satellite.return(satellite)
}
''', '''    // HANDS ON the walk minus the hex
    satellite.return(walk - my_hex)
}
''')
head = '''// 14. THE HEX WALK (hex_loss.satl, the author's own: "my_hex = 547311173 then my_hex = my_hex + 1 (500,000 times)
// then 500,000 - 1 then we should still have the same hex code"; the reviewer found no hole in it, and it is
// unchanged). A hex plus 1 answers a number, and nothing turns a number back into a hex, so the walk is a number.
// THE CHAIN CHANGES: the walk starts at the value handed in plus what the hex x547311173 is worth (a hex counts as its
// worth in a sum), and walks 50 x passes steps up and 50 x passes steps down -- 500,000 each way at the default 10,000
// passes, 1,000,000 operations -- and then the hex's worth is taken out again; the test compared the walk with the hex.
// HANDS ON: the walk minus the hex: the value handed in, when no step and neither meeting with the hex lost anything.
'''
write('14_hex.satl', head + body)
