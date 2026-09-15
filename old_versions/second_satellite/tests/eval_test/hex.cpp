// M19.5's second half through the machine: `satellite.variable.hex` -- the
// literal becoming a value, the width and the digit count as two different
// questions, the six methods, and the two things that must NOT be true.
//
// THE THREE FIXTURES THAT ARE THE SECTION. Most of what is below would still
// pass under a representation that got the type wrong, so these are the ones
// worth knowing by name:
//
//   `x0009 != x9`        the width is part of the value -- DESIGN §8.5's whole
//                        claim, and the reason this is not a number in base 16
//   `xF != b1111`        the two radices never compare equal, which value.hpp
//                        delivers by giving them separate arms rather than one
//                        arm with a radix field somebody has to remember
//   `x00ff == x00FF`     case is NOT part of the value, which is the one thing
//                        hex has that binary does not and the reason `display`
//                        cannot print back what was written
//
// AND THE ABSENCE OF `as_number()` IS TESTED, because it is a decision and not
// a gap. Binary has the row and hex does not: it means "read these characters
// as an ordinary decimal", which works on `b1010` only because `0` and `1` are
// decimal digits too, and cannot work on `x00FF` at all. The row existed for
// part of 2026-09-09, answering 11111111 off the bit expansion, and the author
// dropped it on reading that answer.

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

// bits.cpp's refusal_in(), copied for bits.cpp's stated reason: eval_test.hpp
// is the suite's door and a helper promoted into it is one every section then
// reads past.
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

