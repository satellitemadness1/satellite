# NEW ERROR LIST — 2026-09-25

What is still wrong after the error sweep of 2026-09-25, and what each one needs.
Every behaviour below was run today on BUILD 0072 or 0073 (`~/.satl/satl`), not taken from an
older note.

- **Part A** needs your decision. I can build any of them once you pick.
- **Part B** I can fix without you. I didn't do them today because each is bigger or
  riskier than what I did.
- **Part C** is internal (the build, the test harness, the number engine). Nobody writing a program sees these.
- **Part D** is where your five items in `errors.md` stand.

What I **did** fix is at the bottom, so you know what changed under you.

> **ONE THING I BROKE IN YOUR SETTINGS -- PUT BACK, AT YOUR WORD.** A reviewer agent I ran
> on these changes ran check.sh's fixtures with your real HOME, and one of them
> (`build/arguments_access.satl`) turned the access switch off in `~/.satl/config.ini`.
> You told me to put it back, and it is: `access = true` and `features = b00000000000001`,
> the file byte for byte as it was at 17:33. `features` is satl's register of its 14
> switches, one digit each; the rightmost digit is access, so turning access off had
> cleared it too. The website's `access_switch.satl` example passes again.

---

## Your answers (2026-09-25, evening), and where each one is

| item | your answer | state |
|---|---|---|
| A1 | a string + a number converts the number | **built**, every kind of number; a number first still adds |
| A2 | threads / thread = what satl may create; machine.thread(s) = hardware threads | **built** |
| A3 | the build number in an untracked file, counted on this machine only | **built**: `.satellite_counts_builds` |
| A4 | accept anything valid across any number of lines, strings included | queued |
| A5 | refuse characters that have no meaning | **built**, by name |
| A6 | refuse an unknown escape; `\\` writes a backslash | **built** |
| A7 | main must end with satellite.return(satellite); it quits from anywhere | queued |
| A8 | deep recursion must not crash: a counter, and special code past ~100,000 | queued |
| A9 | an unknown config.ini row gets its own S-code | **built**: S016 CONFIG_ROW_NOT_UNDERSTOOD, a notice naming each row and its line |
| A10 | the edits were your categories and tags: keep them, add a gap under code, publish | in progress |
| new | str.upper()/uppercase()/up(), str.lower()/lowercase() | queued |
| new | satellite.console.display(...).center() | queued |

---

## Part A — needs your decision

### A1. `"n = " + 4` is refused (from your errors.md)

```
satellite.console.display("n = " + 4)
S301: TYPES_DO_NOT_MEET
satl(run): + was given a string and a number, and satellite converts nothing on its own
```

