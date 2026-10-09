# 004 -- A capsule or library value named with one spelling of a method word is folded to that word's first spelling: two capsules color/colour in one spacesuit reach only one of them, m.as_number() runs number(), library values contain/contains collide, and library.upper reads a value written as up

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** spacesuits and library  
**Kind:** wrong-answer  
**Severity:** high

## What happens

Two capsules named with two spellings of one method word (color/colour, str/to_string) in one spacesuit: every call reaches only one of them, silently, while top-level capsules keep both.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit shape()
{
    satellite.public
    {
        satellite.capsule color()
        {
            satellite.return("red")
        }
        satellite.capsule colour()
        {
            satellite.return("blue")
        }
    }
}

satellite.capsule satellite.main()
{
    shape s
    satellite.console.display(s.color())
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
blue

exit 0
```

## What it should do

red. Or, if the two spellings are meant to be one name inside a spacesuit, an S202 'declared twice in the spacesuit' refusal before anything runs.

The user declared two capsules with two different names and called the first. At the top level the same pair keeps both (red, blue). A spacesuit already refuses a real duplicate with S202, so one declaration must never silently hide another.

## Variants

Declaring colour first and color second: both s.color() and s.colour() still print colour's answer. Capsules str() and to_string() in one spacesuit: s.str() and s.to_string() both print to_string's answer (T, T). Top-level capsules color()/colour() print red/blue, which is correct. A spacesuit with only color() works: s.color() finds it through the fallback loop.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/suit_reach.cpp:179-188 (CapsuleTable::member); /home/madness/code/cxx/satellite/satellite/bytecode/token_codes.hpp:400-401 (color and colour both lex to colour_token)

`.color` after an object lexes as the method code colour_token, which reads back as its first spelling, "colour". CapsuleTable::member first tries capsules.find("colour"). Only when that fails does it scan for any capsule whose method_code_of matches. So when both spellings are declared, the exact lookup for the first spelling always wins. The spacesuit's duplicate check compares the raw names, so it never sees the clash.

## Notes

- Why the skeptic kept it: Reproduced. With two capsules color() and colour() in one spacesuit, s.color() runs colour() and prints blue. The program is accepted with exit 0 and no warning, and the capsule the user wrote as color() can never be reached. The same happens with str()/to_string(): s.str() prints to_string's answer. The same two capsules at the top of a file print red then blue. A real duplicate name in a spacesuit gets S202, so a silent replacement is inconsistent. It is a wrong answer. Nothing in the known lists, DESIGN.md or the spacesuit help says two method-word spellings are one capsule name.
- Nearest known entry: none
- Merged: One cause: the lexer turns every spelling of a method word into one method code, and token::method_name_of(code) gives back only the first spelling. The written name is lost before the capsule lookup (capsule_calls.cpp, suit_reach.cpp CapsuleTable::member) and the library lookup (library_values.cpp segment_at). One fix: keep the written spelling. #4 was already marked a duplicate of #2.

### Also seen as: An object's capsule named like a method word answers to every other spelling of that word: m.as_number() runs number()

```satellite
satellite.include(satellite)

satellite.spacesuit motor()
{
    satellite.public
    {
        satellite.capsule number()
        {
            satellite.return(3)
        }
    }
}

satellite.capsule satellite.main()
{
    motor m
    satellite.console.display(m.as_number())
    satellite.return(satellite)
}
```

```
3

exit 0
```

### Also seen as: satellite.library value names that are spellings of one method word (contains/contain, color/colour, str/to_string/string) are folded to one name: two values are refused as one written twice, and a never-written spelling reads the other's value

```satellite
satellite.include(satellite)

satellite.library.contains = 1
satellite.library.contain = 2

satellite.capsule satellite.main()
{
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S202: NAME_DECLARED_TWICE
satl(check): satellite.library.contains is written twice in this file -- a value
is written once, and a second would quietly replace the first

directory: f3c.satl:4
syntax: satellite.library.contain = 2
        /\

this name is already declared where this line stands -- a variable in its
capsule, or a capsule, a satellite.namespace or an included file in its file or
space. One name, one declaration -- the second would quietly replace the first.
machine code 26 name_declared_twice -- satl exits with this.

--------------------------------------------------------------------------------

exit 26
```

### Also seen as: satellite.library.upper and .uppercase read a value written only as satellite.library.up

```satellite
satellite.include(satellite)

satellite.library.up = 5

satellite.capsule satellite.main()
{
    satellite.console.display(satellite.library.upper)
    satellite.return(satellite)
}
```

```
5

exit 0
```
