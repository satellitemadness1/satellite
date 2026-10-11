#pragma once
// satellite/satellite_random/random_spin.hpp -- what makes fast, normal and ultra three
// grades: the throwaway.
//
// THE AUTHOR, 2026-10-02: "for ultra draw a random number between 2000 - 3000 and throw away
// random numbers of the size requested for that many ms, for normal, 1000 - 2000 and for
// fast do 500 - 1000ms". It is the design of his first two satellites -- 001's DESIGN §18 and
// 003's random.hpp, "three tiers that differ in NOTHING a program can see except how long
// they take" -- with the windows he set today.
//
// SO A CALL, in order:
//   1. every check -- the shape, the whole numbers, min under max, the step -- so a call that
//      will be refused never spends two seconds first (random_calls.cpp);
//   2. draws HOW LONG, inside the grade's window, both ends in, from the same stream the
//      answer will come from, so the length of a spin is not a constant anyone can rely on;
//   3. throws away draws OF THE SIZE REQUESTED -- the same count, base and step the answer
//      has -- until that long has passed, AT LEAST ONCE: a do/while, because a window is a
//      minimum and not a budget. A size whose one draw outlasts the window still costs one;
//      a window already passed still costs one; a plain while would throw nothing away for a
//      large enough request and the grade would be a lie (003's note, kept);
//   4. draws the answer.
//
// steady_clock, never system_clock: a wall clock can be stepped by ntp or by a person while a
// spin runs, and a deadline against one that moves backwards does not expire. 001 carried a
// draw-count failsafe for exactly that; a monotonic clock removes the need.
//
// Written 2026-10-02.

#include "random_source.hpp"

namespace satellite004 {

enum class RandomTier { fast, normal, ultra };

constexpr const char *random_tier_name(RandomTier tier)
{
    return tier == RandomTier::fast ? "fast" : tier == RandomTier::normal ? "normal" : "ultra";
}

// The author's windows, in milliseconds, both ends in.
struct SpinWindow {
    long long int least_ms;
    long long int most_ms;
};

constexpr SpinWindow spin_window(RandomTier tier)
{
    switch (tier) {
    case RandomTier::fast: return {500, 1000};
    case RandomTier::normal: return {1000, 2000};
    case RandomTier::ultra: return {2000, 3000};
    }
    return {2000, 3000};
}

// Steps 2 and 3: `discard(context)` draws the same shape the answer will have and lets it
// fall. Answers how many were thrown away -- never 0.
long long int spin(RandomTier tier, LimbSource &limbs, void (*discard)(void *), void *context);

} // namespace satellite004
