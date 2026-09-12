// What `satl --limits` prints. See machine_limits/dump.hpp.
//
// THE COLUMN IS THE POINT, the same way it is in satellite_words/dump.cpp: a
// value, and beside it who decided that value. "What is satl holding to" and
// "why that" are different questions, and only the second one is any use to
// somebody whose machine is behaving unexpectedly -- which is v1's argument for
// `--where` printing which of three candidates answered, applied to seven rows
// instead of one.

#include "machine_limits/dump.hpp"

#include "machine_limits/config_internal.hpp"
#include "machine_limits/limits.hpp"
#include "machine_limits/pool.hpp"
#include "satellite_words/words.hpp"
#include "system_facts/facts.hpp"

#include <cstddef>
#include <string>

namespace satellite::limits {

namespace {

// Where the second column starts. Wide enough for `division_digits`, which is
// the longest name in either half of the file.
constexpr size_t kValueColumn = 20;
constexpr size_t kOriginColumn = 32;

void column(std::string &out, size_t to)
{
    // A name longer than the column gets one space rather than a negative
    // number of them -- the same guard satellite_words/dump.cpp's row() makes,
    // and for the same reason: a column is a convenience and must never be the
    // thing that goes wrong.
    size_t at = out.size() - (out.rfind('\n') + 1);
    out.append(at < to ? to - at : 1, ' ');
}

// A name and one thing said about it -- for the block at the bottom, where the
// right-hand side is a sentence rather than a value with an origin beside it.
// Splitting `61.9 GiB total, 34.0 GiB available` across two columns made it
// look like a value and its provenance, which is what the rows above ARE and
// this is not.
void said(std::string &out, const std::string &name, const std::string &text)
{
    out += "  " + name;
    column(out, kValueColumn);
    out += text;
    out += '\n';
}

void row(std::string &out, const std::string &name, const std::string &value,
         const std::string &where)
{
    out += "  " + name;
    column(out, kValueColumn);
    out += value;
    column(out, kOriginColumn);
    out += where;
    out += '\n';
}

// `satellite_config.ini:6`, or the origin alone when no line produced it.
std::string where(const Held &now, Origin origin, unsigned line)
{
    std::string out(origin_text(origin));
    if (line != 0 && !now.config_path.empty())
        out += ", " + now.config_path + ":" + std::to_string(line);
    return out;
}

// What reads a dial, for the one that nothing reads yet.
//
// NAMED PER DIAL RATHER THAN "not read yet", because the useful sentence is
// what will look at the number. PLAN M6 wrote this list when three of the four
// had no reader at all; two of the three have landed since -- M8's division and
// M9's control stack -- and each row changed from a milestone number into the
// thing that actually reads it, which is what the other two rows always said.
// The fourth landed at M15 -- DESIGN §13 had already redefined it from "the
// dial" into the default length of a float's right half, and that is now the
// thing the row says reads it.
std::string reader_of(DialId id)
{
    switch (id) {
    case DialId::DivisionDigits: return "a division that does not end reads it";
    case DialId::MaxDepth:       return "the evaluator's control stack reads it";
    case DialId::MinFreeMb:      return "the watchdog reads it";
    case DialId::FloatDigits:    return "a result that rounds reads it";
    case DialId::Count_:         break;
    }
    return "?";
}

} // namespace

std::string limits_text()
{
    const Held &now = held();
    std::string out = "satl is holding itself to these.\n\n";

    // EVERY MACHINE FACT THIS COMMAND NEEDS, READ ONCE, HERE. Two blocks below
    // print the machine's own answers beside the settings, and three of the
    // rows above may BE those answers -- so the obvious code asks the machine
    // twice for each and `satl --limits` pays 0.42 ms twice over for
    // physical_cores() alone. Read once and used twice is not an optimisation
    // here, it is the difference between a command that reports the machine and
    // one that reports it from two different instants.
    const unsigned threads = facts::hardware_threads();
    const unsigned cores = facts::physical_cores();
    const unsigned long long total = facts::mem_total_bytes();
    const unsigned long long ceiling = now.memory_max.value_given(total);

    row(out, "THREAD_COUNT",
        std::to_string(now.thread_count.value_given(threads)),
        where(now, now.thread_count.origin, now.thread_count.line));
    row(out, "CORE_COUNT", std::to_string(now.core_count.value_given(cores)),
        where(now, now.core_count.origin, now.core_count.line));
    row(out, "MEMORY_MAX", human_bytes(ceiling),
        where(now, now.memory_max.origin, now.memory_max.line));
    out += "                    " + std::to_string(ceiling) +
           " bytes exactly\n";

    // THE FOUR DIALS, UNDER THEIR PATH RATHER THAN UNDER THEIR NAME, because
    // the name alone would look like three more machine settings and they are
    // not: these are in the language, `satl --words satellite.library.system`
    // finds them, and since M15 a running program has the read and the retune
    // -- this comment claimed "from M8" until M11's landing found that
    // nothing of the kind had been built; the author moved the mechanism to
    // the milestone that owns the fourth dial, and it landed there. The
    // upper-case three above are in no numbering at all.
    out += "\n  " + words::path_text(words::NodeId::LIBRARY_SYSTEM) + "  (" +
           words::number_text(words::NodeId::LIBRARY_SYSTEM) + ")\n";
    for (size_t i = 0; i < kDialCount; i++) {
        const DialId id = static_cast<DialId>(i);
        const Dial &dial = now.dial(id);

        // A SIZE IS PRINTED THE WAY IT WAS WRITTEN, WHICH IS M9's ADDITION.
        // `max_depth` is the only dial that is a quantity of memory rather than
        // a count, and config_internal.hpp's kDialKinds is what says so -- a
        // row reading `67108864` where the file said `64MiB` is this command
        // failing its own rule, which M6 states as: every value satl holds to
        // says where it came from, in the terms it came in.
        const bool a_size = dial.set && kDialKinds[i] == Kind::Size;

        row(out, std::string(dial_name(id)),
            dial.set ? (a_size ? human_bytes(dial.value)
                               : std::to_string(dial.value))
                     : std::string("unset"),
            dial.set ? where(now, dial.origin, dial.line) : reader_of(id));

        // THE EXACT COUNT ON ITS OWN LINE, which is what MEMORY_MAX does eight
        // rows up and for §4.5.4's reason: the rounded form is the readable
        // half of a pair and never the whole answer.
        if (a_size)
            out += "                    " + std::to_string(dial.value) +
                   " bytes exactly\n";
    }

    out += "\n";
    said(out, "the file",
         now.config_path.empty()
             ? std::string("none -- beside the binary is where satl looks")
             : now.config_path);

    // THE POOL'S COUNT IS A SAMPLE, AND A PART-BUILT ONE IS THE MEASUREMENT
    // RATHER THAN A FAULT. The pool takes ~590 us to finish building
    // (§4.5.1.1), so it is normally caught part way -- `satl --words`
    // reproducibly finishes with 3 of 24 parked on this machine and this
    // command with 14 to 19, measured 2026-08-31. That is §4.5.1.1's ramp seen
    // from outside, and it is why the line says WHEN the number was taken
    // instead of presenting it as a state.
    //
    // THE NUMBER WENT UP ON 2026-08-31 AND THE POOL DID NOT GET FASTER. This
    // command reproducibly showed 0 parked until that day, because limits.cpp
    // read the machine for every setting BEFORE calling pool::start() and
    // physical_cores() alone is 0.42 ms of that. The pool starts first now, so
    // it has the whole command to build in instead of the tail of it. A count
    // that moves when nothing about the pool changed is the reason this line
    // exists rather than a fault in it.
    const unsigned asked = pool::wanted();
    const unsigned waiting = pool::parked();
    said(out, "the pool",
         std::to_string(asked) + " threads, " + std::to_string(waiting) +
             " parked just now" + (waiting < asked ? " and still warming" : ""));

    // THE MACHINE'S OWN ANSWERS, BESIDE THE SETTINGS, WHICH IS §4.5.4's THIRD
    // OPEN QUESTION ANSWERED IN A COLUMN. "A setting is not a fact":
    // THREAD_COUNT says what satl MAY USE and this says what the machine HAS,
    // they are allowed to differ, and printing both is what makes the
    // difference visible instead of making one of them lie. It is also where
    // "not cores x 2" stops being a rule in a document -- 24 and 12 are read
    // from two different places and neither is derived from the other.
    out += "\n";
    said(out, "the machine", std::to_string(threads) + " hardware threads, " +
                                 std::to_string(cores) + " physical cores");
    said(out, "", human_bytes(total) + " total, " +
                      human_bytes(facts::mem_available_bytes()) + " available");
    said(out, "", human_bytes(facts::process_memory_bytes()) +
                      " resident -- what THIS run is using");

    // THE STACK, AND WHETHER satl WIDENED IT. M6's rule is that every value
    // satl holds to says where it came from, and this is the one row where
    // "where it came from" is satl itself: `ulimit -s` is a shell's DEFAULT with
    // an unlimited hard limit behind it on an ordinary Linux, so satl raises its
    // own at startup (limits.hpp's share of memory). Printing only the number
    // in force would hide that -- and hiding it is how everybody comes to
    // believe the 8 MiB is the kernel's, which is what this project believed
    // until 2026-08-31.
    const unsigned long long limit = facts::stack_limit_bytes();
    unsigned long long standing = 0;
    const bool known = facts::thread_stack_bytes(&standing, nullptr);
    const Held &holding = held();
    said(out, "the stack",
         (limit == facts::kStackLimitUnknown
              ? std::string("unlimited (RLIMIT_STACK)")
              : human_bytes(limit) + " (RLIMIT_STACK)") +
             (known ? ", " + human_bytes(standing) + " in use on this thread"
                    : std::string(", this thread's stack is not reportable")));
    //
    // AND THE ROW ABOVE HAS AN ORIGIN NOW, WHICH IS WHY THIS LINE IS NEW ON
    // 2026-08-31. The number satl asks for stopped being a constant that day
    // and became a share of what the machine has, so "where did 1.9 GiB come
    // from" has an answer that is not "somebody typed it into a header" -- and
    // M6's rule is that a value satl holds to says where it came from. It is
    // computed from `total`, which was read at the top of this function, rather
    // than by calling wanted_stack_bytes(): limits.hpp's pair exists so that
    // this command does not open /proc/meminfo a second time.
    said(out, "",
         total != 0
             ? std::to_string(kStackPerMegabyte / 1024) +
                   " KiB of stack for every MiB of the machine's " +
                   human_bytes(total) + ", never under " +
                   human_bytes(kStackFloorBytes)
             : std::string("the machine would not say what it has, so satl "
                           "asked for the floor of ") +
                   human_bytes(kStackFloorBytes));
    if (holding.stack_now > holding.stack_before && holding.stack_before != 0)
        said(out, "", "satl raised it from " + human_bytes(holding.stack_before) +
                          " -- the soft limit is a default and the hard limit "
                          "was not in the way");
    else if (now.stack_before != 0)
        said(out, "", "satl asked for " +
                          human_bytes(wanted_stack_bytes_given(total)) +
                          " and this machine did not give it, which costs "
                          "nothing but depth");

    out += "\n"
           "MEMORY_MAX is the only one of these that acts on its own: a run\n"
           "that crosses it is stopped within a second, with a line on stderr\n"
           "and exit status 4. The four dials read back from inside a program\n"
           "as the paths above, and since M15 an assignment to one is the\n"
           "retune. min_free_mb was the exception until M20 and is not one\n"
           "any more: it takes a number of megabytes, or the word\n"
           "\"disabled\" to stop the watch, and the watchdog reads the new\n"
           "floor on its next look.\n";
    return out;
}

std::string walk_note_text(size_t units)
{
    std::string out = "\nThat walk of " + std::to_string(units) +
                      " nodes ran on ONE thread, and the pool was parked\n"
                      "the whole time it did: " +
                      std::to_string(pool::wanted()) + " asked for and " +
                      std::to_string(pool::parked()) +
                      " built and waiting when this\nline was reached.\n";

    // AND THE REASON IS NOT THE FLOOR, WHICH IS A CORRECTION TO M6's OWN
    // DONE-WHEN. It says the walk stays single-threaded "because 254 nodes is
    // far under the 170-line floor" -- and 254 is more than 170, so that
    // sentence cannot be the reason. The floor is measured in satellite-rooted
    // SOURCE LINES being interned (§4.5.1.1), and this command interns nothing:
    // it prints a constexpr table, which §4.5.1 settled separately and firmly
    // -- "threading the table at startup is a guaranteed loss ... the table is
    // constexpr and lands in rodata, so there is no startup work left to
    // thread." Both clauses of the done-when hold; only the reason moves.
    // MILESTONES/M6.md §4 carries it.
    out += "PLAN §4.5.1 is why, and it is NOT the ~" +
           std::to_string(pool::kFloor) +
           "-unit floor: the word table is\n"
           "constexpr and lands in rodata, so printing it is never worth\n"
           "threading at any size. The floor is about interning a source.\n"
           "\n"
           "A count below what was asked for is the pool still being built,\n"
           "not a pool that failed: it costs ~590 us to finish and this\n"
           "command takes about 0.8 ms in total. PLAN §4.5.1.1 measured that\n"
           "ramp; this is it from the outside.\n";
    return out;
}

} // namespace satellite::limits
