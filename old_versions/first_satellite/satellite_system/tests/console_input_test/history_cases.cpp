// The up arrow: what it recalls, and the half-typed line it has to give back.
// And the store underneath it -- the cap, the file, and $SATL_HISTORY.
//
// Part of console_input_test. See console_input_test.hpp.

#include "console_input_test.hpp"

#include "console_input/history.hpp"

#include <cstdlib>
#include <string>
#include <unistd.h>

using namespace satellite;

void test_history_browsing()
{
    History history;
    history.add("first");
    history.add("second");
    history.add("third");

    {
        Editor editor(&history);
        check_eq(type(editor, "\033[A"), "third|", "up recalls the newest");
        check_eq(type(editor, "\033[A"), "second|", "up again goes back one");
        check_eq(type(editor, "\033[A"), "first|", "and one more");
        check_eq(type(editor, "\033[A"), "first|",
                 "up past the oldest entry stays on it");
        check_eq(type(editor, "\033[B"), "second|", "down comes forward again");
    }

    // THE ONE HISTORY BUG EVERY READER NOTICES AND NOBODY CAN DESCRIBE: type
    // half a line, press up to check something, press down to come back, and
    // the half-typed line is gone. It is saved on the way OUT of the live line
    // and handed back on the way in.
    {
        Editor editor(&history);
        check_eq(type(editor, "half typed"), "half typed|", "a line in progress");
        check_eq(type(editor, "\033[A"), "third|", "up leaves it");
        check_eq(type(editor, "\033[B"), "half typed|",
                 "down past the newest gives the half-typed line back");
    }

    // And the live line is saved ONCE, on the first Up -- not on every one, or
    // the second press would overwrite it with the entry the first recalled.
    {
        Editor editor(&history);
        type(editor, "mine");
        type(editor, "\033[A\033[A\033[A");
        check_eq(type(editor, "\033[B\033[B\033[B"), "mine|",
                 "walking all the way up and back returns the live line");
    }

    // Down with no Up before it does nothing: there is nothing in front of a
    // line that has not been left.
    {
        Editor editor(&history);
        check_eq(type(editor, "abc\033[B"), "abc|",
                 "down without having gone up leaves the line alone");
    }

    // A recall puts the cursor at the END. Anywhere else and a line recalled
    // while the cursor sat at column three looks cut off at column three.
    {
        Editor editor(&history);
        check_eq(type(editor, "\033[A"), "third|", "a recall ends at the end");
    }

    // No history at all: Up and Down are ordinary no-ops rather than a special
    // case the read loop has to know about.
    {
        Editor editor(nullptr);
        check_eq(type(editor, "abc\033[A\033[B"), "abc|",
                 "with no history the arrows do nothing");
    }
}

void test_history_store()
{
    {
        History history;
        history.add("one");
        history.add("one");
        check(history.size() == 1, "the same line twice is stored once");
        history.add("two");
        history.add("one");
        check(history.size() == 3,
              "only a CONSECUTIVE repeat is dropped, not every repeat");
    }
    {
        History history;
        history.add("");
        check(history.empty(), "an empty line is not remembered");
        history.add("has\nnewline");
        check(history.empty(), "a line with a newline in it is refused");
    }
    {
        // The cap trims from the FRONT, so what falls off is the oldest.
        History history;
        for (size_t i = 0; i < History::MAX_ENTRIES + 10; i++)
            history.add("line " + std::to_string(i));
        check(history.size() == History::MAX_ENTRIES, "the cap holds");
        check_eq(history.at(0), "line 10", "the oldest entries are the ones lost");
    }

    // The file, round-tripped. Written to the scratch path this test owns and
    // removed afterwards, so running the suite twice is the same as once.
    {
        const std::string path =
            "satellite_system/tests/console_input_test/.history_scratch";
        {
            History writing(path);
            writing.add("alpha");
            writing.add("beta with spaces");
            check(writing.save(), "the history file was written");
        }
        {
            History reading(path);
            check(reading.load(), "the history file was read");
            check(reading.size() == 2, "both entries came back");
            check_eq(reading.at(0), "alpha", "the oldest entry is first");
            check_eq(reading.at(1), "beta with spaces", "spaces survive");
        }
        unlink(path.c_str());
    }
    {
        // A missing file is what a first run looks like and is NOT a failure.
        History history("satellite_system/tests/console_input_test/.no_such_file");
        check(history.load(), "a missing history file is not an error");
        check(history.empty(), "and leaves the history empty");
    }
    {
        // Memory only. Nothing is opened and nothing is written, which is what
        // $SATL_HISTORY=none asks for.
        History history;
        history.add("kept in memory");
        check(history.save(), "saving a memory-only history does nothing, quietly");
        check(history.path().empty(), "and it has no path");
    }

    // $SATL_HISTORY, both jobs: naming the file, and switching it off. The
    // second is why it is an escape hatch rather than a setting -- a variable
    // that can only ADD a behaviour cannot decline one.
    {
        setenv("SATL_HISTORY", "/tmp/satl_history_probe", 1);
        check_eq(History::default_path(), "/tmp/satl_history_probe",
                 "$SATL_HISTORY names the file");
        setenv("SATL_HISTORY", "none", 1);
        check(History::default_path().empty(), "$SATL_HISTORY=none writes nothing");
        setenv("SATL_HISTORY", "", 1);
        check(History::default_path().empty(), "$SATL_HISTORY= writes nothing");
        unsetenv("SATL_HISTORY");
    }
}
