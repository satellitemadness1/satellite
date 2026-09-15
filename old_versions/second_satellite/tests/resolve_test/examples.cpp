// The acceptance programs, resolved -- LAYOUT.md calls the files in example/
// "not samples -- each of these is what a milestone means by done", and
// example/frames.satl is this milestone's.
//
// THE FOUR THAT PARSE MUST ALSO RESOLVE, and that is a stronger claim than it
// looks. Three of them were written for milestones that have not landed --
// advanced.satl for a console that does not exist, thread_test.satl for M23,
// super_advanced.satl for M15's float -- so nothing about them was chosen to
// suit this pass, and every path in them had to be a path the numbering
// actually has. A resolver that quietly answered kNoPath for what it could not
// place would pass every assertion in the other five files and fail here.

#include "resolve_test.hpp"

#include "name_resolver/dump.hpp"
#include "name_resolver/resolve.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace resolve_test {

namespace {

bool read_file(const std::string &path, std::string &into)
{
    FILE *handle = fopen(path.c_str(), "rb");
    if (handle == nullptr)
        return false;
    char buffer[8192];
    size_t got = 0;
    while ((got = fread(buffer, 1, sizeof buffer, handle)) > 0)
        into.append(buffer, got);
    fclose(handle);
    return true;
}

const char *const kPrograms[] = {"hello_world.satl", "advanced.satl",
                                 "thread_test.satl", "super_advanced.satl",
                                 "frames.satl"};

} // namespace

void section_examples()
{
    using namespace satellite::resolve;

    for (const char *const name : kPrograms) {
        std::string source;
        const std::string path = example_directory + "/" + name;
        if (!read_file(path, source)) {
            check(false, std::string("cannot read ") + name);
            continue;
        }

        Run run;
        resolve_source(source, run);
        check(run.parsed_clean(), std::string(name) + " parses");
        check(run.resolved.ok(), std::string(name) + " resolves with no problem");
        check(!run.resolved.frames.empty(),
              std::string(name) + " has at least one frame");

        // THE DUMP IS RUN OVER EVERY ONE OF THEM, because a printer nothing
        // exercises is a printer that does not work -- which is the argument
        // PLAN M2 makes for having a consumer at all, applied to the consumer.
        const std::string dumped =
            dump_text(path, run.parsed.ast, run.words, run.resolved);
        check(holds(dumped, "satl resolved " + path),
              std::string(name) + " has a dump that names the file");
        check(holds(dumped, "the walk"),
              std::string(name) + " has a dump that says what the walk cost");
    }

    // example/frames.satl IS THIS MILESTONE'S DONE-WHEN, so its clauses are
    // asserted one at a time rather than only through "it resolves".
    std::string source;
    if (!read_file(example_directory + "/frames.satl", source)) {
        check(false, "cannot read frames.satl");
        return;
    }

    Run frames;
    resolve_source(source, frames);
    if (!frames.parsed_clean())
        return;

    check(frames.resolved.frames.size() == 3,
          "frames.satl has three capsules and three frames");

    const Frame *main = frame_of(frames, "main");
    check(main != nullptr, "one of them is satellite.main");
    if (main != nullptr) {
        check(main->arguments == 0,
              "whose parameter is DESIGN §7.7's object, in slot 0");

        // §7.4, WHICH IS THE ONE CLAUSE A DUMP CAN SHOW AND A TEST CANNOT
        // PHRASE ANY OTHER WAY: two rows, both called `counter`.
        size_t counters = 0;
        for (const std::string_view name : main->names)
            if (name == "counter")
                counters++;
        check(counters == 2,
              "and `counter`, declared twice in one scope, holds TWO slots -- "
              "DESIGN §7.4, and a list built on the other rule reads back as n "
              "copies of its last element with no error anywhere");
    }

    check(resolved_to(frames, "1 4 2 5"),
          "sort(\"down\") folded to sort_down() 1 4 2 5");
    check(resolved_to(frames, "1 14 1 1 1 1"),
          "arguments.machine.cores resolved, six numbers deep");
    check(resolved_to(frames, "1 14 1 1 1 3"), "and so did .threads");

    // AND THE `.satc` STILL READS THE LITERAL, which is WORD_NUMBERS §1.5's own
    // sentence -- "the file keeps `\"down\"`; the runtime keeps the number.
    // This is the one place where the numbering deliberately says more than the
    // file does." Asserted from both ends in one place, because the two halves
    // are only interesting together.
    const satellite::cache::Source stamp{"frames.satl", 1, source.size()};
    const std::string satc =
        satellite::cache::satc_text(frames.parsed.ast, frames.words, stamp);
    check(holds(satc, "sort(\"down\")"),
          "the `.satc` writes sort(\"down\") and not the folded number");
    check(!holds(satc, "1.4.2.5"),
          "and 1 4 2 5 is nowhere in it -- the fold must never reach a `.satc`");
}

} // namespace resolve_test
