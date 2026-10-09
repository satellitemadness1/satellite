# time_test/satellite.library/python/race_program.py -- the Python equivalent of
# programs/race_program.satl: a 200-digit number times a 280-digit one, 1,000,000 turns.
# A line-for-line translation: the spacesuit is a class (its protected members carry a leading
# underscore, and it is never used, as in the .satl) and main takes the arguments as satl's does.
# The blank line printed first is the one satl prints after its line of dashes, so the answers
# compare byte for byte.

import sys


class my_class:
    def __init__(self):
        # satellite.protected
        self._class_id = 0

    def _set_class_id(self, class_id_input):
        self._class_id = class_id_input

    # satellite.public
    def call_set_class_id(self, call_id_input):
        self._set_class_id(call_id_input)


def main(arguments):
    huge_int = 94287532189763127896139786298436284976532874612879436123874132798463189479231874238956872365327864523981457692874598275928347592385473298457329874618724618272436187236478123648721364873637373773733333

    another_huge_int = 354279879243783425748932872954789453789542798452798245879425789457895478945279825378923478925389724357895347985379823478952438792435789423587945238790423578942357894352789452378942357892345798542379824359782435798452378924597845328745279823457887942387954289754284253894528890542

    register1 = 0

    counter = 0
    target = 1000000

    while counter < target:
        register1 = huge_int * another_huge_int + register1

        counter = counter + 1

    print(register1 % 1000000007)
    return


print()
main(sys.argv)
