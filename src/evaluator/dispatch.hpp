#pragma once

// `handlers[path_id]` -- PLAN M9, and the thing PLAN §1.1 says this whole plan
// hangs off.
//
// WHAT IT REPLACES IS MEASURED AND IT IS THE FIRST SATELLITE'S WORST HOT PATH.
// PLAN §1.1: the dispatch for `satellite.console.display(x)`, the most-called
// thing in the language, was to "flatten the member chain into a
// `vector<string>`, heap-allocate a joined string ON EVERY MODULE CALL
// including the arms that never match, and walk an ordered chain of `full ==
// "..."` comparisons". DESIGN §4's numbering exists so that this is an array
// index instead, and the numbers are allocated before any program exists
// because they belong to the namespace rather than to a program.
//
// SO THIS FILE IS ONE VECTOR AND A LOOKUP, and its size is the whole point.
//
// A PROCESS-WIDE TABLE, WHICH IS SAFE FOR A REASON WORTH WRITING DOWN.
// words_runtime.hpp warns in capitals that "a user's PathId is valid inside one
// run only", and M22 runs many programs in one process. That warning is about
// paths a PROGRAM interned -- a capsule's name, a global's name -- and nothing
// here may be keyed on one. Every handler is installed for a path `words.def`
// numbers, which is constexpr data identical in every run of every process, so
// the table is a property of the BUILD rather than of a run. install() asserts
// it in tests/eval_test rather than trusting the sentence.
//
// EMPTY IN `satl` AT M9, AND THAT IS THE MILESTONE BOUNDARY RATHER THAN A GAP.
// PLAN §8's path ledger says M9 holds none of the 223 numbered paths -- "M5 and
// M9 build the machinery every other row dispatches through" -- so what lands
// here is the table, the receiver-binding tag and the cache, and the first rows
// are M10's console. tests/eval_test installs its own and is the consumer that
// proves the mechanism, which is PLAN M2's rule about a registry needing a
// reader in the milestone that writes it.

#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <vector>

namespace satellite::eval {

class Machine;

// WHAT A HANDLER IS: arguments in, one value out, and a bool saying whether it
// answered at all.
//
// IT RETURNS A BOOL AND NOT A Value, WHICH IS DESIGN §9.1. A handler that
// cannot do what it was asked -- a file that will not open, a division by zero
// -- reports through the machine and answers false. Throwing was measured at
// 181x the cost of returning an enum, and "record the error and return nullptr"
// is what PLAN §7 throws away with 199 call sites behind it.
using HandlerFn = bool (*)(Machine &machine, const Value *arguments,
                           uint32_t count, Value *answer);

// An arity that means "as many as you like".
inline constexpr uint32_t kAnyArity = 0xFFFFFFFFu;

struct Handler {
    HandlerFn fn = nullptr;

    // DESIGN §6.4 QUALIFICATION 2's RECEIVER-BINDING TAG, and it is the only
    // reason `satellite.file.new(path)` `1 8 1` and
    // `satellite.variable.file.new` `1 6 2 1` can coexist. WORD_NUMBERS §4
    // calls that pair "the kind of thing that gets decided by accident at M16",
    // so PLAN §8's M9 entry asks for the tag to be BUILT here rather than
    // legislated from three milestones later. When it is set, argument 0 is the
    // receiver the method was called on and the written arguments follow it.
    bool binds_receiver = false;

    // How many arguments, receiver included, or kAnyArity. Checked before the
    // handler runs, which is PLAN §2.3's "how many arguments, already checked".
    uint32_t arity = kAnyArity;

    // Which milestone owns this row, for `satl --compile` to print. A table
    // whose rows cannot say where they came from is the shape PLAN §1.1 counts.
    const char *milestone = nullptr;

    // DESIGN §6.4's LAST LINE, AS A COLUMN: "a mutating method needs a
    // receiver that names a storage slot". When set, the handler's ANSWER is
    // the receiver's new value and the machine writes it back to the slot the
    // method was called on -- op_method and op_method_global carry the slot,
    // and plain op_dispatch refuses (S0717), which is `foo().append(x)`'s
    // "there is nowhere to write back" enforced by the op rather than by a
    // rule someone has to remember. M11's two rows are `append` `1 6 1 14`
    // and `clear` `1 6 1 15`; M16's list brings the next ones.
    //
    // LAST IN THE STRUCT ON PURPOSE: every non-mutating install site stays a
    // four-field initialiser and the default answers for it.
    bool mutates = false;
};

// The table. One row per language PathId, indexed directly.
class Handlers {
public:
    static Handlers &table();

    void install(words::PathId path, Handler handler);

    // A row, or nullptr. THE HOT PATH, and it is a bounds check and a load.
    const Handler *find(words::PathId path) const
    {
        if (path >= rows_.size())
            return nullptr;
        return rows_[path].fn != nullptr ? &rows_[path] : nullptr;
    }

    size_t installed() const { return installed_; }

    // tests/eval_test needs to put the table back the way it found it, because
    // a process-wide table shared between sections is a test that passes in
    // one order and fails in another.
    void clear();

private:
    Handlers() = default;

    std::vector<Handler> rows_;
    size_t installed_ = 0;
};

// THE WRITE HALF OF THE NAMESPACE -- M15's retune, and the decision M11
// declined to invent mid-milestone (MILESTONES/M11.md §6). PLAN §4.5.3's
// arrangement -- "the file seeds the namespace at startup and the namespace
// is what everything reads afterwards" -- had no run-time half: no document
// said what `satellite.library.system.division_digits = 40` MEANS. It means
// this: AN ASSIGNMENT TO A NUMBERED LANGUAGE PATH IS A HANDLER-SHAPED WRITE.
// The same signature a read dispatches through, handed the one value to the
// right of the `=`, answering into nothing -- and the dial rows store into
// the running machine's Policy, which is what every read already reads, so
// the next division simply observes the new count. One mechanism, decided
// once, in front of `1 14 2 1` through `1 14 2 4` together; the rows
// themselves are satellite_system/handlers.cpp's.
//
// A SECOND TABLE AND NOT A COLUMN OF Handler, config_internal.hpp's own
// argument about kDialKinds: reads and writes answer different questions
// about a path, almost no path is writable, and a column would widen every
// row of the hot table for four entries. A path with a read and no write row
// refuses with S0724, which names the boundary instead of guessing at it.
struct Assigner {
    HandlerFn fn = nullptr;

    // Which milestone owns the row, Handler's column for Handler's reason.
    const char *milestone = nullptr;
};

class Assigners {
public:
    static Assigners &table();

    void install(words::PathId path, Assigner assigner);

    const Assigner *find(words::PathId path) const
    {
        if (path >= rows_.size())
            return nullptr;
        return rows_[path].fn != nullptr ? &rows_[path] : nullptr;
    }

    size_t installed() const { return installed_; }

    void clear();

private:
    Assigners() = default;

    std::vector<Assigner> rows_;
    size_t installed_ = 0;
};

} // namespace satellite::eval
