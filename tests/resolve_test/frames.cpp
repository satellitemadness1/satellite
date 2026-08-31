// DESIGN §7.2 and §7.4 -- what a slot is, and the one that is never reused.
//
// THE FIRST SECTION IS THE MILESTONE'S WHOLE REASON TO EXIST. §7.1: with one
// global registry keyed by `<capsule>.<variable>`, a capsule has exactly one
// slot per local for the WHOLE PROGRAM and no per-call storage, so a recursive
// `fact` returns 1 for every input and eight threads running a capsule with no
// recursion and no shared state produce 1585 wrong results out of 1600. Nothing
// below runs anything -- M9 does that -- so what is asserted is the property
// that makes both of those impossible: the slot numbering is PER CAPSULE and
// starts again at 0.

#include "resolve_test.hpp"

#include "name_resolver/resolve.hpp"

#include <string>

namespace resolve_test {

namespace {

const std::string kTwoCapsules = R"(
satellite.capsule first(satellite.variable.number a, satellite.variable.number b)
{
    satellite.variable.number held = a
    satellite.return(held)
}

satellite.capsule second(satellite.variable.string only)
{
    satellite.return(only)
}
)";

} // namespace

void section_frames()
{
    using namespace satellite::resolve;

    Run run;
    resolve_source(kTwoCapsules, run);
    check(run.parsed_clean(), "the two-capsule fixture parses");
    check(run.resolved.ok(), "the two-capsule fixture resolves clean");
    check(run.resolved.frames.size() == 2, "one frame per capsule");

    const Frame *first = frame_of(run, "first");
    const Frame *second = frame_of(run, "second");
    check(first != nullptr && second != nullptr, "both capsules have a frame");
    if (first == nullptr || second == nullptr)
        return;

    // §7.2's SENTENCE, AS A NUMBER. "Every capsule call gets its own frame;
    // locals resolve to integer slot indices statically, before execution." So
    // the indices are the frame's and not the program's, and `only` in the
    // second capsule is slot 0 exactly as `a` is in the first. Under the first
    // satellite's registry the two would be different keys in one table, which
    // is what made the storage shared.
    check(first->size() == 3, "first has three slots -- two parameters and a local");
    check(first->parameters == 2, "two of them are the parameter list");
    check(second->size() == 1 && second->parameters == 1,
          "second has one slot and it is its parameter");
    check(first->names[0] == "a" && first->names[1] == "b" &&
              first->names[2] == "held",
          "the parameters come first, in order, and the locals after them");
    check(second->names[0] == "only",
          "the second capsule's slots start again at 0 -- a frame is per capsule "
          "and DESIGN §7.1's registry is what happens when it is not");

    // §7.4, AND THE DUMP IS WHERE A PERSON SEES IT. "A redeclaration in one
    // scope REBINDS the name to a fresh slot rather than being an error. The
    // fresh slot is not an implementation detail: a spacesuit is a reference
    // type, so reusing the old slot would leave every handle already taken to
    // the first instance pointing at the second -- and a list built by that
    // idiom would read back as n copies of its last element WITH NO ERROR
    // ANYWHERE."
    Run again;
    resolve_source(R"(
satellite.capsule twice()
{
    satellite.variable.number x = 1
    satellite.variable.number x = 2
    satellite.return(x)
}
)",
                   again);
    check(again.resolved.ok(), "a redeclaration is not an error");
    const Frame *twice = frame_of(again, "twice");
    check(twice != nullptr && twice->size() == 2,
          "a redeclaration takes a FRESH slot -- two rows, not one");
    if (twice != nullptr && twice->size() == 2)
        check(twice->names[0] == "x" && twice->names[1] == "x",
              "and both rows are called x, which is what makes the rule visible");

    // AN INNER BLOCK SHADOWS AND THEN STOPS SHADOWING, which is the same
    // mechanism read from the other end: the binding is popped and the slot is
    // not, because a slot is never reused.
    Run blocks;
    resolve_source(R"(
satellite.capsule nested(satellite.variable.number outer)
{
    satellite.statement.if (outer < 2)
    {
        satellite.variable.number inner = 1
        satellite.console.display(inner)
    }

    satellite.console.display(outer)
    satellite.return()
}
)",
                   blocks);
    check(blocks.resolved.ok(), "a name in an inner block resolves");
    const Frame *nested = frame_of(blocks, "nested");
    check(nested != nullptr && nested->size() == 2,
          "the inner block's local has a slot of its own in the same frame");

    // AND READING IT AFTER THE BLOCK IS AN ERROR, because the binding went out
    // of scope even though the slot did not.
    Run escaped;
    resolve_source(R"(
satellite.capsule leaks(satellite.variable.number outer)
{
    satellite.statement.if (outer < 2)
    {
        satellite.variable.number inner = 1
    }

    satellite.console.display(inner)
    satellite.return()
}
)",
                   escaped);
    check(only_problem(escaped, satellite::errors::Code::RESOLVE_NO_SUCH_NAME),
          "a local read after its block is S0511 -- the slot survives the scope "
          "and the NAME does not");

    // §7.3's ORDER, WHICH IS THE REASON RESOLVE IS NOT IN THE PARSER. "A capsule
    // may call one defined further down the file, and mutual recursion is
    // unresolvable in single-pass recursive descent."
    Run forward;
    resolve_source(R"(
satellite.capsule up(satellite.variable.number n)
{
    satellite.return(down(n))
}

satellite.capsule down(satellite.variable.number n)
{
    satellite.return(up(n))
}
)",
                   forward);
    check(forward.resolved.ok(),
          "two capsules that call each other both resolve -- pass 1 runs before "
          "pass 4, which is the whole of DESIGN §7.3");

    // S0512 AND S0514, THE TWO DUPLICATES. The first is a parameter list with
    // one name twice; the second is the one the PARSER cannot see, because
    // `satellite.main` is looked up rather than defined and the numbering has
    // nothing to say about a name it did not allocate.
    Run twins;
    resolve_source(R"(
satellite.capsule pair(satellite.variable.number a, satellite.variable.number a)
{
    satellite.return()
}
)",
                   twins);
    check(only_problem(twins, satellite::errors::Code::RESOLVE_PARAMETER_TWICE),
          "two parameters of one name are S0512");

    Run mains;
    resolve_source(R"(
satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.return(satellite)
}

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.return(satellite)
}
)",
                   mains);
    check(mains.parsed_clean(),
          "two satellite.mains PARSE -- which is the reason S0514 exists");
    check(raised(mains, satellite::errors::Code::RESOLVE_CAPSULE_TWICE),
          "and resolve is the only pass that can refuse them: S0514");
    check(mains.resolved.frames.size() == 1,
          "and the second one gets no frame, so the dump does not show a file "
          "with two entry points as though it had them");

    // S0513. `satellite` is a legal `primary` -- DESIGN §6 has it and
    // `satellite.return(satellite)` is why -- so the parser accepts the word
    // wherever a name may go and this is the only pass that can refuse it.
    Run reserved;
    resolve_source(R"(
satellite.capsule takes()
{
    satellite.variable.number satellite = 1
    satellite.return()
}
)",
                   reserved);
    check(raised(reserved, satellite::errors::Code::RESOLVE_NAME_IS_RESERVED),
          "a local called `satellite` is S0513 -- DESIGN §2's reservation rule, "
          "which the parser cannot enforce here");

    // NO DEPTH BOUND AT ALL, WHICH IS DESIGN §7.5's RULE AND WAS THIS
    // MILESTONE'S ONE REAL LIMIT UNTIL 2026-08-31. M7 shipped a fixed
    // kMaxDepth = 2000 with S0501 behind it; the author's answer was that an
    // interpreter which stops at a depth is broken rather than bounded, and
    // both were deleted. What this asserted the day it landed -- that 2,200
    // levels are REFUSED -- is now the failure it exists to catch.
    //
    // 20,000 AND NOT 2,200, so the fixture is an order of magnitude past the
    // bound that used to be here. It costs about 60 MiB of the 8 GiB
    // machine_limits/limits.hpp asks the kernel for at startup.
    std::string deep = "\nsatellite.capsule deep()\n{\n    satellite.variable.number n = ";
    for (int i = 0; i < 20000; i++)
        deep += "(1 + ";
    deep += "1";
    for (int i = 0; i < 20000; i++)
        deep += ")";
    deep += "\n    satellite.return(n)\n}\n";

    Run nested_deep;
    resolve_source(deep, nested_deep);
    check(nested_deep.parsed_clean(), "20,000 levels of nesting parse");
    check(nested_deep.resolved.ok(),
          "and RESOLVE, with nothing refused -- the language has no depth "
          "limit, and a walk that stopped here would be the bound this "
          "milestone shipped and the author rejected");
}

} // namespace resolve_test
