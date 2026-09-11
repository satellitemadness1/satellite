// The node table `help_lines/nodes.tsv` is made of -- mark, path, number,
// milestone, arity, receiver binding -- MEASURED by installing every module's
// handlers and asking the dispatch tables, one node at a time.
//
// WHY THIS IS IN THE TREE NOW, HAVING BEEN A THROWAWAY BEFORE. help_lines's own
// README called it "a throwaway the tree doesn't ship" and said to rebuild one
// when the table needed regenerating. That worked exactly once. M19.5 appended
// eight rows, nobody rebuilt it, and MILESTONES/M19.5.md §4 had to record "the
// 8 new rows were NOT re-measured" as a known gap -- so the table's whole claim,
// that the mark is what the dispatch tables actually hold, quietly stopped being
// true for eight rows and stayed untrue for three days.
//
// M20 HIT IT AGAIN AND FROM BOTH SIDES AT ONCE: it appended 43 rows AND built
// 26 paths that were marked `.`, so the mark was wrong for more nodes than any
// milestone before it. A generator that has to be rewritten from a paragraph
// every time it is needed is one that will not be run, and the argument for
// keeping it is the same one help_lines/README.md makes for verify.py: it is
// the argument for having had it, not for having written it.
//
// IT IS NOT LINKED INTO satl AND MUST NOT BE. It has a main(), it installs
// every module, and its only output is a table -- make_support/065-tests.mk
// builds it the way it builds a test, against the tree's own sources.

#include "evaluator/dispatch.hpp"
#include "satellite_arguments/arguments.hpp"
#include "satellite_console/handlers.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_directory/handlers.hpp"
#include "satellite_file/handlers.hpp"
#include "satellite_help/handlers.hpp"
#include "satellite_random/handlers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_system/handlers.hpp"
#include "satellite_time/handlers.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>

using namespace satellite;

int main()
{
    // EVERY MODULE, AND THE SAME LIST programs/run_command.cpp INSTALLS. A
    // module left out here is a subtree that measures as unbuilt, which is the
    // one way this program can be quietly wrong -- so the list is copied from
    // the one place that already has to be complete.
    console::install_handlers();
    scalars::install_handlers();
    containers::install_handlers();
    random::install_handlers();
    time::install_handlers();
    system::install_handlers();
    help::install_handlers();
    file::install_handlers();
    directory::install_handlers();
    // M20. The arguments object's rows dispatch like any other module's, so
    // the walk below sees them only if this line is here -- which is the whole
    // reason the table is measured rather than typed.
    arguments::install_handlers();

    const eval::Handlers &handlers = eval::Handlers::table();
    const eval::Assigners &assigners = eval::Assigners::table();

    for (words::PathId id = 1; id <= words::kNodeCount; id++) {
        const eval::Handler *handler = handlers.find(id);
        const eval::Assigner *assigner = assigners.find(id);

        // `H` MEANS THE DISPATCH TABLES HOLD A ROW FOR THIS NODE, and nothing
        // more. It is NOT satellite.help's `built()`, which also counts a
        // front-end word and anything with a built word underneath it --
        // help_lines/README.md draws that distinction and the two counts differ
        // by design. A node with only an assigner row is marked too: a dial you
        // can write is built.
        const bool marked = handler != nullptr || assigner != nullptr;

        const char *milestone = "";
        if (handler && handler->milestone)
            milestone = handler->milestone;
        else if (assigner && assigner->milestone)
            milestone = assigner->milestone;

        // ARITY IS PRINTED AS 0 WHEN IT IS kAnyArity, and `any` is the column
        // that says which of the two a 0 is -- the table cannot carry
        // 0xFFFFFFFF in a field nodes.tsv reads as a small count.
        const bool any = handler && handler->arity == eval::kAnyArity;
        const unsigned arity =
            (handler && !any) ? static_cast<unsigned>(handler->arity) : 0u;
        const unsigned receiver =
            (handler && handler->binds_receiver) ? 1u : 0u;

        std::printf("%s\t%s\t%s\t%s\t%u\t%u\t%u\n",
                    marked ? "H" : ".",
                    words::path_text(static_cast<words::NodeId>(id)).c_str(),
                    words::number_text(static_cast<words::NodeId>(id)).c_str(),
                    milestone, arity, receiver, any ? 1u : 0u);
    }
    return 0;
}
