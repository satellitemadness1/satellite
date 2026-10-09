# 057 -- When a number comes first and the text spells a number, a refused pair is described by the converted value: 50% + "5" is quoted as "50% + 5", and x1F + "2.5" is called "a hex and a float"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** number string conversion  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

When a number comes first and the text spells a number, a refused pair is described by the converted value, not the string that was written: 50% + "5" is quoted as "50% + 5", and x1F + "2.5" / 50% + "2.5" / b101 + "2.5" are called "a hex/percentage/binary and a float".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display(50% + "5")
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at prog.satl. The second case, display(x1F + "2.5"), is hex.satl in the same folder. The variants are v01..v12.satl and w01..w08.satl there.

## What satl does

```
S301: TYPES_DO_NOT_MEET
satl(run): 50% + 5: a percentage is taken off or added onto a number, so the
number goes first -- 5 + 50%

directory: prog.satl:5
syntax: satellite.console.display(50% + "5")
                                      /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

hex.satl, display(x1F + "2.5"):
S210: NOT_BUILT_YET
satl(run): + was given a hex and a float -- a float meeting a hex is not built
yet: the new types are mixed together later
syntax: satellite.console.display(x1F + "2.5")
                                      /\
this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

exit 27
```

## What it should do

The refusal should describe the program as written: a percentage and the string "5" (or a hex and the string "2.5"). It should say the text was read as the number 5 (or the float 2.5) under the number-first rule, and that this pair is refused. It should not say "+ was given a hex and a float" when + was given a hex and a string. It should not offer "5 + 50%" as a fix without saying that the program's own text, written first ("5" + 50%), joins instead and gives the string 550%.

The task counts a refusal that names the wrong thing or says something false as an error. "+ was given a hex and a float" is false: the operands are a hex and a string. The reason is in the source. satellite_object.cpp:450-458 reads the text as a number and then calls add(read, ...) with the converted object, so every later sentence describes that object. The help says a number meets text "by what the text says" (satellite.help/satellite.variable.number/help_text.txt:24-25; satellite.help/satellite.variable.string/help_text.txt:13-15), so the reader needs to be told the conversion happened. The equivalent forms also disagree. The suggested order, written with the program's own value ("5" + 50%, v02.satl), prints 550% with exit 0 and no warning. 50% + "abc" (v01) joins to 50%abc and 50% + " 5" (v12) joins to 50% 5, so whether this refusal appears at all depends on the text's content. Nothing in the message explains that.

## Variants

FAIL, same fault (the refusal describes the converted value):
- 50% + "0" is quoted as "50% + 0 ... -- 0 + 50%" (exit 27).
- A string variable s = "5" with a percent p = 50%: p + s is quoted as "50% + 5" (exit 27).
- 50% + "2.5": "+ was given a percentage and a float" (S210, exit 14).
- b101 + "2.5": "+ was given a binary and a float" (S210, exit 14).
- x1F + "-2.5": "a hex and a float" (S210, exit 14).
- 1/3 + "2": "the + of 1/3 and 2 is fraction arithmetic" (S210, exit 14). This is word for word the refusal of 1/3 + 2.

PASS (behaves as the rule and help say):
- x1F + "2" gives 33, b101 + "2" gives 7, 1.5 + "2" gives 3.5.
- 50% + "abc" gives 50%abc, 50% + " 5" gives 50% 5, x1F + "abc" gives x1Fabc, 1.5 + "1/3" gives 1.51/3.
- 10 + 50% gives 15. The literal 50% + 5 gives the documented refusal.
- 50% - "5" is refused correctly as "a percentage and a string" (only + converts).

Related, and not part of this finding: the suggested fix "5 + 50%" is itself refused with S402, because 7.5 is not whole. That applies to the literal case too, and the percent help documents it.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/satellite_object/satellite_object.cpp:457-458

In satelliteObject::add, the number-first branch (satellite_object.cpp:454-458) does `if (number_in_text(*other.as_string(), read)) return add(read, out, why);`. It calls add again with the converted object and passes back whatever `why` that call produces, without saying the right-hand side was text. The sentences come from the converted pair. For a percentage they come from percentage_operation, which builds `both = shown(left) + " " + sign + " " + shown(right)` and the "number goes first -- 5 + 50%" advice (object_percentage.cpp:110 and 119-121). For a hex they come from refuse(), which says "+ was given a hex and a float" (object_hexadecimal.cpp:66-73). For a fraction it is object_fraction.cpp's arithmetic refusal. None of these knows the value was once a string.

## Notes

- Why the skeptic kept it: Reproduced on BUILD 0099 exactly as reported. When a number-kind comes first and the text spells a number (the A1 / ERRORS2 #3 rule), satl turns the string into a number or float. If that pair is then refused, the refusal describes the converted value, not what was written. The program contains no 5 and no float. Yet the refusal quotes "50% + 5" and says "+ was given a hex and a float". The second sentence is false: + was given a hex and a string. The same thing happens when the string is in a variable (p + s is quoted as "50% + 5"), and with a binary, a percentage or a fraction first. Nothing in the refusal says the text was read as a number. The suggested rewrite, applied to what the program actually has ("5" + 50%, or s + p), silently gives the string 550%. None of these is in ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md or CONTAINERS.md. ERRORS2 #10 (n.power named power_of) is the same kind of fault, "the refusal should use what was written", but it is a different case. DESIGN.md section 5 and the string and number help only give the rule (4 + "2" is 6, 4 + "abc" joins). The percent help documents the wording only for the literal 50% + 5, so none of them rule on this refusal's wording. check.sh has no row for a percentage, hex, binary or fraction followed by numeric text.
- Nearest known entry: none. The nearest is ERRORS2.md #10 (n.power(2) refused as power_of: the refusal should use what was written), which is a different case.
- Merged: Single finding.
