# 018 -- A method on a bare value word (satellite.bool.true, satellite.console.width, satellite.library.main.arguments.<fact>) is refused at run time with S110 and a false "math sign spacing" hint, while the same value in brackets or in a variable takes the method

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** number string conversion  
**Kind:** valid-program-refused  
**Severity:** medium

## What happens

A method on a bare value word (satellite.bool.true/.false, satellite.console.width/.height, satellite.library.main.arguments.<fact or setting>) is refused at run time with S110 and a false "math sign spacing" hint, while the same value in brackets or in a variable takes the method.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.bool b = satellite.bool.true
    satellite.console.display(b.string())
    satellite.console.display((satellite.bool.true).string())
    satellite.console.display(satellite.bool.true.string())
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at prog.satl. The one-line form is v01.satl in the same folder: it contains only display(satellite.bool.true.string()) and gives the same refusal at line 5.

## What satl does

```
stdout:
true
true
stderr:
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(run): satellite.console.display was given something it could not read to
the end of -- check that every math sign has a space on both sides

directory: prog.satl:8
syntax: satellite.console.display(satellite.bool.true.string())
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

In a declaration (v04.satl: satellite.variable.string t = satellite.bool.true.string()):
[satellite] satl(run): t = ... could not be read to the end -- it stops at something with no meaning there yet (& | << >> !! are undecided), so the part before it is not the whole value (machine_code: 13 satl_line_not_understood)

exit 13
```

## What it should do

A third line printing "true", exit 0, the same as b.string() and (satellite.bool.true).string(). Likewise, satellite.console.width.string() should print 80 when there is no terminal, and satellite.library.main.arguments.cores.string() should print 12, as their bracketed forms do.

1) Two equivalent forms disagree. On the same run, (satellite.bool.true).string() prints true and satellite.bool.true.string() is refused. The same holds for (satellite.console.width).string() (80) against satellite.console.width.string() (refused), and for (satellite.library.main.arguments.cores).string() (12) or args.cores.string() (12) against satellite.library.main.arguments.cores.string() (refused). 2) satellite.help/satellite.variable.string/help_text.txt says "A conversion may be written with or without (), and methods chain left to right". 3) satellite.help/arguments/help_text.txt says the facts "are words of the language too, read by their full name with no brackets: satellite.library.main.arguments.cores". 4) satellite.help/satellite.console/help_text.txt says "satellite.console.width and .height, with no brackets, are how many characters fit across the terminal". Other value words take a method: satellite.infinity().string() prints (infinity), and a number literal with a method works. 5) Both refusal texts are false for these lines. There is no math sign to space, and there is no & | << >> !! operator. 6) The refusal also comes only after earlier lines have run (x07.satl printed "before" first), so the checker does not see it either.

## Variants

FAIL (S110, exit 13, same "math sign" hint): satellite.bool.true.string(); satellite.bool.true.string with no (); .str; .to_string(); satellite.bool.false.string(); satellite.console.width.string(); satellite.console.height.string; satellite.library.main.arguments.cores.string(); satellite.library.main.arguments.cores.bin; satellite.library.main.arguments.access.string() (a flag setting); satellite.library.main.arguments.username.upper() (a text fact). In a declaration it fails with the "& | << >> !! are undecided" sentence instead. Every one also fails after earlier lines have already printed.
PASS: display(satellite.bool.true) -> true; "x" + satellite.bool.true -> "x true"; satellite.bool.true == satellite.bool.true -> true; (satellite.bool.true).string() and (satellite.bool.true).string -> true; a bool variable's b.string() -> true; satellite.console.width -> 80, width + 1 -> 81; (satellite.console.width).string() -> 80; a number variable w set from width, w.string() + "!" -> 80!; (satellite.library.main.arguments.cores).string() -> 12; args.cores.string() + "!" -> 12!; args.cores.bin -> 1100; args.access.string() -> true; args.username.upper() -> MADNESS; satellite.infinity().string() -> (infinity).

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/expression.cpp:1231 (bool literal arm), with the same omission at expression.cpp:1289 (console facts), expression.cpp:1302 (machine facts) and expression.cpp:1273 (flag settings)

In evaluate's primary-word handling in expression.cpp, several arms return their Value directly and never call maybe_a_method: the bool literal arm (1231-1234: `if (code == word::code_of(1, 17, 1) || ...) { ++at; return Value::of_bool(...); }`), the console-fact arm (1289-1292: `return console_fact(code);`), the machine-fact arm (1302-1324: `return answer;` / `return Value::of_number(...)`), and the flag-setting arm (about 1273-1286: `return Value::of_bool(said.flag);`). Other arms do pass their value on: the number-literal arm (1228: `return maybe_a_method(row, at, Value::of_number(...), "that number", context)`), the library-value arm (1252-1257) and the word-call arm (1239-1246). Because these arms skip that step, the `.string()` method token is left unread. call_word then refuses with the generic "could not read to the end ... math sign" text at expression.cpp:1619-1628, and a declaration refuses with kNotReadToTheEnd at program_walk.cpp:78-80, which blames the undecided operators.

## Notes

- Why the skeptic kept it: I reproduced this on BUILD 0099. A method written straight after a bare value word is refused at run time with exit 13. A value word here is a bool literal, a console fact, a machine fact or an arguments setting. The same value works when it is in brackets or held in a variable. The refusal also gives false advice: it points to math-sign spacing, or to undecided operators, on lines that have neither. None of ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md or CONTAINERS.md mentions it. DESIGN.md and the help topics do not describe it as intended; the string help says conversions chain left to right. In the source, the evaluator arms for these words return the value without checking for a following method, which the number-literal and library-value arms do check for.
- Nearest known entry: none
- Merged: Single finding. Those arms of expression.cpp return without calling maybe_a_method.
