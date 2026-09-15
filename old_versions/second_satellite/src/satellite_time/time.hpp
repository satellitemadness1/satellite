#pragma once

// satellite.time -- the clock, and the wait. PLAN M13's half of DESIGN §13.
//
// TWO CLOCKS, AND ONLY ONE OF THEM IS EVER A SATELLITE VALUE -- the author
// confirmed v1's split on 2026-09-04 and DESIGN §13's Time entry carries it
// as the language's statement. The VALUE `satellite.time.now` answers is
// `system_clock` on the Unix epoch, int64 nanoseconds, because a value has to
// mean something outside the process that read it. The TIMER under
// `satellite.time.sleep` is `steady_clock`, because NTP can step a wall clock
// backwards under a running deadline and `steady_clock`'s epoch means nothing
// anyway. "One clock, one epoch" is a rule about the type, and it holds with
// nothing competing.
//
// WHAT THIS MODULE IS NOT: a calendar. `satellite.time.new` `1 9 2` is
// reserved and designed at M29 beside `satellite.variable.date` -- an instant
// constructor cannot be designed apart from the date it constructs from --
// and until M29 numbers the instant's methods, everything a program can do
// with an instant is display it (satellite_value/render.cpp's ISO-8601 arm).

namespace satellite::time {

// The instant, as the value model carries it: nanoseconds since the Unix
// epoch, read fresh on every ask -- a fact, never sampled and cached, the
// same rule system_facts/ applies to memory and M14 will apply to the
// terminal's width.
long long now_nanoseconds();

// What a sleep ended as. Interrupted is not an error and not a shortened
// sleep quietly finished -- the caller decides what it means, which for the
// evaluator is "answer nothing and let the walk stop itself at the next
// statement boundary" (DESIGN §10.2, M11's half).
enum class SleepEnd { finished, interrupted };

// Wait this many nanoseconds against steady_clock. `interrupted` is the same
// seam machine.hpp's Policy carries the flag through -- handed in as a
// function, null meaning nobody is listening -- and it is consulted only when
// the sleep is woken early, because SIGINT is installed without SA_RESTART
// (system_facts/interrupt.cpp, hard-won) and is what does the waking.
SleepEnd sleep_nanoseconds(long long count, bool (*interrupted)());

} // namespace satellite::time
