# 054 -- .bin/.hex on a declared float name pass the checker and are refused with S210 only after earlier lines ran, while f.add and a fraction's .bin/.hex are refused before anything runs

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** number string conversion  
**Kind:** inconsistency  
**Severity:** low

## What happens

.bin/.hex on a declared float name get past the checker and are refused with S210 only after earlier lines ran, while f.add and fraction q.bin/q.hex are refused before anything runs.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.float f = 2.5
    satellite.console.display("before")
    satellite.console.display(f.bin)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: None. Saved at prog.satl. Comparison program: add.satl in the same folder, which is the same program with f.add(1) in place of f.bin.

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S210: NOT_BUILT_YET
satl(run): f.binary: base 2 text of a float (2.5) is not built yet -- a float
meeting a binary is one of the new types mixed together later

directory: .../verify/number-string-conversion-3/prog.satl:7
syntax: satellite.console.display(f.bin)
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

(The same program with f.add(1), add.satl, prints nothing: "S210: NOT_BUILT_YET / satl(check): in satellite.main, f.add is not built for satellite.variable.float yet -- a float has .string and .number so far" [exit 14])

exit 14
```

## What it should do

The checker should refuse f.bin and f.hex by name before anything runs, with S210 and exit 14, as it does for f.add(1) on the same float and for q.bin / q.hex on a declared fraction. "before" should not be printed.

1) The task says an S210 that comes only after earlier lines ran, while the checker refuses others before anything runs, is an error. 2) The fraction sibling does it: satellite.variable.fraction q = 1/2 then display(q.bin) or display(q.hex) is "satl(check): ... q.binary is not built for satellite.variable.fraction yet", and nothing prints. fraction_values.cpp:142-144 says ".binary, .hex and every other method are refused by name before a line runs". 3) The checker's own refusal for a float says "a float has .string and .number so far", yet .bin and .hex get past that same check. 4) The refusal does not depend on the value: float_to_binary and float_to_hexadecimal (satellite/satellite_object/object_float.cpp:254-266) return not_built_yet for every float, including 3.0, so nothing is learned at run time. 5) The float help (satellite.help/satellite.variable.float/help_text.txt) says a float meeting a binary or hex "is not built yet". It says nothing about the refusal coming only at run time, and DESIGN.md has no such ruling.

## Variants

These also fail late, printing "before" and then an S210 satl(run) refusal with exit 14: f.hex (v_hex.satl); satellite.variable.double f = 2.5 then f.bin (v_double_bin.satl); float f = 3 (holds 3.0) then f.bin (v_float_whole_bin.satl); a float parameter g.bin inside a capsule called from main (v_capsule.satl); satellite.variable.string s = f.hex (v_assign.satl); the literal 2.5.bin (v_lit_bin.satl).
These are refused at check time with nothing printed: f.add(1) on a float (add.satl); g.add(1) on a float parameter inside a capsule (v_capsule_add.satl); q.bin and q.hex on a declared fraction (v_frac_bin.satl, v_frac_hex.satl).
These work: f.string on 2.5 gives 2.5; f.number on 3.0 gives 3.
Side note, not part of this finding: the refusal says "f.binary" where the program wrote "f.bin". That is the known "refused under the registry's name, not what was written" entry (ERRORS2 #10, n.power -> power_of).

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/float_values.cpp:82-89 (float_method_check), with the refusal at /home/madness/code/cxx/satellite/satellite/satellite_object/object_float.cpp:254-266

program_check.cpp:550-555, method_right_for, sends a declared float name to float_method_check, and the comment at float_values.cpp:78 calls that answer "final for a float name". float_method_check returns success for to_binary_token and to_hexadecimal_token along with .string and .number. Its comment at float_values.cpp:79-81 says: ".binary and .hex pass the checker as every type's conversions do, and are refused where they are reached -- they wait for the finale". The walker then calls float_to_binary / float_to_hexadecimal (object_float.cpp:254-266), which return not_built_yet whatever the value is, so the refusal comes only at run time. fraction_method_check (fraction_values.cpp:145-151) leaves .binary and .hex out of its list and refuses them at check time. Leaving to_binary_token and to_hexadecimal_token out of float_method_check's success list would make the float behave the same way.

## Notes

- Why the skeptic kept it: I reproduced it exactly. The float name's type is declared, and float_to_binary / float_to_hexadecimal refuse every value, including 3.0. So the checker could refuse f.bin and f.hex before anything runs, as it already does for f.add on a float and for q.bin / q.hex on a declared fraction. Instead, "before" prints first. The task counts this as an error: "an S210 that comes only after earlier lines already ran when the checker refuses others before anything runs". It also contradicts the checker's own sentence, "a float has .string and .number so far", because .bin and .hex get past that same check. Nothing in ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md or CONTAINERS.md covers it. ERRORS2 #8 (a wrong-type literal refused only at run time) and B4 (string methods) are different things. DESIGN.md and the float help topic say nothing about when this refusal should come. The help says only that "a float meeting ... a hex, a binary ... is not built yet", so the refusal itself is correct and only its timing is wrong. The one reason given for letting it through is a source comment ("they wait for the finale"), not an author ruling. check.sh does not test it.
- Nearest known entry: none
- Merged: Single finding. float_method_check passes to_binary and to_hexadecimal.
