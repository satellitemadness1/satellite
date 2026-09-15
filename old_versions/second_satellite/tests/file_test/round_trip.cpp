// The lifecycle, and the failed open that is a value. See
// tests/file_test/file_test.hpp.
//
// WHAT A PROGRAM CANNOT CHECK ABOUT ITSELF IS WHETHER IT GOT THERE.
// example/persistence.satl asserts `f.ok()` is false for a missing file and
// prints PASS -- and it would print PASS just as readily if the language had
// been built to refuse instead, because a refusal ends the program before the
// PASS and the absence of a PASS is what a broken suite looks like too. Every
// fixture here asserts `complained` as well as the answer, which is the
// difference between "answered false" and "never ran".

#include "file_test.hpp"

#include <string>

namespace file_test {

void section_round_trip()
{
    // 1 -- A FAILED OPEN IS A VALUE. DESIGN §9, and the assertion PLAN M19
    // calls the one that cannot be faked.
    const std::string missing = ran(
        "    satellite.variable.file f = satellite.file.open(\"nothing\", \"read\")\n"
        "    satellite.console.display(f.ok())\n"
        "    satellite.console.display(f.error())\n"
        "    satellite.console.display(f.path())",
        "opening a file that is not there answers rather than refusing");
    check(holds(missing, "false"), "a missing file is not ok");
    check(holds(missing, "No such file"), "and it says why in the machine's words");
    check(holds(missing, "nothing"), "and it still knows which path it was");

    // 2 -- new REFUSES TO CLOBBER, and the refusal is a value too. O_EXCL is
    // what makes the acceptance program able to run twice.
    const std::string clash = ran(
        "    satellite.variable.file a = satellite.file.new(\"clash.txt\")\n"
        "    a.write_line(\"kept\")\n"
        "    satellite.console.display(a.close())\n"
        "    satellite.variable.file b = satellite.file.new(\"clash.txt\")\n"
        "    satellite.console.display(b.ok())\n"
        "    satellite.variable.file back = satellite.file.open(\"clash.txt\", \"read\")\n"
        "    satellite.console.display(back.read_all())\n"
        "    satellite.console.display(back.close())\n"
        "    satellite.console.display(satellite.system.delete(\"clash.txt\"))",
        "a second new on one path");
    check(holds(clash, "false"), "the second new is not ok");
    check(holds(clash, "kept"), "and the first file's bytes are untouched");

    // 3 -- write_line ADDS A NEWLINE AND write DOES NOT. The author's decision
    // of 2026-09-08, and the whole reason there are two verbs.
    // A METHOD ON A CALL RESULT IS S0720 AT THIS MILESTONE -- names.cpp folds a
    // selector only through a DECLARED name -- so every `.size()` in this suite
    // is asked of a variable. That is a real limit of the language today and
    // not a style choice; a fixture written the other way refuses before it
    // reaches anything M19 built.
    const std::string both = ran(
        "    satellite.variable.file f = satellite.file.new(\"verbs.txt\")\n"
        "    f.write_line(\"a\")\n"
        "    f.write(\"b\")\n"
        "    f.write(\"c\")\n"
        "    satellite.variable.string all = f.read_all()\n"
        "    satellite.console.display(all.size())\n"
        "    satellite.console.display(all)\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"verbs.txt\"))",
        "write_line and write side by side");
    check(holds(both, "4"), "write_line adds one byte and write adds none");
    check(holds(both, "a\nbc"), "and the bytes are in the order they were put");

    // 4 -- close ANSWERS A STATUS, and closing twice is not a failure.
    const std::string closing = ran(
        "    satellite.variable.file f = satellite.file.new(\"closing.txt\")\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"closing.txt\"))",
        "close answers");
    check(closing == "true\ntrue\ntrue\n", "closing twice is true both times");

    // 5 -- open REOPENS, AND READS FROM THE BEGINNING. The cursor belongs to
    // the open file description it was counting through, so a reopen resets it.
    const std::string reopened = ran(
        "    satellite.variable.file f = satellite.file.new(\"reopen.txt\")\n"
        "    f.write_line(\"first\")\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(f.open())\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"reopen.txt\"))",
        "a handle reopens itself");
    check(reopened == "first\ntrue\ntrue\nfirst\ntrue\ntrue\n",
          "a reopened handle reads from the start again");

    // 6 -- A HANDLE THAT IS ALREADY OPEN ANSWERS TRUE AND DOES NOTHING, which
    // is the only answer that keeps the snapshot contract intact -- opening
    // again would leak the live descriptor and swap the file description out
    // from under every other snapshot with no close having happened. The read
    // cursor NOT moving is what proves nothing happened.
    const std::string again = ran(
        "    satellite.variable.file f = satellite.file.new(\"again.txt\")\n"
        "    f.write_line(\"one\")\n"
        "    f.write_line(\"two\")\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.console.display(f.open())\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"again.txt\"))",
        "opening an open handle");
    check(again == "one\ntrue\ntwo\ntrue\ntrue\n",
          "an already-open handle did not reset its cursor");

    // 7 -- THE FAILED OPEN'S REASON IS CLEARED BY A SUCCESSFUL ONE. `ok` true
    // beside `error` "No such file or directory" would be two true sentences
    // that read as a contradiction.
    const std::string cleared = ran(
        "    satellite.variable.file f = satellite.file.open(\"later.txt\", \"read\")\n"
        "    satellite.console.display(f.error())\n"
        "    satellite.variable.file made = satellite.file.new(\"later.txt\")\n"
        "    satellite.console.display(made.close())\n"
        "    satellite.console.display(f.open())\n"
        "    satellite.console.display(f.error())\n"
        "    satellite.console.display(f.ok())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"later.txt\"))",
        "a retried open");
    check(holds(cleared, "No such file"), "the first attempt gave a reason");
    check(cleared.rfind("true\ntrue\n\ntrue\n") != std::string::npos ||
              holds(cleared, "\n\ntrue"),
          "and the reason is empty once the handle is open");

    // 8 -- TWO HANDLES ON ONE NAME ARE NOT THE SAME FILE. DESIGN §8's
    // reference type: `==` is identity, because two opens are two descriptions
    // with two cursors and closing one leaves the other open.
    const std::string identity = ran(
        "    satellite.variable.file a = satellite.file.new(\"two.txt\")\n"
        "    a.write_line(\"x\")\n"
        "    satellite.variable.file b = satellite.file.open(\"two.txt\", \"read\")\n"
        "    satellite.variable.file c = a\n"
        "    satellite.console.display(a == b)\n"
        "    satellite.console.display(a == c)\n"
        "    satellite.console.display(a.close())\n"
        "    satellite.console.display(b.ok())\n"
        "    satellite.console.display(b.close())\n"
        "    satellite.console.display(satellite.system.delete(\"two.txt\"))",
        "two handles on one name");
    check(identity == "false\ntrue\ntrue\ntrue\ntrue\ntrue\n",
          "two opens are two files, an assignment is one, and closing one "
          "leaves the other open");
}

} // namespace file_test
