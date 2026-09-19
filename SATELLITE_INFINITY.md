# satellite-004 — SATELLITE_INFINITY

`satellite.variable.infinity`, and every number above it: the power, the sat, the
asat … zsat, aasat … and the float they rest on. What each one holds, how every
result is put together, how the numbers are named and displayed, what satellite
refuses, and the milestones that build it.

Written 2026-09-18 from the author's ten messages of that day, **quoted in Part 1,
in order**. The design changed between the messages, and the later message decides.
The first version of this file (`3a72f33`) was built from messages one to four: an
infinity as a *target* and a count of nines. **Message five replaced its heart.**
Part 2 lists every piece it overturned and every piece it kept.

**What is built:** only two config rows, `arguments.infinity = 128` (`239cfae`) and
`arguments.infinity.counter = 999999999` (`95d00ca`). There is no infinity,
no float and no power in 004 yet. Today `satellite.variable.infinity x =
satellite.infinity()` fails with `S110: … satellite.variable is not a call`, and
`12.34` fails with `S120: 12.34 is not a number this can read`.

**The tables are computed, not typed.**
`python3 satellite/satellite_variable_infinity/infinity_oracle.py` prints every computed
table below — the blocks headed in capitals — verbatim, and every figure the prose calls
"computed" (its CHECKS).
The timings in Part 9 were measured for `239cfae`. When the C++ lands it is checked
against the oracle, the way `check_numbers.py` checks `satellite_number` against
Python's integers: the value column must match, and an ERROR row must be refused.

---

## The design in one paragraph

**Every infinity-family value is ONE object: a list of terms, largest first.** Each
term is a **count** and a **unit**. The unit is a plain number, `infinity`,
`infinity-1` (the power), `infinity-2` (the sat), `infinity-3` (the asat), and so on,
or infinity raised to something in between. The first term decides what the object
IS, and its sign. **The terms after it are the author's register**: what is attached.
**Every result is assembled by the author's one rule**: *"either add the number or
flip the sign of the number THEN add the number"*. `+` and `-` use it directly.
`*`, `/` and `.power_of()` first work out which terms the answer has, and the one rule
adds them. **The type is how high the tower of exponents goes**, so `infinity ** infinity
** infinity` is `(infinity-2)`, a sat. **One number is one set of parentheses**, as message ten
writes it (he likens it to how a python list is displayed): `(infinity, -500000000000000)`. Counts are exact decimals of at most
`arguments.infinity` (128) places. The author's nines — count 2 is `1.999…9` — are
**derived** from a count for `.nines()`, and **no answer ever depends on them**.
After 999,999,999 calculations with one object that never reaches the next type,
satl prints the **SATELLITE INFINITY WARNING** (Part 9).

---

# Part 1 — the brief, as the author wrote it

**First:**

