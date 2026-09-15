// One entry per node, and the groups partition. See tests/help_test/help_test.hpp.
//
// MOST OF THIS IS ALREADY A static_assert AND IS CHECKED AGAIN HERE ANYWAY.
// help_text.hpp's order_is_right() runs at compile time and a build that got
// this far has already passed it -- so what these fixtures add is a NAME for
// each property and a failure that says which one broke. words_test makes the
// same trade for the same reason: an assert that fires tells you a file is
// wrong, and a fixture tells you what about it.
//
// AND ONE THING THE ASSERT CANNOT SEE. A row can be present, correctly ordered
// and empty, and the compiler has nothing to say about that. An entry with no
// prose is a node the document skipped, which is the exact failure this
// milestone is for.

#include "help_test.hpp"

#include "satellite_help/help_text.hpp"
#include "satellite_help/render.hpp"
#include "satellite_words/words.hpp"

#include <cstring>
#include <string>

namespace help_test {

using namespace satellite;

void section_entries()
{
    check(help::kEntryCount == words::kNodeCount,
          "help.def carries one row per node of words.def -- 269 of them");

    size_t heads = 0;
    for (words::PathId i = 1; i <= words::kNodeCount; i++) {
        const std::string path =
            std::string(words::path_text(static_cast<words::NodeId>(i)));

        // NO ROW IS BLANK. 139 of the 269 are paths nothing implements and
        // every one of them still says what it will be and which milestone
        // owns it -- "where a path belongs to a milestone nobody has started,
        // the entry says which milestone and shows no example".
        check(help::entry_of(i).prose[0] != '\0',
              "every node has something written about it: " + path);

        // A HEAD IS ITS OWN HEAD, so the groups partition the registry and no
        // entry is unreachable. The static_assert says it; this says which row
        // if it ever stops being true.
        const words::PathId head = help::head_of(i);
        check(help::head_of(head) == head,
              "a group's head is its own head: " + path);
        if (head == i)
            heads++;

        // AND THE HEAD IS WHAT A PERSON TYPES. `1 5 3` is
        // `satellite.console.input(prompt)` and is asked about as
        // `satellite.console.input`, which is a path the trie walks -- so the
        // line help prints in a listing is a line that can be typed back in.
        bool resolved = false;
        const words::Walk found = words::walk(help::query_text(i));
        resolved = found.error == words::WalkError::NONE;
        check(resolved, "the query a listing prints walks back to a node: " +
                            help::query_text(i));
        check(!resolved || help::head_of(found.id) == head,
              "and it walks back to this entry's own group: " + path);
    }

    check(heads > 0 && heads < words::kNodeCount,
          "the registry is more than one group and fewer groups than nodes -- "
          "183 at M18, and a number that moves with the entries rather than "
          "with the language");

    // THE THREE THE MILESTONE OWNS, BY NAME. PLAN M18: "asking about a path
    // must bring up that node AND its children together -- `satellite.help(
    // satellite.main)` answers for `1 3` and `1 3 0` at once." That sentence is
    // this check.
    check(help::head_of(static_cast<words::PathId>(words::NodeId::MAIN_0)) ==
              static_cast<words::PathId>(words::NodeId::MAIN),
          "`satellite.main()` is answered for under `satellite.main`");
    check(help::head_of(static_cast<words::PathId>(
              words::NodeId::CONSOLE_INPUT_PROMPT_TARGET)) ==
              static_cast<words::PathId>(words::NodeId::CONSOLE_INPUT_0),
          "and `input(prompt, target)` under `satellite.console.input`, which "
          "is three sibling nodes and one word");
    check(help::head_of(static_cast<words::PathId>(
              words::NodeId::CONSOLE_DISPLAY)) ==
              static_cast<words::PathId>(words::NodeId::CONSOLE_DISPLAY),
          "and `display` heads its own group rather than joining the module's");
}

} // namespace help_test
