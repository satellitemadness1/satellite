# 056 -- A string + an infinity is refused with the generic "no scenario for that pair" and no .string hint, although infinity.string() joins and a colour gets the hint

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** number string conversion  
**Kind:** inconsistency  
**Severity:** low

## What happens

A string + an infinity (either order, or .add) is refused with the generic "there is no scenario for that pair" and no .string hint, although infinity.string() joins. The colour gets the hint, and ERRORS2 #2 says both do.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.infinity big = satellite.infinity()
    satellite.console.display("big is " + big.string())
    satellite.console.display("big is " + big)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at prog.satl. The colour program to compare with is color.satl in the same folder.

## What satl does

```
big is (infinity)
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): + was given a string and an infinity, and there is no scenario for
that pair

directory: .../verify/number-string-conversion-8/prog.satl:7
syntax: satellite.console.display("big is " + big)
                                            /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

For comparison, color.satl ("c is " + c with c = x00FF00):
satl(run): + was given a string and a color -- a color is not text; .string on
it is its text, to join

exit 27
```

## What it should do

The refusal is still S301 / exit 27, since joining an infinity is unbuilt and was never asked for. The sentence should say what the colour's says, and what ERRORS2 #2 tells the author it says: an infinity is not text, and .string on it is its text, to join. It should not say there is no scenario.

1) SCRATCH.md/ERRORS2.md #2: "A colour or an infinity added to a string is still refused, with \"write .string\"". satl does this for the colour only. 2) satellite.help/satellite.variable.infinity/help_text.txt: ".string is its display", and line 6 of the program shows big.string() joins ("big is (infinity)"), so a scenario exists. 3) The interpreter's own principle in satellite_object.cpp (the comment above refuse_conversion, line ~86): "'no scenario' would be untrue -- the scenario exists and is a conversion the program has to write out loud." 4) Every other non-string kind that has a .string either joins (number, float, percentage, hex, binary, bool) or gets the .string hint (colour); only the infinity gets the generic sentence.

## Variants

Fail the same way ("no scenario for that pair", no hint): big + " inf" ("+ was given an infinity and a string"); "x" + satellite.infinity() (a literal, not a variable); "big is " + -satellite.infinity(); "big is ".add(big). Pass: "big is " + big.string() and "big is " + big.string (both print "big is (infinity)"). The colour gets the hint in every order and form: "c is " + c, c + " is c", "c is ".add(c). "c is " + c.string() prints "c is x00FF00". These join correctly: "p is " + 50% gives "p is 50%", "f is " + 2.5 gives "f is 2.5", "h is " + x1F gives "h is x1F", "b is " + b1010 gives "b is b1010", "t is " + true gives "t is true". "xs is " + list also says "no scenario", which is fair, since a list has no .string (xs.string() is refused as "a container has no to_string"). Both the infinity and the colour are refused at run time (satl(run)), after earlier lines ran, so the two agree on timing.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/satellite_object/satellite_object.cpp:518 (default arm of add() calls refuse_pair, whose sentence is at :79-81); compare /home/madness/code/cxx/satellite/satellite/satellite_object/object_color.cpp:66-67

In satelliteObject::add(), the string + number-kind join (line ~405) and the bool join (~430) do not cover an infinity. refuse_infinity_arithmetic (~174-184) deliberately skips pairs that involve a string: its comment says "`\"a\" + inf` ... is refused as that pair, below". answered_by_its_own_file sends only fraction, colour and hex to their own files. The colour's file has a string-join sentence (object_color.cpp:66-67), but nothing like it exists for the infinity. So string + infinity reaches the switch's default arm and refuse_pair (line 518), which writes "and there is no scenario for that pair".

## Notes

- Why the skeptic kept it: I reproduced it exactly. A string + an infinity, in either order and also as "text".add(inf), reaches the generic default arm and says "there is no scenario for that pair". That sentence is false by the source's own reasoning: the comment on refuse_conversion says "no scenario" is untrue when a written-out conversion exists, and here infinity.string() is that conversion (line 6 of the program joins with it). The kind that sits next to it, the colour, gets "a color is not text; .string on it is its text, to join", and SCRATCH.md/ERRORS2.md #2 tells the author that both the colour and the infinity are refused "with 'write .string'". The refusal itself is intended; the missing hint is not. This is not already known: ERRORS2 #2 describes the hint as present and does not list its absence as an open error, and no other list mentions it. It is not by-design either. SATELLITE_INFINITY.md:1447 ("`"a" + inf` is still the string's own refusal — no milestone builds that pair") and the comment at satellite_object.cpp:170-172 are notes from whoever built it, not a ruling by the author. They only say the pair is refused, and the later ERRORS2 record to the author contradicts the wording. No help topic covers "a" + inf, and check.sh does not pin the sentence.
- Nearest known entry: none. SCRATCH.md/ERRORS2.md #2 mentions the pair, but it records the infinity as refused "with 'write .string'", which is not what satl does. It does not list the missing hint as an open error.
- Merged: Single finding.
