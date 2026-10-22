# KEY_PLAN — every line typed into a console, kept in a file of its day and brought back with the arrow keys

Written 2026-10-20 from the author's message that day, which divides the work into eight milestones and
asks that the eighth, Up and Down, go before Left and Right. A second message the same day names the two
numbers `arguments.input.cache` and `arguments.input.file.cache`, and asks for them as settings once the
key inputs are built and installed (KEY-10). The author then answered the six questions this plan asked,
and those answers are now its rules (Section 6 lists them).

It replaces the proposal in SCRATCH.md/OPEN_ITEMS_2026-10-20.md section 0b, which the author had not
agreed to. Checked against the working tree on 2026-10-20 (build/satl prints BUILD 0066; HEAD b48fb5d56
says 0058), and reviewed the same day. KEY-1 to KEY-6 are built (BUILD 0070, 0074, 0077, 0079 and, for KEY-5 and
KEY-6 together, 0085); nothing after them is yet.

---

## 1. The author's design

> *"into the satellite console I need it to save every key that is entered into the console, and when
> enter is pressed, create a new satellite.variable.string to save the input into, and if the up arrow
> key is pressed, then it cycles up and down through all input typed into the console, and for left and
> right keys to cycle through the individual consoles, so starting with the newest console, the current
> console, then left key takes you to what was typed in the previous console, and then left key again
> takes you to the previous still console input, so that all console input is saved into individual
> files, with a folder hierarchy of dates for folder names, month and day, then the individual consoles
> that were started for that day..."*

The author's eight milestones, as written:

```
1. create the month-and-day named folders to store the individual console files into
2. name the console files console-001.c4
3. create the code that saves each key typed (saving all of the a - z, A - Z, 0 - 9 ~` -_=+[{]}\|;:'",<.>/?
   any key that is in the satellite char set, we are saving the individual keys as satellite.variable.strings
   stored in ascii text files labeled .c4 (the c is for C++ and 4 stands for "quad ai" quad or, 4) which this
   is why we developed this interpreter, to encompass the quad ai in this language, so it is part of the quad
   ai essentially... just a very basic, not very exciting part of it.... So we store any key in the satellite
   interpreter charset as ascii in a .c4 labeled text file, one enter press per line...
4. we build the code that loads the month-and-day named folders into satellite.variable.string objects,
5. that can be cycled through with left and right arrows to cycle through the days that are loaded, saving
   the most recent 32mb worth of files,
6. once input stored reaches a 32mb check, we remove the oldest 15 console files from the 32mb, lowering it
   signficantly for the next 15 consoles
7. every startup the interpreter makes room if the filesizes are above 32 megabytes... this is a significant
   effort to count all of these files sizes,
8. make sure that we have the up and down arrow keys, like to cycle through the individual consoles input...
```

> *"you have to put the 8th milestone where it should be located, before the left and right arrow keys
> are programmed"*

The second message:

> *"and can you set 32 mb and 15 files settings into satellite.variable.arguments arguments, and then
> arguments.input.cache for the 32 megabytes and arguments.input.file.cache = 15 after you build and
> install the actual key inputs"*

> *"so they can configure these variables and they are saved if entered into the prompt, like they have
> the other stuff"*

The answers to Section 6's six questions:

> *"1 yes 2 include the year 3 make it print, as long as the user has not pressed enter, 4 you decide 5
> including all consoles we type with the keyboard into 6 if the user pressed left or right key while
> there is input, it moves the cursor through the input that has already been typed, until it is all the
> way to the left, then it switches at last to the left most set of inputs from the previous console
> file..."*

What it makes, on the disk and at the keys:

```
~/.satl/console/
    2026-10-19/
        console-001.c4
        console-002.c4
    2026-10-20/
        console-001.c4           one line for each Enter press
        console-002.c4           this console

  Left <-                                                                                  -> Right
  2026-10-19 console-001   2026-10-19 console-002   2026-10-20 console-001   2026-10-20 console-002 (this one)
                         Up and Down go through the lines of the console shown