003 joined it after your ruling on 2026-09-12 ("just do it all for them!"):
`"n = " + 4` gave `n = 4`, `4 + "2"` gave 6, `4 + "abc"` gave `4abc` (the left side
decides). 004 never got that rule, and its S301 sentence says the opposite ("converts
nothing on its own"). The website pages teach `.string`, which works either way.

- **Option 1:** port 003's rule to 004. It's one operator's arms, and 003's tests carry over.
- **Option 2:** keep 004's rule, and take this line out of errors.md.

### A2. `arguments.threads` and `arguments.machine.threads` answer different things

In the same program on this machine: `arguments.threads` = **379,923** (how many threads
satl may create, read from /proc) and `arguments.machine.threads` = **24** (the
processor's hardware threads). Your brief says *"arguments.threads = how many threads the
interpreter is allowed to create"* and that the long name is the word, with the short one as
its alias. So the two should agree, and it's unclear which number you want. (The cores pair had the
same split, and your brief settled that one at 12. It's fixed; see the bottom.)

- **Option 1:** both mean "allowed to create" (379,923).
- **Option 2:** both mean "hardware threads" (24), and "allowed" gets another name.

### A3. `git pull` refuses after any `make`

Every `make` rewrites the build row in `satellite/config/satellite_config.hpp`, a
tracked file, and almost every commit changes that row too. So anyone who cloned from
GitHub and built once can't `git pull` without `git restore` first. The website's INSTALL
page works around it with four commands. With the ad campaign sending people to clone,
this is the first thing to break for them.

- **Option 1:** keep the build count in an untracked file (build number = committed
  base + local count). make still counts; pull stops conflicting.
- **Option 2:** leave it, and keep the four-command update on the site.

### A4. A statement spread over several lines

Your 003 programs in ~/code/satl split statements across lines in 258 places. In 004:

- a **capsule header** split over lines works (`add(satellite.variable.number a,` ↵ `b)`);
- a **call** split over lines is refused: `show({` ↵ `"one",` ↵ `})` gives
  `show( is never closed on its line`.

Should 004 accept a call (or a list) that spans lines, the way 003 did?

### A5. A character with no meaning, outside a string, is accepted silently

`é` alone on a line inside main runs, exit 0, no word (already in ERROR.md). The same goes
for a no-break space pasted as indentation.

- **Option 1:** treat the Unicode spaces as whitespace, and refuse every other such
  character by name ("U+00E9 has no meaning in a program").
- **Option 2:** refuse all of them.

### A6. An escape satellite doesn't know keeps its backslash

`"a\qb"` prints `a\qb`. The six escapes (`\" \\ \n \t \r \'`) work. Refuse an unknown one,
or keep it as written (today's behaviour, ERROR #16)?

### A7. Where `satellite.return(satellite)` goes in the first program

`satl --help` shows the first program with `satellite.return(satellite)` **after** main's
closing `}`. The `satellite.return` help topic and the website put it as the **last line
inside** main. Both run. A newcomer sees both. Which one should the help show?

### A8. Recursion in the middle of a capsule (from your errors.md)

errors.md says a capsule calling itself "crashes at about 5,000 deep" and the fix is "the
walker keeping its own stack". Since your ruling of 2026-09-22, a last-line self-call is a
loop that never grows, and a middle self-call crashes past the raised stack **on purpose**.
Measured today: 20,000 deep in the middle runs fine (exit 0). If the ruling stands,
errors.md's line is out of date. If you want the middle case unlimited, it's the
heap-stack rewrite.

### A9. `config.ini` ignores a row it doesn't understand

```
no_such_row = 5
threads_startup = banana
```

Both are ignored without a word, and the program runs (help writers' finding #7, still
true). A misspelled setting silently does nothing. The fix is a notice at start-up
naming the row; it needs a new S-code from you. SATELLITE_ERROR.md owns the numbers,
and S010–S015 are taken.

### A10. The website pages still say "there are no escapes"

Three pages (FULL REFERENCE, STRINGS, DISPLAYING TEXT) say a backslash is kept as
written. That stopped being true when escapes landed on the evening of 2026-09-24. I fixed
the sources in `~/.config/satellite-foundation/site/`, and every example on those three
pages passes. **I didn't publish**, because six posts on the live site were modified
on 2026-09-24 between 21:13 and 21:40 GMT, hours after my last upload, and
`publish.sh` re-uploads every page (a partial upload re-dates posts and breaks their
links). If those edits were yours, publishing would overwrite them. Were they yours? If
not, or if you don't mind, say "publish" and I will.

---

## Part B — I can fix these; not done today

### B1. Type refusals at run time are one line, not a report

```
satellite.variable.number n = "abc"
[satellite] satl(run): n was declared satellite.variable.number, and it holds a string (machine_code: 27 types_do_not_meet)
```

Every other refusal is the full report with an S-code, the line and a caret. This one
(and "takes 1 argument, and was given 2", and a few more in `program_walk.cpp`) comes
through `report_error`, not `raise_at`. It's mechanical but touches several call sites,
and check.sh rows read those sentences.

### B2. A wrong-type literal is refused only when its line runs

In `takes("text")` for a number parameter, or `satellite.variable.number n = "abc"`, the
lines above print first, then the refusal comes. The checker could refuse a literal of
the wrong type before anything runs. That's new checker work.

### B3. A missing `)` is still refused only when its line runs

`satellite.console.display("hello"` now says `satellite.console.display's ( is never
closed -- the line ends before its )`. That's the right sentence, but the lines above it
print first. Refusing it before anything runs means deciding that a call must close on its
line, which is A4.

### B4. String methods (M16, the next milestone)

`s.upper()` says `upper is not one of its methods -- only an object of a
satellite.spacesuit has capsules to call`. That's wrong: upper *is* a string method, just
not built yet, and it should say S210 NOT_BUILT_YET the way `n.power(2)` does. M16 builds
them, which makes this go away.

### B5. 003's spellings get a sentence about something else

`satellite.machine.cores()` gives `machine has no satellite.variable line declaring it`.
It could say 004 spells it `arguments.machine.cores`. The same applies to other 003-only spellings.

### B6. `==` on two windows

The help writers suspected it's refused. It wasn't tested today, because window tests need the
headless compositor.

---

## Part C — internal, nobody writing a program sees these

- **The prompt's pty check** (`satellite/prompt/check_prompt.py`) fails a different
  check now and then under load. That's harness timing, not the prompt (ERROR.md).
- **ERROR.md's older items**, not re-verified today: the prototype runner (#4, #5, #6,
  #9–#12, #16, #17), the test harnesses (#22–#24), satellite_number's internals (#28–#32)
  and the toolchain (#33, #34).

---

## Part D — your errors.md, item by item

| your line | now |
|---|---|
| return inside an if doesn't end the capsule; inside a loop it ends that pass | **fixed** (since 2026-09-22's spacesuit work): return ends the capsule from any depth. Measured today inside an if, a for and a while. |
| a capsule calling itself crashes at ~5,000 | **your ruling of 2026-09-22 covers it**: see A8. 20,000 deep runs today. |
| `satellite.include(./x)` without quotes includes nothing, silently | **fixed today**: `./x`, `../x` and `./a/../b` include the file, and an include that names nothing is refused. |
| a statement outside every capsule is silently skipped | **fixed today**: refused before anything runs, "this line is outside every capsule". |
| `"n = " + 4` is refused | **needs you**: A1. |

---

## What I fixed today (BUILD 0073, check.sh 908 of 908, check_install.sh 20 of 20)

Your 400 programs in ~/code/satl gave **the same exit code** on the old and new satl, all
400. One printed a different message: `quad/quad_main.satl` line 6 is missing its
closing quote, and it is now told that (it used to be told to check its math signs).

1. **A file saved on Windows runs.** `\r\n` and a lone `\r` end a line. Every such file
   was refused at its first capsule over an invisible character.
2. **Unquoted `./` and `../` includes work**, like the quoted ones.
3. **An include that names no file is refused** (`satellite.include(42)`).
4. **A line outside every capsule is refused before anything runs**: a display, a call,
   a variable, `n = 5`. (Your rule: no globals.)
5. **`satellite.main()` without `satellite.capsule`** is S102, with the line to write.
6. **A missing closing `}` is refused.** It used to run and exit 0. If a list's `{` is the one left
   open (`display({1, 2)`), the list is blamed, before anything runs.
7. **An extra `}` is refused.** It used to run and exit 0.
8. **A string with no closing quote is refused before anything runs.** It used to be refused
   only when run, after the lines above had printed, and without a `(` on the line it wasn't
   refused at all.
9. **A misspelled word gets "did you mean"**: `satellite.console.dispaly` gives
   `satellite.console has no word named dispaly -- did you mean satellite.console.display?`
10. **A missing `)` is named as a missing `)`**, not "check every math sign".
11. **Habits from other languages get told what to write here**: `print(...)` → use
    satellite.console.display; `'hello'` → double quotes; a trailing `;` → not needed.
12. **A whole number handed to a float parameter is a float**, as `float f = 3` is 3.0.
    It used to be refused.
13. **`arguments.cores` = `arguments.machine.cores` = 12** (physical cores, your brief).
    The alias said 24.
14. **The build now rebuilds the libraries when `machine_facts.hpp` changes.** It didn't,
    so the cores fix first built "successfully" and changed nothing. Four more headers were
    missing from that list.
15. **`satellite.help()` works in an installed satl.** The installer never copied the
    help files, so every installed satl said "the help files are not beside satl".
16. **`satellite.console.width()`'s refusal says what to write.** It read like a false
    accusation.
17. **README.md's "Where it is today" was wrong** ("revision 02", "not installed by
    make", "satl on a PATH is 003's"). It now says what runs and how to install. That's
    the page GitHub shows your ad visitors first.

A fresh reviewer then attacked these changes. It found three problems, and I fixed all
three before committing:

- A missing `}` together with a second mistake (say, an unknown spacesuit type) crashed the
  checker. It now gives a clean refusal.
- `#!/usr/bin/env satl` as a first line had become refused. A shebang is allowed again.
- A raw carriage return inside a string had started splitting the line. A lone `\r` ends
  a line only in a file that has no `\n` at all.

Things I chose that you may want to change:

- the exact sentences (each is in one place);
- a `{ }` block at the top of a file is still allowed;
- a shebang first line is allowed;
- a line starting with `#` or `/*` at the top is refused with "satellite's comments
  begin with //";
- a line starting with a character that has no meaning is still stepped over (A5).
