# FILE_AND_DRIVE_PLAN — folders, files and drives from the satellite console (FD-1..FD-5)

Written 2026-10-21 from the author's message that day, and from the author's answers to five questions the
same day. Checked against the working tree on BUILD 0079 (HEAD b48fb5d56 says 0058). FD-1 is built (BUILD 0081); FD-2 is
in the tree (BUILD 0083) with its gate not finished; nothing after it is built.

The author's message, as written:

> *"I need some tools built into the satellite console, I need a couple of commands: make empty file, delete
> file, move file, copy file, paste file, mount a drive, display all drives, unmount drive, and remount drive
> which is just unmount and mount together, and I need create directory, and so the syntax needs to be this
> syntax for these commands:*
>
> *satellite.directory.create("/path/dir_name")*
> *satellite.file.new("filename.ext", "text" or "binary" or "empty"? is empty a filetype?)*
> *satellite.file.delete("filename.ext")*
> *satellite.drive.list() // lists all drives*
> *satellite.file.copy("filename") // copies the file to the clipboard,*
> *satellite.file.paste("location" or ()) pastes into the current working directory,*
> *satellite.file.move("/path/filename.ext", "/path/to/dir/")*
> *satellite.drive.mount("drive")*
> *satellite.drive.release("drive")*
> *satellite.drive.remount("drive") // which is just combining the last two together,*
>
> *Shouldn't be too hard to build these commands, and "satellite.file.delete("filename")" we must make the
> command always go through, or ask for the password to the account that owns it, it has to do one of these
> things, so.. we have to build it to do one of those things with C++, because if a user is going to go
> through the trouble of.. typing in "satellite.file.delete("filename.ext") then obviously they are sure that
> they want to delete that particular file, of course..."*

The author's answers, word for word:

> clipboard: *"transfer the file to /tmp/.satl/tmp/file-current and keep a... /tmp/.satl/tmp/file-previous-001
> with no extension, just a number"*; how many: *"just keep every file in /tmp and it will automatically be
> cleaned every 3 days, i'll set the linux kernel to clean every 3 days"*
>
> mounting: *"ask for the sudo password if it is required to mount that drive, or all drives, but ask for the
> password only if that command is typed in..."*; whose password: *"My own password, through sudo"*
>
> a name already there: *"Recommend that it be overwritten but always ask first"*

---
## Context

The author (2026-10-21) wants these to be built into the console and the language:

    satellite.directory.create("/path/dir_name")
    satellite.file.new("filename.ext", "text" | "binary")
    satellite.file.delete("filename.ext")
    satellite.file.copy("filename")            copies the file to the clipboard
    satellite.file.paste("location") / paste() pastes into that folder, or the working directory
    satellite.file.move("/path/filename.ext", "/path/to/dir/")
    satellite.drive.list()                     lists all drives
    satellite.drive.mount("drive")
    satellite.drive.release("drive")
    satellite.drive.remount("drive")           release, then mount

What exists today:
- `satellite.file.new` is built (`1 8 1`, `1 8 4`). It accepts "text" and "binary", but "binary" makes no file
  at all (satellite_file.cpp:312-315 returns before the open).
- Nothing else is built. There is no mount code, no clipboard and no password code anywhere.
- `satellite.directory.system()` (drives.cpp) draws a table of mounted drives only.
- MISSING_SYNTAX.md MS-24..26 planned `copy(from, to)`, `rename` and `delete` with other spellings. Its Q13 asked
  "does delete ask are you sure". This plan replaces those three with the author's spellings, and the author's
  answer to Q13 is no.

"Empty" is not a kind of file. Every new file starts with no bytes. "text" and "binary" only say how its handle
reads and writes later, so `satellite.file.new("name.ext")` already makes an empty file.

### The author's answers (2026-10-21)
1. **delete**: it always goes through, or it asks for the password of the account that owns the file.
   There is no "are you sure".
2. **copy** puts the file's bytes into `/tmp/.satl/tmp/file-current`. The copy before it becomes
   `/tmp/.satl/tmp/file-previous-NNN`, with no extension. Every copy is kept; the author will set /tmp to be
   cleaned every 3 days.
3. **mount / release**: they ask for the author's own password, through sudo, and only when the system
   requires one for that drive. The question is asked only when the command is typed in, never at any other
   time. The author is not in the sudo group yet and will add himself.