```

The milestones are numbered from 1, as the author numbered them, and listed in the order they are built:

| The author's | This plan | What it builds |
|---|---|---|
| 1 | KEY-1 | the day's folder |
| 2 | KEY-2 | the console's own file, console-001.c4 |
| 3 | KEY-3 | each Enter press kept: a satellite.variable.string and a line of the file |
| **8** | **KEY-4** | **Up and Down** |
| 4 | KEY-5 | the day folders found, and read back into satellite.variable.strings |
| 5 | KEY-6 | Left and Right, from console to console, over the most recent 32 MB |
| 6 | KEY-7 | 32 MB reached: the oldest 15 console files removed |
| 7 | KEY-8 | every start: the sizes counted and room made |
| answer 5 | KEY-9 | the same for every console a program makes |
| second message | KEY-10 | `arguments.input.cache` and `arguments.input.file.cache`, settings a person changes |

- The author's 8th is KEY-4, straight after KEY-3, because Up and Down need only the lines KEY-3 keeps.
  Nothing read back from the folders can be reached before Left and Right, so the reading (KEY-5) goes
  beside them.
- KEY-9 carries the work to consoles a program makes, once satl's own console has all of it.
- KEY-10 comes after every key input is built and installed, as the author asked.

---

## 2. The rules

1. **A console is every console a person types into with the keyboard.** There are three kinds:
   - satl's own console, opened by bare `satl`, `satl file.satl`, `--console` or File > New window;
   - the terminal satl was started in, when satl opens no console of its own (SATL_NO_WINDOW);
   - each console a program makes with `satellite.console.new` (KEY-9).

   Lines from a pipe or a file are nobody's typing, and nothing is kept of them.

2. **Each console has one file, in the folder of the day it started.** The file is
   `~/.satl/console/YYYY-MM-DD/console-NNN.c4`.
   - YYYY-MM-DD is the console's start in local time, `2026-10-20`, as every date satl writes is.
   - NNN is one more than the highest number already in that folder. It has three digits, and more after
     999 (`console-1000.c4`).
   - The folder and the file are made at the console's first Enter press. A console that is never typed
     into leaves nothing.
   - The folders are 0700 and the files 0600, because they hold everything a person types.

3. **Every Enter press is a new satellite.variable.string and a line of the file.** The line is kept as
   Enter sends it, after Backspace, Delete and the rest have changed it. Typing `pritn`, two Backspaces and
   `nt` keeps `print`.
   - Every character is kept except the control characters (U+0000 to U+001F, U+007F, and U+0080 to
     U+009F) and any byte that is not UTF-8. The file is UTF-8, so a line typed in ASCII is ASCII byte for
     byte. Up therefore brings back exactly what was typed, `café` and emoji included.
   - It is written at once, in one write(2) with its newline, so a crash or a closed window loses no line
     already entered.
   - An empty Enter is an empty line.
   - It is the same at the prompt, in an answer to `satellite.console.input()`, and for each line of a
     paste.
   - A line stopped with Ctrl-C was never entered, and is not kept.

4. **Up and Down go through the lines of the console shown.** They work as the prompt's do today:
   - the first Up keeps the half-typed line;
   - Up stops at the first line;
   - Down past the last line gives back the half-typed line, in this console and in a console before it;
   - a line that is empty or holds only spaces is stepped over, and so is a line the same as the nearest
     earlier line that is not stepped over.

5. **Left and Right move through what is typed, and past its ends to the consoles beside it.**
   - Left moves the cursor back through the line. When it is all the way to the left, Left shows the
     console before, at its last line, with the cursor at that line's end, so Left goes on through it.
   - Right moves the cursor forward. At the line's end, Right shows the console after, with the cursor at
     the start of its line. Back at this console, the half-typed line returns.
   - Left at the start of the oldest console's line, and Right at the end of this console's line, do
     nothing.
   - The consoles go one at a time, across days.
   - Enter sends the line shown as this console's own, so it goes into this console's file.

6. **The consoles are found at start, newest first, and each is read when Left reaches it.** At every
   start of a console, one walk runs before the prompt or the program reads a line. It lists the day
   folders, their console files, and each file's size.
   - The order is the folders' dates, and inside a day the console's number.
   - A console's lines are read into satellite.variable.strings the first time Left reaches it.
   - Left reaches only the most recent 32 MB.

7. **32 MB: the oldest 15 go.** The console files together hold 32 mb at most
   (`arguments.input.cache`), which is 33,554,432 bytes.
   - When an Enter brings them to that, the oldest 15 that can go (`arguments.input.file.cache`) are
     removed.
   - At every start, if they are at or above it, the oldest 15 that can go are removed, and 15 more while
     they still are. A file that cannot be locked or removed is passed over for the next oldest. The walk
     stops when the store is under, or when no file is left to try.
   - A console that is still running is never removed.
   - Only `console-NNN.c4` files inside dated folders are ever counted or removed.

8. **A console that cannot save keeps working.** Saving can fail because HOME is unset, the disk is
   full, or the folder cannot be written. The console then says so once, with S017 CONSOLE_NOT_SAVED.
   - It is a notice, so the run carries on.
   - After it, the console tries nothing more on the disk this run, so a half-written line never runs
     into the next.
   - The console keeps its lines in memory for the rest of this run.

9. **Nothing kept is ever run again by itself.** A kept line comes back only into the line being typed,
   as Up does today. It runs only when Enter sends it, as 003's ERROR #26 rule says (session.hpp:12-17).

---

## 3. What satl has today

- **satl's own console** is a GTK window, a VTE terminal and a pty, all inside the satl process. The
  console opens at most once a process (open_the_interpreters_console, console_launch.cpp:313-405, the
  refusal at 315-318).
  - It opens for bare satl, `satl file.satl`, `--run`, `--repl` and `--console`
    (structured-library.cpp:172-187).
  - Without `--console`, it does not open with SATL_NO_WINDOW, or with stdout on a pipe or a regular file
    (nobody_gave_satl_a_console, window_run.cpp:53-62).
  - `--console` opens it whatever SATL_NO_WINDOW and stdout say (structured-library.cpp:174-175), and so
    do File > New window and Open…, which start `satl --console` (console_menu.cpp:80-105).
  - Several satl processes can each have one at once. The prompt's `run` and `interpret` fork and exec a
    new `satl --run` (prompt_run.cpp:112-133).
- **Who reads the keys.**
  - The prompt keeps one LineReader a session (session.cpp:612). It is raw for one line at a time
    (line_reader.cpp:150), and interactive when stdin and stdout are terminals (line_reader.cpp:51).
  - At the prompt, `satellite.console.input()` reads through that reader by the input_source() hook
    (read_for_the_program, session.cpp:81-95; set at 615 and cleared at 880-882; console_calls.cpp:479-480).
  - In a file run, input() reads with `std::getline(std::cin, line)` on the cooked pty
    (console_calls.cpp:489). An arrow therefore arrives as ESC [ A inside the line. Ctrl-C there ends satl
    by SIGINT, with no report (check.sh:5999-6003).
- **A console a program makes** is made by `satellite.console.new` (console_new, window_console.cpp:517).
  - `.typed(capsule)` starts the desk thread's watch on its pty (console_typed, 550-579; the watch at 572).
    Before `.typed`, nobody reads it.
  - a_line_was_finished then reads finished lines, in the kernel's canonical mode, with no line editor and
    no history. It posts each one as AnEvent::a_line (225-266; the post at 262-264).
  - Those events are pumped only after main (window_run.cpp:71-111).
  - Closing the console runs the_console_went_away (189-215). Its pty's slave is closed only with its
    handle, so a late read or write meets EIO, never a reused descriptor (satellite_window.hpp:174-182).
  - The desk thread must never write a pty. VTE drains the master on the desk's thread alone, so a desk
    blocked on a full pty never wakes (console_launch.cpp:65-73).
  - RawMode keeps one terminal's settings in statics, because satl's own terminal is put back from a
    signal (raw_mode.cpp:14-27; raw_mode.hpp:16-22). SIGWINCH has one handler for the whole process
    (render.cpp:107-115).
- **The History** is a `std::vector<std::string>`, oldest first. It is kept in memory for one session,
  with no ceiling, and it drops an empty line and a repeat of the line it kept last (history.hpp:5-35;
  history.cpp:7-12).
  - It is filled only at session.cpp:786 and 820, after the session has taken the spaces off a line and
    found something left (session.cpp:769, 783-786, 819-820). So a line of only spaces never reaches it.
  - An input() answer is never added (session.cpp:81-95), yet Up at an input() prompt browses the
    History (line_reader.cpp:159).
  - Its note that "~/.satl is an install directory, not a place for a person's data" (history.hpp:5-7)
    came across from 003 in the port (commit 28364f2f5). It was never the author's ruling.
- **The keys today.**
  - Up and Down browse the History (editor.cpp:176-200). The first Up keeps the half-typed line, Up stops
    at the oldest entry, and Down past the newest gives the half-typed line back (186-197).
  - A line brought back has the cursor at its end (recall, editor.cpp:42-48).
  - Left and Right move the cursor one character, and do nothing at the line's start and end
    (editor.cpp:130-136; keys.cpp:276-294). Ctrl-B and Ctrl-F are Left and Right, and Ctrl-P and Ctrl-N
    are Up and Down (keys.cpp:93-96).
  - Ctrl-Left and Ctrl-Right are word moves. Shift- and Alt-arrows arrive as plain arrows, and so does
    every modified Up and Down (keys.cpp:154-162).
  - VTE keeps Ctrl+Shift+arrows for itself (vte.cc:5164-5253). satl's own key controller takes no arrow
    (the_keys, console_launch.cpp:255-309).
  - Tab becomes four spaces, any other control byte is ignored, and a UTF-8 character is one key
    (keys.cpp:102-125). Inside a bracketed paste every byte is text (keys.cpp:183-218).
  - Keys typed while a line runs are read and dropped (D0.6.5; line_reader.cpp:66-77, 153-154).
- **The character table, "the satellite char set"**, runs as follows (character_table.hpp:22-59;
  REGISTRY.satellite:78-205):

  | Codes | Characters |
  |---|---|
  | 0 | nothing |
  | 1-26 | a-z |
  | 27-52 | A-Z |
  | 53-62 | 0-9 |
  | 63-94 | the 32 signs, `!` first and `~` last |
  | 95 | space |
  | 96 | tab |
  | 97 | newline |
  | 98-127 | the other control codes |

  - Codes 1-95 are exactly the 95 printable ASCII characters, 0x20 to 0x7E. Above 127, a code is the
    character's own Unicode number (satellite_string.hpp:19), so a satellite string holds é and emoji.
  - The author's list in milestone 3 is the letters, the digits and 22 of the signs. Within the 95
    printable codes, "any key that is in the satellite char set" adds `! @ # $ % ^ & * ( )` and the space.
- **satellite.variable.string is satellite_string** (satellite_string.hpp:87).
  - It holds 16 bits a character in `narrow16_` (:206), and an object is 48 bytes.
  - It is made from text with from_utf8, which refuses bytes that are not UTF-8, and turned back with
    to_utf8 (:135, 137).
  - It needs no program. The printing satellite holds string values outside every program
    (printing_satellite.cpp:75-81). A program's name for one exists only inside a running body
    (value.hpp:28-33).
  - The prompt draws every line through shown(), so no byte of a line reaches the terminal raw
    (render.hpp:5-8).
