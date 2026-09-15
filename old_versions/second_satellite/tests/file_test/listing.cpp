// `satellite.directory` `1 18`, and `satellite.system.delete` `1 22 1`. See
// tests/file_test/file_test.hpp.
//
// THE SUITE RUNS IN A DIRECTORY OF ITS OWN, which is what makes a listing
// checkable at all: `list()` in the source tree answers whatever the tree
// happens to hold that day, and a test whose expected value moves is not a
// test. main() makes the directory with mkdtemp and enters it, so what is in
// here is exactly what these fixtures put here.

#include "file_test.hpp"

#include <string>

namespace file_test {

void section_listing()
{
    // 1 -- current() NAMES SOMEWHERE, AND exists AGREES WITH IT.
    const std::string here = ran(
        "    satellite.variable.string d = satellite.directory.current()\n"
        "    satellite.console.display(d.size() > 0)\n"
        "    satellite.console.display(satellite.directory.exists(d))\n"
        "    satellite.console.display(satellite.directory.exists(\"no_such\"))",
        "current and exists");
    check(here == "true\ntrue\nfalse\n", "current names a directory that exists");

    // 2 -- A PLAIN FILE IS NOT A DIRECTORY AND A DIRECTORY IS NOT A FILE, which
    // is what separates `satellite.directory.exists(d)` `1 18 3` from
    // `satellite.file.exists(path)` `1 8 5` and is why M19 minted the second.
    const std::string kinds = ran(
        "    satellite.variable.file f = satellite.file.new(\"kind.txt\")\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.variable.string d = satellite.directory.current()\n"
        "    satellite.console.display(satellite.file.exists(\"kind.txt\"))\n"
        "    satellite.console.display(satellite.directory.exists(\"kind.txt\"))\n"
        "    satellite.console.display(satellite.file.exists(d))\n"
        "    satellite.console.display(satellite.directory.exists(d))\n"
        "    satellite.console.display(satellite.system.delete(\"kind.txt\"))",
        "a file and a directory asked of both words");
    check(kinds == "true\ntrue\nfalse\nfalse\ntrue\ntrue\n",
          "each word answers true for its own kind and false for the other");

    // 3 -- list() IS SORTED, AND IT IS list(d) OF HERE.
    // SORTEDNESS IS CHECKED BY POSITION AND NOT BY THE WHOLE STRING, because
    // the directory holds whatever the fixtures before this one left in it --
    // a listing compared as one string would be a test of the suite's own
    // order. `b.txt` is made FIRST and must come SECOND, which is the property
    // that would fail if the module handed back readdir's order.
    const std::string listed = ran(
        "    satellite.variable.file b = satellite.file.new(\"b.txt\")\n"
        "    satellite.variable.file a = satellite.file.new(\"a.txt\")\n"
        "    satellite.console.display(b.close())\n"
        "    satellite.console.display(a.close())\n"
        "    satellite.container.list<satellite.variable.string> l = "
        "satellite.directory.list()\n"
        "    satellite.console.display(l.contains(\"a.txt\"))\n"
        "    satellite.console.display(l.index_of(\"a.txt\") < l.index_of(\"b.txt\"))\n"
        "    satellite.container.list<satellite.variable.string> named = "
        "satellite.directory.list(satellite.directory.current())\n"
        "    satellite.console.display(named == l)\n"
        "    satellite.console.display(satellite.system.delete(\"a.txt\"))\n"
        "    satellite.console.display(satellite.system.delete(\"b.txt\"))",
        "listing a directory");
    check(listed == "true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n",
          "the listing holds what was made, sorted, and list(d) of here is "
          "list() -- " + listed);

    // 4 -- "." AND ".." ARE NOT CONTENTS, AND DOTFILES ARE. A language with no
    // flags has one answer, so it has to be the true one -- and a caller who
    // joined each name onto the directory it came from and walked down would
    // get an infinite descent rather than a walk.
    const std::string dots = ran(
        "    satellite.variable.file h = satellite.file.new(\".hidden\")\n"
        "    satellite.console.display(h.close())\n"
        "    satellite.container.list<satellite.variable.string> l = "
        "satellite.directory.list()\n"
        "    satellite.console.display(l.contains(\".\"))\n"
        "    satellite.console.display(l.contains(\"..\"))\n"
        "    satellite.console.display(l.contains(\".hidden\"))\n"
        "    satellite.console.display(satellite.system.delete(\".hidden\"))",
        "dots and dotfiles");
    check(dots == "true\nfalse\nfalse\ntrue\ntrue\n",
          "`.` and `..` are absent and a real dotfile is present");

    // 5 -- change ANSWERS FALSE RATHER THAN DYING, both for somewhere that is
    // not there and for a path that is a plain file. PLAN M19's done-when names
    // the second one specifically.
    const std::string moving = ran(
        "    satellite.variable.string home = satellite.directory.current()\n"
        "    satellite.variable.file f = satellite.file.new(\"notadir.txt\")\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.directory.change(\"no_such\"))\n"
        "    satellite.console.display(satellite.directory.change(\"notadir.txt\"))\n"
        "    satellite.console.display(satellite.directory.change(home))\n"
        "    satellite.console.display(satellite.directory.current() == home)\n"
        "    satellite.console.display(satellite.system.delete(\"notadir.txt\"))",
        "changing directory");
    check(moving == "true\nfalse\nfalse\ntrue\ntrue\ntrue\n",
          "a change that cannot happen is false and the program keeps running");

    // 6 -- delete TAKES BOTH SHAPES: a path, and an open handle. The handle
    // shape is the reason `1 22 1` is M19's row at all -- no other milestone
    // can hand it one.
    const std::string removing = ran(
        "    satellite.variable.file byname = satellite.file.new(\"byname.txt\")\n"
        "    satellite.variable.file byhandle = satellite.file.new(\"byhandle.txt\")\n"
        "    satellite.console.display(byname.close())\n"
        "    satellite.console.display(satellite.system.delete(\"byname.txt\"))\n"
        "    satellite.console.display(satellite.system.delete(byhandle))\n"
        "    satellite.console.display(satellite.file.exists(\"byname.txt\"))\n"
        "    satellite.console.display(byhandle.exists())\n"
        "    satellite.console.display(satellite.system.delete(\"byname.txt\"))\n"
        "    satellite.console.display(byhandle.close())",
        "deleting by path and by handle");
    check(removing == "true\ntrue\ntrue\nfalse\nfalse\nfalse\ntrue\n",
          "both shapes remove, deleting what is gone is false, and a handle "
          "whose file was removed still closes");

    // 7 -- clear EMPTIES AND LEAVES THE FILE THERE, which is the whole
    // difference between it and delete.
    const std::string cleared = ran(
        "    satellite.variable.file f = satellite.file.new(\"clearme.txt\")\n"
        "    f.write_line(\"something\")\n"
        "    satellite.console.display(f.close())\n"
        "    satellite.console.display(satellite.file.clear(\"clearme.txt\"))\n"
        "    satellite.console.display(satellite.file.exists(\"clearme.txt\"))\n"
        "    satellite.variable.file back = satellite.file.open(\"clearme.txt\", \"read\")\n"
        "    satellite.variable.string all = back.read_all()\n"
        "    satellite.console.display(all.size())\n"
        "    satellite.console.display(back.close())\n"
        "    satellite.console.display(satellite.system.delete(\"clearme.txt\"))",
        "clearing a file");
    check(cleared == "true\ntrue\ntrue\n0\ntrue\ntrue\n",
          "clear leaves a file of zero bytes that is still there");

    // 8 -- WHAT THE SUITE LEAVES BEHIND, AND WHY IT IS NOT NOTHING.
    //
    // Every fixture that RUNS removes what it made -- the same discipline
    // example/persistence.satl keeps, and PLAN M19 asks for, and for the same
    // reason: `satellite.file.new` is O_EXCL, so a suite that left its files
    // would pass once and fail on every run after. main()'s mkdtemp makes that
    // survivable rather than sufficient.
    //
    // THE REFUSAL FIXTURES CANNOT, AND THAT IS THE POINT OF THIS CHECK RATHER
    // THAN AN EXCEPTION TO IT. section_refusals makes six files and each of
    // those programs ENDS at its refusal, so the cleanup line after it never
    // runs -- which is a true fact about how a refusal behaves, asserted here
    // instead of worked around. The eight are named, so a ninth appearing means
    // a fixture that was supposed to run did not.
    const std::string left = ran(
        "    satellite.container.list<satellite.variable.string> l = "
        "satellite.directory.list()\n"
        "    satellite.console.display(l)",
        "what the suite left behind");
    // AND `closed2.txt` COMES BEFORE `closed.txt`, WHICH IS NOT A TYPO. The
    // listing is sorted as SatStrings over DESIGN §5's code table and not as
    // bytes, so a digit sorts before punctuation and `2` beats `.`. The module
    // says that about itself -- "dotfiles sort last, because the dot is
    // punctuation in that table" -- and this line is where the claim is
    // actually exercised. Written in ASCII order first, and it failed.
    check(left == "[closed2.txt, closed.txt, dir1.txt, dir2.txt, n.txt, pair.txt, "
                  "plain.txt, taken.txt]\n",
          "only the files whose programs refused before their cleanup are left "
          "-- " + left);
}

} // namespace file_test