void section_hex()
{
    using namespace satellite;

    scalars::install_handlers();

    // --- the literal is a value ---------------------------------------------

    check(answers("    satellite.return(x00FF)\n") == "x00FF",
          "a hexadecimal literal COMPILES TO A CONSTANT and displays as "
          "written -- until 2026-09-09 this was a refusal naming the milestone");

    check(answers("    satellite.variable.hex colour = x00FF\n"
                  "    satellite.return(colour)\n") == "x00FF",
          "and it survives a declaration and a name, which is the half of the "
          "done-when a literal alone does not reach");

    // --- the width is part of the value, which is what the type is FOR ------

    check(answers("    satellite.return(x0009)\n") == "x0009",
          "LEADING ZEROS ARE NOT TRIMMED ANYWHERE -- display prints what was "
          "written, DESIGN §8.5, and the day something trims one is the day "
          "this type is a number in another base");

    check(answers("    satellite.return(x0009 == x9)\n") == "false",
          "AND TWO WIDTHS ARE TWO VALUES. This is the fixture the whole "
          "representation is built around: an integer with a length beside it "
          "passes almost everything else in this file and fails here");

    check(answers("    satellite.return(x00FF == x00FF)\n") == "true",
          "same digits at the same width are one value");

    // --- case is not part of the value, and this is hex's alone -------------

    check(answers("    satellite.return(x00ff == x00FF)\n") == "true",
          "BOTH CASES ARE ONE VALUE -- lexer_chars.hpp accepts either spelling "
          "\"because the width is what carries meaning and case does not\", "
          "and storing the bits is what makes that true rather than merely "
          "intended");

    check(answers("    satellite.return(x00ff)\n") == "x00FF",
          "SO DISPLAY CANNOT PRINT BACK WHAT WAS WRITTEN, and prints upper "
          "instead -- the one place hex differs from a bit run, whose digits "
          "ARE its value. DESIGN §8.5 spells every example it has this way");

    // --- the six methods ----------------------------------------------------

    check(answers("    satellite.variable.hex c = x00FF\n"
                  "    satellite.return(c.to_number())\n") == "255",
          "`to_number()` `1 6 11 1` is what the digits are WORTH");

    check(answers("    satellite.variable.hex c = x00FF\n"
                  "    satellite.return(c.width())\n") == "16",
          "`width()` `1 6 11 2` counts BITS and not digits -- the author's "
          "call, 2026-09-09, and what keeps `write(x)`'s multiple-of-eight "
          "rule reading the same on both radices");

    check(answers("    satellite.variable.hex c = x00FF\n"
                  "    satellite.return(c.digits())\n") == "4",
          "`digits()` `1 6 11 4` counts DIGITS -- the question `width()` "
          "stopped answering, and never a remainder, four bits being one digit");

    check(answers("    satellite.variable.hex c = x00FF\n"
                  "    satellite.return(c.to_string())\n") == "x00FF",
          "`to_string()` `1 6 11 3` matches the display exactly, `x` and all, "
          "which is what makes the printed case predictable at all");

    // AND THERE IS NO `as_number()`, WHICH IS A CLAIM AND NOT AN OMISSION.
    // Binary has one at `1 6 5 4`; hex had one for a few hours on 2026-09-09
    // and the author dropped it on sight of its answer. `as_number` means
    // "read these characters as an ordinary decimal", which binary can answer
    // because `0` and `1` ARE decimal digits -- and hex cannot, there being no
    // decimal spelled `00FF`. The built version read the BIT EXPANSION's
    // characters instead and answered 11111111, which no program ever wrote.
    check(holds(refused("    satellite.variable.hex c = x00FF\n"
                        "    satellite.return(c.as_number())\n"),
                "is not a question"),
          "S0723: `as_number` IS NOT A QUESTION A HEX RUN ANSWERS -- hex has "
          "no such row, and this fixture is here because the row existed for "
          "part of one day and a reader who found binary's `1 6 5 4` would "
          "otherwise expect hex's");

    check(holds(refused("    satellite.variable.hex c = x00FF\n"
                        "    satellite.return(c.as_number())\n"),
                "`A` to `F` are not decimal digits"),
          "AND THE REFUSAL SAYS WHY, which the author asked for by name: this "
          "is the one S0723 advice chosen by the SELECTOR and not only by the "
          "type, because a person reaching for `as_number` on a hex has almost "
          "certainly just read binary's and the generic sentence answers a "
          "question they did not ask");

    check(holds(refused("    satellite.variable.hex c = x00FF\n"
                        "    satellite.return(c.upper())\n"),
                "the words numbered under that path"),
          "and every OTHER wrong selector on a hex still gets the general "
          "sentence -- the special row is one row, not a type-wide rewrite");

    check(answers("    satellite.variable.hex c = x00FF\n"
                  "    satellite.return(c.to_binary())\n")
              == "b0000000011111111",
          "`to_binary()` `1 6 11 5` re-spells the same run in the other radix, "
          "and NEVER refuses -- every hexadecimal digit is exactly four bits, "
          "which is the asymmetry with `binary.to_hex()` `1 6 5 6`");

    // --- neither conversion keeps the width ---------------------------------

    check(answers("    satellite.variable.hex c = x0009\n"
                  "    satellite.return(c.to_number())\n") == "9",
          "`to_number()` DROPS THE WIDTH and cannot do otherwise -- a number "
          "has none (DESIGN §8.1), so `x0009` and `x9` both answer 9 and "
          "`width()` is what a program asks first");

    // --- the two radices are never equal ------------------------------------

    check(answers("    satellite.return(xF == b1111)\n") == "false",
          "A HEX RUN IS NEVER EQUAL TO A BIT RUN. DESIGN §8.5 named this pair "
          "before hex was built; it is delivered by the two being separate "
          "arms of the variant, so nothing had to remember to check a radix");

    check(answers("    satellite.variable.hex c = x00FF\n"
                  "    satellite.return(c == c.to_binary())\n") == "false",
          "AND CONVERTING DOES NOT MAKE THEM EQUAL EITHER, which is the same "
          "rule where it is most surprising: the two hold identical bits and "
          "are still two values, because they are two types");

    // --- the alias is the same node -----------------------------------------

    check(answers("    satellite.variable.hexadecimal c = xAB\n"
                  "    satellite.return(c)\n") == "xAB",
          "`hexadecimal` is the language's ONE alias (WORD_NUMBERS §2.3) and "
          "declares the same type -- words.def's alias row, reaching the "
          "evaluator without a second node");

    // --- the refusals -------------------------------------------------------

    check(holds(refused("    satellite.variable.hex empty\n"
                        "    satellite.return(empty.width())\n"),
                "holds nothing"),
          "S0714: a declared name holds nothing until something is assigned, "
          "and a hex run is no exception -- DESIGN §6.4 qualification 3");

    check(holds(refused("    satellite.return(x00FF.to_number())\n"),
                "a selector folds only through a declared name"),
          "A METHOD ON A LITERAL DOES NOT COMPILE, WORD_NUMBERS §1.5's one "
          "hop -- pinned on this type too because a conversion verb is the "
          "shape people chain and this type has three of them");
}

} // namespace eval_test