- **~/.satl.**
  - satl makes only the folder itself, with one mkdir (satellite_log.hpp:150-156; config_file.hpp:250).
  - A person's data is already there: satellite.log and config.ini (satellite_config.hpp:77-81;
    config_file.hpp:18-20).
  - The installer removes or replaces only names it made (060-install-tree.sh:21-96), so a new folder
    there survives a reinstall.
  - Never flock the folder: the installer holds it through the whole install (config_file.hpp:232-237).
  - The folder is 0755 and satellite.log is 0644.
- **Dates, sizes, settings.**
  - Every date satl writes is local time with the year, `%Y-%m-%d` (satellite_log.hpp:104-111).
  - The author's mb (2026-09-24) is bytes / 1024 / 1024 (listing.cpp:58-63), so 32 mb is 33,554,432
    bytes.
  - Standard 2: "Every maximum ... is a `satellite_number` read from
    `satellite/config/satellite_config.hpp`" (DESIGN.md:29-35).
  - The author, 2026-10-04: "the entire arguments variable should be saved into config.ini"
    (argument_settings.hpp:10). A setting is one entry in kSettings (argument_settings.hpp:67-101).
    - A line `arguments.x(v)` or `arguments.x = v` typed at the prompt saves it in config.ini for good.
      In a file it holds for that run (setting_writes.hpp:1-31).
    - Start-up reads each one back from config.ini when its row exists, and refuses a value under its
      least (arguments.cpp:324-354). S016 knows each one's key (structured-library.cpp:317).
    - A setting read as satl starts, as display.buffer is, is too late for a file's line, so only the
      prompt changes it (argument_settings.hpp:45-48).
  - "Start-up is not what 004 is built for: 'if it starts fast, great'" (DESIGN.md:370).
- **Precedents to build on.**
  - O_CREAT|O_EXCL, which never clobbers (make_new, satellite_file.hpp:67-71).
  - One write(2) to an O_APPEND file, retried until whole (satellite_log.hpp:173-187).
  - A lock on a file beside the data, never on the folder (config.ini.lock, config_file.hpp:232-271).
  - A walk that adds up every file's size, opens every level with O_NOFOLLOW, and follows no link
    (count_contents, listing_counts.hpp:55; listing_counts.cpp:114, 201).
  - A notice that the run carries on after, said once by its tally (print_notice,
    critical_report.hpp:364-373). S017 is free (SATELLITE_ERROR.md:293).
- **Ends that skip every cleanup.**
  - A hangup is `_exit(129)` and a second Ctrl-C is `_exit(130)` (session.cpp:52-73).
  - Closing the console sends SIGHUP (window_console.cpp:213-214).
  - A file run installs no SIGINT or SIGHUP handler of its own, so Ctrl-C and a closed console end it by
    the signal's default action. The only handlers it can have are put_back_and_go, set once a program
    changes the terminal's colours (console_style.cpp:108-127), and satellite.test's Ctrl-C guard
    (test_run.cpp:109-143). Both raise the signal again and flush nothing.

  So a line must be on the disk when Enter is pressed.
- **Taken names.**
  - satellite.history and arguments.history (NM-6, NEW_MILESTONES.md:468-483) and config.ini's `history`
    bit (feature_register.hpp:69, "write every value a name ever held to disk") are for program values on
    disk.
  - config.ini's `replay` bit (feature_register.hpp:81; SATELLITE_ERROR.md:757-760, 876, H7, not built)
    would "record the inputs and the seed, so a run can be run again", input() answers among them.
  - This plan's folder, files and settings use none of these names.
- **Measured 2026-10-20**, with a scratch benchmark, in RAM (tmpfs), so a disk is slower:

  | A 32 MB store | Walk, sizes only | Every line read into satellite_strings | One console's file |
  |---|---|---|---|
  | 16,425 files of 2 KB | 28 ms | 262 ms, 129 MB | 0.04 ms |
  | 164,250 files of 200 bytes | 280 ms | 1.1 s, 142 MB | 0.03 ms |

  A file of 200 bytes still takes a 4 KB block, so the second store took 642 MB of disk for its 32 MB.
  Today satl draws its first prompt 5-9 ms after it starts.
- **Tests.**
  - check.sh runs under `HOME=build/check-home` with SATL_NO_WINDOW=1 (check.sh:31-42), and reads a row
    of satellite_config.hpp with `config_row` (check.sh:55).
  - prompt_cases holds the unit cases. check_prompt.py and check_session.py drive a real pty
    (check.sh:2011-2028, 2101-2102).
  - check_session.py sets no HOME (check_session.py:20-31), and its Terminal starts satl with the
    environment it was given (check_prompt.py:148-154). So run by hand, it uses the person's real HOME.
  - prove-console.sh types real keys into satl's own console and into a program's console on a headless
    mutter, and checks files and exit codes afterwards.
  - Every satl source and header is named in a list: INTERPRETER_SOURCES and HEADERS
    (040-sources.mk:120-236), and PROMPT_SOURCES (065-tests.mk:84-85). check.sh:199-209 fails when a
    header the compiler reads is not in the build's list.

---

## 4. The milestones

### KEY-1 — the day's folder — BUILT 2026-10-20, BUILD 0070

- **The store.** A new pair, satellite/satl/console_files.hpp and .cpp, holds the console's store.
  - run_satl makes it with `new`, once a satl, after the commands that print and exit have returned
    (structured-library.cpp:190-231), and never deletes it.
  - It is made when stdin and stdout are both terminals (rule 1). KEY-9 also makes it at a program's
    first console.
  - It is reached by a function in machine/, the way input_source() is (machine/input_source.hpp:21-30).
    The function answers null when there is no console.
  - console_files.cpp joins INTERPRETER_SOURCES (040-sources.mk:120-165) and console_files.hpp joins
    HEADERS (167-236).
- **The console's day.** The store is given a clock. It reads the day once, when the console starts,
  with localtime_r as satellite_log.hpp:104-111 reads the time, and writes it `%Y-%m-%d`. The day is kept
  for the whole run, so a console started at 23:59 keeps its day after midnight.
- **The root** is `$HOME/.satl/console`, as an absolute path, because the prompt has moved to another
  directory by then (structured-library.cpp:187-188). With HOME unset or empty, nothing is made (rule 8).
- **At the console's first Enter**, the root and the day folder are made, 0700.
  - The first Enter is where the reader answers a line at a terminal (line_reader.cpp:143-148, 210-216,
    225-228), or where a file run's getline returns one (console_calls.cpp:489).
  - The folders are made and opened the way the walk reads them (KEY-5): mkdirat, then openat with
    O_NOFOLLOW|O_DIRECTORY, starting from ~/.satl. A folder that is a link is S017, and is never written
    to.
  - ~/.satl is made only when it is missing, as satellite_log.hpp:152-156 makes it.
