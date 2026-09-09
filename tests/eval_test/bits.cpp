// M19.5's rows through the machine: `satellite.variable.binary` -- the literal
// becoming a value, the width surviving display and equality, the six
// methods, and the refusals. tests/eval_test/hex.cpp is the other radix.
//
// THE WIDTH IS WHAT THIS SECTION IS ABOUT. DESIGN §8.5's whole claim is that
// "the width is part of the value", and it is a claim nothing but a test can
// hold: every one of the fixtures below would still pass if a bit run were an
// integer, EXCEPT the three that pin `b0010` apart from `b10`. Those three are
// the section.
//
// AND THE PACKING IS NOT TESTED HERE BECAUSE IT IS NOT OBSERVABLE HERE.
// satellite_bits stores one bit per bit (`std::vector<bool>`), which is a
// memory decision with no behaviour attached -- a program cannot tell it from
// one byte per bit, which is exactly why it was safe to take. What a test can
// see is the width and the value, and both are below.

#include "eval_test.hpp"

#include "satellite_scalars/handlers.hpp"

#include <string>

namespace eval_test {

namespace {

std::string capsule(const std::string &body)
{
    return "satellite.capsule it()\n{\n" + body + "}\n";
}

std::string answers(const std::string &body)
{
    Run run;
    build(capsule(body), run);
    if (!run.built)
        return "<did not compile>";
    return answer_of(run, "it", {});
}

// The sentence a body is refused with -- compiling.cpp's refusal_in(), which
// this section needs for the same three reasons and which is deliberately
// COPIED rather than shared: eval_test.hpp is the suite's door and a helper
// promoted into it is a helper every section then has to read past. Four
// lines duplicated beats a header that grows once per section.
std::string refused(const std::string &body)
{
    Run run;
    build(capsule(body), run);
    if (!run.built)
        return "<did not compile>";
    call(run, "it", {});
    if (last_problems.empty())
        return "<ran without complaint>";
    return satellite::errors::sentence(last_problems.front());
}

} // namespace

void section_bits()
{
    using namespace satellite;

    scalars::install_handlers();

    // --- the literal is a value, which is the whole of the done-when --------

    check(answers("    satellite.return(b1010)\n") == "b1010",
          "a binary literal COMPILES TO A CONSTANT and displays as written -- "
          "until M19.5 this was S0720, and PLAN §8's done-when is that "
          "`satl --compile` no longer has that code in reach for a literal");

    check(answers("    satellite.variable.binary bits = b1010\n"
                  "    satellite.return(bits)\n") == "b1010",
          "a name declared `satellite.variable.binary` holds one");

    // --- the width is part of the value -- DESIGN §8.5, and these three are
    //     the only fixtures here an integer representation would fail -------

    check(answers("    satellite.return(b00001010)\n") == "b00001010",
          "DISPLAY KEEPS THE LEADING ZEROS. A bit run that printed `b1010` "
          "here would be printing a different value from the one it holds");

    check(answers("    satellite.return(b00001010 == b1010)\n") == "false",
          "`b00001010 == b1010` is FALSE -- same value, different width, and "
          "§8.5's `x0009 is not x9` is this sentence one radix over");

    check(answers("    satellite.return(b1010 == b1010)\n") == "true",
          "and two runs written the same ARE equal, so the check above is "
          "about the width rather than about equality being broken");

    // --- the four methods, EACH THROUGH A DECLARED NAME ---------------------
    //
    // AND THE NAME IS NOT DECORATION -- IT IS THE LANGUAGE. A selector folds
    // only through a DECLARED name (WORD_NUMBERS §1.5's one hop), so
    // `b1010.to_number()` does not compile and `bits.to_number()` does. PLAN
    // §8's M19.5 entry predicted this milestone would feel it more than most,
    // "because a conversion verb is exactly the shape people chain", and every
    // fixture below is written the way the entry says to write it. The last
    // check in this section pins the refusal, so the rule is proved rather
    // than merely worked around.

    check(answers("    satellite.variable.binary bits = b1010\n"
                  "    satellite.return(bits.to_number())\n") == "10",
          "`to_number()` `1 6 5 1` is what the bits are WORTH");

    check(answers("    satellite.variable.binary bits = b1010\n"
                  "    satellite.return(bits.as_number())\n") == "1010",
          "`as_number()` `1 6 5 4` is the digits read as decimal -- the "
          "author set which verb is which on 2026-09-08 and these two "
          "fixtures are the record, being one word and 1000 apart");

    check(answers("    satellite.variable.binary bits = b00001010\n"
                  "    satellite.return(bits.width())\n") == "8",
          "`width()` `1 6 5 2` counts the bits WRITTEN, not the significant "
          "ones -- the one question only this type can answer");

    check(answers("    satellite.variable.binary bits = b1010\n"
                  "    satellite.return(bits.to_string())\n") == "b1010",
          "`to_string()` `1 6 5 3` matches what `display` prints, `b` and "
          "all, so the two spellings of one value cannot drift");

    check(answers("    satellite.variable.binary bits = b1010\n"
                  "    satellite.return(\"bits: \" + bits.to_string())\n") ==
              "bits: b1010",
          "and that is what the row is FOR: `+` joins two strings, so this "
          "sentence is unwritable without it");

    // --- both conversions lose the width, by two different routes -----------

    check(answers("    satellite.variable.binary a = b0011\n"
                  "    satellite.return(a.to_number())\n") == "3" &&
              answers("    satellite.variable.binary a = b11\n"
                      "    satellite.return(a.to_number())\n") == "3",
          "`to_number()` cannot keep the width -- a number has none to keep "
          "it in, so `b0011` and `b11` both answer 3");

    check(answers("    satellite.variable.binary a = b0011\n"
                  "    satellite.return(a.as_number())\n") == "11" &&
              answers("    satellite.variable.binary a = b11\n"
                      "    satellite.return(a.as_number())\n") == "11",
          "and neither can `as_number()`, a leading zero being no more a "
          "surviving decimal digit than a surviving bit -- so `width()` is "
          "the only row that answers for it");

    // --- the refusals -------------------------------------------------------

    check(holds(refused("    satellite.return(b1010.to_number())\n"),
                "a selector folds only through a declared name"),
          "A METHOD ON A LITERAL DOES NOT COMPILE, which is WORD_NUMBERS "
          "§1.5's one hop and is pinned here because a conversion verb is the "
          "shape people chain -- PLAN §8's M19.5 entry says so in advance and "
          "says no milestone owns loosening it");

    check(holds(refused("    satellite.variable.binary empty\n"
                        "    satellite.return(empty.width())\n"),
                "holds nothing"),
          "S0714: a declared name holds nothing until something is assigned, "
          "and a bit run is no exception -- DESIGN §6.4 qualification 3");

    // --- the two rows the second radix brought with it ---------------------

    check(answers("    satellite.variable.binary bits = b1010\n"
                  "    satellite.return(bits.digits())\n") == "4",
          "`digits()` `1 6 5 5` counts the digits written, which on this type "
          "is always `width()` -- it earns its row on hex, where the two "
          "answers differ, so that one question is askable of either radix");

    check(answers("    satellite.variable.binary bits = b1010\n"
                  "    satellite.return(bits.to_hex())\n") == "xA",
          "`to_hex()` `1 6 5 6` re-spells the same bits in the other radix, "
          "four bits to the digit");

    check(holds(refused("    satellite.variable.binary odd = b101\n"
                        "    satellite.return(odd.to_hex())\n"),
                "a multiple of 4"),
          "A WIDTH THAT IS NOT A MULTIPLE OF FOUR IS REFUSED RATHER THAN "
          "PADDED -- three bits are no hexadecimal digit, and padding would "
          "write a bit the program never wrote. `write(x)`'s multiple-of-8 "
          "refusal one module over is the same rule one step further out");

    // --- and the refusal this milestone's second half removed ---------------

    check(answers("    satellite.return(x00FF)\n") == "x00FF",
          "A HEXADECIMAL LITERAL IS A VALUE SINCE 2026-09-09 and no longer "
          "names an unbuilt half -- this fixture asserted the REFUSAL until "
          "M19.5 closed, and it is kept pointing the other way so the day the "
          "gap shut has a line that says so");

    check(answers("    satellite.return(b1111 == xF)\n") == "false",
          "A BIT RUN IS NEVER EQUAL TO A HEX RUN, however they are spelled -- "
          "DESIGN §8.5 promised this answer before hex existed and it falls "
          "out of the two being different arms of the variant, not out of any "
          "comparison written for it");
}

} // namespace eval_test
