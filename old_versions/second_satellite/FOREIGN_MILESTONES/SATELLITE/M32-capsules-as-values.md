# M32 — a capsule as a value

**Named by** `foreign.cpp` row for `lambda` (Python); also C++ `[](){}`,
Rust `|x|`, Java `->`, and every callback in every one of them.

## What satellite makes you write instead

Nothing. A capsule can be called and cannot be passed. `list.sort_up(key)` takes
a key, so the language already wants this shape and works around its absence one
row at a time.

**DESIGN §12 has deferred this since the language was designed**, under the name
*"a bare name can be a value"*, and WORD_NUMBERS.md's deferred-call section notes
that `satellite.thread.new(capsule_test(word))` does NOT reopen it: a deferred
call is a CALL whose last act is skipped, which is why threads could ship without
this milestone.

## What it would cost to build

The evaluator already has most of it. `satellite.variable.capsule` `1 6 16` is a
numbered type with no children, minted at M23 to hold exactly this — a packaged
call. What is missing is:

- a bare capsule name resolving to a value instead of being an error
- a call through a value rather than through a number
- what happens to the frame: a closure that captures needs storage that outlives
  the capsule it was written in, which refcounting handles and the control stack
  does not

## What it must not break

DESIGN §7.2's frame-per-call, which is the property that made threads correct
first time (`example/threads.satl` §3 — eight threads, 1600 results, 1600
right). A closure capturing by reference into a frame that has returned is the
classic way to lose that, and it must be decided before this ships, not after.
