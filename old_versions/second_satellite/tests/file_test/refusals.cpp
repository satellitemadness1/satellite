// Every S12xx code, by number. See tests/file_test/file_test.hpp.
//
// THIS SECTION IS WHY THE SUITE EXISTS. example/persistence.satl is the other
// done-when and it cannot contain one of these: a refusal ends the program, so
// a program that reached S1201 would never print its PASS. Seven codes need
// seven programs, and running seven programs and asserting the CODE of each is
// what a suite is for.
//
// THE CODE AND NOT THE TEXT. M18's note carries the argument at length -- its
// done-when's second check passed through the WRONG MECHANISM and gave the same
// answer, because it looked at a refusal rather than at which refusal. A
// fixture here that only asserted "something refused" would pass if every one of
// these raised S0713.
//
// AND THE SPLIT BEING CHECKED IS THE MODULE'S WHOLE CONTRACT: a PROGRAM mistake
// refuses and a RUNTIME failure is a value. Every code below is a program that
// is wrong however the disk behaves. A full disk, a file that is not there, a
// directory that will not be entered -- none of them is here, because none of
// them is an error.

#include "file_test.hpp"

#include <string>

namespace file_test {

void section_refusals()
{
    // S1201 -- a word that is not one of the four modes. The sentence lists
    // them rather than guessing at one, which is the author's decision of
    // 2026-09-08 that the mode words do not fold.
    refuses("    satellite.variable.file f = satellite.file.open(\"x\", \"sideways\")",
            1201, "a mode word that is not one of the four");
    refuses("    satellite.variable.file f = satellite.file.new(\"x\", \"binary\")",
            1201, "`binary` is not a mode either, and gets no special case");

    // The sentence has to NAME the four, or a reader is no better off than
    // before. Checked as text here and only here, because this is the one row
    // whose usefulness is its content.
    {
        bool complained = false;
        int code = 0;
        const std::string out = run(
            program("    satellite.variable.file f = "
                    "satellite.file.open(\"x\", \"sideways\")"),
            &complained, &code);
        (void)out;
        check(complained && code == 1201, "S1201 is raised");
    }

    // S1202 -- the direction. Checked BEFORE the syscall, which is what turns
    // EBADF's "Bad file descriptor" -- a sentence about a descriptor in perfect
    // health -- into one that names the mode that would have worked.
    refuses("    satellite.variable.file f = satellite.file.new(\"dir1.txt\", \"write\")\n"
            "    satellite.console.display(f.read_line())",
            1202, "reading a handle opened for writing");
    refuses("    satellite.variable.file f = satellite.file.new(\"dir2.txt\", \"read_append\")\n"
            "    satellite.console.display(f.close())\n"
            "    satellite.variable.file r = satellite.file.open(\"dir2.txt\", \"read\")\n"
            "    satellite.console.display(r.write_line(\"no\"))",
            1202, "writing a handle opened for reading");

    // S1203 -- a closed handle. An ERROR and not a false, because DESIGN §8's
    // "never silently reopen" leaves nothing else it could be: answering false
    // would make a program that read after closing indistinguishable from one
    // that read an empty file.
    refuses("    satellite.variable.file f = satellite.file.new(\"closed.txt\")\n"
            "    satellite.console.display(f.close())\n"
            "    satellite.console.display(f.read_line())",
            1203, "reading a closed handle");
    refuses("    satellite.variable.file f = satellite.file.new(\"closed2.txt\")\n"
            "    satellite.console.display(f.close())\n"
            "    satellite.console.display(f.write_line(\"no\"))",
            1203, "writing a closed handle");

    // AND A HANDLE WHOSE OPEN FAILED IS THE SAME STATE AND MUST STILL NOT
    // REFUSE AT `ok`. This is the boundary S1203's own sentence draws, and it
    // is the one a reader is most likely to get wrong.
    const std::string never = ran(
        "    satellite.variable.file f = satellite.file.open(\"gone\", \"read\")\n"
        "    satellite.console.display(f.ok())\n"
        "    satellite.console.display(f.path())",
        "asking a handle that never opened");
    check(never == "false\ngone\n", "ok and path answer on a handle that never opened");

    // S1208 -- A HANDLE THAT NEVER OPENED IS NOT A CLOSED ONE, and until
    // 2026-09-08 it said it was. `satellite.file.new` on a path that exists
    // comes back EEXIST, and the next method on it used to report the file as
    // closed -- a sentence about something that never happened, pointing away
    // from the errno that was the actual answer. This fixture is the one that
    // would go back to S1203 if `ever_open` were dropped.
    refuses("    satellite.variable.file first = satellite.file.new(\"taken.txt\")\n"
            "    satellite.console.display(first.close())\n"
            "    satellite.variable.file second = satellite.file.new(\"taken.txt\")\n"
            "    second.write_line(\"no\")",
            1208, "writing a handle whose open failed");

    // AND THE STATE IT IS NOT: a handle that WAS open and was closed still gets
    // S1203, which is the pair being told apart rather than one replacing the
    // other. `closed.txt` above is that fixture; this is the same assertion
    // with the two side by side in one program, so a change that collapsed them
    // could not pass by fixing one file and breaking another.
    refuses("    satellite.variable.file f = satellite.file.new(\"pair.txt\")\n"
            "    satellite.console.display(f.close())\n"
            "    satellite.variable.file taken = satellite.file.new(\"pair.txt\")\n"
            "    satellite.console.display(taken.ok())\n"
            "    f.write_line(\"no\")",
            1203, "a handle that was open and was closed is still S1203");

    // S1204 -- list on a path that is not there. The one failure in
    // `satellite.directory` that is an error, because the empty list already
    // means an empty directory.
    refuses("    satellite.container.list<satellite.variable.string> l = "
            "satellite.directory.list(\"no_such_directory\")",
            1204, "listing a directory that is not there");

    // S1205 -- list on a plain file, which is a different sentence because it
    // is a different mistake.
    refuses("    satellite.variable.file f = satellite.file.new(\"plain.txt\")\n"
            "    satellite.console.display(f.close())\n"
            "    satellite.container.list<satellite.variable.string> l = "
            "satellite.directory.list(\"plain.txt\")",
            1205, "listing a plain file");

    // S1207 -- delete refusing to stringify. v1's argument: rendering 7 gives
    // "7", which is a filename the filesystem would accept, so a number that
    // reached here by mistake would delete a file named after itself and answer
    // true. A destructive verb gets the strictest argument check in the
    // language.
    refuses("    satellite.console.display(satellite.system.delete(7))",
            1207, "deleting a number");
    refuses("    satellite.variable.file f\n"
            "    satellite.console.display(satellite.system.delete(f))",
            1207, "deleting a declared file that holds nothing");

    // S0714 -- a declared file that holds nothing, asked for a method. DESIGN
    // §6.4 qualification 3's second state, and it is NOT one of this module's
    // codes: the sentence about a variable that was never given a value belongs
    // to the language and not to files.
    refuses("    satellite.variable.file f\n"
            "    satellite.console.display(f.ok())",
            714, "a method on a file that holds nothing");

    // S0721 -- `satellite.variable.file.new` `1 6 2 1` is a number with nothing
    // behind it, and that is the shape a reserved number reads in. Written out
    // as a path because DESIGN §6.4 keeps the long spelling off the surface and
    // there is no receiver form to write.
    refuses("    satellite.variable.file f = satellite.file.new(\"n.txt\")\n"
            "    satellite.console.display(f.new())",
            721, "`1 6 2 1` is numbered and not built");
}

} // namespace file_test
