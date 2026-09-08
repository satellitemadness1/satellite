// The cursor -- `read_line` `1 6 2 3`, `read_all` `1 6 2 5`, and the end of the
// file. See tests/file_test/file_test.hpp.
//
// THIS IS THE ONLY THING M19 ADDS TO v1's DESIGN, so it is the section with the
// most in it. v1's `.read()` seeks to 0 on every call and has no cursor at all;
// DESIGN §8.7 requires `read_line` to answer "a line, or nothing", which is a
// sentence with no meaning on a handle that rewinds. Everything here is about
// the position the two verbs share.
//
// AND THE END OF A FILE IS `nothing` AND NOT "" -- §8.7 again, in its own
// words: "An empty line and nothing are two different values, told apart by
// `holding`, and never one empty string." A suite that checked only the printed
// text could not see the difference, because both print as nothing much; every
// fixture below asks `holding` instead.

#include "file_test.hpp"

#include <string>

namespace file_test {

void section_reading()
{
    // 1 -- LINES COME BACK IN ORDER, WITHOUT THEIR NEWLINES, AND THEN NOTHING.
    const std::string walked = ran(
        "    satellite.variable.file f = satellite.file.new(\"walk.txt\")\n"
        "    f.write_line(\"alpha\")\n"
        "    f.write_line(\"beta\")\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.variable.variant end = f.read_line()\n"
        "    satellite.console.display(end.holding())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"walk.txt\"))",
        "walking a file by lines");
    check(walked == "alpha\nbeta\nnothing\ntrue\ntrue\n",
          "the lines, in order, and then nothing");

    // 2 -- AN EMPTY LINE IS A LINE. This is the fixture §8.7's sentence exists
    // for: a reader that answered "" at the end would make a blank line in the
    // middle of a file indistinguishable from the end of it.
    const std::string blank = ran(
        "    satellite.variable.file f = satellite.file.new(\"blank.txt\")\n"
        "    f.write_line(\"\")\n"
        "    f.write_line(\"after\")\n"
        "    satellite.variable.variant first = f.read_line()\n"
        "    satellite.console.display(first.holding())\n"
        "    satellite.variable.string held = first.held()\n"
        "    satellite.console.display(held.size())\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.variable.variant end = f.read_line()\n"
        "    satellite.console.display(end.holding())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"blank.txt\"))",
        "an empty line in the middle of a file");
    check(blank == "string\n0\nafter\nnothing\ntrue\ntrue\n",
          "an empty line is a string of size 0 and the end is nothing");

    // 3 -- A LAST LINE WITH NO NEWLINE IS STILL A LINE, and the nothing comes
    // after it rather than instead of it.
    const std::string ragged = ran(
        "    satellite.variable.file f = satellite.file.new(\"ragged.txt\")\n"
        "    f.write_line(\"whole\")\n"
        "    f.write(\"partial\")\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.variable.variant end = f.read_line()\n"
        "    satellite.console.display(end.holding())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"ragged.txt\"))",
        "a file that does not end in a newline");
    check(ragged == "whole\npartial\nnothing\ntrue\ntrue\n",
          "the unterminated last line is a line, and then nothing");

    // 4 -- read_all IS THE WHOLE FILE FROM THE BEGINNING AND LEAVES THE CURSOR
    // AT THE END. The two verbs share one position and this is the decision
    // that says which way it goes.
    const std::string all = ran(
        "    satellite.variable.file f = satellite.file.new(\"all.txt\")\n"
        "    f.write_line(\"one\")\n"
        "    f.write_line(\"two\")\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.console.display(f.read_all())\n"
        "    satellite.variable.variant end = f.read_line()\n"
        "    satellite.console.display(end.holding())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"all.txt\"))",
        "read_all after a read_line");
    check(all == "one\none\ntwo\n\nnothing\ntrue\ntrue\n",
          "read_all rewound to the start and left the cursor at the end");

    // 5 -- A WRITE DOES NOT MOVE THE READ CURSOR, which is the whole reason
    // every read is a `pread`. "read_append" is O_RDWR | O_APPEND and a write
    // on an O_APPEND descriptor moves the SHARED offset to the end of the file;
    // a read cursor riding on that offset would be silently reset by every
    // write, in the one mode whose purpose is writing and reading back through
    // one handle. This fixture fails on a `read`-based implementation.
    const std::string interleaved = ran(
        "    satellite.variable.file f = satellite.file.new(\"weave.txt\")\n"
        "    f.write_line(\"one\")\n"
        "    f.write_line(\"two\")\n"
        "    satellite.console.display(f.read_line())\n"
        "    f.write_line(\"three\")\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.variable.variant end = f.read_line()\n"
        "    satellite.console.display(end.holding())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"weave.txt\"))",
        "writing between two reads");
    check(interleaved == "one\ntwo\nthree\nnothing\ntrue\ntrue\n",
          "a write between two reads did not move the read cursor");

    // 6 -- A LINE LONGER THAN THE READ-AHEAD IS ONE LINE. The buffer is 65536
    // bytes; nothing about it is a limit on a line, and NO_LIMITS is the rule
    // it would be breaking. Built here rather than written out, because a
    // fixture with 70000 characters in the source would be the test proving
    // something about the lexer instead.
    const std::string huge = ran(
        "    satellite.variable.string long_line = \"\"\n"
        "    satellite.statement.for (satellite.variable.number i = 0; i < 7000; i = i + 1)\n"
        "    {\n"
        "        long_line.append(\"0123456789\")\n"
        "    }\n"
        "    satellite.variable.file f = satellite.file.new(\"long.txt\")\n"
        "    f.write_line(long_line)\n"
        "    f.write_line(\"after\")\n"
        "    satellite.variable.string got = f.read_line()\n"
        "    satellite.console.display(got.size())\n"
        "    satellite.console.display(f.read_line())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"long.txt\"))",
        "a line longer than the read-ahead");
    check(huge == "70000\nafter\ntrue\ntrue\n",
          "a 70000-character line came back whole and the next line followed");

    // 7 -- READING A FILE WITH NOTHING IN IT ANSWERS NOTHING AT ONCE, and
    // read_all answers the empty string. Those are the two right answers and
    // they are deliberately different: the file HAS no lines, and its contents
    // ARE the empty string.
    const std::string empty = ran(
        "    satellite.variable.file f = satellite.file.new(\"empty.txt\")\n"
        "    satellite.variable.variant end = f.read_line()\n"
        "    satellite.console.display(end.holding())\n"
        "    satellite.variable.string all = f.read_all()\n"
        "    satellite.console.display(all.size())\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.system.delete(\"empty.txt\"))",
        "an empty file");
    check(empty == "nothing\n0\ntrue\ntrue\n",
          "an empty file has no lines and its contents are the empty string");
}

} // namespace file_test
