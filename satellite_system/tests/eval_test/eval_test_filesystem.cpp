// The filesystem beyond a single handle: satellite.file.new, which makes the
// file it opens, and satellite.directory and satellite.system.delete, which
// name paths rather than hold them open. Part of the eval_test binary; the
// harness these call is declared in eval_test.hpp.

#include "eval_test.hpp"

#include <cstdio>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

void eval_test_file_new()
{
    // --- satellite.file.new ---------------------------------------------
    {
        const std::string fresh = "/tmp/satellite_eval_test_new.txt";
        remove(fresh.c_str());

        // Open already, in read_append, with no mode argument and no
        // .open() call — which is the whole of "not even require .open()
        // for new files".
        check_output("satellite.variable.file f = satellite.file.new(\"" +
                         fresh + "\")\n"
                     "f.ok()\n"
                     "f.write(\"made\")\n"
                     "f.read().length()\n"
                     "f.close()\n",
                     "true\ntrue\n4\ntrue\n",
                     "new creates, opens and reads back with no mode given");

        // The path now exists, so the second call REFUSES — and refuses as
        // a VALUE, holding EEXIST, exactly as §8.3.1 requires of a failed
        // open. The file is checked afterwards to be sure the refusal was a
        // refusal and not a truncation reported politely.
        check_output("satellite.variable.file f = satellite.file.new(\"" +
                         fresh + "\")\n"
                     "f.ok()\n"
                     "f.error()\n"
                     "satellite.variable.file g = satellite.file.open(\"" +
                         fresh + "\", \"read\")\n"
                     "g.read().length()\n"
                     "g.close()\n",
                     "false\nFile exists\n4\ntrue\n",
                     "new refuses an existing path, and does not truncate it");

        // A mode may still be named. "write" here would have truncated
        // through satellite.file.open; through .new it cannot, because
        // O_EXCL decides before the mode's flags ever apply.
        check_output("satellite.variable.file f = satellite.file.new(\"" +
                         fresh + "\", \"write\")\n"
                     "f.ok()\n",
                     "false\n", "an explicit mode does not defeat O_EXCL");

        remove(fresh.c_str());
        check_output("satellite.variable.file f = satellite.file.new(\"" +
                         fresh + "\", \"append\")\n"
                     "f.ok()\n"
                     "f.write(\"x\")\n"
                     "f.close()\n",
                     "true\ntrue\ntrue\n", "new takes a mode word");

        check_error("satellite.file.new()\n", "takes 1 or 2 arguments",
                    "new checks its arity");
        check_error("satellite.file.new(\"" + fresh + "\", \"sideways\")\n",
                    "must be", "new rejects an unknown mode");

        remove(fresh.c_str());
    }
}

void eval_test_directory()
{
    // --- satellite.directory ------------------------------------------------
    // The working directory is PROCESS-wide, not per-evaluator, so these cases
    // put it back where they found it. A test that leaves the process somewhere
    // else breaks every later test that opens a relative path, and it would do
    // it at a distance, in a different case, for no visible reason.
    {
        char saved[4096];
        const char *back = getcwd(saved, sizeof saved);

        check_output("satellite.directory.exists(\"/tmp\")\n", "true\n",
                     "an existing directory");
        check_output("satellite.directory.exists(\"/tmp/no_such_dir_xyz\")\n",
                     "false\n", "a missing directory");
        // A file is not a directory. stat() alone would say yes; S_ISDIR is
        // what makes the answer mean what the name says.
        check_output("satellite.directory.exists(\"/etc/hostname\")\n", "false\n",
                     "a file is not a directory");

        check_output("satellite.directory.change(\"/tmp\")\n"
                     "satellite.directory.current()\n",
                     "true\n/tmp\n", "change then read back");
        // Like a failed satellite.file.open, a refused change is a VALUE: the
        // program that asked has to survive being told no.
        check_output("satellite.directory.change(\"/tmp/no_such_dir_xyz\")\n",
                     "false\n", "changing to nowhere is false, not an error");
        check_output("satellite.directory.change(\"/etc/hostname\")\n", "false\n",
                     "changing to a file is false");

        check_error("satellite.directory.change()\n", "takes 1 argument",
                    "change checks its arity");
        check_error("satellite.directory.current(\"x\")\n", "takes 0 arguments",
                    "current takes none");
        check_error("satellite.directory.change(7)\n", "wants a satellite.variable.string",
                    "change checks its argument type");

        if (back)
            (void)!chdir(saved);
    }
}

