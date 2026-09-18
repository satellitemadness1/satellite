#pragma once
// THE FEATURE REGISTER -- every debug and bookkeeping feature as one binary, and
// the one value start-up reads.
//
// The author, 2026-09-18: *"we could technically hide all of these into
// something that we build -- a satellite.variable.binary where each value in the
// width is a feature, then we can just scan this single binary and it flips on or
// off all of the features, this is how we'll keep the interpreter really fast!"*
// And: *"satl --rebuild is the command to rebuild the arguments string ... it
// saves it and then it just loads that single value"*.
//
// WHY ONE VALUE AND NOT EIGHT BOOLS, MEASURED RATHER THAN ARGUED. 2026-09-18,
// clang 24 -O2, 2,000,000,000 statements, eight features OFF, two runs agreeing:
//
//     no gates at all (the floor)              0.224 ns a statement
//     eight separate bools                     1.81  ns
//     one bitmask, eight bit tests             1.84  ns   <- NO BETTER
//     one bitmask, ONE test per statement      0.72  ns
//     split loop, tested ONCE on entry         0.225 ns   <- free
//
// READ THE THIRD ROW TWICE. A bitmask tested bit by bit is NOT faster than eight
// bools -- either way the processor runs eight compare-and-branches, and an
// always-false branch is already free to predict. Scanning one binary does not
// remove the eight tests.
//
// WHAT THE ONE VALUE BUYS IS THE TWO ROWS UNDER IT. Because every feature lives
// in ONE word, one test can gate ALL of them -- `if (flags.any())` -- and the
// cost stops growing as features are added. And hoisting that test out of the
// loop entirely makes the features cost NOTHING when off: not a nanosecond, not
// a branch, because the checks are not in the compiled loop.
//
// THE SPLIT WALKER IS NOT BUILT YET AND THAT IS ON PURPOSE (the author,
// 2026-09-18): *"we can leave optimizing it to another milestone later, so it
// doesn't have to be optimized yet"*. This file is the STRUCTURE. SATELLITE_ERROR
// Part 12's F5 is the split, and its final milestone is the sweep that makes sure
// everything is in here.
//
// ---
//
// THE RULE THIS FILE LIVES OR DIES BY: **A BIT'S MEANING NEVER MOVES.**
//
// It is the words.tsv rule again -- a word's code is its ROW, so a row is only
// ever appended -- and it bites harder here. A config.ini written by an older
// build holds a NUMBER. If bit 5 stopped meaning "trace" and started meaning
// "dump every allocation", that old number silently turns on a feature nobody
// asked for, on a machine nobody is watching. So:
//
//     * a new feature takes the next free bit, at the END;
//     * a dead feature's bit is ABANDONED, never reused, and stays in the table
//       marked `retired` so the next person cannot take it by accident;
//     * the order of this enum IS the file format.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace satellite004 {

// EVERY BIT, IN ORDER, AND THE ORDER IS THE FILE FORMAT. Append only.
//
// A FEATURE IS LISTED HERE BEFORE IT IS BUILT, which is deliberate: the bit is
// what the milestone hangs on, and a feature built first and given a bit second
// is a feature that had to be retro-fitted. `built` says which ones do anything
// today, so `--rebuild` can tell a person the truth rather than implying that
// setting a bit turned something on.
enum class Feature : unsigned {
    access = 0,          // keep the last known name, type and value of everything
    history,             // write every value a name ever held to disk
    frames,              // keep a frame stack, so every frame's variables can be read
    statements,          // the statement ring: the last N statements run
    trace,               // stream every statement to a file as it happens
    coverage,            // which statements ran, and which never did
    word_counts,         // how many times each of the 367 words ran
    capsule_timing,      // where the time went, per capsule
    watchpoints,         // stop when a named variable changes
    memory_accounting,   // which variables are holding the memory
    thread_state,        // what each thread was doing
    report_on_success,   // write the full report even when nothing failed
    report_file,         // write the full report to ~/.satl/reports/
    replay,              // record the inputs and the seed, so a run can be run again
    count_               // NOT A FEATURE: how many there are. Always last.
};

