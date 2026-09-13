// A value as characters, and the six live codes answered. See
// satellite_value/render.hpp.
//
// THIS FILE IS WHAT PLAN §6.1 CALLED "REPLACING SIX LINES WITH SIX CALLS", and
// the six calls are in live_values() below. They are here rather than in
// satellite_string/ because the lexer calls decode() on every token's text and
// `--unparse` prints that text back: a live decode inside the alphabet would
// write this machine's thread count into the source of any program containing
// "\threads". satellite_string.hpp carries the argument at length.

#include "satellite_value/render.hpp"

#include "satellite_arguments/rows.hpp"
#include "satellite_bits/bits.hpp"
#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_spacesuit/suit_object.hpp"
#include "satellite_thread/thread_handle.hpp"
#include "system_facts/facts.hpp"

#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

namespace satellite {

namespace {

// The table satellite_string::decode() fills its live codes from, in code
// order 95..100.
//
// ONE FUNCTION AND NOT SIX CALL SITES, so that the ORDER is written once. The
// array is indexed by `code - SAT_LINUX_HOME` and nothing in the type system
// says which entry is which -- the assert in satellite_string.hpp catches a
// code being added and this ordering is what a reader checks against DESIGN
// §5's table. Every entry is filled, including the ones a given string will not
// use, because finding out which are used means walking the string first and
// the walk is what this feeds.
Live live_values()
{
    Live live;
    live[SAT_LINUX_HOME - SAT_LINUX_HOME] = facts::home_dir();
    live[SAT_LINUX_USERNAME - SAT_LINUX_HOME] = facts::username();
    live[SAT_THREADS - SAT_LINUX_HOME] = std::to_string(facts::hardware_threads());
    live[SAT_MEM_TOTAL_MB - SAT_LINUX_HOME] = std::to_string(facts::mem_total_mb());
    live[SAT_MEM_USED_MB - SAT_LINUX_HOME] = std::to_string(facts::mem_used_mb());
    live[SAT_CWD - SAT_LINUX_HOME] = facts::cwd();
    return live;
}

// A container as one line -- `[1, 2, 3]`, `{bolt: 7, nut: 9}` -- v1's
// printer, ported onto a work stack. Rendering nests as deep as the value
// does and the value nests as deep as the program chose, so DESIGN §7.5
// applies to a printer exactly as it does to a search: no C++ recursion over
// user-controlled depth. Each item is either a value still to render or a
// piece of punctuation ready to emit, pushed in reverse so the text comes out
// forwards.
std::string container_text(const Value &root)
{
    struct Piece {
        const Value *value;
        const char *text;
    };

    std::string out;
    std::vector<Piece> pending{{&root, nullptr}};
    while (!pending.empty()) {
        const Piece piece = pending.back();
        pending.pop_back();
        if (piece.text) {
            out += piece.text;
            continue;
        }
        const Value &value = *piece.value;
        if (const List *list = as_list(value)) {
            out += "[";
            pending.push_back({nullptr, "]"});
            for (size_t i = list->size(); i-- > 0;) {
                pending.push_back({&(*list)[i], nullptr});
                if (i)
                    pending.push_back({nullptr, ", "});
            }
            continue;
        }
        if (const MapBody *map = as_map(value)) {
            out += "{";
            pending.push_back({nullptr, "}"});
            for (size_t i = map->entries.size(); i-- > 0;) {
                pending.push_back({&map->entries[i].value, nullptr});
                pending.push_back({nullptr, ": "});
                pending.push_back({&map->entries[i].key, nullptr});
                if (i)
                    pending.push_back({nullptr, ", "});
            }
            continue;
        }
        // A leaf. One bounded call -- text_of routes only containers here.
        out += text_of(value);
    }
    return out;
}

// Whether this string has a live code in it at all.
//
// THE CHEAP HALF OF A DECISION THAT IS OTHERWISE FOUR /proc READS AND A
// getpwuid PER RENDER. Almost no string holds one -- they are reachable only
// through encode()'s backslash names -- so a scan of the codes buys the common
// case out of the machine entirely. The scan is over 16-bit units already in
// cache and the comparison is a range test, which is why this is worth a
// function rather than a comment saying it would be.
bool has_a_live_code(const SatString &text)
{
    for (SatChar c : text)
        if (c >= SAT_LINUX_HOME && c <= SAT_CWD)
            return true;
    return false;
}

} // namespace

std::string live_text(const SatString &text)
{
    if (!has_a_live_code(text))
        return decode(text);
    return decode(text, live_values());
}

std::string text_of(const Value &value)
{
    if (const bool *flag = std::get_if<bool>(&value)) {
        // THE LANGUAGE'S OWN SPELLING AND NOT C++'s. DESIGN §8's table writes
        // a bool as `satellite.bool.true` / `.false`, and M11 is the milestone
        // that gives those two paths a value to answer with. What a person sees
        // printed is the word, because the path is how it is WRITTEN and this
        // is how it READS -- the same split the number type has between
        // `satellite.variable.number` and `3`.
        return *flag ? "true" : "false";
    }

    if (const Number *number = std::get_if<Number>(&value))
        return number->to_string();

    // A FLOAT PRINTS WITH ITS POINT, ALWAYS -- "4.0", never "4" -- so the
    // reader is told which type answered. satellite_float owns the shape;
    // this arm only forwards, the way the number arm above does.
    if (const Flo *held = std::get_if<Flo>(&value))
        return *held ? (*held)->to_string() : std::string("0.0");

    // A BIT RUN PRINTS AS IT WAS WRITTEN, `b` AND ALL -- M19.5, and at its
    // written width, which is the clause PLAN §8's done-when names. `b0010`
    // prints `b0010` and never `b10`: DESIGN §8.5 makes the width part of the
    // value, so trimming a leading zero here would print a DIFFERENT value
    // from the one being displayed. The `b` is the same job the float's
    // always-printed point does one arm up -- it tells the reader which type
    // answered.
    if (const Bin *run = std::get_if<Bin>(&value))
        return *run ? bits::text_of(**run) : std::string("b");

    // A HEX RUN PRINTS AT ITS WRITTEN WIDTH TOO, `x` AND ALL -- `x0009` prints
    // `x0009` and never `x9`, the bit run's clause one arm up.
    //
    // BUT THE CASE IS THIS PRINTER'S AND NOT THE PROGRAM'S, which is the one
    // way hex differs from binary here. A bit run's digits ARE its value, so
    // printing what was written and printing the value are the same act; a hex
    // digit's case is not part of its value -- lexer_chars.hpp takes `x00ff`
    // and `x00FF` as one thing -- so it is not stored and cannot be printed
    // back. bits.hpp picks upper, on the ground that DESIGN §8.5 spells every
    // example it has that way.
    if (const Hex *run = std::get_if<Hex>(&value))
        return *run ? bits::text_of(**run) : std::string("x");

    if (const Str *text = std::get_if<Str>(&value))
        return *text ? live_text(**text) : std::string();

    // AN INSTANT READS AS ISO-8601 IN UTC, ALL NINE FRACTIONAL DIGITS, ALWAYS.
    // v1's printer, ported whole with its two reasons: trimming zeros would
    // make printed instants change width, and a timestamp that cannot be
    // sorted as text has lost most of what ISO-8601 is for; the 'Z' is a fact
    // about the type -- the value is the Unix epoch's clock, DESIGN §13 -- and
    // never a guess about a timezone. This rendering is the WHOLE of what M13
    // gives an instant; the methods are M29's.
    if (const Time *when = std::get_if<Time>(&value)) {
        // Floor division, not truncation: C++ integer division rounds toward
        // zero, so a pre-epoch instant would otherwise land one second late
        // with a negative fraction. Instants before 1970 are rare and being
        // quietly wrong about them is not better than being right.
        long long seconds = when->ns / 1000000000;
        long long fraction = when->ns % 1000000000;
        if (fraction < 0) {
            fraction += 1000000000;
            seconds -= 1;
        }

        const std::time_t as_time = static_cast<std::time_t>(seconds);
        std::tm broken{};
        if (!gmtime_r(&as_time, &broken)) {
            // Outside what the C library can break down. The raw count is
            // still the value, so say it rather than nothing.
            char raw[40];
            snprintf(raw, sizeof raw, "%lldns", when->ns);
            return raw;
        }

        char buffer[64];
        snprintf(buffer, sizeof buffer, "%04d-%02d-%02dT%02d:%02d:%02d.%09lldZ",
                 broken.tm_year + 1900, broken.tm_mon + 1, broken.tm_mday,
                 broken.tm_hour, broken.tm_min, broken.tm_sec, fraction);
        return buffer;
    }

    // A CONTAINER PRINTS AS ONE LINE, WHOLE -- `[1, 2, 3]`, `{bolt: 7}` --
    // and container_text above is the walk, kept off the C++ stack for DESIGN
    // §7.5's reason.
    if (value.is_list() || value.is_map())
        return container_text(value);

    // THE ARGUMENTS OBJECT PRINTS ALL OF ITSELF, WHICH IS DESIGN §7.7's OWN
    // SENTENCE: "displaying it bare prints all of it ... the §1.1 tie-breaker
    // applied to introspection". It is the one arm here that is more than one
    // line of text, and satellite_arguments/render.cpp is the walk -- it reads
    // the registry for the object's children and system_facts for their
    // answers, which is that module's subject and not this file's.
    if (const Arg *held = std::get_if<Arg>(&value))
        return *held ? arguments::object_text(**held) : std::string();

    // A FILE PRINTS AS THE PATH IT NAMES, IN ANGLE BRACKETS, WITH WHETHER IT
    // IS OPEN. M19, and it is the one arm here whose rendering is not the
    // value: a descriptor is a number the program never chose and must never
    // depend on, and the errno is `error` `1 6 2 10`'s to say in words. What a
    // reader of a displayed file wants is which file and whether it worked,
    // which is exactly what `path` `1 6 2 9` and `ok` `1 6 2 8` answer -- so
    // this line is those two, and a program that needs either of them
    // separately has a method for it rather than a string to take apart.
    //
    // THE BRACKETS ARE WHAT KEEP IT FROM READING AS A STRING. `display(f)` and
    // `display(f.path())` must not print the same thing: one is a handle and
    // one is a name, and a renderer that lost the difference would make an
    // unopened handle indistinguishable from the path it failed on.
    // A THREAD AND A DEFERRED CALL PRINT LIKE A FILE AND FOR THE FILE'S REASON
    // -- M23. Neither rendering is the value: a thread's value is a `pthread_t`
    // the program never chose and must never depend on, and a deferred call's
    // is a capsule index into an arena that exists for one run. What a reader
    // of a displayed thread wants is WHICH capsule and WHERE IT HAS GOT TO,
    // which is what these two lines are.
    //
    // THE ANGLE BRACKETS ARE THE FILE'S ARGUMENT EXACTLY: `display(t)` must not
    // look like a string, because a handle and a name are different things and
    // a renderer that lost the difference would be the only place in the
    // language where they were confusable.
    //
    // AND THE WORD IS READ WITHOUT A LOCK, deliberately. `started` and
    // `finished` are atomics and each load is honest on its own; what is not
    // guaranteed is that the pair was true at one instant, so a thread that
    // finishes between the two loads prints "running". That is a display of a
    // moving thing and the alternative is stopping the thing to look at it.
    if (const Thr *handle = std::get_if<Thr>(&value)) {
        if (!*handle)
            return "<thread>";
        const char *where =
            !(*handle)->started.load()    ? "not started"
            : (*handle)->finished.load()  ? "finished"
                                          : "running";
        return "<thread " + (*handle)->body->name + ", " + where + ">";
    }

    // A SPACESUIT PRINTS AS ITS NAME AND WHAT IT IS HOLDING -- M26, and the
    // brackets are the file's and the thread's argument for the third time:
    // `display(b)` must not look like a string.
    //
    // ITS FIELDS AND NOT ITS ADDRESS. An object's identity is a pointer the
    // program never chose and must never depend on; what a reader of a
    // displayed suit wants is what is in it. DESIGN §12 defers a user-defined
    // `to_string` the printer consults -- PLAN §8's M26 entry asks in as many
    // words that this milestone "must not quietly grant either by needing one
    // for its own demonstration" -- so this is the language's rendering and
    // never the program's.
    //
    // THE PROTECTED FIELDS ARE SHOWN TOO, which is worth stating because it
    // looks like a leak and is not. `satellite.protected` is about what a
    // PROGRAM may reach, and DESIGN §1.1's rule is that satellite never hides
    // what is there from the person running it; a debugger that showed half an
    // object would be the language keeping a secret from its author.
    if (const Sui *handle = std::get_if<Sui>(&value)) {
        if (!*handle || (*handle)->layout == nullptr)
            return "<spacesuit>";
        const suit::Layout &layout = *(*handle)->layout;
        std::string out = "<" + layout.name;

        // A COPY OF THE FIELDS, TAKEN UNDER THE OBJECT'S HOLD AND RENDERED
        // OUTSIDE IT -- THREAD.md D1. Rendering a field can render another
        // object, and holding this one while waiting for that one is how two
        // threads rendering each other's objects would hang. A thread already
        // holding an object across a handler only TRIES, for the same reason
        // one level out, and says so in the text if the object is busy.
        std::vector<Value> fields;
        std::unique_lock<std::recursive_mutex> in((*handle)->hold,
                                                  std::defer_lock);
        if (suit::holds_open > 0) {
            if (!in.try_lock())
                return out + " (in use by another thread)>";
        } else {
            in.lock();
        }
        fields = (*handle)->fields;
        in.unlock();

        for (size_t i = 0; i < fields.size(); i++)
            out += (i == 0 ? " " : ", ") + layout.field_names[i] + ": " +
                   text_of(fields[i]);
        return out + ">";
    }

    if (const Cap *handle = std::get_if<Cap>(&value))
        return *handle ? "<capsule " + (*handle)->name + ">" : "<capsule>";

    if (const Fil *handle = std::get_if<Fil>(&value)) {
        if (!*handle)
            return "<file>";
        const bool open = (*handle)->descriptor.load() >= 0;
        return "<file " + live_text(encode_raw((*handle)->path)) +
               (open ? ", open>" : ", closed>");
    }

    // THE RUNTIME PRINTS AS THE WORD THE PROGRAM WROTE. DESIGN §8's table gives
    // `satellite` a row of its own and §3 calls it "the singleton runtime
    // object, not a zero sentinel"; a value that printed as blank or as
    // `nothing` would be that sentinel arriving through the renderer, which is
    // the one thing the row exists to deny.
    if (value.is_runtime())
        return "satellite";

    // NOTHING PRINTS AS A WORD RATHER THAN AS AN EMPTY LINE, which is DESIGN
    // §1.1's rule about never doing anything behind the user's back applied to
    // the smallest possible case: a capsule that returned nothing and a capsule
    // that returned the empty string must not print the same thing. Since M12
    // a program can ASK which it has -- the variant's `holding`, DESIGN §8.7.
    return "nothing";
}

} // namespace satellite
