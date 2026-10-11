# 017 -- Conversions made when a whole name is assigned (binary and hex to number, whole number to float) are skipped for a capsule parameter, a container item and a for counter: a number parameter or list slot keeps b0011, a list slot refuses a hex, list<float> refuses 3, and a for counter refuses k = b0001

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** binary hex color infinity bool, control flow and capsules, lists  
**Kind:** valid-program-refused  
**Severity:** medium

## What happens

A number slot in a container (list/map/multiple) keeps a binary unconverted and refuses a hex, while a number name converts both to their worth.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = b101
    satellite.variable.number h = x1F
    satellite.console.display(n)
    satellite.console.display(h)
    satellite.container.list<satellite.variable.number> t = {1}
    t.append(b101)
    satellite.console.display(t)
    satellite.console.display(t.contains(5))
    t.append(x1F)
    satellite.console.display(t)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at min.satl

## What satl does

```
5
31
{1, b101}
false
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): in t, t.append: it holds a hex

directory: min.satl:13
syntax: t.append(x1F)
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

The list should hold each value the way a number name holds it. b101 goes in as 5, so t shows {1, 5} and t.contains(5) is true. x1F goes in as 31, so the last display is {1, 5, 31} and the program exits 0. python3 agrees: 0b101 == 5 and 0x1F == 31.

satellite.help/satellite.variable.binary/help_text.txt says "b1010 == 10 is true, and a number name given b0011 holds 3". satellite.help/satellite.variable.hex/help_text.txt says "A hex given to a number name keeps its worth". satellite.help/satellite.container.list/help_text.txt says list<type> "checks every item against that type on every way in", which is the same type a number name has. The two equivalent forms in the same run disagree: `number n = b101` shows 5 but the list shows b101, and `number h = x1F` holds 31 but the list refuses x1F. The list's own .contains(5) says false while `t[2] == 5` says true.

## Variants

These fail the same way (binary kept unconverted, hex refused):
- `list<number> t = {b101}` shows {b101}.
- `list<number> t = {x1F}` is refused as the declaration runs, in one line: "t was declared satellite.container.list, and item 1 of it does not fit: it holds a hex (machine_code: 27)".
- `t[1] = b101` gives {b101}. `t[1] = x1F` is refused with S301 "in t[...] = ..., t[1] does not fit: it holds a hex".
- `t.insert(1, b101)` gives {b101, 1}. `t.insert(1, x1F)` is refused with S301 "in t, t.insert: it holds a hex".
- On `index<string, number> m`, `m["a"] = b101` shows {"a": b101} and `m["b"] = x1F` is refused with S301.
- `multiple<number, string> e = x1F` is refused: "it holds a hex, and this name takes satellite.variable.number or satellite.variable.string".
- A capsule parameter `(satellite.variable.number n)` given b101 keeps it as a binary: display(n) prints b101, where a number name prints 5. Given x1F it converts correctly to 31.

These behave correctly:
- `number n = b101` gives 5, and `number n = x1F` gives 31. The same holds after the name is declared (`n = x1F` gives 31, `n = b101` gives 5).
- `number h = x1F` then `t.append(h)` gives {1, 31}, and contains(31) is true.
- In an untyped list {b101, x1F, 2}, .sum is 38 and .max is x1F, both by worth.

A related side-finding, not separately isolated: an untyped list {b101} also answers .contains(5) false and .index_of(5) 0, while t[1] == 5 is true. So .contains does not compare the way == does for a binary or hex item.

## Where it comes from

satellite/bytecode/type_shape.cpp:49-50 (and :62-70); satellite/bytecode/container_calls.cpp:483-489; satellite/bytecode/program_walk.cpp:1158-1159; satellite/bytecode/hexadecimal_values.cpp:165-168

The binary-to-number and hex-to-number conversions happen only when a whole name is stored.
- For a binary it is run_assignment, program_walk.cpp:1158: `if (holds == word::code_of(1, 6, 4) && value.is_binary()) value = Value::of_number(...)`.
- For a hex it is hexadecimal_on_store, hexadecimal_values.cpp:165. It is called from run_assignment and from capsule parameter binding (program_walk.cpp:1956). The binary conversion is not called there, which is why a parameter keeps b101.

The container ways in never convert. .append and .insert (container_calls.cpp:483-489), the a[n] = / m[k] = writes (expression.cpp:1946-2027) and the declaration literal all call only value_fits. value_fits (type_shape.cpp:49-50) says "A NUMBER NAME TAKES A BINARY ... run_assignment converts it" and returns true with no conversion, so the binary is stored raw. It has no matching case for a hex, so a hex falls to the kind check at type_shape.cpp:62-70 and is refused with "it holds a hex".

## Notes

- Why the skeptic kept it: I reproduced it exactly. A number name turns a binary into its worth and a hex into its worth (b101 holds 5, x1F holds 31), and so does a whole-name assignment. A number slot inside a container does neither. It lets a binary in unconverted, so {1, b101} is shown and .contains(5) says false, even though b101 == 5 and t[2] == 5 are both true. It refuses a hex outright, and it does this on every way in: the declaration literal, t[n] = x, .append, .insert, map writes, and multiple. The binary help says "a number name given b0011 holds 3" and the hex help says "A hex given to a number name keeps its worth", so the valid form x1F is refused and the binary gives a wrong answer from .contains. Nothing about binary or hex in containers appears in ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md or CONTAINERS.md. DESIGN.md has no ruling on it, and check.sh has no row that puts a binary or hex into a typed container.
- Nearest known entry: none
- Merged: One cause: the binary conversion is written inline in run_assignment (program_walk.cpp:~1158/1192), and the on_store hooks run only at scalar stores (and at parameters, for hex and float). value_fits, the container writes and run_for never convert. One fix: a single store-conversion step used at every store. lists#7 already names the parameter case.

### Also seen as: A satellite.variable.number parameter given a binary keeps it as a binary (shows b0011), while a number declaration or assignment given the same value holds 3

```satellite
satellite.include(satellite)

satellite.capsule show(satellite.variable.number n)
{
    satellite.console.display(n)
}

satellite.capsule satellite.main()
{
    satellite.variable.number d = b0011
    satellite.console.display(d)
    show(b0011)
    satellite.return(satellite)
}
```

```
3
b0011

exit 0
```

### Also seen as: A whole number is refused as an item of a float-typed container (list<float>, map<..., float>, multiple<float, ...>), while a float name, parameter or field takes 3 as 3.0

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list<satellite.variable.float> prices = {1.5}
    prices.append(4)
    satellite.console.display(prices)
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): in prices, prices.append: it holds a number

directory: minfinal.satl:6
syntax: prices.append(4)
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

### Also seen as: A for loop's number counter refuses a binary or hex start value (k = b0001, k = x01, k = s for a binary name) that a number name accepts and converts

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.statement.for(satellite.variable.number k = b0001; k < 3; k++)
    {
        satellite.console.display(k)
    }
    satellite.return(satellite)
}
```

```
[satellite] satl(run): satellite.statement.for declares satellite.variable.number k, and it was given a binary (machine_code: 27 types_do_not_meet)

exit 27
```
