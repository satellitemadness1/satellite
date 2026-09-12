# M39 — two capsules with one name, and arguments that need not be given

**Named by** overloading (C++, Java), default arguments (C++, Python), keyword
arguments (Python).

## What satellite makes you write instead

Different names — `call_set_type_id`, `call_set_class`, `call_set_type` — which
is most of the bulk in a spacesuit like `madness_type`. And every argument
given every time, so a capsule that could take one takes five.

## What it would cost to build

**Default arguments are the cheap half and should ship alone.** A missing
trailing argument is filled from the declaration; the resolver knows the arity
and can pad the call. No new numbering, no dispatch change.

**Overloading is the expensive half and may not be worth it.** The language
dispatches on a NUMBER — that is DESIGN §4's whole idea — and two capsules with
one name need one number with two bodies, chosen by argument type at the call
site. That is a real change to what a path means.

## What it must not break

The numbering, and the error messages that lean on it. `S0722` says
"`{1}` takes {2} and was given {3}", which assumes one arity per name; with
overloading the message has to list the candidates, which is where C++'s
famously unreadable overload errors come from.
