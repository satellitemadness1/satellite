# 055 -- .reverse() on a float or a fraction is refused as "not built yet" but filed as S301 TYPES_DO_NOT_MEET (exit 27) rather than S210 (exit 14), and only when its line runs

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** number string conversion  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

.reverse() on a float or a fraction is refused as "not built yet" but filed as S301 TYPES_DO_NOT_MEET (exit 27), not S210 NOT_BUILT_YET (exit 14), and only when its line runs.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.float f = 2.5
    satellite.console.display("before")
    satellite.console.display(f.reverse())
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at prog.satl. Comparison programs in the same folder: frac.satl (q.reverse()), lit.satl (12.34.reverse()), qlit.satl (1/3.reverse()), decl.satl, cap.satl, fbin.satl (f.binary), fadd.satl (f.add(1)), qnum.satl (q.number()), pct.satl.

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): f.reverse() was written on a float, and a float's .reverse() is not
built yet (SATELLITE_INFINITY.md FLT-4)

directory: prog.satl:7
syntax: satellite.console.display(f.reverse())
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

S210 NOT_BUILT_YET, machine code 14 not_built_yet, exit 14, with the S210 paragraph ("this word is numbered in the language and the library behind it does not exist yet"). f.reverse() on a declared float name should also be refused before anything runs, the way the checker already refuses f.add(1) and q.number() on the same declared names. The same applies to q.reverse() on a fraction.

The refusal's own sentence says the word is "not built yet" and names the milestone that owes it (FLT-4). SATELLITE_ERROR.md E11 says "S04xx: `not_built_yet` names the word and says which milestone owes it". The S301 paragraph printed here says "this operator has no scenario for the two kinds it was given", which is false. There is no operator and no second kind, and FLT-4 decides the answer: 12.34.reverse() is 34.12. The equivalent form that disagrees is fbin.satl. On the same float name, f.binary is also a run-time "not built yet" refusal, and it prints "S210: NOT_BUILT_YET ... f.binary: base 2 text of a float (2.5) is not built yet" and exits 14. fadd.satl and qnum.satl refuse f.add(1) and q.number() with satl(check) S210 and exit 14, before "before" prints. The exit code matters to a script: 27 and 14 are different documented machine codes.

## Variants

These also fail the same way (S301, exit 27, "not built yet", after "before" printed). frac.satl: q.reverse() on a declared fraction 1/3 says "a fraction's .reverse() is not built yet -- a fraction has .numerator, .denominator and .string so far". lit.satl: the literal 12.34.reverse(). qlit.satl: the literal 1/3.reverse(). decl.satl: satellite.variable.float g = f.reverse(), where the caret points at the end of the line. cap.satl: f.reverse() inside a capsule on a float parameter. These pass or are consistent: fbin.satl, f.binary on a float, gives S210 with exit 14 at run time. fadd.satl, f.add(1), and qnum.satl, q.number(), give S210 with exit 14 from the checker before anything runs. Not part of this finding: pct.satl, p.reverse() on a percentage, gives S301 "that has no order to reverse", exit 27. That is a genuine type refusal, and satellite.help/satellite.variable.percent/help_text.txt says .reverse() is refused. The infinity .reverse() refusal as S301 with exit 27 is intended: check.sh line 264 expects "27|1|1", with the sentence "an infinity has no digits to turn round".

## Where it comes from

satellite/bytecode/expression.cpp:458

In the .reverse() branch of satellite/bytecode/expression.cpp (lines 451-463), every case that reverse_of() does not handle goes through context.refuse(types_do_not_meet, ...), whatever the reason. satellite/bytecode/container_calls.cpp:305 (float) and :314 (fraction) set the reason to "not built yet", so a not-built word is filed as S301 and exits 27. The run-time timing comes from satellite/bytecode/program_check.cpp:590: method_on_a_name returns success for .reverse on every declared type ("allowed on anything here and refused at the value"). So the checker never sees that a float's or a fraction's .reverse() is unbuilt, although it refuses f.add and q.number on the same names with not_built_yet.

## Notes

- Why the skeptic kept it: Reproduced exactly. The refusal for f.reverse() on a float and q.reverse() on a fraction says "is not built yet", and for the float it names the milestone that owes it (SATELLITE_INFINITY.md FLT-4). Even so, it is filed as S301 TYPES_DO_NOT_MEET with exit 27 instead of S210 NOT_BUILT_YET with exit 14. S301's own paragraph ("this operator has no scenario for the two kinds it was given") is false for a method call on a type that FLT-4 says will have it. The clearest equivalent form that disagrees is f.binary: it is on the same float name, also refused at run time, also "not built yet", and it is filed as S210 with exit 14. f.add(1) and q.number() on the same declared names are S210 with exit 14, and the checker refuses them before anything runs. f.reverse() is refused only after "before" has already printed. SATELLITE_ERROR.md E11 says "S04xx: `not_built_yet` names the word and says which milestone owes it", and that is the code this message calls for. None of the known lists mentions float or fraction .reverse(). DESIGN.md does not say this is intended, and neither does any help topic. The nearest known entry is ERRORS2.md Part 1b item E. It notes that S301's explanation paragraph ("this operator has no scenario for the two kinds") reads wrong under a list .append or a [ ] = write. That covers only the paragraph wording, not the wrong S-code and exit code for a not-built word, so this finding is new. Two corrections to the hunter's report. f.binary is not refused "before anything runs": it is refused at run time, with S210. And the S210 sentence in the infinity help is about infinity arithmetic, not about .reverse(). Neither changes the verdict.
- Nearest known entry: none. The only partial overlap is ERRORS2.md Part 1b item E: "S301's paragraph says 'this operator has no scenario for the two kinds it was given' under a list .append or a [ ] = write". That entry covers the paragraph wording only, not using S301/27 for a word that is not built yet.
- Merged: Single finding. The note on reverse-arguments-ignored says why it is kept separate.
