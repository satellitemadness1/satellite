// `built()` over the four kinds, swept over all 269 nodes. See
// tests/help_test/help_test.hpp for what the done-when asks of this section.
//
// THE SWEEP IS THE TEST AND A SAMPLE WOULD NOT BE. PLAN M18 says the output is
// "comparable to the non-null entries of handlers[] by construction", and the
// only way to say that and mean it is to compare every row. 269 nodes against
// two tables is microseconds; picking six of them would prove that six were
// right on the day somebody picked them.

#include "help_test.hpp"

#include "evaluator/dispatch.hpp"
#include "satellite_console/handlers.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_help/built.hpp"
#include "satellite_directory/handlers.hpp"
#include "satellite_file/handlers.hpp"
#include "satellite_help/handlers.hpp"
#include "satellite_random/handlers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_system/handlers.hpp"
#include "satellite_time/handlers.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace help_test {

using namespace satellite;

void install_every_module()
{
    console::install_handlers();
    scalars::install_handlers();
    containers::install_handlers();
    random::install_handlers();
    time::install_handlers();
    system::install_handlers();
    help::install_handlers();
    file::install_handlers();
    directory::install_handlers();
}

namespace {

std::string named(words::PathId id)
{
    return std::string(words::path_text(static_cast<words::NodeId>(id)));
}

} // namespace

void section_predicate()
{
    // BEFORE ANY MODULE IS INSTALLED, THE FRONT END IS ALL THERE IS. This runs
    // first on purpose: the predicate reads the tables live rather than
    // remembering an answer, so an empty table has to give the empty answer.
    // built.hpp's "computed fresh and never cached" is a sentence until
    // something asks it twice and gets two answers.
    eval::Handlers::table().clear();
    eval::Assigners::table().clear();
    {
        const help::BuiltSet bare = help::BuiltSet::now();
        check(!bare.contains(
                  static_cast<words::PathId>(words::NodeId::CONSOLE_DISPLAY)),
              "with no handlers installed, `satellite.console.display` is not "
              "built -- the predicate reads the table rather than a memory of "
              "it");
        check(bare.contains(static_cast<words::PathId>(words::NodeId::MAIN)),
              "and `satellite.main` still is, because a front-end word is not "
              "in that table and never will be");
    }

    install_every_module();
    const help::BuiltSet built = help::BuiltSet::now();

    // 1 -- EVERY HANDLER ROW IS BUILT. Help cannot advertise less than what
    // runs, which is one half of DESIGN §4.6's promise.
    size_t handlers = 0;
    for (words::PathId i = 1; i <= words::kNodeCount; i++) {
        if (eval::Handlers::table().find(i) == nullptr)
            continue;
        handlers++;
        check(built.contains(i),
              "a handler row is built: " + named(i));
    }
    check(handlers >= 106,
          "the sweep found the handler rows at all -- 106 of them at M17, and "
          "this number only goes up");

    // 2 -- EVERY ASSIGNER ROW IS BUILT. M15's dials are the second kind, and
    // they are the one a help built on `handlers[]` alone would have missed:
    // `satellite.library.system.float_digits` is written to and never called.
    size_t assigners = 0;
    for (words::PathId i = 1; i <= words::kNodeCount; i++) {
        if (eval::Assigners::table().find(i) == nullptr)
            continue;
        assigners++;
        check(built.contains(i), "an assigner row is built: " + named(i));
    }
    check(assigners == 4,
          "the four dials under `satellite.library.system` are the whole of "
          "the write half at M18");

    // 3 -- NOTHING IS BUILT FOR NO REASON. Every built node has a handler, an
    // assigner, a front-end row, or a built child. This is the half that stops
    // help inventing a language: a node that answers to none of the four and
    // is still named would be exactly the drift §4.6 exists to remove.
    for (words::PathId i = 1; i <= words::kNodeCount; i++) {
        if (!built.contains(i))
            continue;
        bool because = eval::Handlers::table().find(i) != nullptr ||
                       eval::Assigners::table().find(i) != nullptr ||
                       words::is_front_end(i);
        for (words::PathId c = 1; !because && c <= words::kNodeCount; c++)
            because = built.contains(c) &&
                      static_cast<words::PathId>(
                          words::parent_of(static_cast<words::NodeId>(c))) == i;
        check(because, "nothing is built for no reason: " + named(i));
    }

    // 4 -- AND THE ONES M18 IS ABOUT, BY NAME. Hello world is written in seven
    // paths and exactly one of them is a handler row; a help built on that
    // table alone would print `display` and stay silent about the other six.
    // This is the case that made the front-end list exist, so it is the case
    // that is named rather than swept.
    const words::NodeId hello[] = {
        words::NodeId::SATELLITE,          words::NodeId::INCLUDE_SATELLITE,
        words::NodeId::CAPSULE,            words::NodeId::MAIN,
        words::NodeId::CONSOLE_DISPLAY,    words::NodeId::RETURN_SATELLITE,
    };
    for (const words::NodeId id : hello)
        check(built.contains(static_cast<words::PathId>(id)),
              "hello world's own words are built: " +
                  named(static_cast<words::PathId>(id)));

    // 5 -- AND THE ONES THAT ARE NOT. `satellite.network` is the done-when's
    // example and `satellite.include(spaceship)` is the trap beside it: the
    // parser accepts the line, so "does the front end have a case for it" is
    // the wrong test and "does a program that writes it do what it says" is
    // the right one.
    //
    // `satellite.system.home` WAS IN THIS LIST UNTIL 2026-09-11 AND M20 BUILT
    // IT, which is this check working rather than failing: a list of paths
    // "no milestone has reached" is a list that every milestone shortens, and
    // the one way it could go wrong is by nobody noticing when an entry stops
    // being true. `satellite.variable.thread` took its place -- M21's.
    //
    // AND `LIBRARY_MAIN` WENT THE SAME DAY, WHICH THIS COMMENT HAD PREDICTED
    // IN THOSE WORDS: "**`LIBRARY_MAIN` IS THE NEXT ONE TO GO**: M20's own
    // arguments object hangs under it, so the day that lands, this line fails
    // again and the fix is the same one." It landed, the line failed, and the
    // fix was the same one. The prediction cost one sentence and turned an
    // hour of reading into a minute.
    //
    // `satellite.container.result` `1 4 4` TAKES ITS PLACE, and it should
    // outlast several: it is Satellite Orbit's answer type, PLAN §8 puts it at
    // **M28**, and nothing between here and there hangs anything under it.
    const words::NodeId unbuilt[] = {
        words::NodeId::NETWORK,          words::NodeId::INCLUDE_SPACESHIP,
        words::NodeId::VARIABLE_WINDOW,  words::NodeId::CONTAINER_RESULT,
        words::NodeId::ANALYZE,          words::NodeId::VARIABLE_THREAD,
    };
    for (const words::NodeId id : unbuilt)
        check(!built.contains(static_cast<words::PathId>(id)),
              "and a path no milestone has reached is not: " +
                  named(static_cast<words::PathId>(id)));

    // 6 -- AND THE COUNT MOVES EVERY MILESTONE, which is the property the
    // done-when is really about: "the same unedited program run again at M24
    // prints a different and equally correct language". A range rather than a
    // number, because pinning it would make every later milestone edit a test
    // that is not about it.
    check(built.count() > 130 && built.count() < words::kNodeCount,
          "help names more than half the registry and not all of it -- 144 of "
          "264 at M18, 168 of 269 at M19, and a number that has to change");
}

} // namespace help_test
