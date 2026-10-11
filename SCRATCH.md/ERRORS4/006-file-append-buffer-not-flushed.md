# 006 -- Lines appended through one file handle stay in its 64 KiB buffer when a second handle or satellite.info reads the path: the reader sees too few lines, and the second handle's writes land before the held lines, out of program order

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** files and io  
**Kind:** wrong-answer  
**Severity:** medium

## What happens

Lines appended through one handle are not written before a second handle (or satellite.info) reads the file: the reader sees too few lines, and later writes from the second handle land before the held lines, out of program order.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.file w = satellite.file.new("log.txt")
    w.append("first")
    satellite.variable.file r = satellite.file.open("log.txt")
    satellite.console.display(r.size)
    r.append("second")
    r.save
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: Run from the program's own folder, with no log.txt there beforehand.

## What satl does

```
0
log.txt:
second
first

exit 0
```

## What it should do

r.size is 1, and log.txt holds "first" then "second", the order the program wrote them.

SATELLITE_FILE_OPERATIONS.md 3.7 says the append buffer is written out "when the buffer fills, before anything reads the file, and at save or close". Here a second open reads the file while the line is still held. The same handle answers the full size (w.size 2 in the hunter's version), so two views of one file disagree.

## Variants

Hunter's form: w appends alpha, beta, then r = open, and w.size is 2 while r.size is 0 (the file ends with both lines). After w.save, r.size is 2. satellite.info.file("log.txt") read into a list, then info[1]["line_count"], is 0 right after one append. In a for loop that opens the file three times and appends "entry" once per pass, after an earlier w.append("alpha"): a later open answers size 2 and the file ends as entry, entry, entry, alpha. The earliest-written line is last because it is flushed only when the program ends.

## Where it comes from

satellite/satellite_variable_file/satellite_file.cpp:618-627 (append holds bytes in pending_bytes_ and writes them only past kFlushBytes, 64 KiB) with save_appended at :878; bytecode/file_calls.cpp:341 (open_existing reads the disk and knows nothing of another handle's held bytes)

The append fast path keeps a per-handle buffer. It is written at 64 KiB, at save/close, or when the handle is destroyed. Nothing flushes a handle's held bytes when another handle or satellite.info opens the same path, so readers see the old file. When the held bytes finally go out with O_APPEND, they land after anything written in the meantime.

## Notes

- Why the skeptic kept it: Reproduced. Lines a handle has appended stay in that handle's own buffer, which is not written out when a second handle opens the same file or when satellite.info reads it. The second handle sees a shorter file. When the second handle then writes, the held lines land AFTER its lines, so the file ends up in the wrong order (second, first). SATELLITE_FILE_OPERATIONS.md 3.7 promises the buffer is written "before anything reads the file". Nothing about this is in ERROR.md, errors.md, ERRORS2, NEW_ERROR_LIST, CONTAINERS or the confirmed list. The doc's own 'Open' list names only a failed 64 KiB write and a lock between two runs.
- Nearest known entry: none
- Merged: Single finding.
