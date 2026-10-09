// time_test/satellite.library/cpp/race_program.cpp -- the C++ equivalent of
// programs/race_program.satl: a 200-digit number times a 280-digit one, plus the running
// total, 1,000,000 turns.
// Not line for line: the numbers are bignum (bignum.hpp), a hand-written big whole number,
// because C++ has none; counter and target stay below 2^63, so they are long long.
//
// BUILD: clang++ -std=c++20 -O2 race_program.cpp -o race_program   (or g++)

#include "bignum.hpp"

#include <cstdio>

// The spacesuit the .satl declares and never uses, kept so the program is the same program.
class my_class {
protected:
    bignum class_id = bignum(0);

    void set_class_id(const bignum &class_id_input) { class_id = class_id_input; }

public:
    void call_set_class_id(const bignum &call_id_input) { set_class_id(call_id_input); }
};

int main()
{
    bignum huge_int = bignum::from_text("942875321897631278961397862984362849765328746128794361238741327984631894792318742389568"
                                        "723653278645239814576928745982759283475923854732984573298746187246182724361872364781236"
                                        "48721364873637373773733333");

    bignum another_huge_int = bignum::from_text(
        "354279879243783425748932872954789453789542798452798245879425789457895478945279825378923478925389724357895347985"
        "379823478952438792435789423587945238790423578942357894352789452378942357892345798542379824359782435798452378924"
        "597845328745279823457887942387954289754284253894528890542");

    bignum register1(0);

    long long counter = 0;
    long long target = 1000000;

    while (counter < target) {
        register1 = huge_int * another_huge_int + register1;

        counter = counter + 1;
    }

    std::printf("\n"); // satl's own empty line after its dashes: the race's answer starts there
    std::printf("%llu\n", register1 % 1000000007ull);
    return 0;
}