4. **paste / move onto a name already there**: they overwrite it, but always ask first.

## The words

| word | number | what it does | answers |
|---|---|---|---|
| `satellite.file.delete(path)` | 1 8 7 | Removes a file. A link is removed itself, never what it points to. | bool |
| `satellite.file.copy(path)` | 1 8 8 | Copies the bytes into /tmp/.satl/tmp/file-current; the last one becomes the next file-previous-NNN. | bool |
| `satellite.file.paste()` | 1 8 9 | Puts file-current, under its first name, into the working directory. | bool |
| `satellite.file.paste(d)` | 1 8 10 | The same, into folder d. | bool |
| `satellite.file.move(path, d)` | 1 8 11 | Moves the file into folder d, keeping its name. Works across drives too. | bool |
| `satellite.directory.create(d)` | 1 18 8 | Makes the folder and any missing folders above it. A folder already there is fine. | bool |
| `satellite.drive` | 1 36 | The new family. | |
| `satellite.drive.list()` | 1 36 1 | A table of every drive, mounted or not. | nothing (it prints) |
| `satellite.drive.mount(d)` | 1 36 2 | Mounts a drive. | bool |
| `satellite.drive.release(d)` | 1 36 3 | Unmounts a drive. | bool |
| `satellite.drive.remount(d)` | 1 36 4 | release, then mount; mount is not tried when release fails. | bool |

`satellite.file.new(path, "binary")` changes so that it makes the empty file. Reading and writing through a binary
handle stays FO-8. The numbers are the next free ones. Rows go in `words/words_004.tsv`, then run
`python3 words/make_words.py` and `python3 satellite/bytecode/make_word_codes.py`, never by hand.

## Rules

1. **A word typed alone at the prompt says why it failed**, in one line. This is the
   `is_one_change`/`move_the_prompt` pattern (session.cpp:258-306). `satellite.file.new` joins it too.
   On success it prints nothing, except mount, which prints where the drive went.
   In a program, the words answer true or false and never stop it, as the file family's rule says
   (SATELLITE_FILE_OPERATIONS.md:147). Misuse is still refused before the run, as today: the wrong count or type
   of arguments, or a NUL in a path (S523).
2. **Paths** follow today's rules. At the prompt they are relative to the working directory. In a program it is
   `resolved()` in file_calls.cpp:195, which is moved out of its unnamed namespace so these words can share it.
3. **delete goes through or asks:**
   - It tries `unlink` first. With no question, a write-protected file is deleted.
   - If the system says no (EACCES/EPERM), satl asks for the password of the account that may delete it. That
     is the owner of the folder the file is in; in a sticky folder such as /tmp, it is the file's own owner.
     satl then deletes the file as that account with `/usr/bin/su <owner> -c 'exec /usr/bin/unlink -- "$0"' <path>`.
     The path is passed as an argument and never pasted into the command.
     su runs on a pseudo-terminal of its own, so it works from a terminal and from the GTK console alike.
   - That account may be the person at the console, or one with no login shell or a locked password. Then
     satl asks for their own password and runs `sudo -S` (rule 6).
   - A folder is refused, because delete takes files. So are an immutable file and a read-only drive, each with
     its reason.
4. **The password is never kept:**
   - It is read with echo off, straight from the terminal, and never through the LineReader. So it never
     reaches the .c4 console files or satellite.log.
   - It is written once into su's or sudo's input, then wiped with `explicit_bzero`.
   - It is asked only on the console where the line runs. With no terminal, nothing is asked, and the word
     answers false with the reason.
   - A wrong password is said once and answers false.
5. **Clipboard (`/tmp/.satl/tmp`):**
   - The folder is made 0700. satl refuses it if it is a link or belongs to another account, because a
     predictable path in /tmp could be planted.
   - copy writes beside under a temporary name and renames into file-current, so a stopped copy leaves the
     clipboard as it was.
   - The file-current before it moves to the next number past the highest file-previous-NNN, so the highest
     number is always the newest.
   - Each file's first name is kept in `/tmp/.satl/tmp/names`, one line each (`file-current<TAB>report.txt`),
     because paste needs it.
   - If /tmp fills, the word answers false with the reason. Ctrl-C (`stop_flag()`) stops a long copy.
