// What a name reaches, and what it does not -- the lookup order names.cpp
// states and the one refusal DESIGN §9 asks for.

#include "resolve_test.hpp"

#include "name_resolver/resolve.hpp"

#include <string>

namespace resolve_test {

void section_names()
{
    using namespace satellite::resolve;

    // A LOCAL SHADOWS A CAPSULE OF THE SAME NAME, which is the order names.cpp
    // writes down and is the opposite of words_runtime.hpp's find(). There the
    // LANGUAGE's children are searched first, because a user's name may never
    // answer in place of a word the language owns; here the names are all the
    // user's and the nearer one wins.
    Run shadowed;
    resolve_source(R"(
satellite.capsule helper()
{
    satellite.return()
}

satellite.capsule uses(satellite.variable.number helper)
{
    satellite.return(helper)
}
)",
                   shadowed);
    check(shadowed.resolved.ok(), "a parameter may be spelled like a capsule");
    const Frame *uses = frame_of(shadowed, "uses");
    check(uses != nullptr && uses->size() == 1,
          "and the parameter is the one that gets the slot");

    // A CAPSULE NAME REACHED AS A VALUE IS NOT A SLOT. DESIGN §7.6: "capsules
    // are not in the registry ... a bare capsule name cannot even form a legal
    // registry key", so what a Name node holding one carries is the sentinel
    // and never an index into a frame.
    Run called;
    resolve_source(R"(
satellite.capsule helper()
{
    satellite.return()
}

satellite.capsule uses()
{
    helper()
    satellite.return()
}
)",
                   called);
    check(called.resolved.ok(), "a capsule called by name resolves");

    bool saw_capsule_slot = false;
    for (satellite::NodeIndex node = 1; node < called.parsed.ast.size(); node++)
        if (called.resolved.at(node).slot == kSlotCapsule)
            saw_capsule_slot = true;
    check(saw_capsule_slot,
          "and it reaches kSlotCapsule rather than a frame index -- DESIGN §7.6");

    // S0511, WITH DESIGN §4.6's SUGGESTION OVER THE NAMES THAT ARE IN SCOPE.
    // errors::suggest() cannot answer this one: it walks the frozen trie, and
    // these names were met four milestones after that table was written.
    Run unknown;
    resolve_source(R"(
satellite.capsule reads()
{
    satellite.variable.number count = 1
    satellite.console.display(cout)
    satellite.return()
}
)",
                   unknown);
    check(only_problem(unknown, satellite::errors::Code::RESOLVE_NO_SUCH_NAME),
          "a name with nothing behind it is S0511");
    check(!unknown.resolved.problems.empty() &&
              unknown.resolved.problems.front().suggestion == "count",
          "and `cout` is answered with `count` -- §4.6 over the scope stack");

    // AND NOT OVER EVERY NAME IN THE FILE. The candidate list is what is IN
    // SCOPE plus the capsules, in that order, so a tie goes to the nearer
    // thing. A suggester that searched wider would answer for a local three
    // capsules away, which is the failure suggest.hpp records one level down:
    // "254 paths contain something within two edits of almost any word".
    Run far;
    resolve_source(R"(
satellite.capsule elsewhere(satellite.variable.number couot)
{
    satellite.return(couot)
}

satellite.capsule reads()
{
    satellite.variable.number count = 1
    satellite.console.display(cout)
    satellite.return()
}
)",
                   far);
    check(!far.resolved.problems.empty() &&
              far.resolved.problems.front().suggestion == "count",
          "the nearer candidate wins over one in another capsule's frame");

    // THE INITIALISER IS RESOLVED BEFORE THE NAME ENTERS SCOPE, which decides a
    // real program: `x = x` names the OUTER x if there is one and is unknown if
    // there is not. Declaring first would make it name itself and read a slot
    // that has never been written -- and §7.4's fresh slot is exactly what
    // makes that reachable, because the shadowed x is still there.
    Run selfish;
    resolve_source(R"(
satellite.capsule reads()
{
    satellite.variable.number x = x
    satellite.return(x)
}
)",
                   selfish);
    check(only_problem(selfish, satellite::errors::Code::RESOLVE_NO_SUCH_NAME),
          "`number x = x` is S0511 on the right-hand x, not a slot reading "
          "itself");

    // AND WITH AN OUTER ONE IT IS LEGAL AND MEANS THE OUTER ONE.
    Run outer;
    resolve_source(R"(
satellite.capsule reads()
{
    satellite.variable.number x = 1
    satellite.variable.number x = x
    satellite.return(x)
}
)",
                   outer);
    check(outer.resolved.ok(),
          "and with an outer x it resolves -- to the outer one, which is what "
          "§7.4's fresh slot makes possible");

    // AN UNKNOWN NAME OUTSIDE A CAPSULE IS STILL UNKNOWN, which is where this
    // resolver differs from the M6 draft on purpose. That one answers
    // SLOT_GLOBAL for any unknown name met at the top level -- right for the
    // grammar it was written against, and wrong for this one, whose `top_level`
    // is include, capsule, spacesuit and global and nothing else.
    Run global;
    resolve_source("\nsatellite.library.total = missing\n", global);
    check(global.parsed_clean(), "a global with an initialiser parses");
    check(only_problem(global, satellite::errors::Code::RESOLVE_NO_SUCH_NAME),
          "and an unknown name in it is S0511 rather than a silent global");
}

} // namespace resolve_test
