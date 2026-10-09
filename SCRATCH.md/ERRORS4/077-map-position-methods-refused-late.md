# 077 -- On a declared map, .append/.insert/.remove_at/.truncate/.index_of/.search pass the checker and are refused only after earlier lines ran, while .sum/.max/.min/.join/.reserve are refused before anything runs

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting  
**Kind:** inconsistency  
**Severity:** low

## What happens

On a declared map, .append/.insert/.remove_at/.truncate/.index_of/.search pass the checker and are refused only after earlier lines ran, while .sum/.max/.min/.join/.reserve are refused before anything runs.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.map<satellite.variable.string, satellite.variable.number> age = {"zoe": 30}
    satellite.console.display("this line ran")
    age.append(4)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
this line ran
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): in age, age.append -- an index has keys and not positions: write
age[key] = value

directory: p5a.satl:7
syntax: age.append(4)
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

Refused by satl(check) before 'this line ran' prints, as display(age.sum) in the same position is (satl(check): in satellite.main, age.sum -- an index holds keys and values, so say which ..., nothing printed).

satellite.container.map help: '.append .insert .remove_at .truncate .index_of and .search are refused, because a map has keys and not positions' -- the declared kind decides it, and the checker already refuses the sibling index-only refusals (.sum/.max/.join) before the run. The task brief counts a refusal that comes after earlier lines ran, where the checker refuses others before anything runs, as an error.

## Variants

Late (prints 'this line ran', then satl(run), exit 27): age.insert(1, 4), age.remove_at(1), age.truncate(1), display(age.index_of(30)), display(age.search("z")) -- each 'an index has keys and not positions ... Take its keys first: age.keys.<method>(...)'. Early (satl(check), nothing printed, exit 27): display(age.sum), display(age.max), display(age.join(", ")). Also late: display(age.string()), refused as 'age.to_string -- a container has no to_string' (that part is lists#5). The run-time sentence 'an index has keys' also says index for a name declared map.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:648-655 (only index_refuses is consulted); /home/madness/code/cxx/satellite/satellite/bytecode/container_calls.cpp:146 (a_position_method) and :370 (run-time refusal)

method_on_a_name in program_check.cpp, for a name declared an index/map, checks arity and then calls index_refuses(method, name), which covers only .sum/.max/.min/.join (asks_what_it_holds) and .reserve. It never calls a_position_method, so .append/.insert/.remove_at/.truncate/.index_of/.search return success and are refused only by call_container_method at run time (container_calls.cpp:370).

## Notes

- Why the skeptic kept it: Reproduced. On a name declared map<string, number>, age.sum/.max/.join are refused by satl(check) before anything runs, but age.append(4), age.insert(1, 4), age.remove_at(1), age.truncate(1), age.index_of(30) and age.search("z") all print 'this line ran' first and are refused by satl(run). The map help lists '.append .insert .remove_at .truncate .index_of and .search are refused, because a map has keys and not positions', so the declared kind settles it before the run. The map position-method part is not in any known list and not in lists#5 (which covers .add/.find/.to_string/.to_number, list .keys/.values and argument counts; its only index cases are m.add and m.reverse(1)). The finding's other parts (list.keys/.values, container .string) ARE lists#5 and are not claimed here. Aside noticed: age.string() is refused naming 'age.to_string', a word not written.
- Nearest known entry: Related to but not covered by already-confirmed lists#5 (same checker gap class; lists#5 covers .add/.find/.to_string/.to_number, list .keys/.values and arg counts, not the map position methods).
- Merged: Single finding. index_refuses never consults a_position_method, which is a different missing check from string-number-methods-skip-receiver-check.
