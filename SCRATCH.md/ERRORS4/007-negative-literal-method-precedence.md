# 007 -- A method on a negative literal runs before the touching minus: -5.add(3) is -(5.add(3)) = -8, not -2, and -5.string is refused as "a minus sign was put in front of a string"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** number string conversion  
**Kind:** wrong-answer  
**Severity:** medium

## What happens

A method on a negative literal runs before the touching minus: -5.add(3) is read as -(5.add(3)) = -8, not -2, and -5.string is refused as "a minus sign was put in front of a string".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = -5
    satellite.console.display(n.add(3))
    satellite.console.display(-5.add(3))
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at min.satl. The hunter's original 3-line program was re-created as prog.satl in the same folder and gives -2 / -2 / -8, exit 0.

## What satl does

```
-2
-8

exit 0
```

## What it should do

-2 on both lines (python3: -5 + 3 = -2). Satl should read -5.add(3) as (-5).add(3), the same as n.add(3) for n = -5 and the same as -5 + 3.

satellite.help/satellite.variable.number/help_text.txt:12 says "You write it as digits, with a touching - in front for below zero: 34587, -7", so -5 is how you write the number minus five. The same file says ".add(x) is +". The same value in a variable (n = -5, n.add(3)) prints -2, and -5 + 3 prints -2. The literal and the variable disagree, and satl prints nothing about it. The sign is also treated as part of the literal everywhere else: the documented rule in expression.cpp is that -2 ^ 2 is (-2) ^ 2 = 4, and -5 ^ 2 prints 25. For .string the help says it gives "its base 10 text", which for -5 is "-5" (n.string with n = -5 prints -5). Yet -5.string is refused with "a minus sign was put in front of a string", which is false: the minus was written in front of the number 5.

## Variants

All in number-string-conversion-0.

Wrong answers, exit 0:
- -5.add(10) gives -15 (python: 5).
- -5.add(-3) gives -2 (python: -8).
- -b1010.add(1) gives -11 (python: -9).
- satellite.variable.number m = -5.add(3) holds -8, so the error is not limited to display.
- display(-5 + 3 == -5.add(3)) prints false.

Refused with S301 "a minus sign was put in front of a string", exit 27, at run time only:
- -5.string, -5.binary, -12.5.string and -x1F.string.
- -5.to_string in a declaration (message prefixed "in s = ...").
- In w1.satl an earlier display("before") line had already printed before the refusal.

Refused as float.add (S210): -1.5.add(1), because the float 1.5 has no .add, so the sign never reaches it.

Correct or harmless:
- (-5).add(3) gives -2.
- -5 + 3 gives -2.
- n.add(3), n.string, n.binary and n.hex for n = -5 give -2, -5, -101 and -5.
- -12.reverse() gives -21, which is right only because reverse keeps the sign either way.
- -n.add(3) with n = 5 gives -8. That is the ordinary reading for a minus in front of a named value followed by a call.
- -5 ^ 2 gives 25 and -2 ^ 3 gives -8, as documented.

## Where it comes from

satellite/bytecode/expression.cpp:1021-1024 (with 1228 and 1051)

In one_operand (expression.cpp:1007), a tight_minus_token (expression.cpp:1021) moves past the sign and calls one_operand recursively for its operand (expression.cpp:1024). For a literal, that recursive call reads the digits and then attaches the whole method chain before it returns. The number branch does this with maybe_a_method(row, at, Value::of_number(...), "that number", ...) at expression.cpp:1228; floats and hex do it at 1220, binary at 1173. So the method runs on the unsigned literal 5, and the sign is applied to the method's result afterwards (return Value::of_number(-*inner.as_number()) at 1051). When the method returns a string, the minus finds a string and refuses it with "a minus sign was put in front of " + kind_name (expression.cpp:1045-1049). The comment above this code says unary minus "takes another operand and not an expression" so that -2 ^ 2 is (-2) ^ 2. It never considers the postfix method chain that the operand already carries.

## Notes

- Why the skeptic kept it: Reproduced exactly: -5.add(3) prints -8, while n.add(3) with n = -5 and -5 + 3 both print -2, and it exits 0 with no warning. The help says a number below zero is written as "digits, with a touching - in front for below zero: 34587, -7", and that ".add(x) is +". So -5 is the value -5, and -5.add(3) should be -2. Satl reads it as -(5.add(3)). This is the literal-vs-variable disagreement the task lists as an error. Nothing in ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md, CONTAINERS.md, DESIGN.md or the help covers it, and no test or check.sh line calls a method on a negative literal. The one recorded precedence ruling points the other way: the source comment in expression.cpp says a touching minus binds tighter than every binary operator, so -2 ^ 2 is (-2) ^ 2 = 4 (confirmed: -5 ^ 2 prints 25). So the sign counts as part of the literal against ^ but not against a method. Other symptoms from the same cause: -5.add(10) gives -15 (python: 5). -5.add(-3) gives -2 (python: -8). -b1010.add(1) gives -11 (python: -9). -5 + 3 == -5.add(3) prints false. -5.string, -5.binary, -5.to_string, -12.5.string and -x1F.string are all refused with the false message "a minus sign was put in front of a string", although the minus was put in front of a number (n.string for n = -5 gives "-5"). They are refused only at run time, after earlier lines have already printed.
- Nearest known entry: none
- Merged: Single finding.