- **The notice.** S017 CONSOLE_NOT_SAVED joins s_codes.hpp.
  - It is a constexpr SCode with no machine code, as the kMissing* notices are (s_codes.hpp:478-501).
  - It is said with print_notice, whose tally says it once (critical_report.hpp:364-373). After it the
    console tries nothing more on the disk, so a run has one.
  - SATELLITE_ERROR.md's table gains its row beside S016 (line 195): `| **S017** | CONSOLE_NOT_SAVED |
    *(notice -- the run carries on; said once a console: HOME unset, the disk full, or the folder not
    writable)* | satl/console_files.cpp |`. Its free list (293) loses S017.
- **The tests.**
  - A harness, build/console_files_cases, is built like file_cases, with a room of its own under $TMPDIR
    and never HOME (065-tests.mk:104-115). It joins ALL_TARGETS and runs in check.sh beside file_cases
    (check.sh:2049-2054).
  - The harness gives the store a clock of its own, never the real one.
  - Each check_session.py check that looks in the console folder starts satl in a fresh HOME. The HOME is
    made as check.sh makes build/check-home (mkdir .satl, then `satl --rebuild`; check.sh:31-47), and set
    before the first Terminal (check_session.py:36).
  - Those checks end their sessions with Ctrl-D on an empty line, which ends a session with no line
    answered (editor.cpp:109-111; session.cpp:753-762), so that no `exit` line is kept.

**Done when** console_files_cases passes for:
- a clock at 2026-10-20 gives `2026-10-20`, and one at 2027-01-05 gives `2027-01-05`;
- a clock that answers 2026-10-19 23:59 when the console starts, and 2026-10-20 00:01 at its first Enter,
  makes `2026-10-19`;
- both folders are 0700;
- a root that is a link is S017, and nothing is written through it;
- HOME unset makes nothing and says S017 once.

check_session.py, in its own HOME:
- one typed line makes `console/<today>/`;
- a session with nothing typed makes nothing;
- a piped `--repl` makes nothing.

A check that works out today takes the date before and after the run and accepts either. check_install.sh
shows that an install leaves a `~/.satl/console/` it finds exactly as it was.

**BUILT 2026-10-20, BUILD 0070.** The gate:
- its own rows: console_files_cases 25 cases, check_session.py 49 checks, and check_install.sh's console row;
- the whole of check.sh: 1544 passed, 5 failed. The 5 are the same 5 as before KEY-1, this machine's:
  /usr/bin/time is missing (4 rows) and the umask is 0002 (1 row);
- the help examples (check.sh's own row);
- five review lenses, each with a skeptic, then one fresh reader.

The patch is `key-1.patch` in ~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-20-before-key-1/.
BUILD 0068, the build from before the review, was installed at 18:36 on the author's word.

- AS BUILT, WHERE IT DIFFERS FROM THE LINES ABOVE:
  - **The first Enter is seen in one place in the reader:** LineReader::read(), after read_typed()
    answers a Line. All three of read_typed's answers, and its raw-mode fallback, return through it, with
    the terminal cooked again by then. The other place is console_calls.cpp, after a file run's getline
    at a terminal.
  - **The store is reached through a machine/ header the lines above do not name:**
    satellite/machine/console_store.hpp. It holds console_files() and line_entered(). line_entered() is
    the write hook KEY-3 names, built now so the reader can call the store without linking satl/. KEY-1
    uses only its first call, and keeps no line.
  - **The store is given HOME and how to say S017, as well as the clock:** `ConsoleFiles(Clock, home,
    Say = print_notice)`. So the harness never reads a real HOME, writes no satellite.log, and counts
    S017 itself.
  - **A HOME that is not absolute is S017**, like an unset or empty one. The prompt has moved away from
    whatever folder it was relative to by the first Enter. The rest of satl takes HOME as given.
  - **~/.satl itself is followed when it is a link**, as config.ini and satellite.log follow it: it is
    the person's own arrangement. console/ and the day folder are opened O_NOFOLLOW, and a link there is
    S017.
  - **S017's row** has its neighbours' backticks, and the causes the code gives: HOME unset or not
    absolute, the disk full, a folder not writable, or a link where a folder should be.
  - **check_session.py's whole run has a HOME of its own**, made before its first Terminal, not only the
    console checks.
- WHAT THE REVIEW AND THE FRESH READER CHANGED:
  - **An above_two() descriptor move, built early, went.** "Never on 0, 1 or 2" is KEY-2's rule, for the
    file it keeps open.
  - **The harness pins its time zone** (TZ=XST12), so a store that read the day in UTC fails.
  - **A ~/.satl that is a link being followed is a case now.**
  - **check_session.py gained the checks the review found missing:**
    - a file run's input() at a terminal makes the folder;
    - a line abandoned with Ctrl-C makes nothing;
    - stdin at a terminal with stdout to a file makes nothing;
    - a piped run with stdout on a pty makes nothing;
    - a console folder that is a link says S017 once on the screen, both lines still run, and nothing is
      written through it.
  - **Comments** no longer claim more than the code does: print_notice's tally is once a place, not once
    a run.

### KEY-2 — console-001.c4, the console's own file — BUILT 2026-10-20, BUILD 0074

- **At the first Enter**, after KEY-1's folder, the console claims its number.
  - The number is one more than the highest `console-<digits>.c4` in the day folder, and 001 in an empty
    one. It has three digits, and more after 999. Gaps are not filled.
  - It is claimed with openat on the day folder, O_CREAT|O_EXCL|O_NOFOLLOW, 0600, as make_new never
    clobbers (satellite_file.hpp:67-71). A name another satl took first is passed over for the next.
- **The file stays open for the run**, O_WRONLY|O_APPEND|O_CLOEXEC. It is never one of fds 0-2, which
  are the pty.
- **One file a console.** The prompt's lines and input()'s answers in one satl go into the same file.
  - A satl that `run` or `interpret` starts from the prompt is a console of its own, with its own file
    (prompt_run.cpp:112-133).
  - So is a File > New window (console_menu.cpp:80-105), and so is each console a program makes (KEY-9).
- **A file that cannot be claimed** is S017 (rule 8).

**Done when** console_files_cases passes for:
- an empty folder gives `console-001.c4`;
- 001 and 003 give 004;
- 999 gives `console-1000.c4`;
- a `console-7.txt` and a folder named `notes` are left alone;
- two claims at once in one folder get two numbers;
- the file is 0600.

check_session.py: two sessions in one day make console-001.c4 and console-002.c4, and two at the same
time make two files.

**BUILT 2026-10-20, BUILD 0074,** on the author's word of 19:10 ("make sure that you build those keys...",
read as KEY-2 to KEY-4, one at a time). The gate:
- its own rows: console_files_cases 45 cases, and check_session.py's console checks, KEY-2's five among them;
- the whole of check.sh: 1544 passed, 5 failed, the same 5 machine rows as before;
- the help examples;
- five review lenses, each with a skeptic, then one fresh reader.

The patch is `key-2.patch` in ~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-20-before-key-2/.

- AS BUILT, WHERE IT DIFFERS FROM THE LINES ABOVE, OR SAYS WHAT THEY LEFT OPEN:
  - **The claim is part of KEY-1's first Enter**, under the same mutex. make_the_days_folder now hands
    its open day folder to claim_the_file. The highest number is read through that descriptor (fdopendir
    on a close-on-exec copy), never by a path.
  - **A number is its digits, with no ceiling.** It is compared by length, then digit by digit, and one is
    added with a carry. Zeros in front change nothing: console-01.c4 is 1, so the next is 002, and
    console-0.c4 is 0. Only an exact `console-`, ASCII digits, `.c4` counts. A folder or a link with
    such a name counts by its name: names are read, never followed.
  - **EEXIST passes to the next number, at most 1,000 in a row.** After that, S017, so a folder that
    refuses every name cannot hang a console. Any other failure is S017 at once.
  - **Never on 0, 1 or 2.** Before each openat, every free descriptor among 0-2 is filled with a
    close-on-exec copy of the day folder's, so the file opens at 3 or above and never stands in for
    stderr, even for a moment. A move above 2 stays as the last guard. The file is held for the run, and
    closed only by the store's destructor, which satl never runs.
  - **prompt_run.cpp and console_menu.cpp are unchanged.** Every satl they start is a process of its own,
    with its own store and its own file at its own first Enter. The file is close-on-exec, so no child
    holds it. check_session.py proves this for run and interpret.
  - **S017's row and note also name a console file that cannot be claimed.**
- FOUND ON THE WAY, OLDER THAN KEY-2, AND NOT CHANGED:
  - a satellite.thread started at the prompt crashes satl (SIGSEGV) when the line that started it returns
    before the thread ends: the thread keeps pointers into that line's registry and capsule table
    (BUILD 0066 does the same);
  - check_session.py's "Up recalls the line as it was typed" fails when TMPDIR is 43 characters or longer,
    because the prompt wraps past its 100-column Terminal.

### KEY-3 — every Enter press kept: a satellite.variable.string and a line of the file — BUILT 2026-10-21, BUILD 0077

- **Where a line is kept.**
  - Kept: wherever the reader answers a line at a terminal, typed or pasted: line_reader.cpp:143-148,
    210-216 and 225-228. That covers the prompt's lines, and input()'s answers at the prompt, which
    read_for_the_program reads through the same reader (session.cpp:81-95).
  - Lines 220-224 only queue a pasted line, which 143-148 or 210-216 answers later, so each pasted line is
    kept once.
  - Kept, until KEY-4: in a file run, where input() reads with std::getline at a terminal
    (console_calls.cpp:489).
  - Not kept: a piped line (read_piped), a line stopped with Ctrl-C (line_reader.cpp:231-233), or a line
    the prompt writes for the person (write_for_them, session.cpp:716-721).
- **The filter.** First the control characters (U+0000 to U+001F, U+007F, U+0080 to U+009F) and every
  byte that is not part of a whole UTF-8 character are dropped. Then from_utf8 makes the satellite_string,
  and on what is left it cannot fail.
- **The console's lines** are a new class in satellite/prompt/: this console's satellite_strings, oldest
  first.
  - The store owns them. The LineReader is handed a pointer to them. prompt_reader and prompt_cases hand
    it lines of their own.
  - Like the Editor, the class keeps no terminal and no file (PLAN.md:292-295).
  - It sits beside History, which Up and Down still use until KEY-4.
  - It is added to under a mutex, because input() can be called from a program's threads
    (expression.cpp:2270-2283).
  - It joins INTERPRETER_SOURCES and HEADERS. Its .cpp joins PROMPT_SOURCES beside satellite_string.cpp,
    and PROMPT_HEADERS gains their headers (065-tests.mk:84-94).
- **The file.** The reader adds each line it answers, then calls one write hook: a function pointer in
  machine/ like input_source(), which run_satl sets and the harnesses leave null. The file run's getline
  answer reaches the store through the same hook, and the store holds its satellite_string.
  - The hook writes the line's UTF-8 and a newline in one write(2), retried on EINTR and finished after a
    short write, as satellite_log.hpp:176-187 writes.
  - Nothing is buffered, so a hangup, a second Ctrl-C or a closed window loses no line already entered.
  - A write that fails is S017. After it the file gets nothing more this run, and the lines stay in
    memory.
- **The help.** satellite.help/satellite.console/help_text.txt says what is kept, and where.

**Done when** console_files_cases passes for:
- a line holding `café`, an emoji, ESC, a lone 0xFF, U+009B and four spaces keeps `café`, the emoji and
  the spaces, and nothing else;
- the file's lines are the console's lines;
- an empty Enter is an empty line;
- after a failed write, nothing more is written.

check_session.py, in its own HOME:
- three typed lines, one of them mended with Backspace, are the three lines of console-001.c4 as they were
  entered;
- a pasted block of three lines is three lines;
- a line stopped with Ctrl-C is not there;
- an input() answer at the prompt is there;
- a satl killed with SIGKILL after two Enters has both lines.

A `satl file.satl` on a pty under SATL_NO_WINDOW keeps both answers to its program's two input() calls in
its own file. A piped run writes no file. check.sh passes.

**BUILT 2026-10-21, BUILD 0077, the quick way the author asked for.** The author said, 07:15-07:20: "let's
skip the test and just get something that is going to work", then "skip the implementers last check.sh".
- Built by one implementer. Its review, its fresh reader and its own last check.sh run were stopped
  unrun.
- Proven by hand on a pty under a scratch HOME (`satl --repl`): two typed lines, `café ✓` among them, are
  the two lines of console-001.c4, 0600 in a 0700 day folder, and Ctrl-D ends the session with 0.
- Installed 2026-10-21 07:30.
- The patch is `key-3.patch` in ~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-20-before-key-3/.
- The console's lines are a new class, satellite/prompt/console_lines.hpp and .cpp.

### KEY-4 — Up and Down (the author's 8th) — BUILT 2026-10-21, BUILD 0079

- **The Editor browses the console's lines** instead of History (editor.hpp:55-63; editor.cpp:22-48,
  176-200).
  - Up keeps the half-typed line and goes back one line. Up stops at the first line. Down past the last
    line gives the half-typed line back.
  - A line that is empty or holds only spaces is stepped over, and so is a line the same as the nearest
    earlier line that is not stepped over. That is what History holds today (history.cpp:7-12), with the
    session's trim before it (session.cpp:769, 783-786, 819-820), moved from keeping to browsing,
    because every Enter is now kept.
  - The Editor's line stays a std::string of bytes (editor.hpp:14-17). A line brought back is the to_utf8
    of its satellite_string.
- **History goes.**
  - So do the remember() calls (session.cpp:786, 820; prompt_reader.cpp:35; line_reader.cpp:53-56). Every
    line the reader answers is already among the console's lines (KEY-3), input()'s answers included,
    which History never held.
  - history.cpp and history.hpp leave 040-sources.mk:130 and 182 and 065-tests.mk:84, and their includes
    go from line_reader.hpp:45, editor.cpp:5 and prompt_cases.cpp:10.
- **A file run at a terminal reads input() through a LineReader of its own.**
  - It is declared in run_satl before threads_close_before_the_capsules_go (structured-library.cpp:531-538),
    so it outlives every program thread. It sets the hook as run_session does (session.cpp:612-615), so
    console_calls.cpp:479-480 takes it, and clears it before the reader goes, as session.cpp:880-882 does.
  - It follows read_for_the_program's steps: the flush, the_pty_was_written_directly and
    the_pty_flows_again (session.cpp:81-95).
  - std::getline stays for a pipe or a file.
  - Up, Down, Left, Right, Home and End then work in a program's input(); Backspace already does,
    cooked. An arrow no longer lands in the answer as ESC [ A.
  - Ctrl-D is still S830, exit 55.
  - Ctrl-C still ends satl by SIGINT, as today. The reader gives the terminal back and answers
    Interrupted, and input() raises SIGINT where the kernel raised it before.
  - check.sh:5989-5991 and 5999-6003 are rewritten for the reader's bytes. The second still ends
    `|signal 2`.
  - Keys typed before input() asks are dropped, as the prompt drops them (D0.6.5).
- **The tests.** prompt_cases.cpp's case at 139-150 moves over as it is. The case at 160-166 turns
  round: two Enters of `same` are two lines, Up shows `same` once, and 100,000 more lines have no ceiling.
- **The help.** satellite.help/satellite.console says what Up and Down do.

**Done when** prompt_cases passes for:
- `a`, `b`, a line of four spaces, `b`: Up reaches `b` and then `a`;
- `A`, an empty line, `A`: Up reaches `A` once;
- Down gives back the half-typed line.

check_prompt.py and check_session.py pass as they are, including their Up checks (check_prompt.py:240-241;
check_session.py:63-65, 149-152). check_session.py adds:
- Up brings back an input() answer at the prompt;
- in a file run on a pty, the program's second input() brings back the first answer with Up;
- an answer holds no ESC;
- Ctrl-C at a file run's input() still ends it by SIGINT.

**BUILT 2026-10-21, BUILD 0079, the quick way the author asked for** ("skip the tests"). It was built by one
implementer with no new tests: its Done-when cases and check_session.py's four new checks were not written.
Then the whole check.sh ran once:
- 1544 passed, 8 failed;
- 5 of the failures are this machine's;
- 1 was check_prompt.py stopped by its 600 s timeout in the two-pastes case. That is a deadlock in the test,
  where both sides block in write(), and it happened once before KEY-4 too. Alone, it passed 37 of 37;
- 2 were the rows this plan says KEY-4 rewrites, for a file run's input() at a terminal. Their bytes are now
  the reader's: bracketed paste on and off around input(), and the reader's echo. Ctrl-C answered by the reader
  still ends by SIGINT. Rewritten, they pass.

Installed 2026-10-21. The patch is `key-4.patch` in
~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-20-before-key-4/.

- AS BUILT:
  - **Each line's stop is worked out as the line is added**, and kept in a list of positions. So Up and
    Down take one step a press, with no ceiling.
  - **History is gone:** history.cpp and history.hpp are deleted.
  - **A file run's LineReader is declared in run_satl before threads_close_before_the_capsules_go.** It
    reads one input() at a time, behind a mutex.
  - **The prompt is written as given.** The renderer has an as_given mode (render.hpp): input()'s prompt is
    written once, raw, and the renderer counts only the line after it. It reads the cursor moves in the
    prompt and never takes an escape byte for a column, so a program that draws its own screen, like the
    author's journal.satl, keeps its typing row. This was tried on a copy of journal.satl.

### KEY-5 — the day folders found, and read back into satellite.variable.strings — BUILT 2026-10-21, BUILD 0085

- **The walk** runs at every start of a console (rule 1), when the store is made, before the prompt or
  the program reads a line.
  - It lists the dated folders under the root (`YYYY-MM-DD`, digits and two dashes) and the
    `console-<digits>.c4` files in each. It takes each file's size and inode with one fstatat.
  - It reads as count_contents reads (listing_counts.hpp:55): one directory a level, openat with
    O_NOFOLLOW, no link followed, only regular files. Nothing else is listed.
  - What it finds is "the walk's list": every console file, newest first, each with its size and inode.
- **The order, newest first**, is the folders' dates, latest first, and inside a folder the higher number
  first. A dated name sorts as its date. The walk runs before this console has a file, so its own file is
  never on the list.
- **The earlier consoles** are a list type in satellite/prompt/, which the Editor is given (KEY-6). The
  store fills it from the walk. prompt_cases fills it by hand.
- **Reading a console's file.** It is read into satellite_strings the first time it is needed (Left,
  KEY-6), one line to each newline.
  - It is read only when its inode is the one the walk listed. A file that has gone, or has another inode,
    is passed over.
  - KEY-3's filter is used, so a line read back holds what a line typed would. A file edited by hand still
    cannot put an ESC on the screen.
  - A console read once stays read for the run.

**Done when** console_files_cases passes for:
- on 2027-01-05, the order is 2027-01-05, 2027-01-03, 2026-12-31, 2026-12-30;
- console-002 comes before console-001;
- a link, a stray file, a folder named `notes` and a folder named `10-20` are not listed;
- a file holding ESC, `café` and a lone 0xFF reads back as `café`;
- a file removed after the walk, or replaced by another of its name, is passed over.

check_session.py, with a store of 16,425 files in its HOME, draws its first prompt and answers a line.

### KEY-6 — Left and Right, from console to console, over the most recent 32 MB — BUILT 2026-10-21, BUILD 0085

- **The Editor** gains which console is shown, 0 for this one (editor.hpp:55-63).
  - Left (editor.cpp:130-132) moves the cursor back through the line, as today. When the cursor is all the
    way to the left, Left shows the console before, at its last line, with the cursor at that line's end.
    An empty line has its cursor all the way to the left already.
  - Right (editor.cpp:134-136) moves the cursor forward, as today. At the line's end, Right shows the
    console after, with the cursor at the start of its line. Back at this console, the half-typed line
    returns.
  - Ctrl-B and Ctrl-F, which are Left and Right to the key decoder (keys.cpp:93-94), do the same.
  - Home and End move to the line's two ends, as today. From there, one more Left or Right goes on to the
    next console.
  - Left at the start of the oldest console's line, and Right at the end of this console's line, do
    nothing, as Left and Right do at a line's ends today.
  - A console holding only lines that are stepped over (KEY-4) is stepped over too.
- **Up and Down** go through the lines of the console shown, with KEY-4's rules. Down past the last line
  of a console before this one comes back to this console with the half-typed line, as Down past the
  newest line does today (editor.cpp:186-197).
- **Enter** makes the line this console's, kept in its file (KEY-3). The next line starts at this console.
- **The most recent 32 MB.** "Left's list" is the newest consoles on the walk's list whose sizes add up
  to no more than `arguments.input.cache`.
  - `arguments.input.cache` is a row of satellite_config.hpp, as standard 2 has every maximum
    (DESIGN.md:29-35). It counts the author's mb, so 32 is 33,554,432 bytes. KEY-10 makes it a setting.

    ```
    arguments_vector.push_back({"arguments.input.cache", 32, false, false});   // mb: 32 x 1024 x 1024 bytes
    ```

  - run_satl reads the row and gives it to the store. console_files_cases gives the store values of its
    own and never includes satellite_config.hpp: a bare make rewrites that header's build row while the
    harnesses link, which would make the harness stale (065-tests.mk:57-59).
- **Today's Left and Right checks stay as they are**, because each moves along a line being typed and
  never passes its start: prompt_cases.cpp:104-109 and 152-158, check_prompt.py:235-239 and 329-332. In
  prompt_reader there is no console before, so Left at a line's start does nothing, as today.
- **The help.** satellite.help/satellite.console says what Left and Right do.

**Done when** prompt_cases passes for:
- on `abc` with the cursor at its end, three Lefts reach the start, and the fourth shows the last line of
  the console before, with the cursor at its end;
- Left goes on through that line, and at its start shows the line of the console before that;
- Right at the end of a line from a console before shows the console after, with the cursor at its start,
  and back at this console the half-typed line returns;
- on an empty line, Left goes straight to the console before;
- Home, then Left, goes to the console before;
- Up and Down move inside a console before this one, and Down past its last line comes back here;
- Enter on a line from a console before this one keeps it in this console.

console_files_cases: a 40 MB store gives a Left's list of its newest 32 MB, and a walk's list of all of
it. check.sh: `config_row input.cache` is 32.

check_session.py runs four sessions one after another in one HOME. The first three each type a line of
their own (`satellite.console.display("one")` to `"three"`) and end with Ctrl-D. In the fourth:
- Left on the empty line shows the third's line, and Left past its start the second's;
- Enter on that line runs it and puts it into the fourth's file.

**KEY-5 AND KEY-6 BUILT 2026-10-21, BUILD 0085, together,** on the author's word that day ("Can you accomplish
both of these things right now?", after "The left arrow keys don't bring up ... the previous set of inputs").
Built by one implementer, with every Done-when case of both written and run, then the whole check.sh once:
- its own rows: console_files_cases 79 cases (12 new: KEY-5's five and KEY-6's 40 MB store among them),
  prompt_cases 51 (8 new: KEY-6's seven), check_session.py 66 checks (3 new: the 16,425-file store, whose
  first prompt was drawn in 0.05 s in RAM, and the four sessions), and check.sh's `config_row input.cache` row;
- the whole of check.sh: 1554 passed, 5 failed. The 5 are this machine's, the same 5 as before: /usr/bin/time
  is missing (4 rows) and the umask is 0002 (1 row). FD-2's rows and the detach change's rows passed.

Not installed. The patch is `key-5-6.patch` in
~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-21-before-key-5/.

- AS BUILT, WHERE IT DIFFERS FROM THE LINES ABOVE, OR SAYS WHAT THEY LEFT OPEN:
  - **The walk runs in the store's constructor**, now `ConsoleFiles(Clock, home, cache, Say)`, so it can never
    run after this console's file is claimed. run_satl reads `arguments.input.cache` and gives it as bytes,
    mb x 1024 x 1024. A row under 0 reaches no console; one past what a byte count holds reaches every one.
  - **The walk's list is `ConsoleFiles::walked()`**: each file's day, name, number, size, device and inode.
    A file is read back only when both device and inode are the ones listed.
  - **The list type is `prompt::EarlierConsoles`**, satellite/prompt/earlier_consoles.hpp and .cpp, newest first.
    Each console is added with a function that reads it. It is read the first time Left reaches it, under a
    lock, and a console whose read fails stays passed over for the run. The store's read reopens ~/.satl,
    console/ and the day folder as the walk did, O_NOFOLLOW below ~/.satl, so no descriptor is held between
    the walk and Left.
  - **A dated name is its shape**: ten characters, digits, and dashes fifth and eighth. Two names with one
    number (console-01.c4 beside console-001.c4, which no satl makes) sort by name, so a walk is always the same.
  - **A console is read up to the size the walk listed**, never past it. So Left reaches no more than the
    cache, even of a console still running in another satl and adding to its file.
  - **The piece after the last newline is a line too**, when there is one, as satellite.file reads a file's
    lines: a file edited by hand may end without a newline. "One line to each newline" would drop it.
  - **Left's list ends at the first console that would pass the cache**, so Left never skips a large console
    to reach older ones.
  - **The reader is handed the consoles with `LineReader::left_reaches()`**, beside keep_lines_in(). The
    session's reader and a file run's reader for input() are both handed them, so Left reaches the consoles
    before at a program's input() too.
  - **Right shows the console after at its last line**, the line Left showed it at, with the cursor at the
    line's start. Back at this console the half-typed line returns with its cursor at the start too.
  - **The half-typed line is kept once, as Up keeps it.** A line Up brought back from this console is not it:
    Left from there, then Right back, returns what was being typed before Up.
  - **Ctrl-Left and Ctrl-Right are still word moves** inside the line, and never go to another console.
  - **Nothing on the screen says which console is shown.** Only the line changes.

### KEY-7 — 32 MB reached: the oldest 15 console files removed

- **A second row**, `arguments.input.file.cache`, beside the first. KEY-10 makes it a setting too.

  ```
  arguments_vector.push_back({"arguments.input.file.cache", 15, false, false});   // files removed at once
  ```

- **The total** is the sizes on the walk's list plus this console's bytes, and each Enter adds its line.
  - When an Enter brings the total to `input.cache` or past it, the oldest `input.file.cache` (15) on the
    walk's list that can go are removed, taken from its old end.
  - They come off both lists and off the total.
  - When fewer can go, those that can are removed, and the next Enter checks again.
- **A running console holds a lock on its own file.**
  - It takes flock(LOCK_SH) when its file is claimed (KEY-2).
  - It then fstats the file. An st_nlink of 0 means another satl's start removed it between the two calls,
    and the console claims again.
  - This console's own file is never removed.
  - The lock is on the file, never on a folder (config_file.hpp:232-237).
- **How a file is removed.** Removal opens the file, takes LOCK_EX|LOCK_NB, and unlinks it only when
  fstat shows the inode the walk listed.
  - A file it cannot lock is passed over.
  - A listed file that has gone, or has another inode, comes off the total as though removed.
- **Only the walk's list is ever removed from**: `console-<digits>.c4` files in dated folders under the
  root, and nothing else in ~/.satl. Folders are never removed.
- **The help.** satellite.help/satellite.console says what the 32 MB means.

**Done when** console_files_cases passes for:
- a store 1 KB under the cache and an Enter of 2 KB: the oldest 15 go, and the newest and this console's
  file stay;
- a locked file stays;
- in a store of 10 files, every one that can go goes;
- a file removed between the claim and its lock makes the console claim again;
- a file replaced since the walk is left alone, and comes off the total;
- a `notes.txt` in a day folder, and a satellite.log beside the root, stay.

check.sh: `config_row input.file.cache` is 15.

### KEY-8 — every start: the sizes counted, room made

- **The count.** The walk's list holds every size (KEY-5), so the walk adds them up.
  - If the total is at `input.cache` or above it, the walk removes the oldest 15 that can go, and 15 more
    while the total still is, as KEY-7 removes them.
  - A file it cannot lock or remove is passed over for the next oldest.
  - It stops when the total is under, or when no file on its list is left to try. Then it hands over its
    list as it stands.
- **It runs for a console's start only**, inside the walk, before the prompt or the program reads a line.
  - It never runs for `--version`, `--help`, `--rebuild` or the other commands that print and exit: they
    return before the store is made (structured-library.cpp:190-231).
  - It never runs for a pipe, a file or a satellite.test child.
  - So the `--version` each link runs under the real HOME (050-build.mk:155-171) walks nothing.
- **What it costs.** It holds the start up by the walk's time (Section 3): 28 ms for 16,425 files and
  280 ms for 164,250, in RAM, and more on a disk. Start-up is not what 004 is built for (DESIGN.md:370).

**Done when** console_files_cases passes for:
- a 50 MB store comes down under 32 MB, and its newest consoles are kept;
- a store under 32 MB is left whole;
- a 40 MB store whose files are all locked is left as it is, and the walk ends.

check_session.py:
- a start with a 40 MB store in its HOME leaves it under 32 MB before its first prompt is drawn;
- `satl --version` and a piped run leave a 40 MB store exactly as it was.

### KEY-9 — the same for every console a program makes

From the author's answer 5, *"including all consoles we type with the keyboard into"*.

- **Each console `satellite.console.new` makes is a console** (rule 1).
  - Its day is the day it was made.
  - Its file is claimed at its first Enter, by KEY-2's claim, in the same store, and its lines are kept
    with KEY-3's filter.
  - A satl that started with no console of its own, its stdout on a pipe, makes the store, and runs the
    walk, when its program makes its first console.
- **Its keys go through the prompt's Editor, on a reader of its own.**
  - From KEY-9, `.typed(capsule)` starts a LineReader for that console, where it starts the desk's watch
    today (console_typed, window_console.cpp:550-579). The reader runs on a thread of its own, and reads
    and draws on the console's own pty. It is never on the desk, which must never write a pty
    (console_launch.cpp:65-73).
  - Each line it accepts is posted as AnEvent::a_line, as a_line_was_finished posts it
    (window_console.cpp:262-264), so `.typed` capsules run as today. It is kept as well (KEY-3).
  - It drops nothing typed between two lines, because no line runs between its reads.
  - The console gets raw mode of its own. Its saved settings live in its reader, and RawMode's statics
    stay satl's own terminal's (raw_mode.cpp:14-27). Nothing needs putting back at the end, because the
    pty goes with the window.
  - SIGWINCH has one handler for the whole process (render.cpp:107-115), so the reader asks its own pty's
    size before each draw.
  - Closing the console (the_console_went_away, window_console.cpp:189-215) ends its reader, whose next
    read meets the closed pty (satellite_window.hpp:174-182). Its file is closed with it.
- **Up, Down, Left and Right** work there as in satl's own console (KEY-4, KEY-6). Left from it reaches
  first the consoles this satl opened before it, whose lines are in memory, then the walk's list.
- **One store, one total.** Every console of one satl keeps its lines in the same store and adds to the
  same 32 MB total (KEY-7). The store's lists and total are kept under one mutex, because each program
  console's reader runs on its own thread.
- **The help.** satellite.help/satellite.console says it of `satellite.console.new` too.

**Done when** console_files_cases, on pty pairs it opens itself (openpty, with no GTK), passes for:
- a program console's reader keeps two typed lines in its own file;
- Up brings back the first of them;
- Left at the start of its line shows the last line of the console opened before it;
- two readers on two ptys at once each keep their own file, and neither changes the other's terminal or
  satl's own.

By hand on mutter, as prove-console.sh does:
- a program whose console asks with `.typed` twice keeps both lines in a file of its own;
- Up in that console brings back the first;
- Left at the start of a line there reaches satl's own console's last line;
- closing that console leaves its file whole.

### KEY-10 — `arguments.input.cache` and `arguments.input.file.cache`, settings a person changes

Built after every key input, KEY-1 to KEY-9, is built and installed, as the author asked.

- **Both rows join kSettings** (argument_settings.hpp:67-101). Their config.ini keys are `input.cache` and
  `input.file.cache`:

  ```
  // the most the console files hold, in mb, and how many go at once when they reach it (KEY_PLAN.md)
  {"input.cache", SettingKind::number, 1, SettingRead::as_satl_starts},
  {"input.file.cache", SettingKind::number, 1, SettingRead::as_satl_starts},
  ```

- **Then, as for every setting** (setting_writes.hpp:1-31), `arguments.input.cache(64)` or
  `arguments.input.cache = 64` typed at the prompt is saved in config.ini for good.
  - Start-up reads `input.cache = 64` back from config.ini (arguments.cpp:324-354), and S016 knows both
    keys (structured-library.cpp:317).
  - Each is a whole number from 1 up. A smaller one is refused, as the other number settings' are.
- **Read as a console starts**, as display.buffer is (argument_settings.hpp:45-48).
  - A change typed at the prompt is saved, and holds from the next start.
  - A line in a file is refused as too late (why_a_file_line_is_too_late, setting_writes.hpp:62), so no
    program changes how much of the person's typing is kept.
- **The help.** satellite.help/arguments/help_text.txt names both among the settings (18-22), and gives
  their least beside display.buffer's (98).

**Done when** check.sh passes with rows for:
- `arguments.input.cache(64)` typed at a piped `--repl` in a HOME of its own writes `input.cache = 64`
  into that HOME's config.ini;
- `arguments.input.file.cache = 2` does the same for `input.file.cache = 2`;
- `arguments.input.cache(0)` is refused;
- `arguments.input.cache(64)` in a file is refused as too late, and config.ini is unchanged;
- a config.ini holding both rows draws no S016.

check_session.py: a start whose config.ini says `input.cache = 1` and `input.file.cache = 2`, with a
3 MB store, removes the oldest files two at a time until the store is under 1 mb.

---

## 5. Not in this plan

- **Any key taken where GTK or VTE sees it** (the_keys, console_launch.cpp:255-309). A program started
  with satellite.variable.program shares satl's terminal, and sudo asks for its password there
  (program_spawn.hpp:19-22). Only the lines satl's own readers answer are kept. Keys typed while a line
  runs are read and dropped, never answered as a line (D0.6.5, line_reader.cpp:66-77), so nothing of them
  is kept.
- **satellite.history, arguments.history and config.ini's `history` bit** (NM-6), a different thing.
  **The `replay` bit** (SATELLITE_ERROR.md:757-760, 876, H7) would record input() answers among a run's
  inputs. It is not built, and it neither reads nor writes the console's files.
- **satellite.delete** leaves the console's files alone, as it leaves the prompt's typed lines. That was
  NM-2's choice, still the author's to rule on (NEW_MILESTONES.md:310-311).

---

## 6. For the author

The six questions, answered by the author on 2026-10-20:

1. **Where the day folders go: `~/.satl/console/`.** "yes". Rule 2, KEY-1.
2. **The year: in the name, `2026-10-20`.** "include the year". Rules 2 and 6, KEY-1, KEY-5. A folder
   sorts as its date, so New Year needs no rule, and no folder is used for two years.
3. **The line Enter sends.** "make it print, as long as the user has not pressed enter". Rule 3, KEY-3.
4. **é and emoji: kept, in UTF-8.** "you decide", so this plan decides. The reason is that Up must bring
   back exactly what was typed: `display("café")` comes back as `display("café")`, never as
   `display("caf")`. A line typed in ASCII is ASCII byte for byte. Only the control characters and bytes
   that are not UTF-8 are dropped, and nobody types those; they arrive only inside a paste. Rule 3, KEY-3.
5. **Every console typed into, the consoles a program makes too.** "including all consoles we type with
   the keyboard into". Rule 1, KEY-9.
6. **Left and Right move through what is typed, then on to the consoles beside it.** "it moves the cursor
   through the input that has already been typed, until it is all the way to the left, then it switches
   at last to the left most set of inputs from the previous console file". Rule 5, KEY-6.

Taken without the author's word, so say if one is wrong:

- Left shows the previous console's last line, with the cursor at its end, so Left goes on through it.
  That is this plan's reading of "the left most set of inputs from the previous console file".
- Right is the mirror of Left: at a line's end it shows the console after, with the cursor at the start
  of its line.
- When the console before was left with `exit`, Left's first switch shows `exit`, and Enter on it closes
  this console.
- Every Enter is a line of the file, including an empty line, a line of spaces and a repeat. Up and Down
  step over those, as the prompt's Up does today.
- Nothing wraps around ("cycles", "cycle through").
  - Up stops at a console's first line, and Down past the last line comes back to this console's
    half-typed line. Neither goes on into another console.
  - Left stops at the start of the oldest console's line, and Right at the end of this one's.
- The consoles go one at a time, across days, as the author's first paragraph has it. The author's
  milestone 5 says "the days".
- "saving the most recent 32mb worth of files" (milestone 5) is read as: Left reaches only the newest
  32 MB.
- A console's day is the day it started. Its folder and file are made at its first Enter, so the numbers
  follow first Enters, and a run nobody types into leaves nothing.
- After `console-999.c4` comes `console-1000.c4`.
- `arguments.input.cache` counts mb, as the author says it ("32mb", "32 megabytes"): 32 is 33,554,432
  bytes, by the author's mb (listing.cpp:58-63). The other way is bytes, as object_bytes_max counts them.
  Either way it counts what the files hold, not the disk they take: every file takes at least a 4 KB
  block.
- During a run, the 15 go each time an Enter reaches 32 MB. At a start, they go 15 at a time until the
  store is under, or no file is left to try. "Reaches" and "above" are one test: at or above.
- "Every startup" is every start of a console. `--version`, `--help` and the other commands that print and
  exit, a pipe, a file and a satellite.test child never count or remove.
- The walk runs in front of the start, so a full store holds the first prompt up by the walk's time.
- The two settings are read as a console starts (KEY-10).
  - Typed at the prompt, a change is saved and holds from the next start.
  - A line in a file is refused as too late.
  - Each is a whole number from 1 up.
- The prompt in a plain terminal (SATL_NO_WINDOW) is a console too, so check.sh can prove most of this on
  a pty.
- A file run's input() at a terminal reads through the prompt's reader (KEY-4). So keys typed before it
  asks are dropped, as the prompt drops them. Ctrl-C there still ends satl by SIGINT, as today; the other
  way is the prompt's S810, exit 130, with a report.
- In a console a program makes (KEY-9):
  - a console nobody reads, with no `.typed`, keeps nothing;
  - Ctrl-D on an empty line does nothing, as today;
  - Ctrl-C drops the line being typed, as at the prompt;
  - Left reaches the consoles this satl opened before it, then the older ones on the disk.
- The folders are 0700 and the files 0600, because they hold everything a person types, including any
  answer a program asks for. Today ~/.satl is 0755 and satellite.log is 0644.
- A console that cannot save says S017 once, stops trying the disk for that run, and carries on.
- A running console locks its own file, and removal passes over a locked one, so a console left open for
  days keeps its file.
- Lines read back from a file go through the same filter as lines typed.