inline constexpr unsigned kFeatureCount = static_cast<unsigned>(Feature::count_);
static_assert(kFeatureCount <= 64, "the walker's test is one register compare; see red note 10");

struct FeatureFact {
    const char *name;        // the key in config.ini, and what --rebuild prints
    bool built;              // whether anything reads this bit yet
    bool on_by_default;      // what it is when config.ini does not say
    const char *what;        // one line, for --rebuild's table
};

// THE NAMES ARE THE config.ini KEYS. A person opening the file reads `access`,
// not `satellite.library.main.arguments.access` -- the path is how the LANGUAGE
// spells it, the file is a person's and spells it short.
inline const std::array<FeatureFact, kFeatureCount> &feature_facts()
{
    static const std::array<FeatureFact, kFeatureCount> facts = {{
        {"access",            true,  true,  "keep the last known name, type and value of everything"},
        {"history",           false, false, "write every value a name ever held to disk"},
        {"frames",            false, false, "keep a frame stack, so every frame's variables can be read"},
        {"statements",        false, false, "the statement ring: the last N statements run"},
        {"trace",             false, false, "stream every statement to a file as it happens"},
        {"coverage",          false, false, "which statements ran, and which never did"},
        {"word_counts",       false, false, "how many times each word ran"},
        {"capsule_timing",    false, false, "where the time went, per capsule"},
        {"watchpoints",       false, false, "stop when a named variable changes"},
        {"memory_accounting", false, false, "which variables are holding the memory"},
        {"thread_state",      false, false, "what each thread was doing"},
        {"report_on_success", false, false, "write the full report even when nothing failed"},
        {"report_file",       false, false, "write the full report to ~/.satl/reports/"},   // on_by_default becomes true when H-phase builds it
        {"replay",            false, false, "record the inputs and the seed, so a run can be run again"},
    }};
    return facts;
}

inline const FeatureFact &fact_of(Feature which)
{
    return feature_facts()[static_cast<unsigned>(which)];
}

// THE REGISTER ITSELF. One word, and every test on it is a register test.
//
// A PLAIN uint64_t AND NOT A std::bitset, because the whole point is that the
// walker's question -- "is ANYTHING on?" -- is one compare against zero. bitset
// gives that too, and gives it through a class the optimiser has to see through;
// this is the value the measurement above was taken on.
struct FeatureRegister {
    std::uint64_t bits = 0;

    constexpr bool on(Feature which) const
    {
        return (bits & (1ull << static_cast<unsigned>(which))) != 0;
    }
    constexpr void set(Feature which, bool value)
    {
        const std::uint64_t bit = 1ull << static_cast<unsigned>(which);
        bits = value ? (bits | bit) : (bits & ~bit);
    }

    // THE ONE THE WALKER ASKS. When this is false the split walker runs a loop
    // with no checks in it at all -- which is the 0.225 ns row, and the reason
    // the whole system is free when nobody is debugging.
    constexpr bool any() const { return bits != 0; }

    static constexpr FeatureRegister defaults()
    {
        FeatureRegister made;
        // Written as a loop over the facts rather than a literal, so a feature
        // added to the enum cannot be forgotten here.
        for (unsigned i = 0; i < kFeatureCount; ++i)
            if (feature_facts()[i].on_by_default)
                made.bits |= (1ull << i);
        return made;
    }
};

// WRITTEN THE WAY SATELLITE WRITES A BINARY -- `b` then the bits, high bit first
// (DESIGN: a binary is written with its b). So the value in config.ini is a
// satellite binary a person could paste into a program, not a decimal nobody can
// read. Leading zeros are kept to the full width, so the column a bit sits in
// does not move when a higher feature is turned on.
inline std::string written(const FeatureRegister &reg)
{
    std::string out = "b";
    for (unsigned i = kFeatureCount; i-- > 0;)
        out += reg.on(static_cast<Feature>(i)) ? '1' : '0';
    return out;
}