void eval_test_delete()
{
    // --- satellite.system.delete --------------------------------------------
    // Every case here makes what it deletes, so nothing depends on what a
    // previous run left behind: a test whose "it was not there" is true because
    // an earlier run removed it passes for the wrong reason, and keeps passing
    // after the code stops working.
    {
        const std::string dir = "/tmp/satl_delete_test_xyz";
        const std::string leaf = dir + "/leaf.txt";
        auto touch = [&] {
            (void)::mkdir(dir.c_str(), 0755);
            FILE *f = fopen(leaf.c_str(), "w");
            if (f)
                fclose(f);
        };

        // In quotes at the call site. The file is gone afterwards, and .ok() on
        // a fresh open is how that is asked without trusting the answer above.
        touch();
        check_output("satellite.system.delete(\"" + leaf + "\")\n"
                     "satellite.variable.file g = satellite.file.open(\"" +
                         leaf + "\", \"read\")\n"
                     "g.ok()\n",
                     "true\nfalse\n", "a file named in quotes goes");

        // The same path through a variable. Indistinguishable from the literal
        // by the time the argument is a value, which is the property being
        // pinned: one of these two working and the other not would mean the
        // call was reading syntax rather than a string.
        touch();
        check_output("satellite.variable.string p = \"" + leaf + "\"\n"
                     "satellite.system.delete(p)\n",
                     "true\n", "a file named by a variable goes");

        // An open satellite.variable.file, which names its own path -- and the
        // descriptor survives, because POSIX unlinks the NAME and the inode
        // outlives it.
        touch();
        check_output("satellite.variable.file h = satellite.file.open(\"" +
                         leaf + "\", \"read\")\n"
                     "satellite.system.delete(h)\n"
                     "h.ok()\n",
                     "true\ntrue\n",
                     "a file handle deletes itself and stays readable");

        // A directory with something in it. False, and STILL THERE afterwards:
        // there is no flag asking for a tree and no silent descent.
        touch();
        check_output("satellite.system.delete(\"" + dir + "\")\n"
                     "satellite.directory.exists(\"" + dir + "\")\n",
                     "false\ntrue\n", "a non-empty directory is refused, not emptied");

        // Emptied by hand, the same call answers true -- so the false above was
        // about the contents and not about directories.
        ::unlink(leaf.c_str());
        check_output("satellite.system.delete(\"" + dir + "\")\n"
                     "satellite.directory.exists(\"" + dir + "\")\n",
                     "true\nfalse\n", "an empty directory goes");

        // Twice. Deleting what is already gone is a value and not an error, so
        // a cleanup step is safe to run again.
        check_output("satellite.system.delete(\"" + dir + "\")\n", "false\n",
                     "deleting nothing is false, never an error");

        // The lstat case. The LINK was named, so the link goes and the
        // directory it pointed at is untouched -- stat() here would have aimed
        // rmdir at the target.
        {
            const std::string link = "/tmp/satl_delete_link_xyz";
            (void)::mkdir(dir.c_str(), 0755);
            ::unlink(link.c_str());
            if (::symlink(dir.c_str(), link.c_str()) == 0)
                check_output("satellite.system.delete(\"" + link + "\")\n"
                             "satellite.directory.exists(\"" + dir + "\")\n",
                             "true\ntrue\n",
                             "a symlink is unlinked; its target is not");
            ::unlink(link.c_str());
            ::rmdir(dir.c_str());
        }

        check_error("satellite.system.delete()\n", "takes 1 argument",
                    "delete checks its arity");
        // A number is REFUSED and not stringified: to_string(7) is "7", which
        // is a filename, so accepting it would delete a real file and report
        // success.
        check_error("satellite.system.delete(7)\n", "satellite.variable.string",
                    "delete refuses a number");
        // Nil says how a caller gets here rather than merely that they did.
        check_error("satellite.variable.file f\nsatellite.system.delete(f)\n",
                    "nil names no path", "delete refuses an unopened file");
    }
}