6. **Overwrite asks first.** paste or move onto a name already there asks
   `"<name>" is already in <folder> -- replace it? [y/N]`. Only y replaces it, in one rename. With no
   terminal there is no question, and nothing is replaced.
   - paste writes beside and then renames.
   - move uses `renameat2(RENAME_NOREPLACE)`, then a plain rename after a yes. Across drives (EXDEV) it copies as
     paste does, then unlinks the original. If that unlink fails, it says the file is now in both places.
   - move and paste take files, not folders. Neither asks for a password: a folder satl may not write into
     gives a reason.
7. **drive.list reads only /sys/class/block and /run/udev/data**, plus statvfs for drives already mounted, as
   drives.cpp does. A sleeping disk is never woken; this matters for the 9 TB spinning drives.
   - Its columns are: name (sdd1), label, type, size, mounted on (or "not mounted"), and drive (model, and "usb").
   - A row is any partition or disk that holds a filesystem (udev's `ID_FS_USAGE=filesystem`). Swap and LVM
     members are not rows.
   - It prints the same table at the prompt and in a program.
8. **A drive is named** the way drive.list shows it: `sdd1`, `/dev/sdd1`, or its label (`purple`).
   release and remount also accept the folder it is mounted on.
9. **mount / release ask for the sudo password only when the system requires it:**
   - **First try:** `udisksctl mount|unmount --no-user-interaction -b /dev/X`. This is how the desktop mounted
     /media/madness/purple10 and Ventoy; a USB drive needs no password.
   - **When to fall back:** only if udisks answers `org.freedesktop.UDisks2.Error.NotAuthorized` (error names
     are not translated), or if udisksctl is missing.
   - **The question:** `[sudo] password for madness:`, read hidden as in rule 4.
   - **What runs:** `sudo -S -p '' -v` checks the password, so a wrong one is told apart from a failed mount.
     Then one of:
     - `sudo -n /usr/bin/mount --mkdir -- /dev/X /media/<user>/<label or name>`, adding `-o uid=,gid=` for
       vfat, exfat and ntfs;
     - `sudo -n /usr/bin/umount -- /dev/X`.
   - **When sudo refuses the account:** "not in the sudoers file" is said as that, with what to do.
   - **Already in the wanted state:** a drive already mounted is said and answers true; one already released
     answers true.
   - **remount** asks at most once.
   - Every helper is named by its absolute path and started through `start_a_program`
     (satellite_variable_program/program_spawn.cpp:165), with argument vectors and never a shell. su is the one
     exception, and gets its pty.

## Where it goes (reuse first)

- **Numbers:** `words/words_004.tsv`, then the two generators.
- **File words:** these are the file family's no-library route. In `satellite/bytecode/file_calls.cpp`, change
  `is_file_word` (:358), `file_word_arity` (:344), `file_word_takes` and `call_file_word` (:365). The work itself
  goes in a new `satellite/satellite_variable_file/file_moves.{hpp,cpp}`, holding delete, copy, paste and move.
  Each returns `{bool done; std::string why;}`, so the prompt can print `why`.
  - Reuse `holds_a_nul`, `system_reason`, `what_it_is` and the write-beside-then-rename save in
    `satellite_file.cpp:963-1054` (fchmod, keeping the mode), and `owner_of()` in `satl/listing.cpp:87`.
- **directory.create:** the directory family's library route: a function in `satellite-numbers/directory_words.hpp`,
  a new `satellite-numbers/words/1.18.8.cpp`, a row in `word_table.cpp`, `WORD_SOURCES` in
  `make_support/040-sources.mk:117`, and the bool branch in `call_directory_word` (expression.cpp:2105).
- **Drive family:** a new `satellite/bytecode/drive_calls.{hpp,cpp}`, shaped like file_calls.
  - Dispatch goes in `expression.cpp` beside `is_file_word` (:2229), with exemptions in `program_check.cpp`
    (:2744-2746, :2867).
  - The work goes in a new `satellite/machine/drive_mounts.{hpp,cpp}`: the /sys and udev walk, name resolution,
    mount and release. It reuses `filesystems::mounted_drives()` (machine/filesystems.hpp:115) and drives.cpp's
    `model_of`, `disk_under` and `device_and_drive`, made shareable.
- **Asking at the console:** a new `satellite/machine/ask_at_the_console.{hpp,cpp}` with two functions:
  - `hidden_line(prompt)` turns termios ECHO off on stdin and restores it on every path, including Ctrl-C. The
    terminal is cooked while a typed line runs (prompt/line_reader.hpp:9-10).
  - `yes_or_no(question)`.
- **Running as another account:** a new `satellite/machine/as_another_account.{hpp,cpp}`:
  - `as_owner(account, password, words)` runs su on an `openpty` pty, waits for its prompt, then writes the
    password;
  - `with_sudo(password, words)` uses `sudo -S`.

  Test builds can point both at stand-in programs; this is compiled into the cases binary only, never an
  environment variable of satl.
- **The prompt:** in `satellite/satl/session.cpp`, a typed-alone check for the new codes and `1 8 1`/`1 8 4`,
  next to `is_one_change` (:258). It calls the same helpers and prints `why` in the prompt's lettering.
- **Codes:** reuse the existing ones (file_not_found 39/S510, not_a_file 41/S512, path_holds_a_nul 31/S523,
  file_unwritable 44). A new machine code (next free 85, S515/S524/S53x) is added only if a word must stop a
  program, and none is planned to.
- **Help:** new pages, each with an 80-dash header and a line in `satellite.help/help.txt`:
  - `satellite.help/satellite.file/help_text.txt`: new, delete, copy, paste, move, and directory.create;
  - `satellite.help/satellite.drive/help_text.txt`.

  Mount and release examples are written as typed lines, not `satellite.include(satellite)` programs, so
  `utility/check_help_examples.py` never mounts anything.
- **Docs kept true:**
  - SATELLITE_FILE_OPERATIONS.md:367 and :457 promise `1 8 7` to the read-only open; those lines get its new number.
  - `delete_calls.hpp:31-32` and `satellite.help/satellite.delete` point to `satellite.file.delete`.
    `1 22 1 satellite.system.delete` stays numbered and unbuilt.
  - MISSING_SYNTAX.md MS-24..26 say this plan replaces them.

## Milestones (each on the author's word, one at a time, through the lean gate)

- **Step 0 (on approval):**
  - this plan goes into the repo as `FILE_AND_DRIVE_PLAN.md`, beside KEY_PLAN.md;
  - a memory pointer;
  - a snapshot in `~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-21-before-fd-1/`.
- **FD-1 — BUILT 2026-10-21, BUILD 0081:**
  - all eleven numbers;
  - `satellite.directory.create`;
  - "binary" makes the empty file;
  - the prompt's typed-alone line (rule 1) for new and create;
  - the satellite.file help page started.

  **AS BUILT.** check.sh: 1550 passed and 5 failed, all 5 the machine's (no /usr/bin/time ×4, umask 0002 ×1).
  The patch is backups/2026-10-21-before-fd-1/fd-1.patch. The fresh reader found five things, all fixed:
  - `file_cases` still wanted a binary `new` to make nothing;
  - `create("/data/two/deeper")` in a program went to the filesystem's root when only P/data was there. A
    leading / now stays under the program's folder when the path's FIRST folder is there (`resolved(..., folders)`);
  - `create("a/k.txt")` on a file said "nothing can be made in it"; it now says "nothing was made";
  - `new("sub/")` with no sub said "is a directory"; it now says the path ends in /;
  - SATELLITE_FILE_OPERATIONS.md's read-only open moves from `1 8 7` to `1 8 12`.

  Not FD-1's: check_session.py's "... and its capsule runs" failed on BUILD 0079 too (twice alone, by hand), and
  passed in the final run; its block lines are typed without waiting for the reader.
- **FD-2:** `satellite.file.delete`, `ask_at_the_console`, `as_another_account` (su on a pty, plus sudo -S).

  **IN THE TREE, BUILD 0083, GATE NOT FINISHED (2026-10-21, at the author's /clear).** The snapshot is
  backups/2026-10-21-before-fd-2/.
  - **New files:** machine/ask_at_the_console.{hpp,cpp}, machine/as_another_account.{hpp,cpp},
    satellite_variable_file/file_moves.{hpp,cpp} and file_moves_cases.cpp (`make build/file_moves_cases`).
  - **The fresh reader's 9 findings are fixed:**
    - a NUL in the path;
    - a leading-/ path that followed a dangling link (resolved's `link_itself`);
    - an immutable or append-only file now refused with no question;
    - the password buffer reserved, so it never moves;
    - Ctrl-C in a file run puts ECHO back, then ends satl;
    - su ended when it asks a second question;
    - one whole_path, from the stat to the command;
    - `who_may_delete` exposed and tested;
    - su's and sudo's own lines matched only at the start of a line.
  - **Passing:** file_moves_cases (27 ok), check_session.py alone (63 ok, with 2 new FD-2 rows), and FD-1's and
    FD-2's check.sh rows. The full check.sh on 0083 was cut off when the session ended.
  - **NEXT:**
    - re-run ./check.sh (only the machine's 5 may fail);
    - `sh fd_diff.sh > fd-2.patch` in the snapshot;
    - mark FD-2 BUILT here.
  - **FD-3's code is drafted, not in the tree:** backups/2026-10-21-fd-3-drafts/ (NOTE.md says how to apply it).
- **FD-3:** `satellite.file.copy`, `paste()`, `paste(d)`, `move`, with the /tmp/.satl/tmp clipboard and the
  replace question.
- **FD-4:** `satellite.drive.list()` and the satellite.drive help page.
- **FD-5:** `satellite.drive.mount`, `release` and `remount`.

**The gate** for each milestone (NEW_MILESTONES.md:217-221, made lean on 2026-10-21):
- one implementer;
- its own check.sh rows, then all of check.sh;
- every help example run;
- ONE fresh reader;
- a final fix;
- the patch written beside the snapshot.

**How agents work:**
- They build only with `make INSTALL_AFTER_BUILD=no -j24`. A plain make installs, commits and uploads.
- Every probe uses
  `env -u DISPLAY WAYLAND_DISPLAY=satl-probe-no-display GDK_BACKEND=wayland SATL_NO_WINDOW=1 HOME=<scratch>/home XDG_RUNTIME_DIR=<scratch>/xdg build/satl`.
- The clipboard folder is `$TMPDIR/.satl/tmp`, which is /tmp/.satl/tmp when TMPDIR is unset. That lets check.sh and
  probes point TMPDIR into scratch, so they never touch the author's real clipboard.
- The install, on the author's word, is `MAKE=true sh satellite_enterprise/install.sh`.

## Taken without the author's word (say if one is wrong)

- **Where the password is asked:** on the console where the line runs, whether at the prompt or in a program's
  own console, and never at start-up or for drive.list. The other reading of "only if that command is typed in",
  the prompt only and never from a program, is a one-line change.
- **create** makes missing parent folders, and a folder already there is fine.
- **move's second argument** must be a folder that exists; a new file name there is not taken as a rename.
- **The folder a sudo mount makes** is /media/<user>/<label, else the device name>. It is left in place after
  release; udisks removes its own.
- **Previous copies count upward from the highest number left**, so a gap left by the cleaning is not refilled.

## Verification

- **check.sh rows for each milestone**, all in a scratch folder with `TMPDIR` and `HOME` in scratch:
  - create (nested folders, and a folder already there);
  - new "binary" makes a 0-byte file;
  - delete of the person's own file, a write-protected file, a folder (refused), a link (the link only), and a
    missing file (the why line);
  - copy, then paste twice: file-current, file-previous-001, and the names file;
  - paste and move onto a name already there, with `n` and `y` typed through check_session.py's real terminal;
  - move across two folders;
  - drive.list run on fake /sys and /run/udev trees, in the cases binary.
- **The password paths** are tested in a `*_cases.cpp` with stand-in su and sudo programs: the right password,
  the wrong one, no terminal, and "not in sudoers". A row also checks that the password never appears in
  `~/.satl/console/*.c4` or `satellite.log`.
- **The whole check.sh.** The baseline is 5 failures from this machine (no /usr/bin/time ×4, umask 0002 ×1);
  nothing new may fail. Then `utility/check_help_examples.py`.
- **By hand, only with the author present:**
  - in satl's console, `satellite.drive.release("Ventoy")`, then `satellite.drive.mount("Ventoy")`, which needs no
    password;
  - `satellite.drive.list()`;
  - after the author has joined the sudo group, mount an internal partition (e.g. `sdd1`/"purple") to see the sudo
    question;
  - delete a root-owned scratch file the author makes, to see the su question.
