#include "interpreter/interp.hpp"
#include "interpreter/interp_internal.hpp"

#include "console_output/console.hpp"
#include "environment/env.hpp"
#include "evaluator/eval.hpp"
#include "spaceship_loader/loader.hpp"

#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

// What is left of the 433-line interp.cpp after the split: the retained-image
// machinery (image_lock, images, retain, inherited_tables, status_of) and the
// session path that is the REPL's, run_source and eval_line. The --run path
// went to interp_run.cpp and the prompt's line reading to interp_prompt.cpp;
// interp_internal.hpp says what the first of those still needs from here.

namespace satellite {
namespace {

// Everything the evaluator produced used to die with the call it was produced
// in. An instance of a spacesuit is the first VALUE that can outlive it: a
// top-level declaration writes it into the process-global satellite.library,
// and the REPL reads it back on the next line — by which time its class, and
// the tree its methods are, would be freed memory. One line is enough to reach
// it, because a spacesuit and a declaration of its type fit on one:
//
//     satellite.spacesuit c() { satellite.public { } } c x
//     x                      -> reads x's spacesuit, already destroyed
//
// So a program that declared a spacesuit keeps its image. This is a retention
// rather than a leak — it is exactly what the live objects point at — but a
// blunt one: it retains whether or not an instance actually escaped. The sharp
// version is the ProgramPtr ast.hpp already anticipates, where an Object holds
// a shared_ptr<const Program> and keeps alive only what is reachable.
std::mutex &image_lock()
{
    static std::mutex lock;
    return lock;
}

std::vector<std::shared_ptr<Image>> &images()
{
    static std::vector<std::shared_ptr<Image>> kept;
    return kept;
}

// A capsule typed at the prompt survives to the next line.
//
// §16 left this open on purpose -- "Making the REPL accumulate a program across
// lines is its own design question" -- and this is the answer, which turns out
// to be smaller than the question sounds because retain() had already done the
// hard half for spacesuits. What was missing was never lifetime; it was that
// nothing ever READ the retained images back.
//
// It merges the resolved TABLES and not the source text, and that is the
// decision worth recording. Re-parsing an accumulated prelude on every line was
// the obvious alternative and it is wrong in a way the user sees: every error
// on the line they just typed would be reported at a line number counting the
// whole invisible prelude above it. Merging tables leaves the fresh program one
// line long, so `line 1` still means the line under the cursor.
//
// It is safe because a CapsuleInfo's `const Capsule *` points into an Image
// that retain() is holding, and because resolve() stamped its slots into those
// AST nodes when the definition was typed -- so the body walks exactly as it
// did on the line it was written, with a frame sized by its own slot_count.
//
// NEWEST WINS. The fresh resolve is authoritative for every name it defines, so
// re-typing a capsule replaces it; only names the new program does NOT define
// are inherited, and they are searched newest-first so the most recent
// definition of a name shadows every earlier one. Without that, redefining a
// capsule at the prompt would silently keep running the first version.
ResolveResult inherited_tables()
{
    ResolveResult table;
    std::lock_guard<std::mutex> guard(image_lock());
    for (auto it = images().rbegin(); it != images().rend(); ++it) {
        for (const auto &entry : (*it)->resolved.capsules)
            table.capsules.emplace(entry.first, entry.second);
        for (const auto &entry : (*it)->resolved.suits)
            table.suits.emplace(entry.first, entry.second);
    }
    return table;
}

} // namespace

void retain(std::shared_ptr<Image> image)
{
    std::lock_guard<std::mutex> guard(image_lock());
    images().push_back(std::move(image));
}

// satellite.return(satellite) is success and yields 0. A number becomes the
// status.
//
// CLAMPED, not masked. A process exit status carries 8 bits, and `n & 0xff`
// turns return(256) into 0 — reporting success for what the program said was
// failure. Clamping keeps a failure a failure. A non-finite or negative status
// is 1 for the same reason.
int status_of(const ValuePtr &returned, bool ok)
{
    if (!ok)
        return 1;
    if (!returned)
        return 0;
    if (const Number *n = std::get_if<Number>(returned.get())) {
        // Truncated toward zero first, so satellite.return(2.7) is a status of
        // 2 rather than a refusal; an exit status is an integer and the
        // program said what it meant.
        long long status = 0;
        if (!n->floor().to_integer(status) || status < 0)
            return 1;
        return status > 255 ? 255 : static_cast<int>(status);
    }
    return 0;
}

InterpResult run_source(const std::string &source, const std::string &ns,
                        bool echo, Console *console, bool session)
{
    InterpResult result;

    std::shared_ptr<Image> image = std::make_shared<Image>();
    // A REPL line has no path, so its errors say "line 3" and name no file,
    // and an include in it is resolved from the working directory.
    image->loaded = load(source);
    if (!image->loaded.ok()) {
        // Every syntax error, not just the first: the parser already recovers
        // and keeps going, so burying the rest here would waste that — and the
        // loader keeps going across spaceships for the same reason.
        report(result.output, image->loaded.errors, image->loaded.sources);
        result.status = 1;
        return result;
    }

    // Resolution is a whole pass of its own, between parsing and the walk:
    // every capsule local becomes a frame slot index here, once, rather than a
    // name looked up on every read (§6), and every spacesuit gets its field
    // layout and its method table. It is also where an unknown variable inside
    // a capsule is caught — before any of the program runs, instead of partway
    // through its output.
    // Built BEFORE resolve() and handed to it, not merged afterwards. A
    // spacesuit type is checked while resolve() runs -- `box b` asks the table
    // for `box` in pass 3 -- so a table filled in after resolve() returned
    // would answer a question that had already been asked and failed.
    const ResolveResult session_tables = session ? inherited_tables()
                                                 : ResolveResult();

    image->resolved = resolve(image->loaded.program,
                              session ? &session_tables : nullptr);
    if (!image->resolved.ok()) {
        report(result.output, image->resolved.errors, image->loaded.sources);
        result.status = 1;
        return result;
    }

    Evaluator evaluator(image->resolved, ns, echo);
    evaluator.set_console(console);
    evaluator.run(image->loaded.program);

    // Before the report is built, for the reason spelled out in run_program:
    // "after whatever output preceded it" is only true if that output has
    // actually left.
    if (console)
        console->drain();

    result.output = evaluator.output();
    report(result.output, evaluator.errors(), image->loaded.sources);
    result.ok = evaluator.ok();
    result.status = status_of(evaluator.returned(), result.ok);

    // Capsules join spacesuits in earning a retention, and only in a session.
    // A file run keeps the old rule: it is a whole program, and there is no
    // next line for it to be visible from.
    if (!image->resolved.suits.empty() ||
        (session && !image->resolved.capsules.empty()))
        retain(std::move(image));
    return result;
}

std::string eval_line(const std::string &line, Console *console)
{
    return run_source(line, "main", true, console, true).output;
}

} // namespace satellite