// THE SAME REGISTER AS A PLAIN NUMBER, so a person can pass one around.
//
// The author, 2026-09-18: *"Should we make it an signed long long int so the user
// can paste specific numbers between each other?"* and *"then you can just in the
// switch statements, if it's this number, do this, and if it's this number, do
// this"*.
//
// **YES TO THE NUMBER, NO TO THE SIGN, AND THE SIGN IS THE ONLY DISAGREEMENT.**
// A number is much easier to pass than fourteen digits of binary -- "run it with
// 8193" against "run it with b10000000000001" -- and it makes the switch cases
// READ as numbers, which is the author's second sentence exactly. All of that
// works, and it works better unsigned:
//
//   * **BIT 63 MAKES A SIGNED VALUE NEGATIVE.** With 64 features the top bit is
//     the sign bit, so a register with it set prints as
//     `-9223372036854775808`. That is the opposite of easy to paste, and it is a
//     number a person would reasonably think was an error message.
//   * **SHIFTING INTO THE SIGN BIT IS A TRAP** that C++20 only recently made
//     well-defined, and `1 << 63` on a signed type is a line every reviewer has
//     to stop at forever.
//   * **IT IS NOT FASTER EITHER WAY.** The author's reason was speed -- *"so we
//     can work with it extremely fast and don't have to read it"* -- and that is
//     already true: signed and unsigned are the same register and the same
//     instructions. Nothing is read either way; the value is loaded once at
//     start-up and tested from a register after that.
//
// So: unsigned inside, and a decimal spelling for people. 0 to
// 18446744073709551615, never a minus sign.
//
// AND THE AUTHOR'S "SMALLEST NUMBER" INSTINCT IS ALREADY BUILT, which is worth
// saying because it was a good one: *"the least significant bit is the most
// common option, then the number will stay smaller"*. `access` is bit 0, so the
// ordinary register is `1` -- one character to pass along. The Feature enum is in
// roughly that order and new features go on the end, which keeps common ones low
// by construction.
inline std::string as_number(const FeatureRegister &reg)
{
    return std::to_string(reg.bits);
}

// Read `b1011...` back. Answers false for anything that is not that shape, and
// the caller then says so rather than running on a register it invented.
//
// A SHORTER STRING IS READ AND NOT REFUSED, which is the older-file case and the
// ordinary one after an upgrade: the bits that are there are taken, and the
// features added since take their defaults. A LONGER one is a config.ini from a
// NEWER build -- its extra high bits mean nothing here, and are dropped rather
// than refused, because a person who downgrades should not be locked out of
// their own settings. See red note 12: the author may want this to speak up.
inline bool read_written(const std::string &text, FeatureRegister &into)
{
    // A PLAIN NUMBER IS ACCEPTED TOO, which is the author's pasteable form. A
    // person handed "8193" can put it straight in config.ini or on a command line
    // and it means what the binary would have meant.
    //
    // NO MINUS SIGN, and it is refused rather than folded: a negative register is
    // not a register with the top bits set, it is somebody pasting the wrong
    // thing, and reading it as 2^63 would silently turn on the highest feature
    // there is.
    if (!text.empty() && text[0] >= '0' && text[0] <= '9') {
        std::uint64_t value = 0;
        for (const char c : text) {
            if (c < '0' || c > '9')
                return false;
            const std::uint64_t digit = static_cast<std::uint64_t>(c - '0');
            // A number past 64 bits is a number that was not a register.
            if (value > (~0ull - digit) / 10u)
                return false;
            value = value * 10u + digit;
        }
        FeatureRegister made;
        made.bits = value;
        into = made;
        return true;
    }

    if (text.size() < 2 || (text[0] != 'b' && text[0] != 'B'))
        return false;
    FeatureRegister made = FeatureRegister::defaults();
    const std::size_t digits = text.size() - 1;
    for (std::size_t i = 0; i < digits; ++i) {
        const char c = text[text.size() - 1 - i];   // lowest bit is the last character
        if (c != '0' && c != '1')
            return false;
        if (i >= kFeatureCount)
            continue;                                // a newer build's bit; dropped
        made.set(static_cast<Feature>(i), c == '1');
    }
    into = made;
    return true;
}

// The key `--rebuild` writes and start-up reads. One value, one parse, one run.
inline constexpr const char *kRegisterKey = "features";

} // namespace satellite004