> write the float type satellite.variable.float is just two
> satellite.variable.numbers, and a satellite.variable.infinity we work with it as
> a number that is 0.9999999999999999999999999999999999999999... and we use
> however many digits the machine can process in 0.005 seconds, and we test the
> machine in --configure (change --config to have the alias --config and
> --configure that runs a test of how big of a number the machine can add which is
> at arguments.infinity it's a number of NS, the smallest amount of time the
> machine can measure, so we measure by default what the machine can add together
> in... (however many digits) the test is adding 4096 (all 9's) digits to 4096
> digits, so for arguments.infinity it will be a satellite.variable.number of 1024
> but the infinity floating point number must never reach 1, so it's always
> 0.99999999999999 and if you add two together, you get something.99999999 which
> if your adding one, the new number is
> 1.99999999999999999999999999....99999999999999999999999998 that is the new
> number, then if you added another one, it would be....
> 0.99999999999999999...999999999 + 1.99999999999999...99999999999998 the
> precision (amount of digits, the width, which is saved,) is always at 4096 if
> arguments.infinity is = 4096. So then if you do infinity * 0.5 (50 percent) it
> becomes 0.499999999999999999999999999999999999999999999999999999
>
> Do you need more to build the infinity? The amount of precision must be kept at
> arguments.infinity and the number has to only keep that many digits, build this
> prompt into SATELLITE_INFINITY.md

**Second:**

> so we use this 9999999 but you can write
> infinity_object.resize(amount_of_digits_of_precision_here_as_a_satellite.variable.number)
> to resize ONLY that infinity, and adjusts the infinity WAIT! we can adjust the
> infinity at every single operation, we HAVE to adjust the infinity every time we
> use it, so it starts off as TWO separate numbers, the whole number, and the
> number of nines, and when you add them, you only use the whole number, when you
> subtract, you only use the whole number UNLESS -- UNLESS you can't, when you
> cannot perform the operation, you sometimes use the 9's and adjust to 2.0 for
> infinity + infinity, it's like these 9's are used for something for the infinity
> and I do not know how to do it,

**Third:**

> how do you program an infinity? Store it as a floating point let's do the
> 0.9999999 precision idea, let's try that... and adjust the number for the width
> every time,

**Fourth:**

> that's it ! you adjust the width every single time, and keep it at
> whole_number.99999999999 you adjust the width every time, that is how you do it

**Fifth — the register, the power, the sat, and the names:**

> I wrote this prompt but then I realized the context was almost all used up, so I
> had to run /clear on the last session... we are building satellite.variable.infinity
> FIRST, and it is built out of a single, 4096 precision (amount of 9's) so it looks
> like this: 0.99999999999999..99999999999 with 4096 digits, but really that amount
> of precision, it really shouldn't be necessary, so let's build it out of 128 digit
> width, and set arguments.infinity = 128 for precision, I think we can still work
> this with only the 128 precision width, I don't think that the width of the float
> is going to matter, it's just.... 0.99999999999999998 no matter what you know? Then
> we just adjust and keep the infinity at that width at all times, yess..... and I
> wrote all of this, we need to turn a ll of this into... SATELLITE_INFINITY.md
> yessss..... here is what I wrote, so you gotta figure out the rest of it...
>
> So i've figured out how to actually program an infinity, when I type
> 0.99999999999999999999999999 MINUS 500,000,000,000,000,000 we have to MULTIPLY by
> a particular number, we have to convert the "50000000000000000000000000" into THE
> SAME PRECISION, THE SAME WIDTH, AS the 0.999999999999999 number, or we could do it
> another way, the amount of nines, it always remains
> 0.99999999999999999999999999999999999 so when you subtract, you hold onto
> 0.99999999999999999999999999 you have to use negative numbers for this or
> something, like, the infinity actually becomes, when you type infinity -
> 500,000,000,000 YOU KEEP THE NUMBER THAT BEGINS WITH "500,000" you add the sign and
> the number onto the infinity object, the infinity object is now "0.999999999 -
> number_you_subtracted, or "0.99999999 + the number you just added, so that settles
> add, subtract, multiply and divide, then we just have to do, add subtract multiply
> and divide an infinity and a floating point number, which makes it just -- the
> operation is always, for every single calculation, either add the number or flip
> the sign of the number THEN add the number, and that goes for infinity.power_of()
> becomes a new type of object, so infinity to the power of 2 - 1 infinity = a
> container of 2 things -- a power object and a negative infinity so
> infinity.power_of(infinity) becomes a new TYPE of object, we delete the old object
> and call it something else, which has it's own set of add sub mul div, and it
> keeps a sign, it isn't "9999999 objects in C++", it's a single object, that is
> either positive or negative, but when you type infinity.power_of(infinity) =
> something - infinity, you add an infinity to the power object and give it a
> negative sign, and that is how we do math with an infinity!
>
> Can you convert all of this into SATELLITE_INFINITY.md? The final way that I gave
> to program the infinity??? it becomes a power object we tear down the old object
> and it becomes an object of a different type, and we maintain what is attached to
> it, so if I wrote infinity.power_of(infinity) = something, THEN something * 8 = we
> simply ADD the * 8 onto the power object, so it now remains positive for that
> much, so it's the same object name the entire time, we are just keeping track of
> how much is attached to it, you know? As we add, subtract, multiply, divide, we are
> just adding or subtracting from registers, a power object has a list of
> satellite.container.list<number_object> numbers_register; then we add an object
> that looks like this:
>
>     satellite.spacesuit inf_number_object()
>     {
>         satellite.variable.infinity value = infinity where infinity is the real, infinity object and we supply methods to work with that value, you know? So a power_object is just... and an infinity is the same thing, satellite.variable.infinity for an infinity, satellite.variable.power for the next thing, THEN we can use inside of the "power" object, it's made out of dual 0.9999999999..9999 infinities -- it's two separate infinities, so it looks like this:
>
>     satellite.variable.infinity value = 0.9999999999999999999
>     satellite.variable.infinity power = 0.999999999999999999
>
> so if we program this correctly, we can just do this: remove the second infinity,
>
>     satellite.container.list<satellite.variable.infinity> value = {infinity1, infinity2}
>
> that is a power object, it just depends on how you treat the second infinity -- in
> this is how we program a power object, so now we have satellite.variable.power,
> and MORE power just becomes adding an infinity onto value for each, and it all
> comes down to HOW you treat the NEXT INFINITY ADDED onto value! So does that
> explain how you add power + power then? 1 power + 1 power = 1.9999999999999 power,
> so a power object has to be encoded that way, so that you can add, subtract,
> multiply and divide, and then something larger than a power will become a "sat"
> satellite.variable.sat, which is:
>
>     satellite.container.list<power_object> value = {power1, power2}
>
> AND YOU TREAT THE POWERS THE SAME WAY YOU TREATED THE LIST OF INFINITIES, we just
> have to figure out exactly how to write the code to add and do
> power.to_the_power_of(infinity) = power ** infinity so thats using a method I
> described earlier and you... okay you lost me, but I think i've managed to provide
> enough info to create the "sat" object, and then since "sat" is
> satellite.container.list<power_object> value = {power_object1, power_object2} and
> we can code all the way out with these: we can create names of numbers, and add the
> "+sat" extension to each word -- so the next thing will be a asat_ojbect, then we
> will call it a bsat_object, then a csat object, and we just add another letter --
> so we just continue to define the NEXT type -- it starts at asat, then ends at
> zsat, then we begin aasat, then absat, then acsat, then adsat, so at first it has 1
> a, then 2, then 3, and we just build a directory for the code that these files are
> built out of, we write the code for the "sat" objects IN satellite, and they are
> stored like this:
>
>     /infinity/infinity.satl (maybe you would want to program the infinity in C++?)
>     /infinity/power/power_object.satl
>     /infinity/sat/sat_object.satl
>     /infinity/asat/asat_object.satl
>     /infinity/bsat/bsat_object.satl
>     /infinity/csat/csat_object.satl
>     /infinity/a/aasat_object.satl -- we begin using another folder when we use two aa,
>
> so see if you can figure out how to build all of this in to a set of milestones, we
> will have to like, do alot of work to get even the milestones for
> SATELLITE_INFINITY.md written, it will be alot of work, but it will be totally
> worth it dude, we can now count up to any number literally with this system,
>
> so to access the different asat aasat objects, what we do is this:
> satellite.variable.aasat my_aasat = satellite.aasat() that is the syntax, and it
> starts at... satellite.variable.infinity my_inf = satellite.infinity() and they use
> 0.99999 and everything is built out of that infinity object -- the secret is that
> we delete the object and it becomes a different type of object UNDER the same name,
> you know? So build all of this in to milestones

**Sixth — the display (three messages, the same evening):**

> so when we write infinity * infinity * infinity it gives us what that equals, do
> humans already have a number for something that is greater than infinity? Or do we
> have to use the made up -sat extensions and the "power" word? We have to have a way
> to display the number when you do this:
>
>     satellite.variable.infinity my_number = satellite.infinity()
>
>     satellite.console.display(my_number ** my_number ** my_number) would display like... "1 sat"
>
> and the infinities have to be displayed as "1 infinity" actually, the displaying of
> the numbers they have to be displayed as "1 infinity" "2 infinity" "1 power" "2
> power" "1 sat" "1 asat" "2 esat" "2 infinity-c" let's do that instead, let's use
> this for displaying the numbers: "infinity-1" like it has a class to go with it,
> "infinity-1" is really the "power_object", let's go with that, then "infinity-2" is
> 1 sat object, and infinity-3" is an "asat" object, we just use the asat name as it
> was easier to program it that way isn't it??

> so you will have the display printing "1 infinity-1" is really "1 power object"

> How's it going dude? Does it work?

**Seventh — beyond every infinity-N (two messages):**

> so THEN we need to add this for the next phase of this: 1 infinity-infinity, and we
> STORE the code as an std::string, and when you add... so we have 1
> infinity-infinity-1 as the next thing, then we have infinity-infinity-infinity-1 so
> we need to program this so you can just write... infinity-infinity-infinity-infinity,
> but infinity-infinity-infinity....forever is displayed as 1 a-class-infinity then 1
> b-class-infinity so do you think something like that is even possible? an
> alphabetical numbering? We will specifically write it like this: 1 class-1-infinity
> then class-1-infinity-1 then class-infinity-infinity, somehow there has to be a way
> to program something that can just, count to anything at all, you know, I just
> canot think of it, where the number is displayed with dots, we will use the dot
> system of satellite for the different numbers, so "1 infinity.infinity" is the thing
> that is beyond "1 infinity-infinity", the dashes become dots, and we just add
> another .infinity each time, somehow.... so we can handle as many .infinity's the
> machine can calculate or something, but I want to use the .infinity idea to program
> extremely large numbers, beyond infinity dash infinity is
> infinity.infinity.infinity.infinity THEN we REWORK IN the dash system, so we
> alternate between dashes and dots to display any number at all, and to use any
> number at all, I just cannot think of the code that would properly code all of
> this, you know?

> then for the largest number that we deal with, it's infinity-x and then
> infinity-x-x we just add -x -x -x -x for extremely big numbers, so somehow we have
> to be able to write a number (I could be wrong about this,) but somehow we have to
> write infinity.infinity-infinity-x-infinity, and it still come out as the correct
> number somehow, built from all the way from a float that is just
> 0.9999999999..999999999

**Eighth — the dot and the dash, and asking for a number by name (two messages;
a one-line message sent by accident between them, and the author's "whoops", are left
out):**

> i've got it! We have a function that displays as this, it always displays the
> number as 999.. and that is one infinity, and the number, no matter how big, is
> always displayed as (999...) or wait, that isn't going to work, and we have to use
> the other way I designed it, with the infinity-1 for power, infinity-infinity then
> infinity.infinity-1 then infinity.infinity-infinity, then
> infinity.infinity.infinity-1 so we alternate between adding a dot and a dash, you
> can see how it alternates between a dot or a dash right? You can program that to
> display the numbers, however, the numbers must be coded as different things, like
> satellite.variable.infinity my_number = satellite.infinity(infinity) = displayed as
> 1 infinity-1 (1 power_object), then we just ask for a
> satellite.infinity(infinity(infinity)) then satellite.infinity(infinity(infinity)) =
> something, I dunno, can you figure out how to request these numbers? See if we do
> this:

>     satellite.variable.number count = 0
>     satellite.variable.number target = 99
>
>     satellite.variable.infinity my_inf = satellite.infinity()
>     satellite.variable.infinity my_other_inf = satellite.infinity()
>
>     satellite.statement.while(count < target)
>     {
>         my_inf = my_inf * my_inf
>
>         count = count + 1
>     }
>
> like what number do we end up with doing that? Do we ever reach
> "infinity.infinity-infinity" that way, if we just keep multiplying by itself?? What
> number do we end up with???

**Ninth — what are they for, and `.power(-infinity)` (four messages):**

> so does my_number = my_number * my_number ever reach infinity? No! it doesn't! So
> this isn't really practical BEYOND a single infinity, is it? There's like not much
> use for this system that we have developed that is beyond infinity, I mean what can
> you use anything beyond infinity for anyways??? I mean, before we do anything, what
> is the use of a system beyond infinity used for?

> If you cannot reach any of the other numbers with add, subtract, multiply or divide?
> I just don't get it

> Since we cannot reach any of the other numbers with add, mul, sub, and div, what are
> these numbers used for then??? I guess there's a use for them, I just don't know what
> it is??

> I got it, the use of these numbers is in calling infinity.power(infinity) then you
> can do this:
>
>     infinity.power(infinity).power(infinity)
>
> and you can string .power(infinity) or .power(infinity-1) to get larger numbers, and
> you have to use .power(infinity) and negative infinities, so .power(-infinity) so we
> have to keep a sign with everything, so you can subtract to the power of, and add to
> the power of, and add multiple .power(infinity).power(infinity) to reach bigger or
> smaller numbers, .power(infinity * infinity) so we can just write...
> my_infinite_object.power(infinity * infinity) and it will do the math that is in
> parentheses, this is how you use these numbers, you know?

**Tenth — one number in one set of parentheses, and the warning:**

> yes it displays as... infinity - 5000000000... or whatever the user enters, it holds
> another number, and we keep track of whether that number is positive or negative, so
> it actually should display as like how a python list is displayed, (infinity,
> -500000000000000) we will keep it in parentheses for display, so we know that is a
> single number, that is how we will show the user it is just a single number, and
> when we get into infinities and powers and everything, it is still just adding sets
> of parentheses, like this:
>
> (infinity.infinity, +90440393845) or whatever that particular number is, it has to
> keep track of like, all this stuff, as it works it's way through the interpreter, I
> dunno, just make it look nice, so my_inf = my_inf * my_inf never reaches two
> infinities.... when using the variable satellite.variable.infinity, we have to warn
> the user after so many operations, we pause the interpreter and remind them after so
> many operations... let's call it, 999,999,999 after that many operations on the same
> object, we display a warning like this:
>
>     (one empty line here)
>     -------------------------------------------------------------------------------- (80 chars)
>      SATELLITE INFINITY WARNING (centered)
>     -------------------------------------------------------------------------------- (80 chars)
>     (one blank line here)
>                            OBJECT: \"object_name\" (in quotes)
>                        WILL NEVER REACH INFINITY
>     (one blank line)
>     -------------------------------------------------------------------------------- (80 chars)
>     (one space here)
>
> this is ONLY after 1 billion (999,999,999) and this number is set inside of
> arguments.infinity.counter(999,999,999) and this applies to anything that never
> reaches the next object type and does that many calculations WITH that object,
> display the warning, then reset the counter to 0, so with the infinity object we just
> increase the counter every time it is used, then when it's full, it counts everytime
> the object is NOT destroyed ONLY, if the object is destroyed and turned into a
> different class object, then the counter is destroyed with it! Perfect logic!

---

# Part 2 — what message five changed, and what it kept

| was (`3a72f33`, messages 1–4) | now | why |
|---|---|---|
| width 4096 (and 1024 in message 1) | **128** — built, `239cfae` | *"let's build it out of 128 digit width, and set arguments.infinity = 128"* |
| a finite number is **folded into the target**: `infinity + 5` is target 6, shown `5.999…9` | **attached, never folded**: `(infinity, +5)` | *"YOU KEEP THE NUMBER THAT BEGINS WITH "500,000" you add the sign and the number onto the infinity object"*. Folded, `infinity - 500000000000` is a *negative finite* number (Part 3) |
| the value **is** `target − 10^-width`, so the nines are the value | **the count is held exact; the nines are derived** for `.nines()` | message five: *"I don't think that the width of the float is going to matter"*. Held as the value, the nines drift (Q1) |
| one multiplier float (M11, *"a single satellite float for going up or down"*) | **a count on every term**; the first term's count is M11's multiplier | the register is what message five adds beside it |
| `satellite.infinity.new()` | **`satellite.infinity()`**, and `satellite.infinity(x)` is infinity ** x | messages five and eight |
| display `infinityx2`, `infinityx0.5` (M11), then `1 infinity`, `2 infinity` (message six) | **one number in one set of parentheses**: `(infinity)`, `(2 infinity)`, `(infinity, -500000000000000)`, `(infinity-1)` — a count of 1 is not printed (Q36) | message six named the rungs, *"infinity-1"*; message ten: *"we will keep it in parentheses for display, so we know that is a single number"* |
| — | **the SATELLITE INFINITY WARNING** after `arguments.infinity.counter` (999,999,999, built `95d00ca`) calculations that never reach the next type | message ten |
| old I7: *"`infinity < 1` is true"* | **a positive infinity is larger than every number**, a negative one smaller (Q21) | under the first version the value WAS `target − 10^-width`, so `infinity < 1` followed from it. With the value no longer the nines, it does not |
| old Q2: `infinity - infinity` is **an ERROR**, per M11's summary of 09-16, "an ERROR, never a guess" | **the number that is left**: `0` (Q2, with the ERROR as the alternative) | the 09-16 ruling in the author's words is *"we answer what we can, and give an error on what we can't"*; M11 adds that the infinity carries its x2 *"unless it goes away or causes an error"*; and *flip the sign and add* gives exactly 0 |
| `.resize(n)` sets that infinity's precision (message two: *"resize(amount_of_digits_of_precision_here…)"*) | **sets that infinity's nines width, which only `.nines()` shows** (Q31) | with the count held exact, no digit of precision is lost for `.resize` to restore |
| an infinitely small number was never discussed | **refused until INF-8**, which message nine asks for: *".power(-infinity) so we have to keep a sign with everything"* (Q34) | |
| the float first (old I1–I3), then the infinity | **the infinity first**, arm 12 (Q26) | *"we are building satellite.variable.infinity FIRST"* |
| `--configure` times a test and sets `arguments.infinity` (old Q3, Q4) | message five sets 128 by hand. `--configure` **becomes** a second spelling of `--config`, and dropping the timing test is this file's proposal (Q15) | a width no answer depends on needs no calibration |
| old I3: *"`4 / 3` stops being refused"* | **`4 / 3` is not refused today — it prints `1`** (checked, BUILD 0209) | `number_and_number_divide.hpp` truncates on purpose, so landing the float would CHANGE a passing answer (Q13) |
| old I2: *"the lexer has no decimal token"* | the lexer already reads `12.34` as one number; **the reader refuses it** (S120) | `bytecode_registry.cpp:320`: *"A number, the dot joining it only when a digit follows"* |
| old Q6: the float as `(mantissa, width)` | **M20's decided shape** (08-27) as the default, `(mantissa, width)` as the alternative (Q18) | the first version argued against a shape nobody decided, and never named M20's |
| old Q5: two widths meeting keep the smaller | still a question (Q19). It now touches only `.nines()` | |
| old Q7: shown digits cut, not rounded as M11 says | **still a question**, now split: Q17a for the nines, Q17b for a float. The author's own words are *"we round to 32 digits"* | |
| — | **power, sat, asat … their names, and Part 11** | messages five to eight |

**Kept:** an infinity keeps a sign (09-17); `.resize(n)` changes ONLY that infinity
(message 2), though what it changes has moved (above).

---

# Part 3 — the infinity

## What it holds

    a list of terms, largest first      each term: a count and a unit
        the FIRST term                  what the object is, and its sign
        every term AFTER it             the register -- "what is attached to it"
    and one nines width                 for .nines() only (Part 9)

    satellite.infinity()                (infinity)
    infinity - 500000000000000000       (infinity, -500000000000000000)
    infinity + infinity                 (2 infinity)

The register is what message ten means by *"it has to keep track of like, all this
stuff, as it works it's way through the interpreter"*: every number attached, each with
its sign — *"we keep track of whether that number is positive or negative"*.

**The object's sign is its first term's sign, and each attached term keeps its own.**
Message five: *"it keeps a sign, it isn't "9999999 objects in C++", it's a single
object, that is either positive or negative"*, and *"you add an infinity to the power
object and give it a negative sign"*. A count carries its own sign bool — the float
shape's (Part 10) — so `-infinity` and `infinity * -1` are one value. This file reads message five as the author's answer to
the question M11 left him (`-infinity` against `infinityx-1`). Q20 asks him to confirm
it.

**A count is an exact decimal of at most `arguments.infinity` places**, held in the
float's shape (Part 10). It needs no float arm, so `(0.5 infinity)` and `(0.25 infinity,
+1.5)` can land before the float does. The plain term — the finite number attached —
follows the same rule.

## Every result is assembled by the one rule

The author: *"the operation is always, for every single calculation, either add the
number or flip the sign of the number THEN add the number, and that goes for
infinity.power_of()"*. **`+` and `-` ARE the rule. `*`, `/` and `.power_of()` each
first work out which terms the answer has** — every term times every term, counts
multiplied and exponents added. The rule then adds those terms into one object.
That is how it *"goes for infinity.power_of()"*: `infinity.power_of(2) - infinity` is
built by the rule from the two terms `(infinity^2)` and `(-infinity)`.

    THE INFINITY
        satellite.infinity()                     (infinity)                           infinity
        inf + inf                                (2 infinity)                         infinity
        inf * 50%                                (0.5 infinity)                       infinity
        inf - 50%                                (0.5 infinity)                       infinity
        inf - 500000000000000000                 (infinity, -500000000000000000)      infinity
        inf + 7                                  (infinity, +7)                       infinity
        (inf - 5) * 8                            (8 infinity, -40)                    infinity
        (inf + 6) / 4                            (0.25 infinity, +1.5)                infinity
        inf * -1                                 (-infinity)                          infinity
        inf + inf - inf                          (infinity)                           infinity
        x = x - 1, a thousand times              (infinity, -1000)                    infinity
        (inf - 1) - 1 == inf - 2                 true
        inf > 10 ^ 100                           true
        inf - 10 ^ 200 > 10 ^ 300                true
        inf + inf > inf + 10 ^ 300               true
        inf * -1 < 5                             true
        inf * -1 < 0 - 10 ^ 300                  true

**`*` and `/` by a number scale every count**: `(inf - 5) * 8` is `(8 infinity, -40)`.
That is *"something * 8 … we simply ADD the * 8 onto the power object"*, and the `x8`
lives in the count. It is also why `p + p` and `p * 2` are one value.

`inf - 50%` is `(0.5 infinity)` because 004's percentage is percent-**of** (checked:
`10 - 50%` prints `5`). That is M11's `infinityx0.5`, displayed the new way.

## Why the width no longer matters — the first idea of message five, computed

Message five first reaches for *"convert the "50000000000000000000000000" into THE SAME
PRECISION, THE SAME WIDTH, AS the 0.999999999999999 number"*, then says *"or we could
do it another way"*. The other way is the decision, and computing the first one shows
why. Held as 128 nines, the infinity is a finite number with 128 digits, and a big
enough subtraction goes straight through it:

    CHECKS
        inf - 10^17   as 128 nines: positive and finite;  with the register: above 10^1000 = True
        inf - 10^127  as 128 nines: positive and finite;  with the register: above 10^1000 = True
        inf - 10^128  as 128 nines: NEGATIVE and finite;  with the register: above 10^1000 = True
        inf - 10^200  as 128 nines: NEGATIVE and finite;  with the register: above 10^1000 = True

**Under the register the width never meets the number.** That is the author's
*"I don't think that the width of the float is going to matter"*, and it is exactly
true. The first version's fold hit the same wall sooner: its `infinity -
500000000000` was target −499999999999, a negative finite number.

## The register combines like terms

*"we are just adding or subtracting from registers"*, *"keeping track of how much is
attached"*: an amount, not a log. `x = x - 1` a thousand times is `(infinity, -1000)`,
one term, and `(inf - 1) - 1 == inf - 2` is true. A log of every number typed would
make that `false`, and would grow by one entry on every pass of a loop: at 88 bytes a
`satelliteObject` (measured), 10^6 passes is 84 MiB (arithmetic, not a run). Q8.

## Dividing, and a count that never ends

    DIVIDING
        inf / 4                                  (0.25 infinity)                      infinity
        inf / 3                                  ERROR: a count of 1/3 never ends; cut, it would be wrong by an infinite amount
        (inf * 3 + 1) / 3                        ERROR: a count of 1/3 never ends; cut, it would be wrong by an infinite amount
        inf / 10 ^ 200                           ERROR: a count needs 200 places, more than arguments.infinity (128)
        (inf * inf - 1) / (inf - 1)              (infinity, +1)                       infinity
        5 / inf                                  ERROR: the answer has a part infinitely small; no type holds it before INF-8

**A count that never ends is refused, never cut**, and the reason is computed:

    CHECKS
        1/3 cut at 128 places: (inf / 3) * 3 - inf = -10^-128 infinities, below -10^1000: True

A finite float cut at 128 places is wrong by 10^-128. An infinity's count cut there is
wrong by 10^-128 **infinities**, and that is still infinite (Q3). The plain attached
term follows the same rule, so `(inf * 3 + 1) / 3` is refused for its `1/3` too.

**Division inside an infinity is exact** — `(inf + 6) / 4` carries `+1.5` — while
004's `/` between two plain numbers is whole-number division (`6 / 4` prints `1`). An
infinity is not a whole number, and its register is exact. **Dividing by a sum** works
when every exponent is finite and it divides exactly: `(inf * inf - 1) / (inf - 1)` is
`(infinity, +1)` (Q32). Dividing a number by an infinity leaves something infinitely
small — below every positive number and above 0 — and no type holds that until INF-8
(message nine).

## A rounded percentage going in

    A ROUNDED PERCENTAGE GOING IN (satl prints 100% / 3 as 33.33333333333333333333333333333333%)
        inf * (100% / 3)                         (0.3333333333333333333333333333333333 infinity) infinity
        inf * (100% / 3) * 3 - inf               (-0.0000000000000000000000000000000001 infinity) infinity

004's percentage rounds every answer half away from zero at 10^-32 percent
(`satellite_percentage::rounded_divide`). So `100% / 3` is a decimal that ends, and
the infinity takes it exactly. **The infinity is exact on what it is given**, and the
rounding happened one step earlier. The second row is the price: an infinity that
"should" be 0 comes out as −10^-34 infinities, which is below every number. The
default accepts it and shows it (Q23); the alternative is to refuse an infinity times
a percentage that came from rounding. A float cut at 128 places going in behaves the
same way.

## When the infinity goes away

    WHEN THE INFINITY GOES AWAY
        inf - inf                                0                                    number
        (inf + 5) - inf                          5                                    number
        inf / inf                                1                                    number
        inf * 0                                  0                                    number
        (inf + 6) / 4 - inf / 4                  1.5                                  number

M11: the infinity carries its x2 *"unless it goes away or causes an error"*. Every
infinity is the same unit — that is why `inf + inf` is exactly `(2 infinity)` — so *flip
the sign and add* makes `inf - inf` exactly 0. **The name keeps what is left**, a
plain number: the author's *"a different type of object UNDER the same name"*, run the
other way, and it is the author's 09-16 ruling at work: *"we answer what we can, and
give an error on what we can't"*. The first version made this an ERROR, on M11's
summary of that ruling ("an ERROR, never a guess"); Q2 keeps that as the alternative.
The first version's other worry — a `-0.000…1` residue — is gone with the register.

**The last row has no home until the float exists.** 004's plain number is whole, so
until FLT-3 a left-over `1.5` is refused the way `3 - 50%` is refused today: *"… is
not a whole number, and there is no satellite_float yet"*. From FLT-3 it is a float
(Q22).

## Comparing

**The order is the sign of the first term of `a - b`.** At the first place where two
values differ, the larger unit wins only when its count is positive. So **a positive
infinity is larger than every number, and a negative one is smaller than every
number**, whatever is attached: `inf * -1 < 0 - 10 ^ 300` is true (written that way
because satl reads `-10 ^ 300` as (−10)^300). That is what attaching means: a finite
number can move an infinity by a finite amount only. It is this file's
reading of *"we can now count up to any number literally with this system"*, and Q21
asks the author to confirm it. The one comparison serves `<`, `==`,
`.sort().by_value()` and the keys of an index. `.reverse()` and `number(...)` on an
infinity are refused. `reverse_token` is *"a list, a string, a number, a binary or a
hex"*, and an infinity has no digits to turn round.

---

# Part 4 — the power, the sat, and every level above

## Do humans already have a number greater than infinity? (message six)

**Yes.** Georg Cantor wrote the infinity as ω, in the 1880s, and counted past it.
`inf * inf` is his ω², and `(infinity-1)` is his ω^ω. The arithmetic below is John
Conway's (the surreal numbers, 1970s), written in Cantor's normal form: it is
commutative, it subtracts, and its counts can be fractions. That is why `infinity - 5`
exists here, which in Cantor's own ordinal arithmetic it does not. Mathematicians
have symbols for these numbers but no everyday words. The names below are the
author's, and nothing competes with them.

## What are they for? (message nine)

Message nine: *"what is the use of a system beyond infinity used for?"* — **the author
is right that `+ - * /` from ordinary numbers never reach them. They never reach
infinity itself either**: no number of `+ 1`s arrives. That is what makes infinity
useful — it names "this never ends" — and each rung above names a stronger kind of
never ending. They are for measuring, not for counting up to:

    WHAT THEY ARE FOR: which grows faster, with n put in as infinity
        1000 * n * n + n                         (1000 infinity^2, +infinity)         power
        0.001 * n * n * n                        (0.001 infinity^3)                   power
        the second is larger in the end          true
        (n * n + 1) / (n * n - n)                ERROR: does not divide exactly: the answer is an endless series of infinitely small parts

- **How fast things grow.** Put infinity in for n and compare: the table says
  `0.001 n³` overtakes `1000 n² + n`, exactly. That is Big-O, the analysis of algorithms,
  done without estimating (G. H. Hardy, *Orders of Infinity*, 1910).
- **Proving a program stops.** Give each pass of a loop a number that shrinks; if it
  always shrinks, the loop ends — even when the number is infinity-sized, as for a
  loop inside a loop (`infinity * outer + inner`). The ACL2 theorem prover, used to
  check AMD's chip arithmetic, proves termination with exactly these numbers, written
  in exactly this normal form. satellite's numbers with whole, positive counts are
  ordered the same way. For QUAD AI reading code, this is the use that matters most.
- **`infinity-infinity`** (Part 11) is the number Gerhard Gentzen used in 1936 to
  prove ordinary arithmetic consistent.
- **Games.** Conway found these numbers by analysing games, and Elwyn Berlekamp used
  them to solve Go endgames.

**The last row is where message nine points next.** Its answer is 1, plus an endless
tail of infinitely small parts. With infinitely small numbers (INF-8, *".power(-infinity)"*)
and a rule for where to cut such a tail (Q34), satellite could compute a limit
exactly, as calculus does. Message nine's other uses already work: `.power(infinity *
infinity)` does the math in the parentheses first (`(infinity^(infinity^2))`, a sat),
and `.power(infinity).power(infinity)` is the same number.

## `infinity * infinity`, and the author's own example

    THE POWER (infinity-1)
        inf * inf                                (infinity^2)                         power
        inf * inf * inf                          (infinity^3)                         power
        inf.power_of(2) - inf                    (infinity^2, -infinity)              power
        inf.power_of(100000)                     (infinity^100000)                    power
        inf.power_of(inf)                        (infinity-1)                         power
        satellite.infinity(satellite.infinity()) (infinity-1)                         power
        satellite.infinity(2)                    (infinity^2)                         power
        p + p                                    (2 infinity-1)                       power
        p * 8                                    (8 infinity-1)                       power
        p - inf                                  (infinity-1, -infinity)              power
        p - inf + 5                              (infinity-1, -infinity, +5)          power
        p * p                                    (infinity^(2 infinity))              power
        p / inf                                  (infinity^(infinity, -1))            power
        inf.power_of(inf + 1)                    (infinity^(infinity, +1))            power
        (inf * inf).power_of(inf)                (infinity^(2 infinity))              power
        (inf + 5).power_of(inf)                  ERROR: a sum raised to an infinite or non-whole power is an endless series
        (inf + inf).power_of(inf)                ERROR: a count other than 1 (2) raised to an infinite or non-whole power is not built
        inf.power_of(-1)                         ERROR: a negative exponent is infinitely small; no type holds it before INF-8
        inf.power_of(50%)                        ERROR: an exponent of 0.5 lies between the numbers and one infinity; no type holds it before INF-8
        (inf * inf).power_of(50%)                (infinity)                           infinity
        inf.power_of(150%)                       (infinity^1.5)                       power
        inf.power_of(150%) / inf                 ERROR: an exponent of 0.5 lies between the numbers and one infinity; no type holds it before INF-8
        2.power_of(inf)                          ERROR: 2 to an infinite power has no type
        p > inf * 10 ^ 100                       true
        inf * inf * inf < p                      true
        1000 * p < p * p                         true
        p - inf < p + 5                          true
        p * -1 < inf                             true
        my_inf = my_inf * my_inf, 99 times       (infinity^633825300114114700748351602688) power
        ... and that is still < p                true

(`p` is `inf.power_of(inf)`.) **Multiplying infinities adds their exponents.**
Message six asks what `infinity * infinity * infinity` equals: `(infinity^3)`, which
is bigger than any count of infinities and smaller than `(infinity-1)`. The display
needs the `^3`, because `(infinity-1)` is a different, larger number (Q5). Message six's
display example uses `**` instead (`my_number ** my_number ** my_number`), and that
one is `(infinity-2)`, the *"1 sat"* he expected. Q4 asks which he meant in the first
sentence.

*"infinity to the power of 2 - 1 infinity = a container of 2 things -- a power object
and a negative infinity"* is the third row exactly: the first term is the power, and
the register holds `(-infinity)`.

**`(infinity-1)` is `infinity.power_of(infinity)`**: *"dual 0.9999999999..9999
infinities -- it's two separate infinities"*, a base and an exponent. *"1 power + 1
power = 1.9999999999999 power"* is `(2 infinity-1)`, whose nines are `1.999…9` (Part 9).
*"you add an infinity to the power object and give it a negative sign"* is `p - inf`.

**`satellite.infinity(x)` is infinity ** x** — message eight's way to ask for a
number: *"satellite.variable.infinity my_number = satellite.infinity(infinity) =
displayed as 1 infinity-1 (1 power_object)"*. `satellite.infinity()` is `(infinity)`,
`satellite.infinity(2)` is `(infinity^2)`, and each nesting climbs one rung:
`satellite.infinity(satellite.infinity(satellite.infinity()))` is `(infinity-2)`. It is
Cantor's own ω^x, the function every number in this Part is built from. Q27a.

**Message eight's loop** — `my_inf = my_inf * my_inf`, 99 times — ends at `(infinity^633825300114114700748351602688)`: the exponent doubles each pass, to 2^99. It
is **still smaller than `(infinity-1)`**. Every pass adds a finite amount to the
exponent, so no loop of any length reaches infinity ** infinity. That is the whole
reason each rung needs a new operation, and Part 11 needs a new symbol.

## What `.power_of()` refuses

*"we maintain what is attached to it"* holds for every exact operation after a
promotion (`p - inf`, `p * 8`). It cannot hold INSIDE the promotion. Keeping the `+5`
attached would answer `(infinity-1, +5)`, but:

    CHECKS
        (x + 5)^x / x^x at x = 10^6: 148.4113   (e^5 = 148.4132)

The true value is about 148 powers, not one power plus 5, so any comparison against
`(2 infinity-1)` would come out the wrong way round. A wrong number cannot be taken
back; an ERROR can (Q7). **An answer with an exponent that is negative, or between 0
and 1, is refused** — judged on the answer, at every depth, so `(inf * inf).power_of(50%)`
is `(infinity)`, while `inf.power_of(50%)` and `inf.power_of(150%) / inf` are refused.
No type sits between the numbers and `(infinity)`, and none below the numbers, until
INF-8 builds them (Q33, Q34).

## Sat and above: the type is how high the exponent tower goes

    SAT AND ABOVE
        inf ** inf ** inf  (right to left)       (infinity-2)                         sat
        (inf ** inf) ** inf                      (infinity^(infinity^2))              sat
        satellite.sat()                          (infinity-2)                         sat
        p.power_of(p)                            (infinity^(infinity^(infinity, +1))) sat
        inf ** inf ** inf ** inf                 (infinity-3)                         asat
        satellite.asat()                         (infinity-3)                         asat
        satellite.sat() + satellite.power()      (infinity-2, +infinity-1)            sat
        satellite.sat() - 1                      (infinity-2, -1)                     sat
        satellite.sat().power_of(inf)            (infinity^(infinity^(infinity, +1))) sat
        satellite.sat().power_of(satellite.sat()) (infinity^(infinity^(infinity-1, +infinity))) asat
        x = inf.power_of(x), 300 passes from inf (infinity-300)                       klsat
        satellite.sat() > 10 ^ 6 * inf.power_of(inf * 10 ^ 6) true
        satellite.zsat() < satellite.aasat()     true

**The RANK of a value is read from its first term**: an exponent of 1 is rank 0, an
infinity. A finite exponent above 1, or an exponent of rank 0, is rank 1, a power. An
exponent of rank k is rank k+1. **Rank k is shown `infinity-k`**, and the canonical
`infinity-k` is a tower of k+1 infinities: `infinity-1` is inf^inf, `infinity-2`
is inf^inf^inf, `infinity-3` is inf^inf^inf^inf. Three hundred passes of `x =
inf.power_of(x)` from `x = inf` build a tower of 301, which is `(infinity-300)`, a
klsat. The author's `my_number ** my_number ** my_number` is `(infinity-2)`. It reads
right to left, as 004's `^` already does: `2 ^ 3 ^ 2` prints `512`, checked.

**Why this reading, and not the other.** *"TREAT THE POWERS THE SAME WAY YOU TREATED
THE LIST OF INFINITIES"* can be read two ways:

- **a sat is a LIST of powers, and a longer list is a taller tower.** Then *"MORE
  power just becomes adding an infinity onto value for each"* keeps a four-high tower
  a power, and that power outgrows `sat{power, power}`:

        CHECKS
            3^3^3^3 = 3^7625597484987, and (3^3)^(3^3) = 27^27 = 3^81: the four-high "power" is larger

  which breaks *"something larger than a power will become a sat"*.
- **a sat is a power whose exponent is a power** — the same operation, one level
  higher. Then every sat is larger than every positive power:

        CHECKS
            115 positive values; 4351 pairs where one has the higher type; the higher is larger in all but 0

The second is built (Q4). It overrules the first reading of *"MORE power just becomes
adding an infinity onto value"*: under it, the third infinity in a tower makes a sat,
not more power. Three consequences for the author:

- **`infinity-1 ** infinity` is a sat**, `(infinity^(infinity^2))`. That answers the
  question message five left open: *"we just have to figure out exactly how to write
  the code to add and do power.to_the_power_of(infinity)"*. But **`infinity^k **
  infinity` is still a power**: `(inf * inf).power_of(inf)` is
  `(infinity^(2 infinity))`. `** infinity` climbs from infinity, and from a power whose exponent is
  infinite, and not from a sat.
- **`L.power_of(L)` climbs exactly one level at every level**: infinity → power → sat
  → asat.
- **Equal values display equally.** `p.power_of(p)` and `satellite.sat().power_of(inf)`
  come out as the same number, printed the same way.

## The same name, a different type

*"the secret is that we delete the object and it becomes a different type of object
UNDER the same name"*: **every level is ONE C++ arm** (Part 7), so `x =
x.power_of(x)` replaces the value in `x`'s slot and the arm never changes. The checker
already has two precedents for a name accepting more than its word: a number name
given a binary converts it (`type_shape.cpp:29-34`), and a
`satellite.container.multiple<a, b>` name accepts any arm it lists
(`type_shape.cpp:19-23`). **The tower rule is built the way `multiple` is:** a name
declared with any tower word accepts every level of the family, and a plain number
(Q24). A `satellite.variable.number` name refuses an infinity, as today it refuses a
string: `x was declared satellite.variable.number, and it holds a string
(machine_code: 27 types_do_not_meet)`.

**`.power_of()` answers a new value, the way `+` does.** It is not a mutator: `x =
x.power_of(x)` is how `x` becomes a power, and `z = x.power_of(x)` leaves `x` an
infinity (Q9).

## Every walk keeps its own stack — and 004 has none yet

A loop of `x = inf.power_of(x)` nests exponents one deeper every pass, as deep as
memory allows. Add, compare and display all walk through exponents, so each must keep
its own stack on the heap — **never a depth bound** (satellite has no limits). **004's
walker does not do this yet**: 8,000 nested `for`s crash it, and the fix is open as
MILESTONES M20.A item 2. The infinity's walks will be the first code in 004 built this
way. INF-5 is where it is tested.

---

# Part 5 — names, words and folders

## The display name and the code name

Message six: *"we just use the asat name as it was easier to program it that way isn't
it??"* — **The display is actually the easier one to program.** Its `k` is the rank
itself, a `satellite_number`, so it needs no letters and never runs out. The letters
stay where a minus sign cannot go: in a word and in a folder name. *"2 infinity-c"*,
also in message six, gave way to the author's own numbers in the same breath (*"let's
use this for displaying the numbers: "infinity-1""*).

**A level's dash touches its word; an attached number follows a comma with its own
sign.** `(infinity-1)` is a power, and `(infinity, -1)` is one infinity minus one, so
the two never meet on the screen (Q5). In a program the minus is spaced, as every
math operation in 004 already must be (*"every math operation is written with a space
on both sides"*, S110, checked on `2 ** 3`).

    NAMES AND FOLDERS
        infinity      satellite.variable.infinity satellite.infinity() /infinity/infinity.satl
        infinity-1    satellite.variable.power    satellite.power()    /infinity/power/power_object.satl
        infinity-2    satellite.variable.sat      satellite.sat()      /infinity/sat/sat_object.satl
        infinity-3    satellite.variable.asat     satellite.asat()     /infinity/asat/asat_object.satl
        infinity-4    satellite.variable.bsat     satellite.bsat()     /infinity/bsat/bsat_object.satl
        infinity-5    satellite.variable.csat     satellite.csat()     /infinity/csat/csat_object.satl
        infinity-28   satellite.variable.zsat     satellite.zsat()     /infinity/zsat/zsat_object.satl
        infinity-29   satellite.variable.aasat    satellite.aasat()    /infinity/a/aasat_object.satl
        infinity-30   satellite.variable.absat    satellite.absat()    /infinity/a/absat_object.satl
        infinity-31   satellite.variable.acsat    satellite.acsat()    /infinity/a/acsat_object.satl
        infinity-32   satellite.variable.adsat    satellite.adsat()    /infinity/a/adsat_object.satl
        infinity-54   satellite.variable.azsat    satellite.azsat()    /infinity/a/azsat_object.satl
        infinity-55   satellite.variable.basat    satellite.basat()    /infinity/b/basat_object.satl
        infinity-704  satellite.variable.zzsat    satellite.zzsat()    /infinity/z/zzsat_object.satl
        infinity-705  satellite.variable.aaasat   satellite.aaasat()   /infinity/a/a/aaasat_object.satl
        rank 10^40 is bjljjhnkklzngagyvknayfmprfjlnsat; satellite.unsat is rank 562

**The letters count like spreadsheet columns:** a … z, then aa, ab … az, ba … zz, then
aaa. That reproduces every example the author gave: asat, bsat, csat, zsat, aasat,
absat, acsat, adsat. *"at first it has 1 a, then 2, then 3"* also fits an a-run
reading (after azsat comes aaasat). **The two readings first differ at infinity-55**
(basat against aaasat), and the a-run reading cannot hold large ranks:

    CHECKS
        an a-run name at rank 10^40 would be about 3.85e+38 letters; bijective base 26: 32

The spreadsheet count is built (Q10).

**Folders** are the author's, with one rule derived for three letters and more: a
one-letter level gets its own folder (`/infinity/asat/`). From two letters on, every
letter but the last is a folder, so `aasat` is `/infinity/a/aasat_object.satl` (the
author's) and `aaasat` is `/infinity/a/a/aaasat_object.satl` (derived, Q11). No two
paths collide: letter folders are one letter long, and the one-letter levels' folders
end in `sat`. **Where `/infinity/` lives is open.** In 004 a leading slash is relative
(`include_shape.cpp:145`, *"A LEADING SLASH IS RELATIVE"*): `/infinity/x` resolves
beside the file that includes it, and 004 has no installed library folder. SAT-5
builds one, and its location is Q29.

## `satellite.aasat()` is made in constant memory

`satellite.aasat()` is the canonical `infinity-29`, a tower of thirty infinities.
Built by 29 nested `.power_of()`s it is 29 exponents deep, and for rank 10^40 it would
never finish. **The C++ holds a canonical unit as "unit k", with k a
`satellite_number`**, and expands it only when an operation needs its inside, so
every constructor costs the same. Identical exponents are shared and immutable, never
copied.

## Words — one row per spelling, and a level after it

The levels never end, and **every word today is one registry row**. Word codes run
4097 to 8191, and `words.tsv` has 384 rows since INF-1. `key_of` (`word_codes.hpp:40`) answers 0
for any path number above 255. The REGISTRY's promised fallback for a word with no
code (`word_number_token`) is defined, and nothing writes it. So:

- **three new rows**, each the first free number: `1 6 17`
  `satellite.variable.infinity`, `1 26` `satellite.infinity`, `1 26 0`
  `satellite.infinity()`. There are **no rows** for power, sat or any lettered level.
- **one lexer rule**, matching `infinity`, `power` and `<letters>sat` directly under
  `satellite` or `satellite.variable`, per spelling:

        satellite.variable.<name>     1 6 17  then a level token
        satellite.<name>              1 26    then a level token
        satellite.<name>()            1 26 0  then a level token
        (no level token)              rank 0, the infinity itself

  The level token is counted and carries the name as written. It goes **after** the
  word on purpose: a reader not yet taught the family meets an unexpected token and
  refuses loudly. A payload before the word would be skipped by the payload sweeps,
  and `satellite.aasat()` would silently run as `satellite.infinity()`.
- **`make_words.py` refuses any future row** under those two parents whose last
  segment matches the pattern. A real row would win the whole-path lookup and silently
  shadow a level.
- The pattern **reserves** every name ending in `sat`, plus `infinity` and `power`,
  under those two parents, for good: `satellite.unsat` is rank 562. No existing word
  collides (Q25).

## Methods and operators

- **`.power_of(x)`**, with **`.to_the_power_of(x)`** (message five) and **`.power(x)`**
  (message nine) as other spellings; the precedent is `contains`/`contain` (Q35). It is new, and distinct from the
  unbuilt 003 word `satellite.variable.number.power(a, b)` (`1 6 4 10`).
- **`**` as a second spelling of `^`**, right to left as `^` is (Q6). The author ruled
  on 2026-09-16 that power is `^` (REGISTRY.satellite:235), and that stays: `**` is a
  second way to write it, because the author writes `**` twice (*"power ** infinity"*,
  `my_number ** my_number ** my_number`). Today `2 ** 3` is S110, and a `for` step's
  `i ** 2` is refused by name (`program_walk.cpp:240`, *"power is written ^"*).
  MILESTONES M20.A left that refusal as a question for the author. The default
  accepts `**` there too, and rebases `tests/for_power_stars` rather than deleting it.
- **`.resize(n)`** (message 2) and **`.nines()`** (Q14).
- **A REGISTRY fix comes first:** the method family's `free` row reads
  `0000101100011110`, which is `sort_token`'s own code. The first free method code is
  `0000101100100100`. The duplicate check in `make_token_codes.py` missed this because
  it skips `free` rows, so the missing check is: **a family's `free` row must start
  above the highest code that family uses.**

---

# Part 6 — `satellite.spacesuit inf_number_object`

The author's sketch (closed here; message five's line runs on):

    satellite.spacesuit inf_number_object()
    {
        satellite.variable.infinity value = infinity
    }

He goes on: *"where infinity is the real, infinity object and we supply methods to
work with that value"*. It is what lets *"a negative infinity"* sit in a register
declared `satellite.container.list<number_object>`: a register entry that holds an
infinity instead of a number. **In the C++ engine it is simply a term whose unit is not
a plain number**; the normal form already holds numbers and infinities in one list. As
a `.satl` spacesuit it needs the SAT track (Part 7): M8's grammar, and M35's supertype
so that it can be a `number_object`.

---

# Part 7 — where the code lives: C++ first, the author's `.satl` files after

**One C++ arm holds every level**, because `std::variant` arms are fixed when the
interpreter is compiled, and the levels never end. It is **arm 12**. The header's rule
is *"Arms take their numbers in the order they are BUILT"*, and message five: *"we
are building satellite.variable.infinity FIRST"*. The float moves to 13 and hex to 14;
both exist only as comments today, but PLAN red note 9 says *"satellite_float is arm
12"* (Q26). The payload sits **behind a handle** (`std::shared_ptr<const
satellite_infinity>`). The size is not the reason: a term list held inline is a 24-byte
vector, and a `satelliteObject` is 88 today. The reasons are that a term's exponent
is itself a value, so the type is recursive; that a copy is O(1); and that identical
exponents are shared, immutable, never copied.

**DESIGN.md §10 has its own layout**, in the author's words: *"`satellite.variable.
infinity my_infinity_name = satellite.infinity.new()`, with its fast path in
`satellite/infinity.cpp` and the rest in
`satellite/infinity/special.<name>.satellite.cpp`"*. This file puts the C++ in
`satellite/satellite_variable_infinity/`, beside every other `satellite_variable_*`
type (Q28). §10's three open questions are answered here: the arithmetic (Parts 3–4),
the order (Part 3), and *"what `list.size()`, a loop bound or `sleep(infinity)` do
when handed one"* (Q27).

**This departs from the author's words, and it is Q12.** Message five: *"we write the
code for the "sat" objects IN satellite"*. The author offered C++ only for the
infinity: *"maybe you would want to program the infinity in C++?"*. Written in
satellite, a power or a sat needs things 004 does not have:

| needed | today |
|---|---|
| spacesuit grammar | **none** — PLAN M8: *"The machine is built and the grammar is not"* |
| a spacesuit that answers `+`, `-`, `*`, `/` | no operator reaches user code |
| a spacesuit that copies on `b = a` | 003 DESIGN §7.4 (`old_versions/second_satellite/DESIGN.md:1005`): *"a spacesuit is a reference type"*, so `b.resize(10)` would resize `a` too |
| a spacesuit with a supertype (`inf_number_object` as a `number_object`) | MILESTONES M35, a sketch awaiting three rulings |
| `satellite.container.list<a spacesuit>` | lists of built-in types only |
| an installed library folder | none (Part 5) |
| a file per level | the levels never end, so they cannot all be files |

So **the C++ engine is built first**: it is what makes the numbers work at all. The
`.satl` files are the **SAT track** after it: `infinity.satl`, `power_object.satl` and
`sat_object.satl`, then the lettered levels. Every lettered level follows the same
rule, so their files are **generated from one template**, the way `words.tsv` is
generated, and `check.sh` fails if one is stale. The repo carries every level the
author named — asat, bsat, csat, esat, zsat, aasat, absat, acsat, adsat — and the
generator writes any other on request (Q29).
A level with no file still works, because the engine needs none; a file adds that
level's own methods. Each file is checked equal to the C++ through the oracle.

---

# Part 8 — display

Message ten: *"we will keep it in parentheses for display, so we know that is a single
number ... when we get into infinities and powers and everything, it is still just
adding sets of parentheses"*, and *"I dunno, just make it look nice"*.

    (infinity)                              satellite.infinity()
    (2 infinity)                            infinity + infinity
    (0.5 infinity)                          infinity * 50%
    (infinity, -500000000000000000)         the register: every attached number with its sign
    (infinity, +7)                          a plus sign shows too
    (infinity, +1)                          a plain 1 prints
    (-infinity)                             the first term shows a sign only when negative
    (-infinity-1)                           a negative power: minus (infinity-1), not (-infinity, -1)
    (infinity^3)                            between the named rungs, the exponent is shown (Q5)
    (infinity-1)                            a power
    (2 infinity-1)                          1 power + 1 power
    (infinity-2, +infinity-1)               a sat with a power attached
    (infinity^(infinity, +1))               an exponent in the family is its own parentheses
    0                                       what an infinity leaves when it goes away: bare

**His rules:** one number is one set of parentheses, and every attached number carries
its sign — *"we keep track of whether that number is positive or negative"*.

**This file's, under his *"just make it look nice"*:**

- the first term shows its sign only when it is negative: `(infinity, -500000000000000)`,
  his own example, has none;
- **a count of 1 in front of an infinity is not printed** — message ten writes
  `(infinity, -500000000000000)` where message six wrote `1 infinity`, and the later
  message decides — but a plain number prints whatever it is, 1 included. A negative
  power is then `(-infinity-1)`, which reads differently from `(-infinity, -1)`,
  minus infinity minus one, only by the comma (Q36);
- **an exponent in the family is shown in its own parentheses**, `(infinity^(infinity,
  +1))`. That is this file's reading of *"it is still just adding sets of
  parentheses"*; his example straight after it, `(infinity.infinity, +90440393845)`,
  has one set (Q5);
- a plain number shows bare, so parentheses always mean the family;
- every number inside the parentheses prints exactly, all its places — a count and a
  plain term are exact by construction — while a float on its own shows
  `arguments.infinity_display` (32) places (Q43). `.nines()` shows 32 too.

**Cut or round is the author's, and it splits in two.** M11 records his words:
*"displayed as a rounded thing... we round to 32 digits"*. **The nines view is cut**
(Q17a): rounded to 32 digits, one infinity would print
`1.00000000000000000000000000000000`, the one value message one says it must never
reach. **A float rounds, half away from zero** (Q17b): that is his word, 003 DESIGN's
rule (*"Wherever the language rounds — a float's right half"*), and what 004's
percentage already does.

---

# Part 9 — `arguments.infinity`, the nines, `.resize` and `--configure`

**`arguments.infinity = 128` is built** (`239cfae`). Measured on this machine with the
`satellite_number` that exists (median of 7):

| width | add | multiply | to_text |
|---|---|---|---|
| 128 | 92 ns | 148 ns | 380 ns |
| 1024 | 172 ns | 4,306 ns | 7,384 ns |
| 4096 | 484 ns | 62,564 ns | 166,371 ns |

**What 128 is for, now that no answer depends on the nines:**

1. **the most decimal places a count may have** (Part 3). `inf / 10 ^ 200` is refused
   for it.
2. **the places a float's fraction keeps** (Part 10), unless Q18 hands that to
   `float_digits`.
3. **the nines each count is shown with by `.nines()`** — the author's encoding:
   *"1 power + 1 power = 1.9999999999999 power, so a power object has to be encoded that
   way, so that you can add, subtract, multiply and divide"*. His purpose is that the
   arithmetic works. The exact count serves that purpose, and the nines are kept to
   show it — which departs from his letter, since the nines are no longer what is
   added (Q1):

        A COUNT IN NINES, FOR .nines() (128 nines, shown cut to 32)
            count 1    0.99999999999999999999999999999999
            count 2    1.99999999999999999999999999999999
            count 0.5  0.49999999999999999999999999999999
            count 8    7.99999999999999999999999999999999
            count -1   -0.99999999999999999999999999999999
            count 2, .resize(10):     (C) 1.9999999999   (A) 1.9999999998
            count 10^-20, .resize(10): 0.000000000000000000009   (the nines follow the last digit)

   `x.nines()` shows the first term's count this way: its size minus 10^-w, the sign in
   front, where w is the nines width — widened past the count's own last digit when
   the count has as many places, so the nines always come after it. It shows
   min(w, `arguments.infinity_display`) places, cut. That is the first version's (C), so
   the nines never wear away and never reach the next whole number.

**(A) against (C) is not only a display question (Q1).** Under (A) — work it out and
cut, which is message one's `1.999…98`, and the letter of message five's *"encoded that
way, so that you can add"* — the nines ARE the count, and the count drifts:

    CHECKS
        under (A), inf * 50% * 2 == inf: False;  under (C): True

That is a wrong answer, not a different display, so (C) is the default. For (C):
message four's *"keep it at whole_number.99999999999"* and message five's *"1 power + 1
power = 1.9999999999999 power"*. Message five's *"0.99999999999999998 no matter what"*
fits neither exactly — one infinity is all nines under both.

**`.resize(n)`** (message 2) changes one infinity's nines width, and so only what
`.nines()` shows (Q31: or also the most places its counts may have). It is a mutator on
the variable's real slot. `b = a; b.resize(10)`
leaves `a` alone, because an infinity is a value, as a list is. When two widths meet
in one answer, the smaller is kept (Q19).

**`--configure` becomes a second spelling of `--config`.** Today `satl --configure`
answers *""--configure" is not a word satl takes"*. Message one's timing test is not
built: it would set a width no answer depends on, and the author set 128 by hand
(Q15). The first version's measurement still stands for anyone who builds it: time a
MULTIPLY, never an add, because add is linear and multiply quadratic.

## `arguments.infinity.counter` and the SATELLITE INFINITY WARNING

Message ten: *"when using the variable satellite.variable.infinity, we have to warn the
user after so many operations, we pause the interpreter and remind them after so many
operations... let's call it, 999,999,999 after that many operations on the same
object"*. **The row is built** (`95d00ca`): `arguments.infinity.counter`, 999,999,999,
the author's own dotted name (`arguments.memory` and `arguments.memory.total` are
already two rows). Nothing reads it yet.

**The rule, in his words and then exactly:**

- **Which objects.** *"when using the variable satellite.variable.infinity"*, and
  *"this applies to anything that never reaches the next object type"*. **Every name
  holding a value of the family has a counter** — a name declared with any tower word
  (Q24), not only `satellite.variable.infinity`. A plain number has none: when an
  infinity goes away the counter goes with it, and a name holding `0` is never told it
  will not reach infinity. A value with no name (half of a longer expression) has none,
  because the warning names the object (Q40).
- **What counts — ANSWERED 2026-09-18:** *"whenever the number changes"* (the author, on
  Q39). **A calculation whose answer comes back into the object with the same type adds
  1**: `x = x + 1`, `x = x * 2`, and `x = x * x` once `x` is a power. Reading the object
  changes nothing and counts nothing — `y = x + 1`, comparing (`x > 5`), displaying.
  `x = x + 0` counts too: the object was written by a calculation, and comparing every
  answer with the old value to see whether it moved would cost as much as the value is
  big.
- **What destroys it.** *"if the object is destroyed and turned into a different class
  object, then the counter is destroyed with it"*. An answer of a **different type**
  coming back into the object — infinity → power, or the plain number left when it goes
  away — ends the object and its counter. A new type starts a new counter at 0.
- **What happens.** *"display the warning, then reset the counter to 0"*. At
  `arguments.infinity.counter` the warning is printed and the count starts again.
  **ANSWERED 2026-09-18 (Q37):** *"It doesn't seem like to me there would ever be a good
  time to pause the script, unless we are at the prompt already, in that case it
  would"*. **A program never pauses** — it prints the warning and goes on. **At satl's
  prompt** (satl run with no file, M0.6) it waits for Enter. It goes to stderr, with
  satl's other reports, so a program's own output is not broken into (Q38, open).

**His example is the case it is for:** *"so my_inf = my_inf * my_inf never reaches two
infinities"*. His "two infinities" is message five's power, *"two separate
infinities"* — `(infinity-1)` — and the loop never reaches it (Part 4: after 99 passes it
is still below it). **But the loop's first pass does change the type.** Message five
calls infinity to the power of 2 *"a power object"*, and so does Part 4: `(infinity^2)`
is a power. So pass 1 destroys the counter, and from then on every pass stays a power
and never reaches a sat, the next type. With the row set to 3 so the trace is short:

    THE COUNTER: arguments.infinity.counter set to 3, my_inf = my_inf * my_inf
        pass 1   power    (infinity^2)                     destroyed, counter 0
        pass 2   power    (infinity^4)                     counter 1
        pass 3   power    (infinity^8)                     counter 2
        pass 4   power    (infinity^16)                    WARNING, counter 0
        pass 5   power    (infinity^32)                    counter 1
        pass 6   power    (infinity^64)                    counter 2
        pass 7   power    (infinity^128)                   WARNING, counter 0
        pass 8   power    (infinity^256)                   counter 1
        the other reading (Q42), the type changes only at a named rung: warnings at passes [3, 6]
        y = x + 1          x is infinity read, not changed: no count
        x = x * 2          x is infinity counter 1
        x = x * 2          x is infinity counter 2
        x = x * x          x is power    destroyed, counter 0
        x = x * 2          x is power    counter 1
        x = x * 2          x is power    counter 2
        x = x * 2          x is power    WARNING, counter 0
        x = x - x          x is number   destroyed; a plain number has no counter
        x = x + 1          x is number   a plain number: no counter

The other reading — the type changes only at a named rung, so `(infinity^2)` is not yet
new — warns one pass sooner, at 3 and 6 — the author confirmed the power reading (Q42). In
the second trace `y = x + 1` only reads `x` and counts nothing, `x = x * x` turns `x`
into a power and destroys the counter, and once `x = x - x` leaves a plain number there
is no counter at all.

**The warning**, from the oracle, eighty columns (the rules are exactly 80 dashes, as in
the SATELLITE CRITICAL ERROR REPORT, `critical_report.hpp`):

    --------------------------------------------------------------------------------
                               SATELLITE INFINITY WARNING
    --------------------------------------------------------------------------------

                                    OBJECT: "my_inf"
                               WILL NEVER REACH INFINITY

    --------------------------------------------------------------------------------

One empty line before it and one after. His drawing gives the first as *"(one empty
line here)"* and the last as *"(one space here)"*; the last is read as an empty line
too, not a line holding one space (Q40b). The title is centered, as he marked it. **The
other two lines are centered too**: he marked only the title *"(centered)"* and set the
other two by hand, 23 and 19 spaces in, where centered is 29–32 and 27 (Q40a). The
wording is his: *"WILL NEVER REACH INFINITY"*. In his usage "reach infinity" is reaching
the next rung — message nine asks of the same loop, which starts at infinity, *"does
my_number = my_number * my_number ever reach infinity? No! it doesn't!"* — so the words
stand as written (Q41).

---

# Part 10 — the float

Message one: *"satellite.variable.float is just two satellite.variable.numbers"*. Both
shapes below fit that. **MILESTONES M20, decided 2026-08-27:** *"a float is a bool and two
`satellite_number`s — left of the point exact and unbounded, right of it bounded,
because repeated multiplication grows digits downward"*. 003 DESIGN §8.6 writes it
`3.14 = (true, 3, 0.14)`. **That is the default here**: a sign bool, the whole part,
and the fraction held at `arguments.infinity` places. 12.05 is (12, 05000…) and 12.5
is (12, 50000…). The first version proposed `(mantissa, width)` — one number and the
count of its places — and argued against a `(whole, fraction)` that loses 12.05. M20's
shape does not lose it, because its fraction keeps a fixed number of places. Q18 asks
whether to keep M20's shape (the default), and whether `arguments.infinity` or 003's
`satellite.library.system.float_digits` (`1 14 2 4`, numbered in `words.tsv`) sets
its places.

- **The literal:** `12.34` already lexes as one number token. The change is in
  `expression.cpp`'s number arm, which refuses it today (S120).
- **Division** keeps `arguments.infinity` places, the last one rounded half away from
  zero (Q17b). The row is read, not hard-coded.
- **What the float changes** (Q13): **an answer refused today for want of a float
  becomes a float, and an answer given today keeps its answer.** `2 ^ -1` and `3 -
  50%` become `0.5` and `1.5`. `4 / 3` stays `1`: it prints `1` today by design, and the
  refusal for a touching `5/4` tells people to *"write a space on both sides for
  whole-number division"*; `4.0 / 3` is the float. M20 says the float *"retires three
  refusals"* including non-whole division, which is not a refusal. The touching `5/4`
  stays S210, because it asks for a fraction type, which is not this.
- **`.reverse()`**: `12.34.reverse()` is `34.12`, and `.reverse().reverse()` is
  `21.43` (PLAN red note 9, decided). *How* is red note 9's default (a), *"unless you
  say otherwise"*: read the chain, never a hidden mark on the number. Under (a), `y =
  x.reverse(); y.reverse()` is `12.34`, not `21.43` (Q30).
- **Arm 13**, after the infinity (Part 7). `tests/not_understood.satl` goes red when it
  lands, on purpose: it is rebased onto the next unbuilt type, never deleted (M20).

---

# Part 11 — the next phase: `infinity-infinity`, the dots, and the dashes (a sketch)

Messages seven and eight go past every `infinity-k`. **Nothing here is a milestone
yet, and nothing in this Part is a default** (Q16). It records what the author wrote,
one reading of it, and the questions to settle first.

## What the author wrote

- Message seven: *"1 infinity-infinity"*, then *"1 infinity-infinity-1 as the next
  thing, then we have infinity-infinity-infinity-1"*. The endless dash chain is shown
  as a class, and he chose the spelling: *"We will specifically write it like this: 1
  class-1-infinity then class-1-infinity-1 then class-infinity-infinity"* (over an
  alphabetical `a-class-infinity`, `b-class-infinity`). Then the dots:
  *""1 infinity.infinity" is the thing that is beyond "1 infinity-infinity""*, and
  *"we alternate between dashes and dots to display any number at all"*. Then
  *"for the largest number that we deal with, it's infinity-x and then infinity-x-x"*,
  and *"infinity.infinity-infinity-x-infinity"* must *"still come out as the correct
  number somehow, built from all the way from a float that is just
  0.9999999999..999999999"*.
- Message eight, later, gives the alternation as a sequence: *"infinity-1 for power,
  infinity-infinity then infinity.infinity-1 then infinity.infinity-infinity, then
  infinity.infinity.infinity-1"*.

Message ten shows how such a number is displayed: *"(infinity.infinity,
+90440393845)"* — one set of parentheses, the attached number with its sign, the same
as every number in Part 8.

## One reading of message eight's sequence

Read as a grammar, message eight is regular: **the dots count layers, `-k` is the k-th
rung of a layer, and `-infinity` is where a layer's rungs run out.** Layer one is Part
4's towers: `infinity-1`, `infinity-2` … and `infinity-infinity` past all of them.
Layer two does to `infinity-infinity` what layer one did to `infinity`, and so on up.
In the mathematicians' names (the arithmetic of Part 4, Conway's; the names are
Cantor's and Oswald Veblen's):

| the author's | one possible reading | what it is |
|---|---|---|
| `infinity-k` | ω^ω^…^ω, k+1 high | Part 4, built |
| `infinity-infinity` | ε₀ (Cantor, 1897) | the smallest ordinal above every `infinity-k`; the first ordinal x with infinity ** x = x |
| `infinity.infinity-1` | ε₀^ε₀ | layer two's first rung |
| `infinity.infinity-infinity` | ε₁ | the next ordinal x with infinity ** x = x |
| `infinity.infinity.infinity-1` | ε₁^ε₁ | layer three's first rung, the last the author wrote |
| (extrapolated) `infinity.infinity.infinity-infinity` | ε₂ | and one more dot per ε |
| (extrapolated) dots forever | ε_ω | |

"First" and "next" are among ordinals. Among Conway's numbers, which Part 4 uses, there
are more fixed points in between (ε_{1/2} lies between ε₀ and ε₁); the table uses the
ordinal ones.

**One fact shapes the phase:** infinity raised to `infinity-infinity` is
`infinity-infinity` again. At ε₀ the tower stops climbing — which is what message
eight's loop runs into one rung lower — and so a new symbol is needed to go on. The
author's switch from dashes to dots is that symbol.

**The same reading applied to message seven** gives different values. Part 4's dash
means one tower up, so `infinity-infinity-1` would be ε₀^ε₀ (message eight's
`infinity.infinity-1`), and not ε₁. Messages seven and eight name some of the same
numbers two ways, and the author should say which spelling stays.

**Beyond the dots.** Past ε_ω the ladder goes on. Veblen (1908) listed the fixed
points of each ladder as a new ladder: ζ₀ is where the ε-numbers' own index catches up
with itself. φ_ω(0) is where a chain of such ladders runs out, and Γ₀ (Feferman and
Schütte, 1960s) is where the NUMBER of ladders is itself written with the ladders.
*"THEN we REWORK IN the dash system"* reads naturally as that kind of feeding back.

## Can one system count to anything at all?

It can always be extended: past any rung there is a next one, and you can add a
symbol for it — the author's dash → dot → dash move. **No single system names every
size.** Every name is a finite string, and strings can be listed, so a system names
countably many sizes. Any countable collection of these sizes has a size past all of
them, which that system cannot name. So *"count to anything"* is honest as **"there
is always one more symbol"**, never as one system that finishes the job.

## Questions before any of this is built (Q16)

- **Which spelling stays**: message seven's `class-1-infinity` for the endless dash
  chain, or message eight's dots, or both for different things.
- **What `x` is** in `infinity-x`, `infinity-x-x`. The author put it at the top: *"for
  the largest number that we deal with"*. Two readings: a placeholder for any rung,
  or a symbol above everything named so far.
- **What a dot means where a dot already means something.** `.` separates a word's
  path (`satellite.variable`) and calls a method (`x.power_of`). Is
  `infinity.infinity` shown on the screen only, or typed in a program too?
- **The stored form.** Message seven: *"we STORE the code as an std::string"*. Message
  eight, later: *"You can program that to display the numbers, however, the numbers must
  be coded as different things"* — which may already separate the display string from
  what is stored. The proposal is to store a tree (which symbol, applied to what) and print the string,
  with the string reading back to the same tree. The reasons: parsing a string on
  every `+` is slow, and two spellings of one number must compare equal. That is a
  proposal against his explicit words, and it is his call.
- **How a program asks for them.** Message eight's `satellite.infinity(x)` reaches
  every rung of Part 4 (Q27a). Past `infinity-infinity` no nesting of it arrives, so
  each new symbol needs its own constructor.

---

# Part 12 — the author's questions, each with what is built if he says nothing

| | question | built unless told otherwise |
|---|---|---|
| **Q1** | A count held exact with its nines derived ((C): count 2 shows `1.999…99`), or the nines as the count ((A): `1.999…98`, message one)? Under (A), `inf * 50% * 2 == inf` is false. | **(C)** |
| **Q2** | `inf - inf`, `(inf + 5) - inf`, `inf / inf`, `inf * 0`: the number that is left (`0`, `5`, `1`, `0`), or an ERROR, as the first version had it on M11's summary "an ERROR, never a guess"? | **the number that is left** — 09-16: *"we answer what we can"*; M11: *"unless it goes away"* |
| **Q3** | `inf / 3`: refused, or a count held as an exact fraction and shown cut? (Cutting the count is ruled out: wrong by an infinite amount.) | **refused** |
| **Q4** | The type is how high the exponent tower goes, so `inf ** inf ** inf` is `(infinity-2)` and every sat beats every power — overruling the reading of *"MORE power just becomes adding an infinity onto value"* that keeps a taller tower a power? And did message six's *"infinity * infinity * infinity"* mean `**`? (`*` gives `(infinity^3)`; `**` gives `(infinity-2)`, his "1 sat".) | **yes** — message six's `**` example |
| **Q5** | Between the named rungs, show the exponent (`(infinity^3)`, `(infinity^(2 infinity))`), and an exponent in the family in its own parentheses, as this file reads *"it is still just adding sets of parentheses"* (his example after it, `(infinity.infinity, +90440393845)`, has one set)? | **yes, nested** |
| **Q6** | `**` as a second spelling of `^` (09-16's ruling that power is `^` stands), right to left, and accepted in a `for` step too (M20.A's open question)? | **yes** |
| **Q7** | `(inf + 5).power_of(inf)`: an ERROR, rather than keeping the `+5` attached (a wrong number)? | **ERROR** |
| **Q8** | The register as an amount per unit (combined), not a log of every number typed? | **combined** |
| **Q9** | `.power_of()` answers a new value (`x = x.power_of(x)` turns `x` into a power); it does not change `x` by itself? | **a new value** |
| **Q10** | After azsat: basat (spreadsheet), or aaasat (an a-run)? | **basat** |
| **Q11** | Three letters and more: `/infinity/a/a/aaasat_object.satl`? | **yes** |
| **Q12** | The C++ engine first, and the `.satl` files as the SAT track after it? | **yes** |
| **Q13** | When the float lands, an answer refused today for want of a float becomes one (`2 ^ -1` is `0.5`, `3 - 50%` is `1.5`), and an answer given today keeps it (`4 / 3` stays `1`; `4.0 / 3` is the float); a touching `5/4` stays S210? | **yes** |
| **Q14** | `x.nines()` shows the first term's count in nines, to min(its nines width, `arguments.infinity_display`) places? (The name is this file's.) | **yes** |
| **Q15** | `--configure` becomes a second spelling of `--config`, with no timing test? | **yes** |
| **Q16** | The next phase (Part 11): which spelling, what `x` is, what a dot means, the stored form. | **nothing built until answered** |
| **Q17a** | The nines view cut, where M11 has *"we round to 32 digits"*? Rounded, one infinity shows as `1.000…`. | **cut** |
| **Q17b** | A float's digits — shown, and the last one held — round half away from zero, per M11's *"we round"*, 003 DESIGN and 004's percentage? | **round** |
| **Q18** | The float as M20's 08-27 shape — a bool, the whole part, the fraction at a fixed number of places — rather than `(mantissa, width)`? And its places set by `arguments.infinity` rather than `float_digits` (`1 14 2 4`)? | **M20's shape; `arguments.infinity`** — message five: *"set arguments.infinity = 128 for precision"* |
| **Q19** | Two nines widths meeting in one answer keep the smaller (the first version's Q5)? | **the smaller** |
| **Q20** | `-infinity` and `infinity * -1` are one value, reading message five's *"a single object, that is either positive or negative"* as the answer M11 left open? | **yes** |
| **Q21** | A positive infinity is larger than every number whatever is attached, and a negative one smaller? | **yes** |
| **Q22** | Until FLT-3, a non-whole number left when an infinity goes away (`1.5`) is refused, as `3 - 50%` is today; from FLT-3 it is a float? | **yes** |
| **Q23** | A percentage that came from rounding (`100% / 3`) times an infinity: taken as the decimal it is (`inf * (100% / 3) * 3 - inf` is −10^-34 infinities), or refused? | **taken as it is** |
| **Q24** | A name declared with any tower word accepts every level and a plain number (like `multiple`), so the declared level is a synonym, not a bound? | **yes** |
| **Q25** | Reserve every name ending in `sat`, plus `infinity` and `power`, under `satellite` and `satellite.variable`? | **yes** |
| **Q26** | The infinity takes arm 12, the float 13 and hex 14 (red note 9 says the float is 12)? | **yes** |
| **Q27** | An infinity handed to a word that wants a whole number (a loop count, `list.size()`, `sleep`, an index): refused by name. Comparing one is fine: `while (count < inf)` runs. | **refused, except comparing** |
| **Q27a** | `satellite.infinity(x)` is infinity ** x (message eight)? | **yes** |
| **Q28** | The C++ in `satellite/satellite_variable_infinity/`, not DESIGN §10's `satellite/infinity.cpp` and `special.<name>.satellite.cpp`? | **yes** |
| **Q29** | The SAT track: where the installed `/infinity/` library lives, and which lettered files the repo carries? | **every level the author named; location open** |
| **Q30** | A float's `.reverse().reverse()` read from the chain (red note 9's default (a)), so `y = x.reverse(); y.reverse()` is `12.34`? | **yes** |
| **Q31** | `.resize(n)` sets that infinity's nines width only, or also the most places its counts may have? | **nines width only** |
| **Q32** | Divide by a sum when every exponent is finite and it divides exactly (`(inf * inf - 1) / (inf - 1)` is `(infinity, +1)`)? | **yes** |
| **Q33** | Until INF-8, refuse any answer with an exponent that is negative or between 0 and 1, judged on the answer at every depth? | **yes** |
| **Q34** | INF-8, from message nine's *".power(-infinity)"*: build the infinitely small numbers? What are they called and shown as (`(5 infinity^-1)`?), is `infinity^0.5` one of them, and where is an endless series (`(n * n + 1) / (n * n - n)`) cut — after how many terms, or at what exponent? | **build them, shown with `^`; the cut is his to set** |
| **Q35** | `.power(x)` (message nine) as a third spelling of `.power_of(x)`? | **yes** |
| **Q36** | A count of 1 in front of an infinity is not printed — `(infinity, -500000000000000)` as message ten writes it, not `(1 infinity, …)` as message six's "1 infinity" would — so a negative power is `(-infinity-1)`, beside `(-infinity, -1)`? Or print the count when a sign touches a rung (`(-1 infinity-1)`)? | **ANSWERED 2026-09-18: not printed** — *"q36 looks right to me"* |
| **Q37** | *"we pause the interpreter and remind them"*: when does it wait for Enter? | **ANSWERED 2026-09-18: only at satl's prompt; a program never pauses** — *"there would ever be a good time to pause the script, unless we are at the prompt already"* |
| **Q38** | *"we display a warning"*: to stderr, with satl's other reports, or to stdout, where `satellite.console.display` writes? | **stderr** |
| **Q39** | What counts: one per calculation the object survives, or every use? | **ANSWERED 2026-09-18: "whenever the number changes"** — a calculation written back into the object with the same type; reading it counts nothing |
| **Q40** | Which objects: every name holding a value of the family (*"anything that never reaches the next object type"*), or only names declared `satellite.variable.infinity` (*"when using the variable satellite.variable.infinity"*)? A plain number and a value with no name have none. | **every name holding the family** |
| **Q40a** | The OBJECT and WILL NEVER lines centered like the title, or at his hand-set 23 and 19 spaces? | **ANSWERED 2026-09-18: centered** — *"yes it's all centered"* |
| **Q40b** | The warning's last line, his *"(one space here)"*: an empty line, like his *"(one empty line here)"* at the top, or a line holding one space? | **an empty line** |
| **Q41** | The wording *"WILL NEVER REACH INFINITY"* as he wrote it (his "reach infinity" is the next rung, message nine), or naming the rung (`WILL NEVER REACH INFINITY-2`)? | **his wording** |
| **Q42** | For the counter, is `(infinity^2)` already the next type — a power, as message five's *"infinity to the power of 2 ... a power object"* says (his loop warns at passes 4 and 7 with the row at 3) — or does the type change only at a named rung (warnings at 3 and 6)? | **ANSWERED 2026-09-18: the power** — *"looks like you have that one correct"* |
| **Q43** | Inside the parentheses every number prints exactly, all its places; a float on its own shows `arguments.infinity_display` places? | **yes** |

**Not questions — decided by the author:** the width is 128 (`239cfae`); a sign as a
bool (09-17); `satellite.infinity()` rather than `.new()`, and `satellite.aasat()` for
the levels (message five); the rungs shown as `infinity-1`, `infinity-2` (message six),
and one number in one set of parentheses, every attached number with its sign
(message ten); the warning's title and its two lines as he drew them, the 80-dash
rules, the name in quotes, and `arguments.infinity.counter` at 999,999,999, reset to 0
after each warning and destroyed with its object (message ten, the row built
`95d00ca`) — where the warning goes, the pause, its layout and its last line are Q37–Q41; `12.34.reverse()` is `34.12` and twice is `21.43` (red note 9).

---

# Part 13 — milestones

Each milestone ends with a program whose output is checked, and each answer is
checked against `infinity_oracle.py`. **"Prints the oracle's table"** means the value
column matches, and an ERROR row is refused with a named reason.

**INF-0 — the oracle.** `satellite/satellite_variable_infinity/infinity_oracle.py`.
**Built with this file.**

**INF-1 — groundwork. BUILT, `6408294`** (the author, 2026-09-18: *"let's do INF-1 then"*). The
REGISTRY's method-family `free` row is fixed, and `make_token_codes.py` now refuses a
`free` row that starts on or runs over a used code — it caught the old row, and one
more: the catch-all "unclaimed" range ran over `wide_token`, and is split. The method
tokens `power_of` (spelled `power_of`, `to_the_power_of` and `power`), `resize` and
`nines` exist, and are refused by name on every type until INF-2. A spaced `**` lexes
to the power token itself, so it answers and groups exactly as `^` (`2 ** 3 ** 2` is
`512`), in a `for` step too; `tests/for_power_stars` runs from 2; a touching `**` is
refused by name, in and out of a `for`, with the caret on it. The words `1 6 17`
`satellite.variable.infinity`, `1 26` `satellite.infinity` and `1 26 0`
`satellite.infinity()` are appended (384 words, codes 4478–4480).
`satellite.variable.infinity x = satellite.infinity()` is no longer *"satellite.variable
is not a call"*: it is refused by name as a declaration that is not built yet — S110,
as `satellite.variable.float` is; whether a numbered but unbuilt type should be S210
instead is open, and would move `tests/not_understood.satl` with it.
`satellite.infinity()` alone is S210, *"has no library built for it yet"*.

Built with it, from its review: the method names come from ONE table, generated from
the registry (`token::method_name_of`), where three hand-kept copies fell through to
*"that method"*; a method a type lacks is refused with whose it is — *"so far it is a
file's"*, *"a container's"*, or *"no type has it"*; and check.sh now reruns both
bytecode generators in a copy of the tree and compares their headers byte for byte.

**INF-2 — arm 12, the constructor, the display, the order.** In
`satellite/satellite_variable_infinity/`, beside the oracle. The arm is a handle to an
immutable term list. A count is an exact decimal in the float's shape, and each value
carries a nines width. The comparator — the sign of the first term of `a - b` — is
built here, whole, together with the checker rule for `satellite.variable.infinity`.
The arm comment in `satellite_object.hpp` moves the float to 13 and hex to 14. *Done
when* `satellite.console.display(satellite.infinity())` prints `(infinity)`;
`satellite.infinity() > 10 ^ 100`, `5 < satellite.infinity()` and
`satellite.infinity() == satellite.infinity()` are true; a `satellite.variable.number`
name refuses an infinity; and `.reverse()` and `number(satellite.infinity())` are
refused by name.

**INF-3 — the register.** `+` and `-` between any two values of the family, and
against numbers and percentages; `*` and `/` by numbers and percentages; like terms
combine; a count that never ends, or needs more than `arguments.infinity` places, is
refused; an infinity goes away when nothing infinite is left (Q2), and a non-whole
leftover is refused until FLT-3 (Q22); an infinity handed to a word that wants a whole
number is refused by name (Q27). **Nothing on this path may hold a copy of a Value.**
*Done when* the oracle's THE INFINITY table prints; DIVIDING prints its first four
rows; WHEN THE INFINITY GOES AWAY prints its rows without `/ inf`, with the last
refused until FLT-3; A ROUNDED PERCENTAGE GOING IN prints; `a[satellite.infinity()]` is
refused by name; a list of infinity-family values sorts by value; and **a growth
check** passes: `x = x - 1` 10^6 times against 10^5 times takes about ten times as
long (check.sh times it the way it times the append ratio), and the peak memory
`/usr/bin/time -f %M` reports is within 1.5 times at the two sizes.

**INF-4 — products, whole powers, and dividing by an infinity.** Every term times
every term; `.power_of()` with a whole exponent; division by one term, and by a sum
when every exponent is finite and it divides exactly. *Done when* the oracle's THE
POWER table prints its first four rows and message eight's 99-pass loop; DIVIDING
prints its last two rows; and `inf / inf` is `1`.

**INF-5 — infinite exponents, the type read from the tower, `satellite.infinity(x)`,
and walks that keep their own stack.** The first code in 004 that walks without
recursing through C++ (Part 4). *Done when* the oracle's THE POWER table prints
entire, and SAT AND ABOVE prints every row whose only family constructor is
`satellite.infinity()`: `inf ** inf ** inf`, `(inf ** inf) ** inf`, `p.power_of(p)`,
`inf ** inf ** inf ** inf`, and the 300-pass row. From `satellite.variable.infinity x
= satellite.infinity()`, 300 passes of `x = satellite.infinity().power_of(x)` print
`(infinity-300)`, and **100,000 passes print `(infinity-100000)` rather than crashing**
(the oracle is checked at 300; the C++ at 100,000). After `z = x.power_of(x)`, `x`
still displays `(infinity)`.

**INF-6 — the family words.** The lexer pattern, the level token after each spelling,
names ↔ rank in both directions, canonical units in constant memory, the
`make_words.py` guard, and the checker rule for every tower word (Q24). *Done when* SAT
AND ABOVE prints the rest — every row that names `satellite.power()`,
`satellite.sat()` or a lettered level; `satellite.variable.aasat my_aasat = satellite.aasat()` displays
`(infinity-29)`; a rank-10^40 constructor runs in the same time and memory as
`satellite.power()`; and a hand-typed row `1 6 18 satellite.variable.power` in
`words_004.tsv` is refused.

**INF-7 — `.resize`, `.nines`, and `--configure`.** *Done when* the oracle's A COUNT
IN NINES table prints: `x = inf + inf; x.resize(10); x.nines()` prints `1.9999999999`
(it would be `1.9999999998` under (A)); `(inf / 10 ^ 20).resize(10)` shows its nines
after its last digit; `(inf * -1).nines()` prints `-0.99999999999999999999999999999999`.
After `b = a; b.resize(10)`, `a.nines()` still shows 32 nines; after `a.resize(10)`,
`(a + b).nines()` shows 10 places (Q19); and `satl --configure` does what `satl
--config` does.

**FLT-1 — the float arm (13) and its literal**, in M20's shape (Q18).
`tests/not_understood.satl` is rebased onto the next unbuilt type, never deleted.
*Done when* `12.34` displays `12.34` and `12.05 == 12.5` is false.

**FLT-2 — float arithmetic.** Division keeps `arguments.infinity` places, read from
the row, the last one rounded (Q17b). *Done when* `2.0 / 3` shows `…667` (it would
be `…666` if Q17b says cut); the same program with `arguments.infinity` set to 10 holds
10 places; `4 / 3` still prints `1`; `2 ^ -1` prints `0.5` and `3 - 50%` prints `1.5`;
and a touching `5/4` is still S210.

**FLT-3 — floats inside an infinity.** *Done when* `inf * 0.5` prints `(0.5 infinity)`,
`inf + 0.25` prints `(infinity, +0.25)`, and `(inf + 6) / 4 - inf / 4` prints `1.5`.

**FLT-4 — a float's `.reverse()`** (red note 9). *Done when* `12.34.reverse()` is
`34.12`, `12.34.reverse().reverse()` is `21.43`, and `y = 12.34.reverse();
y.reverse()` is `12.34` (Q30).

**INF-8 — the infinitely small numbers** (message nine, Q34). An exponent that is
negative or between 0 and 1 stops being refused: `5 / inf` is `(5 infinity^-1)`, and
`inf.power_of(inf * -1)` is `(infinity^(-infinity))`, below every positive number and
above 0. They compare, add and multiply by the same rules, and a tower-declared name
holds them. The oracle is extended first: its display checks for a named rung only when
an exponent is at least 1, and an infinitely small value gets a type of its own, for the
checker and for the counter (Q34 names it). An endless series stays refused until Q34
sets where it is cut. *Done
when* `5 / inf > 0`, `5 / inf < 1` and `inf.power_of(inf * -1) < 5 / inf` are true, and
`(inf + 5) * (1 / inf)` prints `(1, +5 infinity^-1)`.

**INF-9 — the counter and the SATELLITE INFINITY WARNING** (message ten). A counter in
each variable's slot, beside the value and never inside it — the value is immutable
and shared, and **nothing on the mutating path may hold a copy of a Value** — so one
increment per calculation costs no copy. The warning is rendered beside
`critical_report.hpp`, from its 80-column rule. *Done when*, with
`arguments.infinity.counter` set to 3, the oracle's THE COUNTER trace happens: the
warning for `my_inf` on stderr at passes 4 and 7 and at no other pass (none at pass 3,
where it would come had pass 1 counted), byte for byte as `infinity_warning('my_inf')`
returns it; in the second trace `y = x + 1` counts nothing, one warning comes, for
`x`, on the third `x = x * 2` after `x = x * x`, and there is no counter once `x` is a
plain number; a program goes on after the warning without waiting, and satl's prompt
waits for Enter (Q37); and at the default, 999,999,999, a short program prints no
warning at all.

**SAT-1 … SAT-6 — the author's `.satl` track**, after Q12. SAT-1 is M8 (the spacesuit
grammar) with M35's supertype. SAT-2 lets a spacesuit answer `+ - * /` and
`.power_of`. SAT-3 gives a number-like spacesuit value semantics. SAT-4 is
`satellite.container.list<a spacesuit>`. SAT-5 is an installed library folder (Q29).
SAT-6 writes `infinity.satl`, `power_object.satl` and `sat_object.satl`, plus the
lettered levels generated from one template. Each is checked equal to the C++ through
the oracle.

**Order:** INF-0 → INF-1 → INF-2 → INF-3 → INF-4 → INF-5 → INF-6 → INF-7 is the
infinity, and needs no float. INF-8 comes after INF-5, once Q34 is answered. INF-9
comes after INF-5, because its trace needs a promotion. FLT-1 →
FLT-2 go any time after INF-2. FLT-3 needs FLT-2 and INF-3. FLT-4 needs FLT-1. The SAT
track waits for Q12, and starts with M8 and M35. **Part 11 waits for Q16.**

**Owed elsewhere, not changed here** (plan documents and code comments; the author's to
schedule, or done by the milestone named):

- MILESTONES M11: *"the default is 4096"*, *"Neither row is added yet"*, *"Depends on
  M20"*, and *"DISPLAYED rounded to 32"* (pending Q17a/Q17b). MILESTONES' decisions
  table, row D11.1, and PROGRESS.md's D11.1 say the same (4096, `infinityx2`, rounded).
- MILESTONES M20: *"non-whole division"* is listed as a refusal the float retires, and
  it prints `1` today; *"`5/4` the fraction"* stays S210 (Q13); the float's shape is
  pending Q18.
- PLAN M11: `satellite.infinity.new()` in `satellite/infinity.cpp`. PLAN red note 9:
  *"`satellite_float` is arm 12"* (Q26), and *"`4 / 3` still answers "not a whole
  number…""* (it prints `1`).
- DESIGN §10: the `.new()` spelling, the layout (Q28), and three questions this file
  answers.
- `satellite_config.hpp:77-80` and `arguments.cpp:186-191`: comments on `infinityx2`
  and *"a multiplier with no digits cannot hold x2"* — at INF-2.
- `satellite_object.hpp`'s arm comment — at INF-2. `tests/not_understood.satl` — at
  FLT-1. `tests/for_power_stars` — at INF-1.
