# 037 -- A name spelled like a hex or binary literal (x1, xa, xff, b0, b1, b10) cannot be declared or used as a parameter, and the refusal either quotes a naming rule the name meets ("not a name -- a name is made of a-z, A-Z, 0-9 and _") or says there is no name

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** control flow and capsules, integers, realistic programs, syntax and layout  
**Kind:** misleading-refusal  
**Severity:** medium

## What happens

Names shaped like hex/binary literals (x1, b1, xff) are refused with a false sentence: 'not a name -- a name is made of a-z, A-Z, 0-9 and _'.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number x1 = 5
    satellite.console.display(x1 + 1)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.variable.number is followed by
something that is not a name -- a name is made of a-z, A-Z, 0-9 and _, and does
not start with a digit

directory: nm_x1.satl:5
syntax: satellite.variable.number x1 = 5
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

A refusal that names the real reason, for example 'x1 is written like a hex (x then hex digits), so it cannot be a name -- choose another', or accepting x1 as a name.

x1 meets the name rule the refusal itself states, so the sentence is false. The real cause (x + hex digits lexes as satellite.variable.hex, b + binary digits as binary) is never mentioned, and x1/y1/b1 are common names.

## Variants

b1 and xff give the same sentence. y1, bad and xy work (prints 6). A for counter x1 gives 'satellite.statement.for begins with satellite.variable.number <name> = <value>'. A string named ff00aa works.

## Where it comes from

satellite/bytecode/program_check.cpp:2066-2070 (the 'is followed by something that is not a name -- ' + kNameRule refusal; kNameRule at line 73). program_check.cpp:1851 for the for form.

The lexer reads x followed by hex digits as a hex literal and b followed by binary digits as a binary literal, so the declared name never lexes as a name token. The checker's generic 'type with no name after it' branch then gives kNameRule, which is written for names like 9lives or !@#$, without checking whether the rejected token is a hex or binary literal.

## Notes

- Why the skeptic kept it: Reproduced. x1, b1 and xff are refused as names with the sentence 'is followed by something that is not a name -- a name is made of a-z, A-Z, 0-9 and _, and does not start with a digit'. x1 obeys every clause of that rule, so the sentence is false about what was written. y1, bad and xy work. That x1F is a hex literal is documented (satellite.variable.hex help), so refusing x1 as a name is a consequence of the design. But no help or ruling says such names are reserved, and the refusal never gives the real reason. The for-counter form says the for 'begins with satellite.variable.number <name> = <value>', which is what was written. No known list has it.
- Nearest known entry: none
- Merged: The same finding reported four times: the lexer makes x1 a hex literal, and the declaration and parameter checks give kNameRule without noticing the literal (program_check.cpp:2066-2070).

### Also seen as: A name spelled like a hex or binary literal (x1, xa, xface, b0, b1, b10) is refused with a name rule it satisfies, or as a parameter with 'there is no name after it'

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number x1 = 4
    satellite.console.display(x1)
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.variable.number is followed by
something that is not a name -- a name is made of a-z, A-Z, 0-9 and _, and does
not start with a digit

directory: n_x1.satl:5
syntax: satellite.variable.number x1 = 4
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

### Also seen as: A name that looks like a hex or binary literal (x1, xa, x0, xface, b0, b1, b01) cannot be declared, and the refusal quotes a naming rule the name meets

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number x1 = 3
    satellite.console.display(x1)
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.variable.number is followed by
something that is not a name -- a name is made of a-z, A-Z, 0-9 and _, and does
not start with a digit

directory: x1.satl:5
syntax: satellite.variable.number x1 = 3
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

### Also seen as: A name that spells a hex or binary literal (x1, x2, y... no: x1, b1, xa, xface, b10) is refused with the name rule it meets: 'not a name -- a name is made of a-z, A-Z, 0-9 and _, and does not start with a digit'

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number x1 = 3
    satellite.console.display(x1 + 1)
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.variable.number is followed by
something that is not a name -- a name is made of a-z, A-Z, 0-9 and _, and does
not start with a digit

directory: x1.satl:5
syntax: satellite.variable.number x1 = 3
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```
