#pragma once
// What one numbered library tells the interpreter about itself.
//
// Every library under satellite-numbers/ is compiled with no main and exports
// ONE function, `satellite_number_describe`, which fills in a LibraryRow: the
// path's name, its numbers from WORD_NUMBERS.md, and a pointer to each likely
// scenario it has. The interpreter calls it once, at start-up, and never
// looks anything up by name again while a program runs.
//
// A scenario is chosen by the kind of value the call is given. nullptr means
// the number has no scenario for that kind (yet).

#include "../strings/string_method.hpp"

#include <csignal>
#include <string>
#include <vector>

namespace satellite004 {

inline constexpr unsigned int kMaxDepth = 16;

// WHAT A DIRECTORY WORD ANSWERS (PLAN M0.6). `satellite.directory.change(d)` is
// a VALUE and never an error -- true or false -- and `list()` answers the names
// it read. A name is BYTES both ways: a POSIX name is arbitrary bytes, and 004's
// strings refuse anything that is not UTF-8, so a path that could not be a
// satellite string still reaches the system call that takes it.
struct DirectoryReply {
    signed long long int code = 0;             // success, or why it could not
    bool flag = false;                         // change(d): moved or not
    std::vector<std::string> names;            // list(): the entries, sorted, `.` and `..` dropped
    std::string reason;                        // the system's own word for a failure
};

// `given` is false for `list()`, which reads the working directory. `stop` is the
// session's Ctrl-C flag, read between entries so a listing of a million names can
// stop and answer `interrupted`; null when nothing can interrupt this call.
using DirectoryScenario = DirectoryReply (*)(const std::string &path, bool given, const volatile sig_atomic_t *stop);

// WHAT A SETTING ANSWERS -- a word a program can both read and write.
//
// ONE FUNCTION POINTER AND NOT TWO, with `writing` choosing which way it runs.
// A getter and a setter as separate fields would let a library fill in one and
// leave the other null, and a setting that can be read and not written is a
// half-built word that nothing in the row would say was half-built. One pointer
// cannot be half-filled.
struct SettingReply {
    signed long long int code = 0;   // success, or why it could not
    bool flag = false;               // what it says now -- after a write, what was written
    std::string reason;              // the system's own word for a failure, empty on success
};

// `writing` false reads and ignores `value`; true writes `value` and answers it
// back, so a caller never has to read again to know what it has.
using FlagSettingScenario = SettingReply (*)(bool writing, bool value);

// WHAT A WORD THAT ANSWERS A FACT ABOUT THE MACHINE SAYS.
//
// ONE REPLY FOR BOTH A COUNT AND SOME TEXT, because the alternative is two
// scenario fields and a library that fills in neither or both. `is_text` picks
// which field is the answer, and a library that answers a count leaves `text`
// alone.
//
// A COUNT IS AN `unsigned long long int` AND NOT A satellite_number, because a
// library is compiled on its own and satellite_number is the interpreter's. The
// caller turns it into one. Every fact here -- bytes, cores, threads -- fits.
//
// NOTHING IS CACHED. A machine fact is READ every time it is asked for, which is
// the point: `arguments.memory.free` that answered what was free a minute ago is
// a wrong answer that looks like a right one. SATELLITE_ARGUMENTS says the same
// thing about the register: "composing them would mean caching a fact that
// changes".
struct FactReply {
    signed long long int code = 0;              // success, or why it could not be read
    bool is_text = false;                       // false: `count` is the answer
    unsigned long long int count = 0;
    std::string text;
    std::string reason;                         // the system's own word for a failure
};

using FactScenario = FactReply (*)();

struct Scenarios {
    // The most likely scenario: display a string.
    signed long long int (*text)(const std::string &text, bool endline) = nullptr;
    signed long long int (*count)(unsigned long long int value, bool endline) = nullptr;
    signed long long int (*flag)(bool value, bool endline) = nullptr;
    signed long long int (*size)(long double value, const std::string &unit, bool endline) = nullptr;
    // A method of satellite.variable.string (1 6 1 n): see strings/string_method.hpp.
    StringMethod string_method = nullptr;

    // APPENDED LAST, AND EVERY NEW SCENARIO MUST BE: a library built before this
    // field existed still describes itself correctly, because everything it fills
    // in is still where it was. satellite.directory's three words (PLAN M0.6).
    DirectoryScenario directory = nullptr;

    // APPENDED LAST AGAIN -- THE SETTING, 2026-09-18, AND THE FIRST SCENARIO
    // THAT ANSWERS A VALUE INSTEAD OF CONSUMING ONE. Every scenario above takes
    // what a program hands it and reports how it went; `arguments.access` is
    // read as well as written, so it is the shape none of them has.
    FlagSettingScenario flag_setting = nullptr;

    // APPENDED LAST AGAIN -- A FACT, 2026-09-18. SATELLITE_ARGUMENTS B7-B11.
    //
    // THE SECOND SHAPE THAT ANSWERS RATHER THAN CONSUMES, and it is not the
    // first one widened. `flag_setting` is READ AND WRITTEN and answers a bool;
    // a fact is READ ONLY and answers a count or some text. Widening the setting
    // to carry both would have given every setting a write path for a thing that
    // cannot be written -- `arguments.memory.total` is what the machine has, not
    // a preference -- and a word that can be assigned to and must not be is a
    // word whose refusal has to be written somewhere. Read-only by having no
    // write path at all is the version with nothing to get wrong.
    FactScenario fact = nullptr;
};

struct LibraryRow {
    const char *name = nullptr;                    // "satellite.console.display"
    unsigned long long int numbers[kMaxDepth] = {};
    unsigned int depth = 0;                         // 3 for `1 5 1`
    Scenarios scenarios;
};

// The one symbol each library exports. extern "C" so its name is not mangled
// and dlsym can find it by exactly this spelling.
inline constexpr const char *kDescribeSymbol = "satellite_number_describe";
using DescribeFunction = signed long long int (*)(LibraryRow *row);

} // namespace satellite004
