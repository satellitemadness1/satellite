// satellite.variable.file: one handle, opened, read, written, closed, cleared
// and reopened. Part of the eval_test binary; the harness these call is
// declared in eval_test.hpp.

#include "eval_test.hpp"

#include <cstdio>
#include <string>

// Both sections below work on this one path, so it is named once here rather
// than twice. It was a local of the single block the two of them were, and
// that block is what became the functions -- satellite.file.new runs between
// them, and lives in eval_test_filesystem.cpp because there was no room for it
// here.
static const std::string path = "/tmp/satellite_eval_test_file.txt";

void eval_test_file_basics()
{
    // --- satellite.variable.file, §8.3 --------------------------------------
    // A reference type with an EXPLICIT close that returns a status, because a
    // destructor cannot report that close failed with ENOSPC or EIO and
    // buffered writes commit at close.
    remove(path.c_str());

    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"write\")\n"
                 "f.ok()\n"
                 "f.write(\"one\\ntwo\\n\")\n"
                 "f.close()\n",
                 "true\ntrue\ntrue\n", "open, write, close");

    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"read\")\n"
                 "satellite.variable.string t = f.read()\n"
                 "f.close()\n"
                 "t.length()\n"
                 "t.starts_with(\"one\")\n",
                 "true\n8\ntrue\n", "open, read, close");

    // encode_raw, never encode. §3.3's defect in its third home: a source
    // file containing \home would otherwise be rewritten to the reader's
    // home directory before the lexer ever saw it.
    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"write\")\n"
                 "f.write(\"C:\\\\home\")\n"
                 "f.close()\n"
                 "satellite.variable.file g = satellite.file.open(\"" + path +
                     "\", \"read\")\n"
                 "satellite.variable.string t = g.read()\n"
                 "g.close()\n"
                 "t.length()\n",
                 "true\ntrue\ntrue\n7\n",
                 "a backslash read from a file is not expanded");

    // Append does not truncate.
    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"append\")\n"
                 "f.write(\"!\")\n"
                 "f.close()\n"
                 "satellite.variable.file g = satellite.file.open(\"" + path +
                     "\", \"read\")\n"
                 "satellite.variable.string t = g.read()\n"
                 "g.close()\n"
                 "t.length()\n",
                 "true\ntrue\ntrue\n8\n", "append does not truncate");

    // A failed open is a VALUE, not an error: "does this file exist" has to
    // be answerable without killing the program that asked.
    check_output("satellite.variable.file f = "
                 "satellite.file.open(\"/tmp/satellite_no_such_file_xyz\", \"read\")\n"
                 "f.ok()\n"
                 "f.error().length() > 0\n",
                 "false\ntrue\n", "a failed open is a value, not an error");

    // Closing twice is not a failure, and nothing ever silently reopens.
    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"read\")\n"
                 "f.close()\n"
                 "f.close()\n"
                 "f.ok()\n",
                 "true\ntrue\nfalse\n", "close is idempotent, reopen never");

    check_error("satellite.variable.file f = satellite.file.open(\"" + path +
                    "\", \"read\")\n"
                "f.close()\n"
                "f.read()\n",
                "closed file", "reading a closed file is an error");

    check_error("satellite.file.open(\"" + path + "\", \"sideways\")\n",
                "must be", "an unknown mode is rejected");

    check_error("satellite.file.open(\"" + path + "\")\n",
                "takes 2 arguments", "open checks its arity");

    // --- read AND write through one handle ------------------------------
    // "read_append" is a fourth MODE WORD and not a second axis on the
    // second argument, and this block is what the choice buys: every case
    // above still passes with its call written exactly as it was, because
    // the slot never changed meaning. §8.3.1's "the three words say at the
    // call site what a bitmask never does" is now four words saying it.
    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"write\")\n"
                 "f.write(\"seed\\n\")\n"
                 "f.close()\n"
                 "satellite.variable.file g = satellite.file.open(\"" + path +
                     "\", \"read_append\")\n"
                 "g.write(\"more\\n\")\n"
                 "g.read().length()\n"
                 "g.close()\n",
                 "true\ntrue\ntrue\n10\ntrue\n",
                 "read_append writes and reads back through one handle");

    // .read() IS THE WHOLE FILE, and the second call proves it. Before this
    // change the same program printed 10 then 0, because the offset was at
    // the end and "" is also how an empty file reads — a different fact
    // wearing the same string, which is why the answer had to change rather
    // than be documented.
    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"read\")\n"
                 "f.read().length()\n"
                 "f.read().length()\n"
                 "f.close()\n",
                 "10\n10\ntrue\n", "read is the whole file, every time");

    // A handle knows which way it was opened, and says so instead of
    // letting the kernel answer EBADF about a descriptor in perfect health.
    check_error("satellite.variable.file f = satellite.file.open(\"" + path +
                    "\", \"write\")\n"
                "f.read()\n",
                "not opened for it", "a write handle refuses to read");
    check_error("satellite.variable.file f = satellite.file.open(\"" + path +
                    "\", \"read\")\n"
                "f.write(\"x\")\n",
                "not opened for it", "a read handle refuses to write");

    // --- .clear() -------------------------------------------------------
    // Zero bytes, and the handle stays usable: this is truncate AND rewind,
    // because ftruncate alone leaves the offset where it was and the next
    // write would pad the gap with NULs — a "cleared" file longer than the
    // one that was there.
    // Seeded in the same program rather than relying on what an earlier
    // case left behind: a check_error above opens this path in "write",
    // which truncates, so a test that assumed the previous contents was
    // passing on a coincidence — and did, until the ordering moved.
    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"read_append\")\n"
                 "f.clear()\n"
                 "f.write(\"seeded\")\n"
                 "f.read().length()\n"
                 "f.clear()\n"
                 "f.read().length()\n"
                 "f.write(\"after\")\n"
                 "f.read().length()\n"
                 "f.close()\n",
                 "true\ntrue\n6\ntrue\n0\ntrue\n5\ntrue\n",
                 "clear empties the file and leaves the handle usable");

    check_error("satellite.variable.file f = satellite.file.open(\"" + path +
                    "\", \"read\")\n"
                "f.clear()\n",
                "not opened for it", "a read handle refuses to clear");

    check_error("satellite.variable.file f = satellite.file.open(\"" + path +
                    "\", \"write\")\n"
                "f.close()\n"
                "f.clear()\n",
                "closed file", "clearing a closed file is an error");
}

void eval_test_file_reopen()
{
    // --- .open() reopens; nothing else does -----------------------------
    // §8.3: "After close, other snapshots see a closed handle and get a
    // clean language-level error; never silently reopen." Both halves are
    // pinned here. The methods still refuse a closed handle — that is the
    // check_error above — and the reopen that IS allowed is the one written
    // down in the program, with a status the caller reads.
    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"write\")\n"
                 "f.write(\"reopen me\")\n"
                 "f.close()\n"
                 "satellite.variable.file g = satellite.file.open(\"" + path +
                     "\", \"read\")\n"
                 "g.close()\n"
                 "g.ok()\n"
                 "g.open()\n"
                 "g.ok()\n"
                 "g.read().length()\n"
                 "g.close()\n",
                 "true\ntrue\ntrue\nfalse\ntrue\ntrue\n9\ntrue\n",
                 "open reopens a closed handle, explicitly");

    // Already open is true and does nothing, the same answer close gives to
    // a second close. Opening again would leak the live descriptor and swap
    // the file description out from under every other snapshot with no
    // close having happened, which is the thing §8.3's sentence protects.
    check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                     "\", \"read\")\n"
                 "f.open()\n"
                 "f.ok()\n"
                 "f.close()\n",
                 "true\ntrue\ntrue\n", "open on a live handle is a no-op");

    // A failed open can be retried once the reason has gone away, and the
    // stale errno goes with it: .ok() true beside .error() still naming
    // ENOENT would be two true sentences that read as a contradiction.
    {
        const std::string later = "/tmp/satellite_eval_test_later.txt";
        remove(later.c_str());
        check_output("satellite.variable.file f = satellite.file.open(\"" +
                         later + "\", \"read\")\n"
                     "f.ok()\n"
                     "satellite.variable.file g = satellite.file.new(\"" +
                         later + "\")\n"
                     "g.close()\n"
                     "f.open()\n"
                     "f.ok()\n"
                     "f.error()\n"
                     "f.close()\n",
                     "false\ntrue\ntrue\ntrue\n\ntrue\n",
                     "a failed open can be retried, and clears its errno");
        remove(later.c_str());
    }

    // --- there is no "text" and no "binary" -----------------------------
    // REFUSED, not invented. POSIX has no newline translation to switch
    // off; encode_byte maps every one of the 256 bytes to one SatChar and
    // decode maps it back, so .read() already round-trips arbitrary bytes;
    // and the one transformation that WOULD differ is encode(), which is
    // §3.3's defect in its third home. A flag that did nothing would be
    // worse than no flag, so the refusal says why rather than listing the
    // words that are allowed and leaving the caller to guess.
    check_error("satellite.file.open(\"" + path + "\", \"text\")\n",
                "needs none", "there is no text mode, and the refusal says why");
    check_error("satellite.file.open(\"" + path + "\", \"binary\")\n",
                "already holds arbitrary bytes",
                "there is no binary mode either");
    check_error("satellite.file.new(\"" + path + "\", \"text\")\n",
                "needs none", "new refuses the same word for the same reason");

    // The claim the refusal rests on, run rather than asserted in prose:
    // every byte from 0 to 255 that a file can hold survives .read(). NUL
    // and 0x80..0xFF are the interesting ones — a NUL would end a C string
    // and a high byte would be half a character in UTF-8, and satellite
    // text is neither.
    {
        const std::string binary = "/tmp/satellite_eval_test_bytes.bin";
        FILE *raw = fopen(binary.c_str(), "wb");
        if (raw) {
            for (int byte = 0; byte < 256; byte++)
                fputc(byte, raw);
            fclose(raw);
        }
        check_output("satellite.variable.file f = satellite.file.open(\"" +
                         binary + "\", \"read\")\n"
                     "f.read().length()\n"
                     "f.close()\n",
                     "256\ntrue\n",
                     "all 256 bytes read back, with no binary mode to ask for");
        remove(binary.c_str());
    }

    // A declaration cannot open anything, so a file starts nil — the same
    // rule a spacesuit-typed field follows, for the same reason.
    check_output("satellite.variable.file f\nf == satellite\n", "true\n",
                 "an unopened file is nil");

    remove(path.c_str());
}
