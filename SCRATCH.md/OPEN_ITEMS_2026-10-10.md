# OPEN ITEMS -- written 2026-10-10, FINISHED ~19:30: everything open, and what closed today

- 0. Where things stand -- the builds, what is installed, the backups.
- 1. Today's questions waiting on him: A-H (sections, time) and A-G (the `.satellite.` spelling); NM-8's next types.
- 2. Today's gaps, left open on purpose.
- 3. CLOSED since 17:00 -- his answers and what was finished.
- 4. Older questions waiting on him, by day, with their own letters.
- 5. Known bugs and gaps, not fixed.
- 6. Plans not built, and work owed.
- 7. Doc fixes owed (one doc at a time), and state still to act on.

Sections 4-7 are the 16:47 sweep's 688 raw items (journal:
/home/madness/.claude/projects/-home-madness-code-cxx-satellite/40c89c9d-a845-43d9-8998-9ededbf86dd8/subagents/workflows/wf_bdc4ec37-12d/journal.jsonl),
merged and de-duplicated by four readers at ~19:10, read-only against the tree. Each section ends with a
"probably settled -- check before acting" list.

## 0. Where things stand (updated ~23:55 on 10-10: BUILD 0028 -- `.satellite.` for strings and lists)

- **BUILD 0029** = 0028 + the fresh reader's fixes (no HIGH finding; PROGRESS.md section 1 lists them) + 9 check.sh
  rows for the new spelling and the prompt's dir(). Installed, committed and uploaded by its own build.
- **BUILD 0028 -- INSTALLED, commit 542d65dd, uploaded to repo-6** (201 files, 3.9 MB in 12 s; 38,282 linked from repo-5;
  check.sh 1340/1340). Built late on 10-10, after his *"A yes B yes C yes D yes E I don't know what you mean by this, F. yes"*):
  a string's and a list's words are written `.satellite.` and the old spelling is refused (NEW_MILESTONES.md NM-8);
  `dir("/nowhere")` at the prompt says the folder is not there; a full server's oldest slot is erased by a build's
  upload itself. Every program, check.sh, satellite.test, the help pages and the reference docs converted
  (utility/satellite_spelling.py). BUILDS 0021-0027 were compile checks, not installed (a half-converted satl would
  have refused every program). Backup: backups/2026-10-10-before-satellite-spelling/ (tree, config.ini,
  satl-build-0020, satellite_backup's file before F, its patch satellite_backup_main-F.patch).

- **EVERY BUILD NOW INSTALLS, COMMITS AND UPLOADS** (his A and B of 21:15, yes): `make INSTALL_AFTER_BUILD=yes`
  (or a bare make's yes, or `make install`) runs install.sh, then satellite_enterprise/commit_and_upload.sh -- one
  commit on the checked-out branch, never a push, then ~/code/py/satellite_backup's `satellite next --yes`, which
  uploads git's own file list to the first free slot, linking unchanged files from the newest slot. My builds put
  what changed in `COMMIT_MESSAGE=...` (with the Co-Authored-By line last). The server password is in
  ~/code/py/satellite_backup/satellite.password.txt.c4 (chmod 600), at his ask -- never in the tree or git.
- **BUILD 0019** = 0018 + the prompt's `directory("d")`, `dir("d")`, `list()` (his, ~21:20/21:40; section 3) +
  the pipeline. INSTALLED (~/.satl/satl and the word satl). FIRST COMMIT e7315250 on
  milestones-install-and-no-console-handover (parent 69b1d93: 35,561 files, the weeks since then and the vendor
  source). UPLOADED to repo-4: 35,346 files, 537 MB in 205 s, 3,135 linked from repo-3. Vendor's unpacked source is
  in git now, the tarballs are not (his B; .gitignore says why); a clean copy of exactly git's list built satl with
  its window from a plain make (scratchpad/clean/make.log of session d46e09bf).
- **BUILD 0020** = 0019 + the review's fixes (section 3): INSTALLED; commit d68e4d51; UPLOADED to repo-5 -- 12 files,
  1 MB in 8 s, the other 38,469 linked from repo-4. ./check.sh on 0020: 1340 passed, 0 failed (check_session.py 41).
- Earlier tonight: BUILD 0018 INSTALLED 21:11; build/satl = VERSION 004 REVISION 10 BUILD 0018: everything of BUILD 0014 below, and every string and every
  list method running its own number (NEW_MILESTONES.md NM-8; section 3 below). INSTALLED 2026-10-10 21:11 by
  `make install` (his words: *"i'm still on build 0007 myself, and install.sh needs to be run now and with every
  build..."*): ~/.satl/satl and the word `satl` are BUILD 0018. His BUILD 0007, all of ~/.satl and the launcher are
  in backups/2026-10-10-before-install-0018/; config.ini and the launcher came out unchanged, nothing was removed.
- ./check.sh on BUILD 0018: 1337 passed, 0 failed. satellite/satellite_variable_window/prove-sections.sh (by hand;
  opens windows on its own headless mutter): 17 ok on BUILD 0014 (not re-run since: nothing in it changed).
- BUILD 0014 = satellite.variable.time + satellite.time.sleep (reviewed, fixed) and window SECTIONS (NM-7;
  reviewed, fixed).
- EVERY BUILD INSTALLS (his words, 21:10, above): build with `make INSTALL_AFTER_BUILD=yes` -- it builds what
  changed, runs satellite_enterprise/install.sh and asks nothing. Never `INSTALL_AFTER_BUILD=no` again. The commit
  and the upload after it are not built yet (section 1, THE NEXT TASK).
- Today's backups and patches, in ~/.claude/projects/-home-madness-code-cxx-satellite/backups/:
  2026-10-10-before-time-and-sleep (time-and-sleep.patch), -before-window-sections, -before-time-review-fixes
  (time-review-fixes.patch), -before-sections-build (sections.patch), -before-sections-review-fixes
  (sections-review-fixes.patch), -before-open-items-docs, -before-string-numbers (string-numbers.patch, and
  satl-build-0014 kept for timing), -before-list-numbers (list-numbers.patch, satl-build-0016).

## 1. Questions waiting on the author (today's; "yes to all" works)

**ASKED ~23:55 -- AFTER `.satellite.` FOR STRINGS AND LISTS (letters A-D):** -- ALL FOUR ANSWERED 2026-10-11 (*"A. ask
to install, but what is going to be asked when someone pulls this from the git repo?? B. yes C. yes D. yes"*) and done:
SCRATCH.md/OPEN_ITEMS_2026-10-11.md section 3.

- A. (his E of 22:20 again, plainer) Plain `make` stops at the end and asks "Install it into ~/.satl, commit and
  upload? [Y/n]" -- Enter, or a minute of waiting, does all three; n does none. Keep that question? (yes = keep it;
  no = plain make does all three without asking)
- B. His own programs in ~/code/satl still use the old spelling and are refused where they call a string's or a
  list's word. Convert them with the same tool, after a backup? (yes?)
- C. The help readers found older wrong facts, not tonight's: the list page says `a[0]` is S501 (satl says S413);
  satellite.container's page says main's arguments are refused (S201), and they run. Fix them? (yes?)
- D. C++ comments, SCRATCH.md notes and the loss chain's build tools (SCRATCH.md/SATELLITE_TEST/chain_tools) still
  show the old spelling: leave them as the record they are? (yes?)

**ASKED ~22:20 -- AFTER THE PIPELINE AND THE PROMPT WORDS (letters A-E of 22:20; "yes to all" works).**

- A. Next is his E: strings and lists switch to `.satellite.` now -- `s.satellite.find("x")`,
  `l.satellite.append(3)` -- and the old `s.find("x")` is refused, naming the new spelling (his D). A tool built on
  the real lexer converts every program, test and doc. Then the window. (yes?)
- B. In a chain every call gets it: `s.satellite.upper().satellite.find("X")`. (yes?)
- C. His own spacesuit's methods stay as he writes them (`d.call_speak()`); only built-in words take `satellite.`.
  (yes?)
- D. `dir("/nowhere")` prints a line saying the folder is not there -- today it says nothing: the word answers
  false and never stops a program (003's rule, directory_words.hpp). (yes?)
- E. A plain `make` still asks "Install it into ~/.satl, commit and upload? [Y/n]" (Enter, or a minute of silence,
  is yes) -- keep the question (his 10-07 design)? (yes?)
- F. When all 100 slots on the server are used, a build erases the oldest slot by itself and uploads into it --
  today it asks at a terminal (a minute of silence is no) and fails without one. Every bare make commits and so
  takes a slot (the build number file is tracked): about 95 builds from now they are full. (yes?)

**ASKED 21:15 -- ANSWERED ~21:20: *"A. yes B. yes C. I don't know what you mean by this, D. yes E. yes we do this
before we do anything further..."*** A and B are BUILT (section 0). C meant "the window is the next type to get
numbers" -- E's yes ("the window now") already says it. D and E are NM-8's rulings now (NEW_MILESTONES.md).
**ASKED 21:15 -- THE FIVE HE ASKED FOR AFTER THE 21:08 /clear (letters A-E of 21:15).**
His words first (~21:12-21:14): *"as far as the gtk stuff goes, source_editor_main.satl is the finalized syntax that
will create a list of files covering 50% of the screen, so i'm going to change it to be 250px instead of... 50%..."*,
then *"i've changed it to the number 250, and that should be a files display covering 250pixels of the screen, for a
window, something like that anyways..."*, then *"so i've changed the word from "displays" to "display""*. His file
(satellite_source_editor/source_editor_main.satl, HIS -- never edited by me) now runs, uncommented:
`main_window.satellite.section("files_display", 100%, 250)` and
`main_window.files_display.satellite.widgets.display.file("list_of_files", 100%, 100%)`. Installed BUILD 0018 stops
at its line 8 (S110); with line 8 in today's spelling, at line 10. A plain `250` is already pixels (`250px` is S201).
Today's family is `widgets.displays.file` / `widgets.displays.input` (help + window code; no .satl file uses them).

- A. Every build also commits (this branch, no push) and uploads to the next free satellite_backup slot -- built
  before the window work. (yes?)
- B. Vendor goes into git and the upload UNZIPPED: its 627 MB / 36,082 files of unpacked source as plain files, the
  118 MB of tarballs (vendor/new/) left out, and make builds GTK from the unpacked source -- reversing his 09-20
  "tarballs in git" rule (.gitignore). (yes?) -- no = keep the tarballs; a plain make already unpacks them itself
  (vendor/build_stack.py). Today git would take 3,704 files, 186 of them in vendor/.
- C. The window is the next NM-8 type: its words get numbers, and his file runs exactly as written -- ahead of maps,
  files and numbers. (yes?)
- D. The old spellings `main_window.section(...)` and `widgets.displays.file` / `.input` are refused with a message
  naming the new one, not kept as second spellings. (yes?)
- E. Each type switches to `.satellite.` as soon as it has its numbers (the window now, strings and lists right
  after), instead of every type at the very end. (yes?)

**THE NEXT TASK, HIS WORDS (~19:40-19:50) -- BUILT ~22:00 (section 0):**
*"build needs to start running install.sh, wherever it is"*; *"and we need to commit as part of the build
process, just not necessarily to the main branch..."*; *"and we need to upload to the next available source using
satellite_backup, but I can do that part I guess, but maybe it should be built in, yea build that in too, "next"
free repository on the satellite server,"*; *"have satellite_backup upload the same file list as
.gitignore"* -- the upload takes exactly the files git takes; and *"right now it uploads 24 files, it needs to
backup the entire satellite source code, and the backup needs to work if a machine runs plain "make" on it, no
zipped vendor stuff,"* -- the whole source, vendor as plain files, proven by a plain make in a clean copy of what was
uploaded (HOME pointed at a scratch dir). Ask him: are vendor's .tar/.zip archives left out entirely? So every build: build -> install (satellite_enterprise/install.sh) ->
git commit on the current branch (milestones-install-and-no-console-handover; never main unless he says; no push)
-> upload with ~/code/py/satellite_backup's `next` (the first free slot of 100). THIS REVERSES 10-05's "no git at
all". What I found: a plain `make` ALREADY runs install.sh (in a terminal it asks, Enter = yes); he stayed on
0007 because I built with INSTALL_AFTER_BUILD=no. **THE INSTALL HALF IS DONE (21:11, section 0); the commit and
the upload are still to build.** Before the first commit: read what `git add` would take --
68 untracked files on 10-10 (tests 26, satellite 16, experiments 6, SCRATCH.md 5, satellite.help 5,
satellite-numbers 3, vendor 1, ...), 140 modified, 34 deleted since 69b1d93; nothing big or secret may go in.
Back up ~/.satl/satl (BUILD 0007) before the first install.

- A. Sections fill left to right and wrap to a new row (built that way). (yes?)
- B. A % stays a % when the window is resized (built). (yes?)
- C. Sections can hold sections (built). (yes?)
- D. `ms` means millisecond -- he wrote "microsecond" beside ms, but his 003 `sleep(90, "ms")` was 90 milliseconds. (yes?)
- E. A bare `sleep(5)` is refused; write 5s, 5ms or 5ns. (yes?)
- F. ANSWERED 21:10 -- *"install.sh needs to be run now and with every build"*: BUILD 0018 installed (section 0).
- G. A widget given no size inside a section fills the whole section. (yes?)
- H. For now a window uses sections OR `.append(piece, x, y)`, not both -- each way round is refused with a
  message until he decides how they mix. (yes?)

**Asked ~17:20, about the `.satellite.` spelling (NM-8's LAST step) -- not answered: he chose numbers first.**
His words: *"we need to place everything that isn't under the name "satellite" ... under the name "satellite""*.

- A. The old `s.find(...)` is refused, not kept as a second spelling. (yes?)
- B. His own spacesuit methods stay as they are; only built-in ones get `satellite.` (yes?)
- C. Every step of a chain gets it: `s.satellite.upper().satellite.find("X")`. (yes?)
- D. The arguments variable too: `args.satellite.username`. (yes?)
- E. Words in quotes stay, like `"text"` in `satellite.file.new(path, "text")`. (yes?)
- F. Symbols stay: `+`, `==`, `5ms`, `50%`. (yes?)
- G. Names like `size` or `section` become free for his own things (today they are taken). (yes?)

**Next for NM-8 -- MY SUGGESTION, NOT YET ASKED (his rule: one type at a time, check.sh green, at most 5%
slower):** the map/index next, then files, numbers (their methods are matched by
NAME today), float/hex/color/fraction, thread, program, window (~50 methods with no number), time, the
arguments variable's rows, infinity -- then `.satellite.`.

## 2. Gaps left open today, on purpose

- `.satellite.`: a wrong period is refused when the line runs (not before) where the checker cannot know the
  receiver -- an item of a list declared with no <type>, a capsule's answer, a multiple, a row of the arguments
  variable. (An item of a list<T> or a map<K, V> IS judged before the run since BUILD 0029.)
- OLDER THAN TONIGHT, found by the review: a line that STARTS with a literal is stepped over whole -- `"abc".size`,
  `{1, 2}.append(3)` as lines of their own are neither judged nor run, and exit 0 (program_check.cpp ~3121 and
  run_statements). Refusing such a line (it does nothing) is the fix; not made. A folder move given
  by a name (`dir(d)`) still says nothing when the folder is not there. utility/satellite_spelling.py reads a whole
  page as one program, so in prose it can be fooled (the doc readers caught two); its --apply needs a satl built with
  the recording switch, which is not in satl.

- Time: `+ - * /` on times not built; satellite.time.now and .new unbuilt; the unit check reads only the first
  token (`t = 1/2` is told "expected 1s"; once `*` and `/` exist it would refuse `t = 2 * pause`); `sleep(-5ms)`
  is refused when it runs, not before; `[ ]` straight after any literal (`5[1]`, `1s[1]`) is refused when it runs.
- Sections: no button or label can be made in a section yet (file displays, inputs and sections only) -- his
  design to come; `section` is a method word now, so no widget can be named "section"; a size past 3276700% or
  32768 pixels prints S410's general text; other window methods hand the desk a GtkWidget* read before
  on_the_desk (an old race across the window module, not fixed).
- The 10-09 window widgets: the empty `.append` area is hidden until a piece is appended (no 46-pixel strip
  under his file display any more).
- His source-editor sketch (satellite_source_editor/source_editor_main.satl) still uses unbuilt words:
  `main_window.widgets.text.source(...)`, calling a widget like `main_window.file_display("/home/...")`,
  `text_input.write(...)`, `satellite.directory.open(...)`, and an if with a declaration inside it.

## 3. CLOSED since 17:00 (the author's answers, and what was finished)

- **His answers of ~22:40 to the 22:20 questions: *"A yes B yes C yes D yes E I don't know what you mean by this,
  F. yes"*** -- A, B, C: `.satellite.` for strings and lists, every link of a chain, his spacesuit's capsules as he
  writes them -- BUILT (BUILD 0028, section 0; NM-8). D: `dir("/nowhere")` says the folder is not there -- BUILT
  (session.cpp's is_one_change / move_the_prompt; only a folder written in quotes; a name is moved to as before). F:
  a full server's oldest slot erased by a build's upload itself -- BUILT (satellite_backup_main.py next_slot, `yes`;
  tested against a fake full server; an upload without --yes still asks). E: asked again, plainer (section 1, A).

- **His ruling on built-ins** (~17:25): *"that is how I want to do it"* -- every method gets its own number and
  runs it, numbers first, `.satellite.` last (NEW_MILESTONES.md NM-8, PROGRESS.md section 1, first entry).
- **His "A yes B yes C if it's 5% slower, then that is still passing, but other than that yes"** (~17:30): keep the
  copy of a method programs ran; strings first; a type is done at check.sh green and at most 5% slower.
- **STRINGS DONE** -- BUILD 0016: 24 files satellite-numbers/words/1.6.1.n.cpp, new numbers 1 6 1 23-26;
  check.sh 1333/1333 then; 260 tests/ programs as before; one fresh reader found no difference (its comment fixes
  and its one speed fix applied); 4-6% faster. Two stray files a reader's test run left in tests/ were removed
  after checking they were new.
- **His "A yes B yes C yes D yes"** (~18:30): lists next -- DONE; strings/test_string_methods.cpp and
  strings/check_string_methods.py DELETED (copies in both backups); PROGRESS.md's lines about them FIXED;
  `.to_number` = 1 6 1 20's second name, settled.
- **LISTS DONE** -- BUILD 0017: 24 files satellite-numbers/words/1.4.2.n.cpp, new numbers 1 4 2 27-28;
  check.sh 1336/1336; 260 tests/ programs as before; 10.7% faster on lists. Its fresh reader found ONE
  regression (a list's word written as a call ran the lines before it, then failed) -- FIXED in BUILD 0018
  (function_table.hpp's two views); check.sh 1337/1337; 260 tests/ as before. Patch: list-numbers.patch.
- **THE PIPELINE AND VENDOR -- BUILT** (~21:30-22:10), his A yes, B yes: section 0. Tested against a private sshd
  first (git list, symlinks, link-dest, the marker, a failed upload, a detached HEAD, no profile), then for real.
  Backups + patches: backups/2026-10-10-before-build-pipeline/ (tree, git index, satellite_backup's files,
  build-pipeline-and-prompt-words.patch, satellite_backup_main.patch, satellite_profile_lists.patch, satl-build-0018).
- **THE REVIEW AND ITS FIXES -- BUILD 0020** (one fresh reader, ~22:30): every finding fixed but one that is older
  than tonight (INSTALL_AFTER_BUILD=yes in the ENVIRONMENT runs the installer twice; harmless now) and the one that
  is his to decide (question F of 22:20). Memory: build-install-commit-upload-2026-10-10.md has the list. Patches:
  backups/2026-10-10-before-review-fixes/ (review-fixes.patch, satellite_backup_main-review-fixes.patch).
- **THE PROMPT'S SHORT SPELLINGS -- BUILT** (BUILD 0019), his words *"I need "directory("some/dir/") to work on the
  prompt, and it changes the directory, and as an alias dir("/some/dir") to ONLY work when entered at the prompt"*,
  then *"list() as an alias to satellite.directory.list() and directory("some/dir") is just an alias of
  satellite.directory.change("some/dir")"*: satellite/satl/prompt_shorts.hpp; a program is refused with a hint
  (capsule_reach.cpp); check_session.py +7, check.sh +3.
- **BUILD 0018 INSTALLED** (21:11), his words *"i'm still on build 0007 myself, and install.sh needs to be run now
  and with every build..."*: `make install` (nothing rebuilt; install.sh proved it on hello_world). Backup:
  backups/2026-10-10-before-install-0018/ (tree, all of ~/.satl, config.ini, satl-build-0007, the launcher).
- **His standing rule** (~19:00): *"you need to prepare for clear everytime your done"* -- written into memory
  (prepare-for-clear-every-time.md, the first line of MEMORY.md).

## 4. Older questions waiting on the author

Merged from the 16:47 sweep (six readers, 306 raw question items). Today's questions (10-10) are in section 1 and
are not repeated here. Each question sits under the newest day it was asked; older days are in brackets. Letters
and numbers are the ones that day used, never renumbered. **(yes?)** marks a line where "yes" means my suggestion,
so "yes to all" answers every (yes?) line; a line without it needs his own answer. Sources are in brackets.

### 2026-10-09

**His source-editor sketch of 14:55** (satellite_source_editor/source_editor_main.satl). Written down that day, not
recorded as put to him. These are the 10-09 letters, not today's A-H. Since 17:10 today his file has these lines
commented out (section 2 lists the same unbuilt words). Never edit his file. [next-session-after-clear-2026-10-09-late.md]
- A. `satellite.variable.string s = input.listen()` as a live string? Suggest no: `input.text` is always what is
  typed, and a copied string cannot change. (yes?)
- B. `main_window.file_display("/path")`? Suggest keeping `.directory(...)` only (built in 0007). (yes?)
- C. `main_window.widgets.text.source("source_display")`, a source-code area? Suggest building it next. (yes?)
- D. Widgets left to right (files left, source right), top to bottom inside a widget? The same question as today's
  section question A (section 1).
- E. `.write("...")` on an input? Suggest `.text("...")` instead. (yes?)
- F. `satellite.directory.open` / `satellite.console.input(">>")` to hold the window open? Suggest not needed. (yes?)

**The toolkit builders' 13 questions** (~/code/py/satellite_toolkit/QUESTIONS_2026-10-09.md, asked as "yes to all?").
His 15:30 *"A yes B yes C yes"* answered three other things (tree changes, the history rewrite, `all` on mom's); the
tree change applied installer B-D. Whether the other ten are yes is not recorded: one line settles it.
[revision-10-and-toolkit-main-2026-10-09.md]
- Main file: A. step 9 stops when `make window` fails, per his *"there is no no-window build"*; B. `--force` re-runs
  a finished step without `--rebuild`/`--restart`; C. required + wanted packages in ONE command, optional ones in a
  second; D. on Debian/Ubuntu the kernel step passes `--keep-boot-default`; E. `env` (the ~/.bashrc block) is not
  part of `all` -- suggest yes to A-E; F. `dir:PATH` with `--remote` uploads a local tree? -- suggest no, github is
  the way. (yes?)
- Installer: A. keep the window required; E. keep `--allow-root` for the container/chroot tests; F. `perl-interpreter`
  instead of `perl` on EL; G. never enable EPEL -- suggest yes to each. (yes?) (B-D: see the last list.)

**Also 10-09**
- What does a bit operator do with a binary's sign (`-b0101`)? It must be settled before `& | ~ << >>` are built.
  Suggest my 09-22 defaults: the digits as written, sign ignored (`-b0101 & b1111` = `b0101`); `&` and `|` answer at
  the wider width; `<<` widens only when a 1 would fall off; `>>` keeps the width; hex the same. (yes?)
  [DOC_QUESTIONS_2026-10-09.md Q8 work list; satellite.help/binary_and_hexadecimal; also asked 09-22 as Q9]
- `5 / 4` still displays 1 (whole numbers divide to a whole number, cut toward zero); number_arithmetic.hpp said to
  revisit that once the fraction landed, and it has (`5/4` touching is the fraction). Which should `5 / 4` answer?
  And should fraction arithmetic answer a fraction (2/3) or a float? (S210 today.) [MILESTONES.md M20; README.md;
  also asked 09-22 as Q4]
- Static linking is open again: the reason for a shared libstdc++ went with the libraries on 10-07. Add
  `-static-libstdc++` (libstdc++ and libgcc_s leave NEEDED)? If yes, re-run check.sh's /dev/full row and the window
  proofs. libwayland-client can never be static. [DESIGN.md 3.4; make_support/048-link.mk:19, 050-build.mk:163-169;
  GTK_AND_NO_DEPENDENCIES.md DEP-2; a-static-library-that-meets-a-driver.md; open since 10-07]

### 2026-10-08

- Mom's machine, step 4 of his order: auto login (GDM) -- for which user? Nothing records it set up.
  [moms-machine-2026-10-08.md]
- His *"change to a random 32-character password every... 6 hours"*: which accounts get it -- this workstation's
  claude_code, mom's machine, the Dell? Asked, unanswered; no crontab exists yet. [claude-code-accounts-2026-10-08.md]

### 2026-10-07

**File position words**, asked after his A-E yes; nothing built yet. [file-operations-2026-10-07.md]
- F. The position starts at the end (size + 1), so `.write` appends until `.position(n)`. (yes?)
- G. `.read()` past the end stops the program, as `f[n]` does; a file is walked with `while (f.position <= f.size)`. (yes?)
- H. `.read(n)` leaves the position at n + 1. (yes?)
- My picks: the position follows its line when lines above it move; a number written into a binary file is one byte
  0-255; a binary is padded to whole bytes; text is its UTF-8 bytes; `read_all` of a binary file is one value. Keep? (yes?)
- `current_source_file`: his B answer says *"maintain position objects for current_source_file, for all files that
  are open, what else?"* Did he mean a built-in handle on the running .satl? My reading: one position per open
  handle, and a program's own source is one of them when it opens itself; no word made for it. [file-model-2026-09-24.md]

**Also 10-07**
- The PGO patch: `sudo bash ./madness_make.sh` failed because sudo's PATH loses ~/opt's clang 24, so clang 21 met
  clang 24's profile. A patch makes the profile follow the compiler: drafted, not applied, waiting on his word. It
  sits only in an old session's /tmp (.../f00b04df-.../scratchpad/pgo-fix/) and needs rebasing on 10-09's
  045-optimise.mk. Apply it? [doc-surgery-2026-10-07.md]
- NM-2a: a user capsule named `main` or `prompt` keeps its own record now, but its place NAME reads the same as
  `satellite.legal.main` / `.prompt`. Refuse those names, or spell the places apart? [NEW_MILESTONES.md NM-2a;
  satellite-legal-built-2026-10-06.md]
- NM-2a, my choice: a spacesuit's field judged alone reads an unlisted place of its own (the other way: a place named
  after its spacesuit). Keep? (yes?) [NEW_MILESTONES.md NM-2a]
- The console's status bar shows 2048 columns while sideways scrolling is on. Show the visible count instead? (His
  to ask for; scrolling was never measured on a compositor.) [GTK_AND_NO_DEPENDENCIES.md GTK-17]

### 2026-10-06

**NEW_MILESTONES.md section 5: 14 questions, none answered** (his numbers; 1-4 block NM-1). [NEW_MILESTONES.md
section 5; satellite-delete-built-2026-10-06.md]
- 1. (NM-1) In this interpreter, or in a child? His words read as this interpreter; a child protects the prompt.
- 2. (NM-1) Do `interpret <file>` and `run <file>` become NM-1's way in, or stay a child beside it?
- 3. (NM-1) A missing `satellite.return(satellite)`: his words make all three the condition; MS-8 (planned) makes it
  S031 and carries on. Which holds for the receiving interpreter?
- 4. (NM-1) The windows a file opened: closed at the end of its run, or kept while the interpreter runs?
- 5. (NM-3) 20% of what? My reading: the machine's memory (`arguments.memory.total`) -- or the memory free at start?
  By his naming rule the setting is `arguments.access.percent(20%)`.
- 6. (NM-3) When the 20% is full: the oldest file's objects let go, or nothing more kept?
- 7. (NM-3) How is `filename` written: `satellite.access("hello.satl")` (a string) or a bare name? And for a name
  with a dot, a dash or a folder, or two files of one name in two folders?
- 8. (NM-3) A file run twice: do the second run's objects replace the first's?
- 9. (NM-3) Two capsules each with an `x`, or one capsule called many times: one object per NAME (last write wins),
  or one per capsule and name?
- 10. (NM-3) Is his 09-18 `satellite.access(object_name)` *"when the program stops"* now
  `satellite.access.filename.object_name` only? (`satellite.access(name)` in scope stays as built.)
- 11. (NM-4) How is a name deleted from a file that already ran, once out of scope:
  `satellite.delete(satellite.access.filename.name)`?
- 12. (NM-5) More than one console open: which interpreter receives the file? With none open, does `satl file.satl`
  open one that stays open?
- 13. (NM-5) Where does the file's output go, and what exit status does the shell get?
- 14. (NM-6) "history" and ".history": will any of history be held in memory, and is the name deleted there?

**satellite.legal** [NEW_MILESTONES.md sections 1-2; satellite-legal-built-2026-10-06.md]
- D. What happens to a file, window, thread or program when its name ends? His answers: *"4. I don't know, but let's
  keep a std::string on each object of it's satellite.legal status for each object"*, and *"D. where is D?"* when my
  lettered list had skipped D. My default until he rules: a file is saved and closed at its block's `}`; threads,
  programs and windows carry on. Keep it? (yes?)
- His *"3. I don't know"* answered a question whose text was not kept, so it cannot be asked again as it was.

**satellite.delete (NM-2)** [NEW_MILESTONES.md NM-2; satellite-delete-built-2026-10-06.md]
- Left for his ruling: the name is NOT deleted from the prompt's up-arrow history, the screen, satellite.log, or
  `arguments.x` / `satellite.library.x`; a started thread, program or window carries on with no name. Delete it from
  any of these too?
- My choices: a delete is a line of its own with one name, in the SAME block as the declaration; a name deleted in a
  block is free again only there; fields, a for's number and `arguments` are refused; thread labels now read
  `(thread box.call_take, ...)`; a deleted name later gets S201 with its own sentence (a code of its own is his to
  number, if he wants one). Keep? (yes?)

**satellite.test** [SCRATCH.md/SATELLITE_TEST.md; satellite-test-design-2026-10-06.md]
- The writers' questions -- is each right as it is? `{b0101} == {5}` and `{1.0} == {1}` are false while `b0101 == 5`
  and `1.0 == 1` are true; negative map keys order -40 before -5; `5 + "2.5"` gives 7.5; `.remove(x)` with x there
  twice removes the first; `.insert(size + 1, x)` appends; `"straße".upper()` keeps the ß; `console.clear()` and
  `.home()` write escape codes into a pipe; `.ok()` on a never-filled spacesuit field is refused at run time --
  answering false would give a way to test an empty field. (also asked 10-02 and 09-27, SPACESUITS.md)
- Seven choices: each program runs as a child satl in an empty folder with a private HOME; an answers program is
  judged by its exact output and writes nothing to satellite.log; full() repeats its set 6 times (~60 s); limits
  60/120/900 s; the speed twins moved into namespace satellite004; windows are left out of full() (a test has no
  display -- he was never told); loss() lasts as long as the disk's fsyncs take (10,000 passes, his number). Keep? (yes?)

**Floats**
- A float holds 128 places but shows 32, and `.string` keeps only those 32, so float -> text -> float loses places
  33-128. Show all, give it a row of its own, or keep? [loss-and-speed-tests-2026-10-06.md; also asked 09-22 as Q1]

**arguments** [SATELLITE_ARGUMENTS.md Part 4G; arguments-item-value-2026-10-06.md; next-session-after-clear-2026-10-06.md]
- Told him, unanswered: with `access` saved false, every start says S011 until `satl --rebuild` (it follows his
  register design). OK as it is?
- One shared table for rows a program writes? Does `args.x = 5` reach `arguments.x` elsewhere, or stay main's (as
  today)? (also asked 10-04)
- My choices: program-made rows are never saved and are only main's; a setting read only at start-up
  (startup_display, directory.default, display.buffer) is refused in a file; a wrong-kind value is refused when
  worked out (its NAME before the run); `satellite.library.arguments.<row>` stays MS-2's spelling (except access);
  the name `arguments` is reserved; the warm-thread rows are read-only until he sets a ceiling; `display(args)` shows
  main's copy. Keep? (yes?) Also told him: `args.m.set("k", 5)` on a map in a row is refused "no capsule named set".

**His start-up note** (BUILD 0027, shown as he wrote it) names things that do not exist -- raised 10-06, unanswered.
[startup-note-text-2026-10-06.md]
- 1. *"SAVE BYTECODE THAT IS COMPILED WITH "save <filename.satl>"; SAVES TO "<filename.sate>" ..."*: .sate was dropped
  on his 10-03 ruling, and no `save` exists. "The current working directory listed below" implies the prompt shows it.
- 2. It names satellite.help("quick"), ("bytecode"), ("interpreter"), ("satellite"), ("newbie"), ("tutorial"),
  ("advanced"), ("what_now"), ("work"). Which exist is unchecked, and today's spelling is unquoted:
  `satellite.help(include)`.
- 3. *"RED TEXT IS CLICKABLE"*: console links are not built.
- 4. `INTERPRETER\'S` is shown as INTERPRETER'S (my choice, said to him). Keep? (yes?)
- Also told him: bring back the old start line's "exit, quit or Ctrl-D" hint now that his note replaced it? (Most
  likely what was meant; the note keeps no more.) [next-session-after-clear-2026-10-06.md]

### 2026-10-05

- Is a GPU required? Not ruled: *"we are not that far yet, we have only dropped support for machines that do not have
  monitors"*, and NVIDIA stays: *"I will never drop support for nvidia GPUs"*. (No monitor = S730, exit 50.)
  [SATELLITE_WINDOW.md WIN-9; README.md; PROGRESS.md; satl-console-handover-trap.md]
- Should the console shadow default to on even with no GPU (measured ~37-40 fps there, far more CPU)? Asked as a/b/c,
  never answered; the notes did not keep what a/b/c were. His: *"I don't want to build further color options at this
  time"*. Also his to see: solid cells (selection, cursor, SGR backgrounds) cast block shadows, and text on them has
  none. [console-text-shadow-2026-10-04.md]
- Font weight 500 (IBM Plex Mono Medium): he has not said how it looks. Thicker or thinner is the
  `console.font_weight` row or File > Settings' "Text weight", no rebuild. [next-session-after-clear-2026-10-05.md]
- The website's install page links to the GitHub repo (public again since 10-08). Its wording is his: change
  anything? (Never touch pages I did not make.) [next-session-after-clear-2026-10-05.md; github-deploy-key-2026-10-09.md]

### 2026-10-04

- Does an included file's `arguments.missing` false count? He said *"I don't know what you mean by 'an included
  file's switch is its own'"*; the demo: main.satl includes helper.satl whose top says false -- `satl main.satl` ->
  true, `satl helper.satl` -> false. Keep that (only the file being run sets it), or let any file's false count?
  [missing-syntax-plan-2026-10-03.md; SATELLITE_ARGUMENTS.md Part 4F]
- MS-2/MS-3 choices: a bare true/false on the arguments is always the value, even where a variable named false
  exists; `satellite.library.arguments.missing` is a spelling of the top line only; the old place
  `satellite.library.main.arguments.missing` is refused; MS-3's "means" sentences drafted from his words, corrected
  lines printed under them; S151-S156 on exits 78-83, not 77-82, because test drivers read 77 as SKIPPED (his
  *"add numbers && renumber as you want to"*). Keep? (yes?) [SCRATCH.md/MISSING_SYNTAX.md MS-2, MS-3]

### 2026-10-03

**SCRATCH.md/MISSING_SYNTAX.md section 7** (Q4 answered 10-06; the rest open).
- Q3. S032 "missing a window" on every run of every program with no window (Hello, World! included)? Console,
  satellite.log, or both? Every run, or once? Does `satl --repl` count as `arguments.interpreter = true`?
- Q5. What does the tutorial show: the next step's line or two, the whole next program, or a command that walks the
  steps (`satl --tutorial`, or a help topic)?
- Q6. Does every window carry a console, or only one the program uses? Suggest: every window, taking no room until
  first used. (yes?)
- Q7. Missing elements said before the run, or at its end? Suggest: S031 and S15x at the checker; S032-S034 when the
  run ends, as S742 is. (yes?)
- Q8. A declared, never-opened window: `.ok` is false; display shows `(window "t")` open and `(closed window "t")`
  closed. A third word for "not opened yet"?
- Q9. What may a program do to a window before `.open()`? Suggest: everything that builds it (.append .title .resize
  .colour .background .font .menu .key .clicked .every .closed .display); refuse what needs a screen (.focus .message
  .ask .choose_a_file .input, a button's .press). Sections and widgets are new since. (yes?)
- Q10. Is a `satellite.console.new` console declared too, and opened by `.open()`? Suggest yes: one rule for every
  frame (003's own example wrote `my_terminal.open()`). (yes?)
- Q11. His *"create a new file (we already have that) but we don't have a error command built for it"*: which does he
  mean -- (a) satl saying it unasked, (b) a missing-syntax line for a program that never asks `.ok`, (c) an error
  command in the tutorial's program, or (d) errors for copy/rename/delete?
- Q12. What do copy, rename and delete answer: a file with `.ok` and `.error`, as `new` does, or a bool?
- Q13. Can a delete be undone: does the prompt ask "are you sure", does the program decide, or neither?
- Q14. Parsing beyond `--help` (options with values, quoted paths, several words, one parser for both): satellite
  words a program calls, or a capsule the tutorial's program writes for itself?

**Windows and consoles** [SATELLITE_WINDOW.md Part 2c; window-open-built-2026-10-03.md]
- Should a window opened again bring its pieces back? It means keeping the window's column alive, so a closed text
  box's words stay readable -- a ruling, not a fix. Today pieces close with the window.
- Should a closed console reopen, as a new pty and an empty screen? Today `.open()` on it is S505.

**Also 10-03**
- `arguments.threads`: he wrote *"set the arguments.threads thing to be how many cores the machine has x2, so it
  checks how many cores the machine has, then starts double that amount, so 24 threads on this machine??"* -- built as
  the warm-thread count (cores x 2 = 24); the ceiling (505,689 here) untouched. Keep it so? (yes?) Note: cores x 2
  is the logical thread count only on 2-way SMT (an 8P+8E gets 32); switching to logical threads is one line.
  [warm-threads-cores-x2-2026-10-03.md]
- Arguments table, my choices: the old spelling `satellite.library.main.arguments` still answers; a few machine facts
  are cached and re-read on a new hostname; `--rebuild` sets first_start; with no config.ini nothing is written (his
  09-18 ruling); config.ini is read once, so another program's edit is unseen until restart. Keep? (yes?)
  [arguments-table-and-first-start-2026-10-03.md]
- MS-1 changed what six of his probes mean (~/code/satl: mythkeepers/probes/threads_skeleton, display_mock,
  threads_overlap_skeleton, shell_skeleton; acorps/probes/arguments, guard_probe): each `> 1` must become `> 2` and
  each `- 1` become `- 2`. His files: he changes them, or asks me to. [SCRATCH.md/MISSING_SYNTAX.md MS-1]
- The scraper: trim its queue to priority 90 only, backing up first? Link-following added 1,964,304 URLs on top of
  the 26,134 priority-90 pages. [quad-test-2026-10-03.md]
- Delete the old .sate files (3,652 in the tree, 6 in ~/code/satl, more elsewhere)? .sate went on his *"skip .sate
  altogether"*; deleting was offered, not done. With no git: only on his word, after a copy. [sate-dropped-2026-10-03.md]

### 2026-10-02

**Spacesuits and pointers** [SCRATCH.md/SPACESUITS.md; pointer-reference-built-2026-10-02.md]
- `this`: a way for an object to hand itself on? There is none; his programs fill a self field with `call_set_this`.
  (also asked 09-27)
- `.pointer()`/`.reference()` choices: an empty pointer stops the program when used (S260; `p.ok()` asks first);
  every object answers `ok()`; the language's words win over a spacesuit's own pointer/reference/ok (four of his
  programs declare `pointer()`); a reference copies all the way down, but shares value-lists, files, windows, threads
  and programs; the copy keeps the lock flag; only objects have them; a pointer's `.pointer()` is itself; an object
  and a pointer at it count as one for .contains/.index_of/.remove and list ==. Keep? (yes?)

**Maps**
- His INCOMPLETE.md question: should `my_map.append({key, value})` work as a second spelling? Today it is refused on
  purpose at run time ("write my_map[key] = value"); `my_map[key]` and `.set("a", 1)` work. His 10-09 *"d leave
  incomplete"* kept the file. [SCRATCH.md/INCOMPLETE.md; which-milestone-2026-09-30.md]

**Random** [SCRATCH.md/RANDOM.md 'Open -- his'; random-built-2026-10-02.md]
- Is a seed this thread's, or the whole program's? Today `seed(n)` reseeds only the calling thread; a process-wide
  seed (each thread folding in its index) is one function away.
- May any grade ever carry the word "secure"? None does, and the help page says so.
- Ctrl-C during a spin: in a file run satl dies at once (no output, no destructor); at the prompt it lands up to 3 s
  later. OK as it is?
- What should `satellite.random.fast` without brackets say? Today it is S201 (an undeclared name) before the run.
- Ten choices: a bare `fast()` refuses (S430, his 003 ruling kept); the step must land on max (S434); no
  100,000-digit ceiling; the spin's length from the same stream; a throwaway is a whole draw; 512-bit outputs as eight
  limbs; pi's digits for the LCG constants; one generator per thread, made lazily; codes S430-S434; a binary or hex
  given to a draw is refused. The table is 16384, not his 16386 (a power of two). Keep? (yes?)

### 2026-10-01

**Running programs** [SCRATCH.md/RUNNING_PROGRAMS.md; running-programs-talk-2026-10-01.md]
- Q7. Should Ctrl-C stop a running program too? Today it reaches both (same process group); POSIX_SPAWN_SETPGROUP
  would keep it out. (also in FAST_PRINTING's open list, 09-26)
- Pipe or pty for a program's output (#6 in the note)? Built as a pipe: colours go, as in any pipe.
- What does a LIST given to a bash name mean: the lines of a script (`{"cd /tmp", "make"}`), or a line plus the
  words bash hands it as $1, $2 (the safe way to pass names with spaces)? Refused for now.
- `end()` on a program satl may not stop (sudo, pkexec): wait for it to end by itself, saying so once (S743, today),
  or give up and leave it running?
- Programs started from satl's console inherit nice 19 (`make -j16` runs at the lowest priority). Start them from a
  thread that was never niced, so they get 0?
- `(command &)` leaves a child that end() cannot reach. A process group or cgroup per program would reach it, and
  either touches Ctrl-C (Q7). Want it?
- Not built: keeping a program's output as a string (he said *"just show the output live"*); a working directory
  or environment for one run; reporting a program started at the prompt and never joined (stopped quietly today).
- My choices: cannot start = not a refusal (ok() false; join 127/126); a signal end answers 128 + n; every method
  needs brackets; S741/S740; never joined = stopped at the run's end and the run fails (S742); output and errors one
  stream, a partial line shown after 50 ms of quiet; input /dev/null until pass(); `bash -c -- "line"`, not login or
  interactive; an empty line refused; program and bash names interchangeable; end() answers for the run it ended;
  .exit/.quit/.shutdown/.return codes of their own; programs from the console get the machine's environment;
  `start("hdie")` refused. Keep? (yes?)

### 2026-09-29

- Accept bare `true`/`false` as literals everywhere? Since build 0125 a bare true/false read as a value is S201 with
  the hint "write satellite.bool.true"; MS-2's D22 (10-04) made them the bool only on the arguments variable.
  Suggest accepting them. (yes?) [map-entry-and-s413-2026-09-26.md; satellite-project-state.md; also asked 09-22,
  and 09-21 as GTK-3's "a true and a false to type"]

### 2026-09-27

**Spacesuits** [SCRATCH.md/SPACESUITS.md; spacesuits-state-2026-09-27.md; spacesuits-built-2026-09-22.md]
- Two recursive shapes are not tail calls: `satellite.return(count_to(i + 1, n))` on the last line (904 MB at
  100,000 deep) and `next[1].sum_rest(acc + value)` (~10 KB a node). His ruling said *"only as the last line ... and
  do the tail call thing"*. Does `satellite.return(self(...))` count? Does the same capsule on another object?
- Class features with no word yet -- which does he want? An empty value for object fields (test it, set it back); a
  super call (`super.speak()`); a type test before a downcast; sorting objects by a key; static fields or capsules;
  abstract capsules or interfaces; a destructor; a supertype's nested spacesuits by bare name. (`==` and
  `.reference()` are built; `this` is under 10-02.) None is built without his word.
- M35, a built type as a supertype (`satellite.spacesuit a(satellite.variable.string)`, S210 today). His words:
  *"Well right now I Don't think satellite.protected needs arguments just yet, just leave it marked down as it's own
  milestone and we'll just never get to it... but a spacesuit still has to take a supertype, that is certain."*
  Open: may a supertype be another spacesuit (works today), and what is `satellite.supertype.<name>` when the
  supertype has no such parameter? Minting `satellite.supertype`'s number is his. [MILESTONES.md M35; PLAN.md red
  note 5; also 09-22 and 09-18]
- POLYMORPH M1-M7 (POLYMORPH/M1-M7.md, written 09-14): M1 D1/D2, a spacesuit declared inside a capsule -- per class
  or per object, and what a second run does (S210 today); M2 D1-D3 (`satellite.polymorph` has no word; D1 partly
  answered by "Both spellings", PLAN.md:851); M3 D2; M4 D1-D3 (D3: is `new_class.namespace('x')` a string or a
  written name?); M5 D1-D5; M6 items 2-7 (satellite.cxx, user commands); M7 D2. [also 09-22; DOC_SURGERY.md 10-07]
- `Type(args)` inside a line: today an object is made only by a declaration line (S110 says so). [also 09-22, M8]

**Maps** [SCRATCH.md/MAP_SORT.md; map-entry-and-s413-2026-09-26.md]
- Should a map's `.reverse()` answer the map reversed? Today it answers its KEYS as a list, so
  `count.sort("value").reverse()` is the keys largest-first; and `m.sort("value")[1]` is the key 1, not the first
  entry. [also asked 09-18, PLAN red note 10: "Say which"]
- How do negatives, decimals and words order? Built (my reading): strings shorter first, then his a-z, A-Z, 0-9
  table; whole numbers by sign, width, digits; floats and fractions by worth; a tie keeps insertion order; kinds
  with no order between them are refused. Keep? (yes?)
- .sort choices: it answers a COPY (`m = m.sort()` keeps it); `.by_name()`/`.by_value()` on a map and
  `list.sort("key")` are refused; any word but key/value is refused; the name satelliteMapEntry. His words: *"I
  wanted 3 different functions, .sort() which is just an alias for sort by key, and .sort("value") which sorts by
  value"*. Keep? (yes?)

### 2026-09-26

**Strings and containers (M13, M14, M16)** [MILESTONES.md M13, M14, M16]
- `.find` counts from 0 while every other position counts from 1, so `s.substring(s.find(","), ...)` is one off
  (check.sh pins .find from 0). Which should it be?
- M14, the variant's asking half (`.holding .holds .held .clear`, word 1 6 14): `satellite.container.multiple` with
  no `<>`, or a word of its own? His words: *"a satelliteContainer which is satellite.container.list
  satellite.container.map satellite.container.variant and containers can hold other containers so we need a
  shared_ptr"*. Not built; his the_combine stops at it. [also asked 09-23]
- A line that starts with a literal, or a method that changes nothing (`"abc".append("d")`, `5 + 3`, `s.size`), runs
  nothing and says nothing. Refuse such a line?
- Does *"just convert the number to the string"* cover a float like 4.5? A whole number where text goes is its digits
  (S020), but a float or bool is refused (S301) when the line runs: `s.contains(42)` works, `s.contains(4.5)` stops.
- `[ ]` straight after a method's answer or a literal (`s.split(",")[2]`, `"abc"[1]`) is refused with the way round
  (name the value first). Build it?
- M16 choices: positions count from 1 (`s[1]`, `s.at(1)`, `substring(start, end)` keeps both ends, 0 is S413);
  `s[n] = x` is refused; `append`/`clear` on a literal inside an expression are refused; `resolved` answers the
  string as it is. Keep? (yes?)
- Should `{}` given to a map name make an empty map? Today `{}` is an empty LIST, refused for a map
  (`satellite.container.map()` makes one). [SCRATCH.md/CONTAINERS.md; containers-and-access-2026-09-26.md]
- satellite.access, my choices (they answered his three questions): a header, the value, one reach line and one
  fill line per level, `.size`/`.keys`; prints alone on a line, answers a string inside an expression; works at the
  prompt; object fields shown with their values. Keep? (yes?) (ERRORS4 013/014/028/079 are bugs in it.)

**Errors (SCRATCH.md/ERRORS3.md)**
- #6. What does a bare `satellite.variable.number n` hold? A bare string starts "" and a container empty; a bare
  number holds nothing, and reading it is refused. (0 would be C's global rule.)
- #14. A mistyped huge `float.decimal` in config.ini makes every float division hang, silently. Suggest a notice at
  start-up when a division would take over ~1 s (the others: Ctrl-C-able division at the prompt, or a cap -- a cap
  would overrule his ruling that any value must work). (yes?)

**Printing** [SCRATCH.md/FAST_PRINTING.md; display-cost-is-word-family-checks.md]
- Open, his: (1) an overrun at the prompt stops the line and the prompt returns (built) -- or should it end satl?
  (2) a slow reader (`| less`, Ctrl-S) holding 131,072 jobs crashes, by his ruling, unless he changes it; (3) what
  counts as one item, and a byte cap too? (4) S840, machine code 65 and its message: his to reword; (5) SECRET.md
  (*"i'm going to write SECRET.md for another time"*); (6) Ctrl-C for programs (10-01, Q7); (7) the mini satl for
  printing, someday. Also: the caret pointing at the loop's `}`.
- Every std::cerr write holds the next console feed for 1 ms (kPtySettles), so a program alternating a report and a
  display would be held to ~1,000 a second in satl's console. Nothing does that today. His to weigh.

**Machines**
- satl needs glibc 2.38 (EL10); it does not start on AlmaLinux/RHEL 9, Ubuntu 22.04 or Debian 12. Build for older
  machines, on the oldest supported system (an AlmaLinux 9 container), not static? Not started.
  [SCRATCH.md/RUNNING_PROGRAMS.md; GTK_AND_NO_DEPENDENCIES.md DEP-3]

### 2026-09-25

**SCRATCH.md/ERRORS2.md** (the file holds the rest of that sweep's state)
- #3. A number plus text that does NOT spell a number joins (`4 + "abc"` = `4abc`, 003's rule); his words covered
  only *"we need 4 + "2" to return the number 6"*. Confirm the join, or refuse it (one branch)? [error-sweep-2026-09-25.md]
- #4. His satellite.include help page is wrong twice: "absolute path", though by his 09-16 ruling a leading slash is
  RELATIVE; and "relative to a changing current_working_directory", though an include is relative to the INCLUDING
  file's folder (`./x` and `../x` work unquoted). His page: he edits it, or says I may. [still wrong 10-07; also
  asked 09-22 as Q10]
- #5. Blocks at the prompt: (1) a level is 4 spaces, not the tab he wrote; (2) `{ statement }` on ONE line is
  refused, though his *"accept anything valid regardless of lines"* suggests it should work; (3) re-declaring a
  spacesuit that objects were made from leaves them on the old layout.
- #6. Choices of 09-25: `.center()` into a pipe or file prints the text unchanged; a byte-order mark is dropped; a
  shebang first line is allowed; a line starting with `#` or `/*` is refused; the config notice is S016 (he offered
  SC01 or any free one); upper/lower use the C library's tables, so ß stays ß; a `{ }` block at a file's top is
  allowed. Keep? (yes?) [also NEW_ERROR_LIST.md]
- His quad/quad_main.satl line 6 never closes its string: his file, his fix (satl names the bug exactly). [ERRORS2.md Part 3]

**Logging and listing**
- M5 satellite.log, my choices: every report goes in, not only warnings (003 logged warnings only); two repeated
  joins on two lines are two S724 notices; a name clash follows the scope chain. Keep? (yes?) (Where the log lives
  was ruled 10-09.) [MILESTONES.md M5; DESIGN.md 7; m5-satellite-log-2026-09-25.md]
- Listing, info and sizes, my choices: in info NUL = not text, `+` = unreadable, `~` = the drive's own count;
  size_type `b` (his README) where the table writes "bytes"; `info.file` of nothing is file_not_found; progress for
  any long listing; `free()` in mb; gb/tb/pb/eb past mb; "1 byte"; 1024.000 shown as 1.000 of the next unit;
  directories and links show "-"; numbers right-aligned. Keep? (yes?) [listing-files-and-sub-2026-09-25.md]

**A language written IN satellite** (SCRATCH.md/LANGUAGE_IN_SATELLITE.md, from his *"can you build a set of
milestones that we could do so we CAN build a programming language in satellite?"*). Each is his to design.
- L2. Characters a program can ask about: a character's code (`"A".code()` is 65, and back), or `.is_digit()`,
  `.is_letter()`, `.is_space()`? Is a "letter" a-z, or every language's letters?
- L5. `x.kind`, answering "number", "string" and so on.
- L6. Errors a program can catch: `satellite.statement.try { } satellite.statement.catch(e) { }`, or errors as values
  (asked `.ok`, as files are)? Today every refusal ends the run.
- L9. A capsule held in a value or a table (optional).

### 2026-09-24

**Threads** [THREADS.md T1, T2; satellite-threads-t1-2026-09-23.md; MILESTONES.md M38, M39]
- Two proposals: a check mode (off by default, so free) that names the line where two threads wrote one object with
  its lock off -- a race can CRASH satl (ERRORS4 001); and a lock that lets go when its capsule ends. His words: *"by
  default it's not on, so the programmer has to lock everything themselves"*. Build them? [also 09-23]
- Should `satellite.library` be writable at run time (S250 today)? And T3, windows on threads (S727 today)? [also 09-23]
- My choices: a fresh OS thread for every start(), not a pooled warm one; a 128 MB stack per thread; output locked
  one line at a time once a thread exists. A join lets go of its locks, so in `total = total + w.join()` another
  thread may change total -- his: *"I don't know, i'm not going to use total = total + w.join() but should we have
  it?"* Keep? (yes?)

**Files** [file-model-2026-09-24.md; escapes-sizes-prompt-memory-2026-09-24.md]
- Not covered by 10-07's A-H: do both orders, `(n, "s")` and `("s", n)`, mean a line? How to go back to appending
  (`.append` always at the bottom)? Delete a line (does `remove_at` stay)? Out-of-range numbers? First vs every
  occurrence? Read-only open (FO-5)? Big files (FO-9)? `.write("text")` on a binary file (my choice: its UTF-8 bytes)?
- Accept a bare `text`/`binary` as a `file.new` argument? His own `satellite.file.new('somethjing', text)` is
  refused: bare text reads as a variable, so it must be quoted. Never asked.

**Also 09-24**
- Extensions: parked by him -- *"let's leave this as an .md file with where we got to"*. EXTENSIONS.md section 6 keeps
  its eight questions; nothing until he reopens it.
- An eval word (`satellite.run(list_of_str)` style) is his call: the string form is what QUAD AI needs; writing a .satl
  and running it with a second satl already works. Nothing built. [escapes-sizes-prompt-memory-2026-09-24.md]
- Prompt memory, my choices: redeclaring a kept name REPLACES it at the prompt (programs still refuse); kept files
  are saved after every line. Keep? (yes?) Offered: typing a kept name alone could echo its value (today it "does
  nothing") -- want it?
- The website's claude WordPress user is an Administrator; suggest making it an Editor. (yes?)
  [satellite-foundation-website.md]

### 2026-09-23

**Processors** [CPU-PLAN.md; ARCHITECTURES.md; MILESTONES.md M37; satellite-cpu-plan-and-fresh-clone-2026-09-23.md;
satellite-arguments-and-cpus-2026-09-23.md]
- M37's per-processor builds (`make cpus`, build/cpu/ with 53 builds, satl-cpu-level, the nine satl.<name>
  executables): his words *"those are the names that I want to use ... as we will be supporting these with these
  names"*. My recommendation: one satl, keeping `make CPU=<name>` only if a race shows it matters. Never ruled. Only
  haswell_test/satl.haswell exists; build/cpu is stale (REV 08, 4.1 GB).
- Should an install put satl-cpu-level's choice into ~/.satl? Today it installs the ordinary build.
- Should `arguments.cpu.architecture` answer the chooser's word (raptorlake on his 13700K) instead of 003's
  haswell/baseline? (A Raptor Lake or Zen 4 answers "haswell" today.) [also SATELLITE_ARGUMENTS.md Part 4E]
- Kernels: his 64/128/256/512 against the recommended 64 / MULX / MULX+ADX / IFMA. Measured: clang emits MULX but
  never ADCX/ADOX, hand-written MULX intrinsics were slower, and the 13700K has no AVX-512.

**Also 09-23**
- Input from several threads (003's Q3): a thread blocked in `satellite.console.input` stops only when its line
  comes. [THREADS.md T1]
- Console colour (M36), my choices: a coloured string holds its codes everywhere (== and .find see them; files keep
  them); display strips them into a pipe and drops colours under NO_COLOR; a see-through colour is refused; `()` puts
  back the terminal profile's colours. Keep? (yes?) [MILESTONES.md M36; satellite-console-colour-2026-09-23.md]
- satellite.library, my choices: values literal-only and untyped; 003's `satellite.library.capsule_name.variable`
  not built (no program uses it); `#000000` refused, pointing at `x000000`; the `satellite.library.system.*` dials
  refused. Keep? (yes?) [satellite-library-built-2026-09-23.md]
- On an index, `.sum .max .min .join` refuse and ask which half (write `s.values.sum`). Keep? (yes?)
  [satellite-list-from-003-2026-09-23.md]
- Arguments rows, three choices: a row wins over a no-bracket method; `args.color` is filed as "colour"; `.remove`
  takes satl's own rows out of the copy. Keep? (yes?) [SATELLITE_ARGUMENTS.md Part 4D]

### 2026-09-22

**The numbered list of 09-22** ("WAITING ON HIM", next-session-after-clear-2026-09-22c.md). Asked again later: Q1
(10-06), Q4 and Q9 (10-09), Q10 (09-25). Q7 and Q11 are probably settled (last list).
- 2. `12.5.number()` is refused -- or should it cut, or round?
- 3. The whole side of a float rounds past `float.whole`, though M20 (08-27) said "exact and unbounded". Which holds?
- 5. Hex's `x` is mandatory (our reading of his *"just make the prefix b mandatory and move on"*), and `b1100 + xFF`
  answers 267 by worth (a check row pins it). Keep? (yes?)
- 6. Colour transparency 0 = solid (our choice), and saturation left out, as he leaned: *"we may just have to leave
  saturation out for our color variable, it makes the .satl code simply too complicated"*. Keep? (yes?)
- 8. Which outputs drop a binary's sign? Suggest: `.binary()` gives the bits only. (yes?)
- 12. Delete the old folders under .claude/worktrees? All seven still exist: errors and m16 (merged in 8923ad3),
  wf_5ba28f6d-9e4-1..4 (merged), and wf_29773d3e-91c-1, which is NOT mine -- leave it. The note said `git worktree
  remove`, but no git may run now: it needs his word, a backup, and a way without git.

**Windows and GTK** [SATELLITE_WINDOW.md; GTK_AND_NO_DEPENDENCIES.md; satellite-project-state.md]
- Window font: 11 or 12 px? He wrote both `my_window.title(12px)` and *"leave it unchangeable at 11px"*. No font is
  set for a window today (the console uses 11, now a Settings row). [WIN-3; GTK-10; also asked 09-19]
- Q-WIN-11c: silence the accessibility bus, as the portal was? With no a11y registry GTK prints three Gtk-CRITICALs
  at start; GTK_A11Y=none would quiet them and turn off screen readers for every satl window. Suggest leaving it on. (yes?)
- Q-VTE-1, gnutls for VTE: (a) vendor gnutls + nettle + gmp (recommended), (b) patch the line out, (c) live with it.
  The red warning itself is gone since d1c9131 (the terminal is reset once at birth). Suggest (a). (yes?) [GTK-17]
- May a satellite line WAIT for a person (`c.input()`), or is `.typed` (a capsule) the only way? Built as the
  capsule; a blocking input is his to ask for (MS-20 plans `my_window.input("> ")`). [GTK-11, GTK-17]
- A draw capsule for a canvas: his to ask for. GTK-15 was built as a display list; a draw capsule needs a re-entrant
  walker first.
- GTK choices built as recommended, each reversible: a radio group as `satellite.window.one_of({...})` asked
  `.chosen`; a tab's name on the piece (`.title`); GTK-9 shape (1); submenus `file.menu(recent)`, `file.separator()`;
  keys as the character or a lower-case name; the canvas click as `.across/.down` (chosen, not recommended) vs
  `.x/.y`; `.outline/.thickness/.arc`; `satellite.console.new(title, w, h)`; `.display` takes text or a number; no
  `--hold`. Keep? (yes?)
- His *"this will finally allow satl to start a satellite.console.new, and we can incorporate code written in
  satellite into the interpreter"*: GTK-17 built the first half; the second half is his to say more about first.
- Licences (DEP-6, WIN-8): cairo elected LGPL-2.1 -- or MPL-1.1, "arguably the better election" (reversible)?
  gperf and meson are unsettled in licenses/README; should `satl --licences` print the OFL (IBM Plex Mono is
  embedded)? THIRD-PARTY-NOTICES.md is not written. [also 09-20]

**Spacesuits (M8)** [MILESTONES.md M8; spacesuits-built-2026-09-22.md; next-session-after-clear-2026-09-22b.md]
- My choices: a method dispatches on the OBJECT's spacesuit (003 was static); a field declared again in a subtype is
  a second slot; `darkening() { }` (C++'s constructor, 20 of his lines) is refused with the sentence to write
  `satellite.constructor`; a spacesuit-typed field with nothing after its name starts empty; overrides must match
  access, parameters and answer. Keep? (yes?)
- A file reaching what its includes include: built narrow; he is still thinking -- *"I think differently for only the
  last thing ... still thinking"*. The one place to widen it is CapsuleTable::reach (capsule_reach.cpp).
- Bare calls into another file: he approved refusing them when I said there were 0. The truth: 757 bare calls in 7
  of his files (view_forge_main, global_view_main + 3 _probe files, the_blue_moon x2, written before his 09-13
  ruling). Still refuse, or allow when exactly one included file declares it? [count-through-the-real-reader.md]

### 2026-09-21

- Q-WIN-11b: should a button press whose capsule is refused take the whole run down? It does today; the usual GUI
  answer is to report and carry on, which needs somewhere to put the report. [SATELLITE_WINDOW.md Part 2b]

### 2026-09-20

- Since satl links GTK it carries libgio, which can open a socket: "satl cannot reach a network" is now only "does
  not" (no word calls gio's network classes; check.sh splits it into two rows, 488 and 497). OK? His to rule on.
  [SATELLITE_WINDOW.md ~322-336; DOC_SURGERY.md]

### 2026-09-18

**SATELLITE_ARGUMENTS.md Part 5, red notes**
- 5. His *"can we alias them as arguments.direct_variable for the arguments?"* was read as short direct spellings
  for the deep names (the alias table). If he meant a word `arguments.direct_variable`, it is one more row.
- 6. "Reserved" in `arguments.memory.system.reserved`: touch every page (really resident; start-up pays), mlock
  (needs privilege), or reserve address space only (free; reserves nothing)? It decides if B15 is 20 minutes or a day.
- 7. Does `directory.history` have a bound? (The store's bound is now NEW_MILESTONES section 5, 5 and 6.)

**SATELLITE_ERROR.md**
- Note 11: can a program turn a debug feature on mid-run? Rule 3 says no (a data race); a middle way re-chooses at a
  capsule boundary (`satellite.trace_on()` around one capsule) -- a real design, not free.
- My proposals, unconfirmed, needed before R12/R15: a statement ring of 256 entries (4 KB), overridable in config.ini
  (he asked *"what do you mean by "ring"??"*); the report's environment allowlist HOME PATH PWD SHELL TERM LANG
  SATELLITE_* plus LD_LIBRARY_PATH, LD_PRELOAD, USER/LOGNAME, TZ. Accept? (yes?) [Part 15, notes 5-9]

**SATELLITE_FILE_OPERATIONS.md Part 4**
- R1-R10 are recommendations, never confirmed (R0 answered; R3 built as 0). R1 (no seek/write) is overtaken by his
  `.position`; R10 is moot (every word is inside satl). The rest: R2 the include path rule for files, R4 replace hits
  the first occurrence, R5 a newline in append means several lines, R7 read-only yes, R8 no sandbox now, R9.
  Confirm those? (yes?)

**SATELLITE_INFINITY.md** (built "unless told otherwise") [Part 11, Part 12; DESIGN.md section 10]
- Part 12, Q1-Q43: answered only Q20 (10-09), Q36, Q37, Q39, Q40a, Q42. Every other one has a default, e.g. Q2 inf -
  inf answers the number left; Q3 inf / 3 refused; Q12 C++ first, then .satl (against his *"we write the code for the
  "sat" objects IN satellite"*); Q21 the order; Q27 a word that wants a whole number; Q28 where the C++ lives; Q29
  where /infinity/ lives. Read Part 12 when he has time.
- Q16, the next phase (dots and dashes): which spelling stays (message seven's `class-1-infinity`, message eight's
  dots, or both); what x is in infinity-x; what a dot means where a dot means something already; the stored form (a
  tree, against his *"we STORE the code as an std::string"*); how a program asks for each new symbol. Nothing built.
- Q34 (INF-8), the infinitely small numbers: build them? Named and shown how? Is `infinity^0.5` one? Where is an
  endless series cut? His: *".power(-infinity) so we have to keep a sign with everything"*.
- INF-1: a numbered but unbuilt type gets S110 -- should it be S210? (It moves tests/not_understood.satl.)

**MILESTONES and PLAN**
- M34 (--rebuild): eleven features have a bit and no behaviour (history, frames, trace, coverage, capsule_timing,
  watchpoints, memory_accounting, thread_state, report_on_success, report_file, replay). Does `--debug` stay a flag?
  Numbers cannot be bits: leave them, compose a second value, or bit-pack small ones? [MILESTONES.md M34]
- PLAN red note 8: a list handed to a word that takes text reads back as typed (`{1, "two"}`). Or should it refuse
  to print, as bytecode and a spacesuit do?
- PLAN red note 9: a float's `.reverse()` was decided 09-18 (`12.34.reverse()` is 34.12), not built (FLT-4). Build
  way (a), which reads the chain, not the value, "unless you say otherwise". (yes?) And should a whole number act
  like a binary (`120.reverse()` is 21, `b1010.reverse()` is `b0101`)? [bytecode/container_calls.cpp:339]

### 2026-09-17

- The satelliteArguments class (A7-A23) is compiled and used by nothing; the old store (struct Argument,
  add_text/add_count) still runs. Wire it in, or delete it? His words then: *"remove any other arguments code that we
  have, except the code we need for the wiring"*. [SATELLITE_ARGUMENTS.md Part 4]
- Red note 2: should the arguments value have a hex arm? (Arm 10 is reserved for it; the hex type exists now.)
  [SATELLITE_ARGUMENTS.md Part 5]
- M0.5: `shown()` does not escape a backslash, so a name holding the text `\x1b` prints like one holding ESC. Left to
  him by the 09-17 review. [satellite/machine/shown.hpp]
- M0.5 readings: `satl --debug` alone is bare satl; `--debug` twice is `--debug`; build/satellite-004 links to
  build/satl; D0.5.2: a code below 0 or above 254 exits 255, and 255 is never given to a code. Keep? (yes?)
  [MILESTONES.md M0.5; DESIGN.md 2]
- PLAN M0.6: D0.6.3, how a name shown as `\xNN` is typed back (decided when M6 puts names into strings); D0.6.5,
  keys typed while a line runs are dropped (003's way, built as recommended). Keep D0.6.5? (yes?) [PLAN.md]
- M20.A item 3: split program_walk.cpp (2,636 lines) and program_check.cpp (3,238) to his 300-line rule? It puts
  run_statements in a header -- a real change, not a move. Say whether to do it.
- M20.A item 4: an empty CONDITION in a for (`for(number i = 0; ; i++)`, C's forever loop) -- wanted? S110 today.
- M20.H switch/case, not built. His own question: *"what else should the interpreter provide for "switch"?"*; his
  design: a case may be any piece of a string switch; for a number, code between the cases runs no matter what.
  Numbering `switch` is his, and `satellite.statement.finally` has neither a number nor a milestone. [PLAN red note 1]
- A negative binary is a different VALUE (`-b0101` is worth -5; 741a017) -- my reading of his *"give it a different
  number and keep a sign with all of these things"*. Right? (yes?) [MILESTONES.md M20]
- Percentages, three open: `percentage * number` answers a number but `percentage / number` a percentage, so `50% / 4
  * 4` is refused (S402); `percentage / percentage` has no scenario (`50% / 25%` S301); the `50% - 5` hint suggests
  `5 - 50%`, a different quantity. A percentage that would answer a float is still refused. [MILESTONES.md]
- PLAN red note 3: no entries/ folder exists -- milestones were to write [entry] blocks to satellite.log, but
  check.sh and the harnesses answer exit statuses. Adopt entries under M5, or change the method?
- PLAN red note 6: handing a marked batch to a parked thread (256 warm threads tokenise, then park) has no
  milestone, though it is "the whole point of the pool". Give it one?

### 2026-09-16

- D9.1 and D9.2 (polymorph, M9 not built): what *"re-included into the individual capsules"* means, and what `args`
  are passed to. (D9.3 answered 09-16.) POLYMORPH M1-M7 are under 09-27. [MILESTONES.md M9; PLAN.md M9; PROGRESS.md 4]
- D12.1, the parallel-group syntax in the numbered program (blocks M12 and M30). PLAN recommends one form with a
  count (`#parallel 3`, then the lines); the four batch codes are a proposal. Also his: what runs on the warm threads
  next. Suggest PLAN's form. (yes?) [MILESTONES.md M12; satellite-16-bit-open-questions.md]
- M24: is a stored program a token transcription that may be RE-LEXED, or a 16-bit code stream? It waits until
  something stores a program, and decides what a converter may record. Never propose type_open/type_close tokens.
  [MILESTONES.md M24; PLAN.md; satellite-16-bit-open-questions.md]
- QUESTION rows in REGISTRY.satellite with no ruling: `+= -= *= /= %=` (QUAD wants +=), `/* */`, `::`, `->`, `?`,
  `'`, and four inside a literal (`\`, `/` path, `.` decimal, `~` home). They keep their codes; never delete one.
  (The 8 operator rows were ruled ANSWERED 10-09.) The `/` row's reason is stale: an unquoted include path lexes the
  slash since 09-16. [REGISTRY.satellite:246-295; MILESTONES.md M24; PROGRESS.md 6]

### 2026-09-14

- One more thing to remove from 004's words, which he could not remember then. [PROGRESS.md 4; MILESTONES.md M25;
  DESIGN.md 3.2]

### Probably settled -- check before acting

- (10-09) The GitHub repo, pushed and public on 10-08: memory records it public since 10-08 and pushes from a copy
  with the deploy key. Check live before saying anything.
- (10-09) Toolkit installer B, C, D (wayland .pc via pkg-config, EGL headers, PGO only when its runtime links):
  applied after his 15:30 *"A yes"*, in BUILD 0005. B's Debian/Ubuntu half was never tried.
- (10-09) The 14:28 file-display questions A-F: answered and built in 0007.
- (10-08) MILESTONES' A-D of 10-08 (renumber M34/M35, errors.md, M21, installing test + legal): ruled 10-09,
  errors.md deleted; M21 closed today with the string work.
- (10-07) Doc surgery's C-F answers never came: superseded by the 10-09 A-F he answered.
- (10-07) The source-viewer WordPress plugin: his *"nevermind"* -- he builds it on his server.
- (10-06) MISSING_SYNTAX Q4 (where else the switches are set): answered 10-06.
- (09-26) DISPLAY_THREADS' open questions: moot since FAST_PRINTING replaced that design. Record as moot, not decided.
- (09-23) Should a plain make build GTK itself: the MEMORY.md index says the GTK half was applied 10-09.
- (09-23) `.foreground/.background` on pieces or on satellite.console: dfdc64b built the console half; the pieces
  half was never answered.
- (09-22) The console status bar's look: never commented on, but the console look was approved 10-05.
- (09-22) Q7, a float parameter refusing a plain number: built -- a whole number handed to a float parameter is a
  float (check.sh:6846).
- (09-22) Q11, which first -- the help writers' 13 bugs or the operators: most of the 13 were fixed later (b6b2a98,
  b603f03, 3fdd50a, c4edcd2, 8923ad3).
- (09-16) The include's leading slash: ruled 10-09 -- `include(/test)` keeps meaning ./test beside the file.
- (09-16) What a .sate is: ruled 10-03, no .sate at all (deleting the old files is asked under 10-03).
- (09-15) D0.7.1, a prompt that dies: written for satl-term, removed 09-22.
- (09-15) D0.7.2, Ctrl-C copying a selection: built 10-09, with a right-click menu.
- (09-14) DESIGN section 6, Q1 and Q3: Q3 cannot come up (the return is checked before anything runs); Q1's 10-line
  case was never measured.
- (08-28) The redundant `export PATH="$HOME/.satl:$PATH"` in ~/.bashrc: no longer there (checked 10-10).
- (08-27) What `(0)` means in WORD_NUMBERS.md: defined 08-28 in 003's WORD_NUMBERS.md:104-134; the `?` half was not
  re-checked.

## 5. Known bugs and gaps, not fixed

Merged on 2026-10-10 from the sweep's 121 known-bug items (journal wf_bdc4ec37-12d). Nothing below is fixed.
"(re-checked)" means the named code or text was read today, read-only; satl was not run. "(not re-checked)" means
it was not looked at today. ERRORS4 numbers are pages in SCRATCH.md/ERRORS4/: 78 of its 85 were still there on r09
BUILD 0054 (2026-10-07), and none has been run on r10. Today's string and list work (BUILDS 0015-0017) was checked to
answer byte for byte as before, so the string and list bugs below most likely still stand. Time and window-section
gaps are in section 2.

### The checker and the walker (every program)

- **HIGH: a line that begins with `{` silently ends the capsule, exit 0.** `{1, 2}` alone, `{1, 2}.append(3)` (the
  list help's own refused example) or a bare `{ }` block: the rest of main never runs, satellite.return included;
  inside a while the loop never advances (a hang). Known since 2026-09-23; his call. Programs:
  SCRATCH.md/SATELLITE_TEST/findings/ brace_line, bare_block, brace_line_in_capsule. (not re-checked; seen on r09
  0058) (MILESTONES.md M14 and "Found 2026-10-07 to 2026-10-09"; NEW_MILESTONES.md NM-2a; SATELLITE_TEST.md HIGH;
  ERRORS4/002; memory satellite-legal-built-2026-10-06, satellite-list-from-003-2026-09-23)
- **HIGH: a statement that starts with a code that cannot start one is skipped** (ERRORS4 003). Signs, literals,
  `!`, `/*` and `)` are stepped over: a continuation line that starts with `-` or `+` is dropped (`100 / - 30` holds
  100), `-n = 9` runs as `n = 9`, and a `/* ... */` block runs its contents. (not re-checked) (ERRORS4/003)
- **A `// comment` where only a line end is expected breaks if/else/while/for**: beside `satellite.statement.else`,
  between a header and its `{`, around an if's `}`. A valid program is refused S110. (not re-checked) (ERRORS4/016;
  SATELLITE_TEST.md MEDIUM)
- **A method on a negative literal runs before the minus**: `-5.add(3)` is -8, and `-7.string` is refused.
  (not re-checked) (ERRORS4/007; SATELLITE_TEST.md MEDIUM)
- **`for` (M20.A item 5)**: `i ++` is read as `i++`; junk after the header's `)` is ignored (`for(...; i++))` runs);
  a step like `i + 1 & 2` is refused only after one turn has run. (not re-checked; seen on r09 0058)
  (MILESTONES.md M20.A item 5)
- **Deep nesting crashes the C stack.** 8,000 nested `for`s exited 139 (2026-09-17; not re-run since satl widened its
  stack on 09-22). M20.A item 2: the fix is the walker keeping its own stack, never a depth bound. Nesting inside ONE
  statement (about 27,000; `display(display(...))` 30,000 deep; file words about 20,000 deep) he accepted on
  2026-09-16: *"we are never going to program 1,000 ever... so leave it"*. Reopen if QUAD's generated code hits it.
  (not re-checked) (MILESTONES.md M20.A item 2; ERROR.md "The walker recurses"; SATELLITE_INFINITY.md Part 4;
  SATELLITE_FILE_OPERATIONS.md Part 8 item 4; ERRORS2.md #13)
- **`&&` and `||` are not built**, though he ruled the C symbols on 2026-09-22 (*"let's use c symbols as much as we
  can for this then, and also allow .and() .or() like we allow .power()"*). Inside an if they get the
  "(& | << >> !! are undecided)" sentence, and REGISTRY.satellite still marks them QUESTION. QUAD met this.
  (re-checked: program_walk.cpp:87; REGISTRY.satellite:263-264) (ERRORS3 #10; memory quad-test-2026-10-03,
  satellite-operator-spellings)
- **Operators are judged only when they run**: the catch-all "check that every math sign has a space on both sides"
  still meets `3/-4` halfway through a run, and a touching operator (`n+1`, `n**3`) is accepted in a branch not taken
  or a capsule never called (ERRORS4 022). (re-checked: the sentence is at expression.cpp:1933) (ERRORS3 #10;
  ERRORS4/022)
- **A name with no value gets four different messages**: `display(x)` is S210 exit 14; `x + 1` is "+ was given
  nothing and a number"; a method says "x has no value yet -- give it one with ="; `display()` passes the check and
  fails at run as S210. Suggested: one sentence wherever a name holding nothing is read, and the checker refuses
  `display()`. (not re-checked) (ERRORS3 #11; ERRORS4/041)
- **Block scope when arguments.access is OFF**: a name declared in a block lives on after its `}`, and a declaration
  in an if that did not run still gives the false "y has no satellite.variable line declaring it" at run time. With
  access on it is refused before the run (built 2026-10-06). (not re-checked) (ERRORS3 #4/#5; ERRORS2 1b-C)
- **A closed `{ }` at a file's top is transparent**, so a switch inside one is read; and **S150** names a bare
  `satellite.main()` line even in a file that has a main. (not re-checked) (SCRATCH.md/MISSING_SYNTAX.md MS-2/MS-3
  "Seen, older, and left")
- **A satellite word that does not exist** (`satellite.history(x)`, `satellite.cxx()`) on its own line is refused S110
  "satellite is the first legal object ... immutable" -- the wrong reason; inside an expression it is "no capsule
  named history". Q25 (his *"looks good"*, 2026-10-09) says to tell the word is not built yet; not done.
  (not re-checked) (SCRATCH.md/DOC_QUESTIONS_2026-10-09.md Q25 and defects)
- **Valid programs refused** (ERRORS4): 017 conversions done on whole-name assignment (binary or hex to number, whole
  to float) are skipped for parameters, container items and for counters; 018 a method on a bare value word
  (`satellite.bool.true.string()`) gets a false math-sign hint; 019 a blank or comment line between
  `satellite.return(satellite)` and main's `}` is S103; 021 `thread.new(list[i].method())` gets a false S721.
  (not re-checked) (ERRORS4/017-021)
- **Invalid programs accepted** (ERRORS4): 023 `s.reverse("x")`, `"abc".reverse(1, 2, 3)` and `n.reverse(99)`
  accept and ignore the argument; 030 a statement that only computes a method's answer is accepted and does nothing.
  (not re-checked) (ERRORS4/023, 030)
- **Misleading refusals** (ERRORS4): 037 a name spelled like a hex or binary literal (`x1`, `xa`, `b1`) cannot be
  declared, and the refusal quotes a naming rule it meets; 038 a capsule named `x1` or `5` is dropped silently and its
  body blamed; 039 an expression that stops early (`n = n -1`, `0x1F`, `1.2.3`) gets the "undecided" or math-sign
  sentence; 042 a capsule on an empty spacesuit field gets a false reason; 043 display or join of objects that declare
  to_string() says they have none; 044 members reached through a map item are not judged by the checker.
  (not re-checked) (ERRORS4/037-039, 042-044)
- **Explanations that describe the wrong thing**: a list's `.remove(7)` when 7 is absent is S420 TEXT_NOT_FOUND, whose
  paragraph says "the text looked for is not in this string" (the line's own sentence is right now); `l[-1]` is S410
  "... Lines count from 1"; at the prompt, `1 / 0`'s report ends "satl exits with this", though the prompt carries
  on. (re-checked: s_codes.hpp:253, :281-282, :651; satellite-numbers/words/1.4.2.19.cpp:45) (ERRORS3 #12)

### Numbers, floats and the other number kinds

- **The numbered number methods refuse two ways**: `n.power(3)` is S210 not built, while `n.abs`, `n.floor`,
  `n.digits` and `n.sqrt(4)` are S110 "not one of its methods"; all are numbered 1 6 4 n. It goes when
  number_methods.cpp is wired (Q24, his ruling 2026-10-09: finish it). His 10-10 ruling -- every built-in method runs
  its own number -- has reached strings and lists so far. (re-checked: call_number_method still has no caller)
  (DOC_QUESTIONS_2026-10-09.md defects and Q24; ERRORS4/036)
- **`satellite_number x = -1;` holds 18,446,744,073,709,551,615**: the one-argument constructor is not `explicit`, so
  a signed literal converts silently under -Wall -Wextra; `from_signed()` is the safe way in. (re-checked:
  satellite_number.hpp:46) (MILESTONES.md M23 item 1; ERROR.md #28)
- **satellite_number's other debts, never adversarially reviewed**: to_text is quadratic (7.7-8.5 s at a million
  digits; schoolbook multiply and divide); out of memory throws std::bad_alloc (caught at the top as S999), not a
  machine code; the header shows `unsigned __int128` and `[[gnu::always_inline]]` to every includer; the x1.05 speed
  bar is missed (`i = i + 1` x1.49 with clang, x3.0 with g++; wide strings x1.19-1.24; emoji decode x1.12, encode
  x1.83). (re-checked: the header still has both; timings not re-run) (MILESTONES.md M23 items 2-5; ERROR.md
  section 5; PROGRESS.md section 5)
- **Arithmetic gaps**: infinity has no `+ - * /` (S210, INF-3; only negate and compare work); fraction arithmetic is
  S210; `binary + binary` answers a number that cannot go back into a binary (planned with the operators);
  `m.binary()` answers a string, and to_binary()/to_hexadecimal() are not method spellings; `my_hex = my_hex + 1` is
  refused (hex + 1 answers a number) and `satellite.variable.hex(n)` "has no library built for it yet", so number to
  hex is not built. (re-checked: word_table.cpp has no 1 6 11 row; the rest as of 2026-10-03/06) (memory
  window-race-2026-10-03, 004-owes-what-003-worked-out, loss-and-speed-tests-2026-10-06)
- **`5 / 4` displays `1`**: two whole numbers divide to a whole number, cut toward zero; number_arithmetic.hpp says to
  revisit this when the fraction lands. (`5.0 / 4` is 1.25, and touching `5/4` is the fraction.) (not re-checked;
  seen on r09 0058) (MILESTONES.md:610-611 and "Found 2026-10-07 to 2026-10-09"; DOC_SURGERY.md)
- **A float's `.string` keeps only 32 places** (infinity_display), so float to text to float loses places 33-128.
  Found by the loss tests. (not re-checked) (SATELLITE_TEST.md "FROM THE LOSS TESTS")
- **Number help and refusals that disagree**: binary `.width` is refused ("so far no type has it") while hex has it;
  the number help quotes an old refusal for `n.max(3)`; position 0 is S413 (exit 66), not the S501/S411 the help
  pages name. (not re-checked) (SATELLITE_TEST.md LOW; ERRORS4/049)

### Strings

- **`satellite.variable.string(x)` (word 1 6 1 18) is always refused** S210 "has no library built for it yet", exit
  14, even given a string. Its function is in satellite-numbers/words/1.6.1.18.cpp, but nothing reaches it.
  (re-checked: number_row.hpp's 2026-10-10 comment says string() and string(x) "are not methods, and nothing calls
  them") (DOC_QUESTIONS_2026-10-09.md defects; PROGRESS.md section 1)
- **M13's chain is silent**: `s.find("c").add("X")` answers 2X with no refusal, and `s.find("b").add("x")` alone on a
  line runs nothing, exit 0 (while `.nonsense("x")` is refused). (not re-checked) (MILESTONES.md M13 and "Found
  2026-10-07 to 2026-10-09"; memory which-milestone-2026-09-30; DOC_SURGERY.md)
- **`s = s + ...` in a loop is quadratic**: two full copies per pass (0.34 / 1.30 / 5.66 s for 20k / 40k / 80k
  passes). Suggested: in run_assignment, when the value is the name plus a string and the name holds the only
  reference, append in place. Matters for QUAD's string throughput. (re-checked: string_and_string_add.hpp still copies
  the left side) (ERRORS3 #8; ERRORS4/031)
- **Reading a string backwards** when it holds one wide character walks from the front each time, so a loop from
  s.size down to 1 is quadratic (forwards keeps a bookmark). (not re-checked) (MILESTONES.md M16, review of
  2026-09-26)
- **A string has no `.length()`** (M16 has `.size`); his safe_recovery stops at `.length()`. (re-checked: no string
  length row in words.tsv or aliases.tsv) (memory satellite-list-from-003-2026-09-23, satellite-library-built-2026-09-23)
- **A satellite.container.multiple holding a string refuses most string methods** (S210) before the run, though its
  help says it behaves exactly as a plain string. (not re-checked) (SATELLITE_TEST.md MEDIUM)
- **`.upper()` and `.reverse()` on a coloured string rewrite its escape codes** (ESC[...M deletes lines at a
  terminal). (not re-checked) (ERRORS4/008)
- **003's live escapes are not ported**: `\threads`, `\home`, `\user`, `\memtotal`. In 004 `\threads` reads as a tab
  and "hreads", and other unknown escapes are refused (since A6, 3fdd50a). The notes never record raising it with him.
  (not re-checked) (memory escapes-sizes-prompt-memory-2026-09-24, error-sweep-2026-09-25)

### Lists, maps and multiples

- **`[n]` works only straight after a name**: `make_list()[2]`, `m.keys[2]`, `l.sort().by_value()[1]` and
  `{10, 20}[2]` are refused when they run (honestly worded since r09 0054). Suggested: a `[` after any value reads
  through index_into in maybe_a_method; writing through a call's answer stays refused. After a literal: section 2.
  (re-checked: expression.cpp:2362) (ERRORS3 #9; ERRORS4/045)
- **A multiple with two arms of one kind** (`multiple<list<number>, list<string>>`) chooses neither, so a write below
  it is unchecked (`{{true}}` goes in). Re-checking after each write made 20,000 appends take 7.8 s; doing it right
  needs a write that can be taken back. (not re-checked) (SCRATCH.md/CONTAINERS.md "Still open"; memory
  containers-and-access-2026-09-26)
- **List matching ignores cross-kind equality**: `a[3] == 3.0` is true but `a.contains(3.0)` is false (also
  `.index_of`, `.remove`). (not re-checked) (ERRORS4/010)
- **A map has no `.to_string()`, and maps and lists are copied into and out of capsules** (objects are references).
  QUAD met both. (re-checked: no map to_string row in words.tsv) (memory quad-test-2026-10-03)
- **Slow, not wrong**: each map `.remove(k)` is O(n), so emptying a map is quadratic (16,000 remove_last in 2.3 s;
  ERRORS4 032); a list<number> passed to a typed parameter re-checks every item on every bind (033). (not re-checked)
  (ERRORS4/032-033; SATELLITE_TEST.md "NOT A WRONG ANSWER, BUT SLOW")

### Spacesuits (objects)

- **HIGH: a constructor that makes its own spacesuit runs away** at about 1.7-2 GB/s: `node made` inside node's
  constructor (also through a subtype, or two spacesuits making each other) is not refused, because rings_of_making
  (suit_reach.cpp) skips constructor bodies. Suggested: refuse only the UNCONDITIONAL make -- inside an if it is finite
  recursion -- and never a depth bound. (not re-checked; seen on r09 0054) (SCRATCH.md/SPACESUITS.md "Broken" #1;
  memory spacesuits-state-2026-09-27)
- **HIGH: names are folded to a method word's first spelling** (ERRORS4 004): `color()` and `colour()` in one
  spacesuit reach only one; `m.as_number()` runs `number()`; `o.contain()` runs `has()`; library values
  contain/contains collide; satellite.library.upper reads a value written as up. Argument rows do the same
  (`any_name.color` is filed as colour; str and string are one row -- SATELLITE_ARGUMENTS Part 4D, "left as they
  are"). (not re-checked) (ERRORS4/004; SPACESUITS.md; SATELLITE_ARGUMENTS.md Part 4D)
- **HIGH: a capsule named `lock()` or `unlock()` never runs** (ERRORS4 005): the built-in object lock takes its place,
  and the declaration is not refused. (not re-checked) (ERRORS4/005)
- **A wrong type is caught only when it runs**: `two b = a` prints "before", then S301. The same for a wrong spacesuit
  handed to a parameter, a literal to an object name, an object into a built-type name, and a literal to any capsule's
  parameter including constructors (`num a("hello")`) -- the half of ERRORS2 #8 that 0da03d9 never covered.
  Help-writers #12 (a type-mismatch store prints a one-line report with no S-code) is the same. (not re-checked)
  (SPACESUITS.md "Broken" #2; memory spacesuits-state-2026-09-27, findings-from-the-help-writers)
- **An override that never answers is caught only when it runs** (S240): the override check compares access and
  parameters, not whether the replacement answers. (not re-checked) (SPACESUITS.md "Broken" #3)
- **Tail calls**: `satellite.return(self(...))` and `next.same_capsule(...)` are not tail calls (9-10 KB per level);
  which reading to take is his. A comment line after a capsule's last self-call stops the tail-call collapse (1.1 GB
  at 300,000; ERRORS4 035). (not re-checked) (memory spacesuits-state-2026-09-27; ERRORS4/035)
- **Pointer-review leftovers**: `x.call_take()` on a field never given an object says "could not be read to the end
  ... (& | << >> !! are undecided)", not "holds no object yet"; a thread kept in a field of the object it runs on
  keeps that object alive -- a strong circle, even when started through a pointer (thread_calls.cpp keeps self).
  (not re-checked) (SPACESUITS.md "THE REVIEW"; memory pointer-reference-built-2026-10-02)
- **Freeing a long chain of objects recurses once per object** (M22; 003 used a staged burial). Whether more of 003's
  thread access list (THREAD.md D1/D2) is owed was not re-checked either. (not re-checked) (MILESTONES.md M22;
  PROGRESS.md 6.8 item 5)
- **Refusals that say something untrue** (SPACESUITS LOW): C-02 access on a base-typed name lists a subtype's
  capsules, and the listed line is refused; B-12 a loose capsule is called "inside the satellite.protected part";
  C-12 S230 says only base's capsules can reach it, though a subtype's reach it bare; A-07 an object declared in an if
  that did not run is "no capsule named get"; D-09 an inner spacesuit reading the outer's field is called undeclared;
  D-10 S301 gives two types one name; D-11 a protected inner spacesuit is filed S201; D-12 a hint names one of two
  files; D-14 a namespace inside a spacesuit is described as a field; C-13 `map = {}` is refused only at run; E-11
  `satellite.system.memory.this.used` gives "no value here", not S210. (not re-checked) (SPACESUITS.md "Broken" #5)

### Threads

- **HIGH: unlocked shared writes from two threads corrupt the heap** (ERRORS4 001): two threads writing one shared
  field (string, list or big number), or appending to one file, without `.lock()` end in exit 134 ("free(): double
  free detected in tcache 2"), 139 or a false S999, and the file is left empty -- 3 of 5 runs on r09 0054. No ruling
  lets satl crash: THREADS T2's "share it freely" covers lost updates. (not re-checked: thread stress tests are not
  run on this machine) (ERRORS4/001)
- **A partly refused warm-thread pool ends the run with exit 21**, though the comments say it carries on with the
  threads that started. This is MEMORY.md's "the exit-21 bug not fixed"; his call. (re-checked: the return after
  wait_until_warm is at structured-library.cpp:450-452; startup_threads.cpp:79 sets 21) (memory
  warm-threads-cores-x2-2026-10-03 "Found, NOT fixed")
- **Process-wide state** (THREADS T1): the statements and word_counts debug counters keep one table, so with threads
  their counts mix; satellite.directory.change moves the whole process, every thread with it. (not re-checked)
  (THREADS.md T1 "Known and not yet handled")
- **Every started thread stays in the process registry until satellite.return** (ERRORS4 034): a joined thread keeps
  its answer; an unjoined finished one keeps about 17 KB of stack. (not re-checked) (ERRORS4/034)
- **S725 THREAD_CANNOT_START and S726 THREAD_STOPPED have no test** and no check.sh row; nor does the locked-file
  sharing result (1,000 of 1,000 lines). (re-checked: neither code is in check.sh or tests/)
  (DOC_QUESTIONS_2026-10-09.md defects)

### Arguments and config.ini

- **An inline `# comment` on a float row in config.ini stops every program that uses a float**:
  `float.decimal = 5 # five` is refused "is not a precision", exit 20, because the value is everything after `=`.
  Suggested: strip a `#` after whitespace, outside quotes, in read_value, with a check row. (re-checked:
  config_file.hpp:127-140) (ERRORS3 #13)
- **`args.m.set(...)` on a map held in an arguments row is refused.** Told him on 10-06. (not re-checked) (memory
  arguments-item-value-2026-10-06, next-session-after-clear-2026-10-06)
- **The bare `arguments` is rebuilt on every use**: 0.58 s per 2,000 `.size()` calls, and a huge arguments.infinity
  slows infinities. Left as is on 10-06 and told him. (not re-checked) (memory arguments-item-value-2026-10-06)
- **`satellite.main` with two parameters**: the checker takes both for the arguments, only the first is handed them,
  so the second name is undeclared at run time. And `satellite.main(satellite.variable.number n)` is accepted, with n
  holding the arguments index. (not re-checked) (SATELLITE_ARGUMENTS.md Part 4D; SATELLITE_TEST.md MEDIUM and LOW)
- **Written rows, "KNOWN, AND LEFT AS THEY ARE"**: a row wins over a method read without brackets
  (`any_name.keys = "mine"`); `.remove` and `.clear` take satl's rows out of the variable's copy. (not re-checked)
  (SATELLITE_ARGUMENTS.md Part 4D)
- **`args[1] = satellite.bool.false` files a row under the key 1**, while `args.argument1` still says satl's path.
  (not re-checked) (SCRATCH.md/MISSING_SYNTAX.md MS-2/MS-3)
- **Stale or late argument reads**: `display(args)` shows a stale row after another capsule changed it by its bare
  name; a method straight after `satellite.library.arguments.<row>` is refused at run time; an argument row's
  `.number` refusal says "that argument.number" (ERRORS4 067); `arguments.length()` is read by the checker as a
  capsule call. (not re-checked) (SATELLITE_TEST.md LOW; ERRORS4/067; memory satellite-list-from-003-2026-09-23)
- **The satellite.history help page disagrees with satl**: it says reading `arguments.history` is S110 "there is no
  value here to work with"; satl says S201 "arguments.history is not one of the arguments", exit 25, though
  words.tsv:367 and :528 number it. (re-checked: help_text.txt:15 and :41 still say S110; satl not run)
  (DOC_QUESTIONS_2026-10-09.md defects)
- **"access = false written by hand is ignored silently" (ERRORS3 #15) could not be reproduced**: after
  `satl --rebuild` it prints S011 REGISTER_IS_STALE and reads false. The testers' exact steps are needed (a second
  access row? no config.ini? the prompt?). (not re-checked) (ERRORS3 #15)

### Files and paths

- **Three small file-word defects** kept so they are not lost: (1) append answers true when the line is held but the
  automatic 64 KiB write failed -- there is no "held, not on disk yet" answer; (2) open after a refused new opens that
  existing file, so a "new" handle sits on an old file; (3) a leftover `.<name>.saving.<pid>` from a crashed run with
  the same pid blocks full rewrites until removed. (The fourth, depth, is under the walker.) (not re-checked)
  (SATELLITE_FILE_OPERATIONS.md Part 8)
- **Two handles on one file disagree** (ERRORS4 006): lines appended through one handle stay in its 64 KiB buffer when
  a second handle or satellite.info reads the path, so the reader sees too few lines and writes land out of order.
  (not re-checked) (ERRORS4/006)
- **Two path rules**: satellite.file resolves a relative path against the program's folder (SATELLITE_FILE_OPERATIONS
  3.6), while satellite.info, directory.list and directory.change use it as written, against the cwd (the p12_shadow
  demo shows it); and `f.path` answers the absolute resolved path, not the path as written (ERRORS4 065).
  (not re-checked) (memory running-programs-talk-2026-10-01; ERRORS4/009, 065; SATELLITE_TEST.md LOW)
- **`satellite.directory.list()` and `list(d)` in a program are refused** S210 "satellite has no list type built yet
  to hold them (MILESTONES M14)", exit 14, though the list type has run since 2026-09-18. (re-checked:
  expression.cpp:1853-1855) (ERRORS4/040; MILESTONES.md M0.6)
- **Includes**: paths are not normalised -- `shapes` and `./shapes` are "two files", and a mutual `./` include grows
  the path (ERRORS4 020); a file named like a hex or binary literal (`x1`) cannot be included (026); and 003's ERROR
  #26, never checked on 004: a kept include must keep the canonical path it first resolved to, or a
  satellite.directory.change at the prompt changes which file a program includes (the prompt refuses
  satellite.include today, so it may be unreachable). (not re-checked) (ERRORS4/020, 026; ERROR.md section 4;
  PLAN.md "WITHOUT A MILESTONE" red note 2)
- **Two more 003 defects 004 must not port, never checked on 004** (ERROR.md section 4): a NUL byte in a file's text
  cuts errors::render output short (#25); S0734's caret sits on "satellite", not on the argument (#27).
  (not re-checked) (ERROR.md section 4; DOC_SURGERY.md)

### The prompt and the console

- **Ctrl-C tails**: at the prompt, a capsule that calls itself in the MIDDLE of its body is not stopped by Ctrl-C
  (suggested: read the flag between statements in run_statements); S810's paragraph says "satl exits 130", though at
  the prompt the session goes on. (re-checked: s_codes.hpp:341) (ERRORS3 #7)
- **`interpret <file>` against a file the prompt holds open**: when the child writes it, the prompt's copy goes stale,
  and a later non-append save from the prompt overwrites what the child wrote. (not re-checked) (memory
  escapes-sizes-prompt-memory-2026-09-24)
- **`satellite.help` in a program is S210**, though its page calls it a prompt word. (not re-checked)
  (SATELLITE_TEST.md LOW)
- **Console colour gaps**: a stack overflow on another thread, or Ctrl-Z, leaves the terminal colours set;
  `input(prompt, target)` and `typed()` are not built; a plain display costs 6.4% more instructions (callgrind
  3,685 to 3,922, about 80 of them the pipe scan). (not re-checked) (memory satellite-console-colour-2026-09-23;
  findings-from-the-help-writers #10)
- **console.input keeps a CRLF `\r` and a BOM** (ERRORS4 011). (not re-checked) (ERRORS4/011)
- **The 1,000-print race row still loses in satl's own console**, on frame pacing, not on the text's path; the guess
  that variant A's time is one 60 Hz frame was never verified. (not re-checked) (memory
  display-cost-is-word-family-checks; SCRATCH.md/FAST_PRINTING.md)
- **The printing ring's memory was never re-measured**: step 3 took 200 x 1 MiB strings from 26 MB to 440 MB peak
  (each slot keeps its value for 1,024 displays). He kept it (*"keep step 3"*); an older build (0113) re-timed it, but
  nobody measured the memory again. (not re-checked) (memory display-cost-is-word-family-checks)

### Window

- **`==` on two windows is probably refused**: identity covers lists, indexes, files, colours, and objects and
  pointers since 4a37fa9; windows were never checked. (not re-checked) (memory findings-from-the-help-writers #13)
- **The window race (satl vs tkinter) is undiagnosed**: on headless mutter each window after the first takes 3.5 ms
  or 11.5 ms, varying run to run (with WAYLAND_DEBUG every run is fast); there is no perf or bpftrace here. Not like
  for like: Tk waits for the window manager's map, satl's GTK never waits. His 27-line race checks nothing, so a
  crashed run still prints a time; checks outside his harness were offered, not built. (not re-checked) (memory
  window-race-2026-10-03, race-harness-stays-tiny)
- **His gtkcar/cxx_race.cpp swaps the two labels**: it prints satl's time as "pypy:" and pypy's as "satl:". His file:
  told him, left alone. (re-checked: lines 24-25) (memory window-open-built-2026-10-03)

### Build, install and licences

- **Five carried projects are missing from licenses/ and `satl --license`**: vte, lz4, simdutf, fmt and fast_float
  (fmt IS linked: 150 fmt::v symbols in ~/.satl/satl). (re-checked: none of the five has a folder in licenses/)
  (DOC_QUESTIONS_2026-10-09.md defects; GTK_AND_NO_DEPENDENCIES.md DEP-6)
- **A window-less satl still lists GTK among its licences**: the licence data is built from every
  licenses/*/license.txt, whatever the window setting. The no-window link fix's stubs were never applied either.
  (re-checked: make_support/040-sources.mk:176-179) (memory satellite-cpu-plan-and-fresh-clone-2026-09-23,
  no-window-link-fix-2026-10-08)
- **`make cpus` (M37)**: 055-cpus.mk folds btver1 into amdfam10 and c86-4g-m8 into c86-4g-m7, though they differ
  (`__SSSE3__`, `__SHSTK__`), so "54 distinct sets" is wrong (a reader re-derived 56; no build is wrong). build/cpu/ is
  stale: 53 folders, 4.1 GB, its haswell satl from 2026-09-23 (REVISION 08 BUILD 0024) beside 62 .so nothing reads.
  The fingerprint never covered the licences, the window data or the two generator scripts; the note that satl calls
  no intrinsic is stale (`__builtin_ia32_pause` since 2026-09-26); znver6 is never chosen (AVX512BMM). Keeping
  `make cpus` at all is his decision (CPU-PLAN CPU-1). (re-checked: 055-cpus.mk:19, :31-32; build/cpu sizes and
  dates) (MILESTONES.md M37; DOC_QUESTIONS_2026-10-09.md defects; CPU-PLAN.md)
- **The toolchain here (M28)**: g++ is GCC 17.0.0 (experimental), configured --with-arch=native, so a g++-built satl
  dies with an illegal instruction on an older CPU (clang builds the shipped binary); it also miscompiled a reserve +
  nested push_back loop at -O2 (the code was written around it). (re-checked: g++ -v; the miscompile not re-checked)
  (MILESTONES.md M28; ERROR.md section 6)

### Tests and harnesses

- **race.sh reads no exit status**, so a failing run still ends in a ratio (-1 ns counts as fastest). (re-checked:
  satellite/race/race.sh) (ERROR.md section 1 #9; DESIGN.md section 11)
- **The prototype runner** (ERROR.md section 1): a refused write names the wrong line -- std::cout buffers about
  4 KB, so it should say "at or before" (#4); in --debug a refused state line is never reported on early-return paths
  (#5). (not re-checked) (ERROR.md section 1; DESIGN.md section 11)
- **words/make_words.py needs 003's built satl** (old_versions/second_satellite/satl), or the word table cannot be
  regenerated (ERROR.md #24; #22 and #23 went with strings/ today). (re-checked: make_words.py:55-56)
  (MILESTONES.md M27; ERROR.md section 3)
- **The pty checks flake**: check_prompt.py fails a different check each run (wait_for's 0.05 s poll and one trailing
  pump(0.1)); under load above 70 its 20,000-line paste times out, and check_session.py ("if waits for its else") and
  the Ctrl-C-into-a-loop check fail too. Not regressions: repeat a check.sh failure before believing it.
  (not re-checked) (ERROR.md "FLAKY -- OPEN"; ERRORS2.md #15; memory m5-satellite-log-2026-09-25,
  random-built-2026-10-02 and three others)

### Running programs

- **Gaps from the 2026-10-01 check, never recorded as handled**: (1) a child inherits satl's 1.94 GiB stack limit and
  its ignored SIGPIPE (the spawn now uses clone with a stack hook, so this may be handled); (2) a pty per run breaks
  sudo's per-terminal credential cache, and SETPGROUP needs tcsetpgrp; (3) carriage-return progress output vs step
  6's whole lines; (4) program output under the 131,072-item S840 crash rule; (5) non-UTF-8 bytes. (not re-checked)
  (memory running-programs-talk-2026-10-01)

### Other

- **S210's paragraph and messages still speak of libraries**, though no word has been a library since 2026-10-07:
  "the library behind it does not exist yet" (s_codes.hpp:66) and "has no library built for it yet"
  (program_check.cpp:1917, info_calls.cpp:203, session.cpp:253). (re-checked) (DOC_QUESTIONS_2026-10-09.md defects)
- **satellite.legal**: a program cannot list it (NM-2a "NOT YET"), and `satellite.console.display(satellite.legal)` is
  refused S201 "legal has no satellite.variable line declaring it", naming "legal" alone. (not re-checked)
  (NEW_MILESTONES.md NM-2a; DOC_QUESTIONS_2026-10-09.md defects, probe p16)
- **Stale source comments**: structured-library.cpp:507 "THE CHECKER IS OWED" (the checker exists);
  startup_threads.hpp:13 "Nothing hands them work yet" (cascade_convert.cpp hands them every line); thread_stop.hpp:28
  "Set once, never cleared" (display_overrun_let_go() clears it); "DESIGN §5.6" at bytecode_registry.hpp:132 and
  bytecode_registry.cpp:417, :965 means 003's DESIGN; REGISTRY.satellite:66-68 says "nineteen" QUESTION tokens,
  while 23 rows carry QUESTION; check.sh:2857-2858 says both scroll rows are true, but arguments.scroll.horizontal is
  off unless written. (re-checked all but check.sh) (DOC_QUESTIONS_2026-10-09.md defects, lines 49-74 -- the editors'
  whole list)
- **His password program draws from satellite.random**, which is PCG (pcg32_k16384) seeded from the kernel -- not fit
  for passwords; a password generator must read the OS random source, and satl has no word that does. (re-checked:
  satellite_random/random_source.cpp; the satellite.random words in words.tsv:188-205, 475) (memory
  claude-code-accounts-2026-10-08)
- **satellite_toolkit's satellite_remote.py**: the keeper's short-socket fallback uses tempfile.gettempdir() with no
  length check, so a long remote TMPDIR gives "AF_UNIX path too long" (found 2026-10-09; not among the review fixes).
  (re-checked: satellite_remote.py:454 and satellite_toolkit_main.py:633) (memory
  revision-10-and-toolkit-main-2026-10-09)

### Low ERRORS4 tickets: small wrong sentences and late refusals (all still there on r09 0054; not re-checked)

- **Values and methods**: 048 find/add/to_string and others skip the receiver check and fail late; 049 the S210
  "so far whose" hint is incomplete (`.width` on a binary; `x.lock()` "a file's"); 053 the help still lists `!` as
  COMING though it works; 054 `.bin`/`.hex` on a float name are refused late; 055 a float's `.reverse()` is S301 at
  run, not S210; 056 string + infinity gets no `.string` hint; 057 a refused pair quotes converted text; 059
  `a[1] += 2` gets an unrelated S110; 064 a file word's text argument refuses a float; 066 display of a file is S210
  with no reason; 070 a fraction's slash sentence is wrong for floats. (not re-checked) (ERRORS4/048-070)
- **Parsing and carets**: 024 a colour variable spelled in hex letters with a method is refused on its declaring
  line; 025 `list<` then a new line then `number>` is not joined; 027 an else after a `}` that is not an if's passes
  the checker and is refused at run; 050 "did you mean" only at a statement's start; 051 "ERROR: expected 1%" is built
  from the first literal; 052 digits touching letters (`1_000`, `1e5`) are blamed on a fragment name; 058 a missing
  comma is called "never closed with }"; 068 a trailing operator before `)` puts the caret at column 0; 069 a `**`
  refusal names `^`; 080 `satellite.return(satellite)}` is refused S103 past the file's end. (not re-checked)
  (ERRORS4/024-080)
- **Containers and access**: 012 (changed) a map's `.reverse()` still answers keys -- what it should do is his; 013
  access drops the inner condition for a multiple in a multiple; 014 access shows strings unescaped (`\x09` forms satl
  refuses); 015 a see-through colour in a container shows ", 50" with no delimiter; 028 a map key type that can never
  be a key is accepted; 029 list `.max`/`.min`/`.sort` order bools while `<` and `>` refuse them; 060 `list<type>()`
  is refused at run; 061 a typed-container refusal drops `<type>`; 062 `.sum` refuses strings, contrary to its help;
  063 `truncate(-1)` says "items count from 1"; 075 (changed) a map position-method suggestion is itself refused; 076
  map refusals say "index"; 077 map position methods are refused late; 078 a multiple drops the inner reason; 079
  access position letters collide with the name (`m[n][m]`). (not re-checked) (ERRORS4/012-079)
- **Objects and threads**: 071 display of an object is refused late; 072 `counter.bump()` on a spacesuit's own name is
  called undeclared; 073 `things[1].v = 5` gets an unrelated S110; 074 `other.cents` inside the spacesuit gets a
  sentence that does not fit; 081 a run-time S240 names the header line, not the call; 083 `display(t.join())` of a
  thread that answers nothing is S210; 084 a self-join is S110, not S728; 085 S728's paragraph assumes a lock.
  (not re-checked) (ERRORS4/071-085)

### Probably settled -- check before acting

- **Window and console handlers dead after main (exit 64)**: fixed 2026-10-09 -- structured-library.cpp clears
  program_quit() before each window capsule; in the installed 0007. (memory window-handlers-dead-after-main)
- **The D-Bus portal hang in gtk_init_check**: fixed 2026-09-22 -- window_desk.cpp:96-108 turns the portal off before
  the display opens (re-checked). Other GTK programs still want `env -u DBUS_SESSION_BUS_ADDRESS`. (memory
  satl-press-a-button-script)
- **LD_RUN_PATH baking an RPATH**: handled -- 048-link.mk:16 links with `env -u LD_RUN_PATH` (re-checked). Still,
  call no local binary portable without `readelf -d`. (memory ld-run-path-bakes-rpath)
- **Two threads writing config.ini at once can mix it**: rewrite() now holds copy_lock() and config.ini.lock through
  read, change, write and rename, with a per-pid temporary (config_file.hpp:236-290, re-checked); never re-tested.
  (THREADS.md T1; memory satellite-threads-t1-2026-09-23)
- **satellite.help() listed 31 topics while 32 folders existed**: help.txt:71-75 now lists 32) satellite.random,
  33) satellite.time, 34) satellite.variable.time (re-checked); in build/satl, not installed. (DOC_QUESTIONS defects)
- **satellite.time() is S210 but satellite.time.now is S110**: the time words were built on 10-10 (time_calls.cpp),
  and section 2 already lists .now and .new as unbuilt; run both to see whether the two refusals still differ.
  (DOC_QUESTIONS_2026-10-09.md defects)
- **"Program root first" never applied to include**: ruled by Q19 (2026-10-09) -- include keeps what it does; only
  doc fixes remain. (DOC_QUESTIONS_2026-10-09.md defects and Q19)
- **ERRORS4 046, 047, 082**: fixed, verified on r09 0054; the ticket pages stay as they are (his *"f yes"*).
  (SCRATCH.md/DOC_SURGERY.md)
- **An unclosed `{` literal swallows the next statement**: an "unclosed {" fix was merged in 8923ad3 (2026-09-26);
  never verified. (memory containers-and-access-2026-09-26)
- **machine.cores answered 24 where gather() says 12**: b603f03 (2026-09-25) set arguments.cores to the physical core
  count; not re-checked. (memory satellite-arguments-variable, error-sweep-2026-09-25)
- **003's memory.this.used(unit) measured the thread stack**: 004 numbers satellite.system.memory.this
  (words.tsv:340-343) but has not built it; check what it measures when it is built. (memory satellite-m26-progress)
- **Two implementations of "a runnable file"**: file_can_run() alone decides; check_satl, compile_satl and run_calls in
  satl/satl_file.cpp are called by nothing (dead code; removing it was not asked for). (memory
  satellite-16-bit-open-questions; PROGRESS.md:227, 668-673)
- **Other mutating container rows still copy**: that was 003's code; 004 copies nothing while the chain is on the
  variable (expression.cpp:379-381), but 004's insert, remove and reverse were never measured. (memory
  satl-list-append-is-quadratic)
- **console_test has no make alias**: 003's; 004's 065-tests.mk has none. The lesson stands: after `make <x>_test`,
  check the binary's mtime. (memory make-alias-silent-green)
- **ERROR.md section 1 #6, #10, #11, #12, #17, and race.cpp loading build/satellite-numbers from the cwd**: all about
  per-word .so files, probably moot since every word was built into satl (2026-10-07); re-check before closing.
  (ERROR.md section 1)

## 6. Plans not built, and work owed

Merged from the 16:47 sweep (plan and owed-work items), checked read-only against the tree on the evening of
2026-10-10. Sections 0-3 above are not repeated here. Each bullet starts with whose it is: **His plan** = his design
or ruling (his words in italics where he wrote any) · **Plan** = written in a plan doc, no words of his recorded ·
**Owed** = work I said I would do, or that his rulings call for · **Offered** = I offered, he has not said yes · **His
to run** = only he can do it.

### Every built-in under satellite

NM-8 in NEW_MILESTONES.md. Section 1 already lists the types still to number and the spelling questions A-G; these are
the pieces it leaves out.

- **Owed.** The number step: M17 was ruled on 10-09 (*"looks good"*): finish it, do not delete. But
  `bytecode/number_methods.cpp` is in no make file; `clamp` needs 1 6 4 5 and `bytes` a number (next free 1 6 4 21).
  NM-8 now gives each method its own numbered file, so check how this fits before building. His words: *"so .power()
  becomes something you can call on that object"*. (MILESTONES M17; PROGRESS 6.7-6.8; DOC_QUESTIONS Q7/Q24; checked
  10-10)
- **Owed ("for now").** Take out the fixed limit of 16 numbers a word may have (`kMaxDepth`, number_row.hpp:22). The
  16-bit key has its own limit of 7 (word_codes.hpp:35); `word_number_token`, the way past it, is written by nothing.
  (DESIGN 3.3; DOC_QUESTIONS Q10)
- **Owed.** What M21's "drop" left behind: `strings/` is still there (the 32-bit string class, `string_cases`,
  `check_strings.py`, `string_method.hpp`; make test still builds it), and the constructors 1.6.1.0 and 1.6.1.18 still
  take the 32-bit string. His rule: *"If your gonna delete an entire file, ask me first"*. (MILESTONES M21;
  DOC_QUESTIONS Q16)
- **Owed.** Doc leftovers of the same: MILESTONES M27 and ERROR.md §3 still list #22/#23 (about the two string files
  deleted today); M21 still says "Nothing is deleted yet"; the comment at directory_words.hpp:35-39. (MILESTONES M21,
  M27)
- **Owed.** Numbered words that are not built (M25): satellite.network (1 20) and satellite.variable.network (1 6 12);
  tests/not_understood.satl goes red on purpose when network is built (move it, never delete it). Also the variant's
  asking half (M14), console.input/.typed (1 5 4, 1 5 5), directory current()/exists(d), satellite.help inside a
  program, satellite.system.delete(x) (1 22 1), satellite.variable.string(x) for other kinds, and `list()`/`list(d)`
  in a program (S210 naming M14, though lists exist). (MILESTONES M25, M20; PROGRESS "Numbered, and not built yet";
  NEW_MILESTONES 3.10)
- **Owed (gap).** A string method written as its full word, like `satellite.variable.string.add("x")`, is refused when
  it runs (S210) -- all 24 numbered string methods; older than NM-8. (memory
  satellite-prefix-for-builtins-2026-10-10.md)
- The operators' method words (`.and()` `.or()` `.not()` `.shift_left(n)` `.shift_right(n)`, `.xor()`) are under
  "Operators and binary" below.

### The 2026-10-09 rulings still waiting for their work

His answer to the editors' list was *"looks good"* (SCRATCH.md/DOC_QUESTIONS_2026-10-09.md, "Work the rulings call
for"). Checked on disk the evening of 10-10: none of these is done.

- **Owed.** Q3: licences for VTE, lz4, simdutf, fmt and fast_float -- licenses/<name>/license.txt from each tarball in
  vendor/new/, regenerate THIRD-PARTY-NOTICES.txt, add them to licenses/README.md and DEP-6; README's "27 licences ...
  26 other projects" becomes 32 and 31. (README Q3; DOC_QUESTIONS Q3; GTK_AND_NO_DEPENDENCIES DEP-6)
- **Owed.** Q8: mark REGISTRY.satellite's eight operator rows (263-271, not the `~` home row) ANSWERED with his words,
  regenerate token_codes.hpp, fix the header's "nineteen" (67-70), give the method-token rows a header of their own.
  (PROGRESS 6; MILESTONES M19, M24; DOC_QUESTIONS Q8)
- **Owed.** Q8: the refusal at program_walk.cpp:87 still says "& | << >> !! are undecided" -- change it and the five
  places that quote it (binary_and_hexadecimal help 39-41, manifest.tsv:60, test_programs.cpp:2725/2736,
  full_numbers_25.satl:5). (DOC_QUESTIONS Q8; MILESTONES M19; ERRORS4/039)
- **Owed.** Q9: text still says scroll.horizontal is on unless written (it is off): check.sh:2983 and its comment, the
  window help (lines 60-63); also check the console help 76-81, GTK-17, SATELLITE_ARGUMENTS 4G, README's revision 10
  paragraph. (PROGRESS section 1; DOC_QUESTIONS Q9)
- **Owed.** Q11: standard 2's rows -- the 27-nines default and maxima for a whole number's digits, nesting and command
  length (config row, help line, SATELLITE_ARGUMENTS row each); and make something read arguments.object_bytes_max and
  arguments.file_size_max_bytes (nothing does). (DESIGN 1, 4; DOC_QUESTIONS Q11)
- **Owed.** Q18: take out arguments.magic (M33 dropped). FIRST copy his words from satellite_config.hpp:59-60 into a
  document -- *"the magic is step, so it just jumps at that step, preparing every 5 lines by default"* -- then the
  row, its help lines and check.sh's sample rows that use it. (MILESTONES M33, M34; DOC_QUESTIONS Q18)
- **Owed.** Q22: satellite.help's own page (line 33) still says access "needs a register of every name in use"; he
  ruled access reads the name where it is now. Reword that clause. (MILESTONES M34 Part 4; DOC_QUESTIONS Q22)
- **Owed.** Q25: a line naming a satellite word that does not exist (`satellite.history`, `satellite.cxx()`) must say
  it is not built yet; today it gets "satellite is ... immutable" (S110). New refusal + check.sh rows; keep that
  sentence for S202 and satellite.delete(satellite); fix satellite.help/satellite.history lines 8-10. (NEW_MILESTONES
  3.9; DOC_QUESTIONS Q25)

### NEW_MILESTONES: NM-1 to NM-6 (his; each built only on his word)

Built: NM-2, NM-2a, NM-7, and NM-8 begun. His questions 5.1-5.14 (NEW_MILESTONES section 5) come first. Which comes
next is his to pick (on 10-06 it was between NM-2b and NM-1). (NEW_MILESTONES; memory
next-session-after-clear-2026-10-06.md)

- **His plan.** NM-1, receive a file and run it in this process, staying ready: *"making sure that the interpreter is
  already running for when it receives a file that has satellite.include(satellite), satellite.capsule
  satellite.main(), and satellite.return(satellite)"*. (NM-1; questions 5.1-5.4)
- **His plan.** NM-2b, boxes by number -- a name read by its box, not searched by spelling: *"a design that would
  perhaps lead us to way faster, more efficient software!"* Same idea as CPU-PLAN's CPU-6 (names to slots). Timed
  against the build before; slower means stop. (NM-2b; CPU-PLAN CPU-6)
- **His plan.** NM-3, *"satellite.access(filename) will give a list of all things for a file that has been ran, the
  last state of each variable..."*, kept *"up to 20%"*; needs NM-1 and NM-2. The same last-known store as
  SATELLITE_ERROR section F: *"for satellite.history and for satellite.access we keep the LAST KNOWN variable's name
  type and value, under any circumstances"*. (NM-3; SATELLITE_ARGUMENTS 4B; SATELLITE_ERROR 7.3, R11, R20; questions
  5.5-5.10)
- **His plan.** NM-4: *"BUT I just realized that satellite.delete is going to delete FROM satellite.access, but we
  will still build satellite.delete first, then build satellite.delete for satellite.access"*. (NM-4; question 5.11)
- **His plan.** NM-5 (near the end): *"I want the interpreter to be already running if the console window is open"*.
  Today `run`/`interpret <file>` start a child satl, so every name dies with it. (NM-5; questions 5.12-5.13)
- **His plan.** NM-6 (LAST): *"satellite.history needs to be disabled by default, however if it is enabled then it is
  ready-to-run on any file that is ran on that interpreter, however we will not build satellite.history part right
  now"*. His older M20.B/M20.C words stay in MILESTONES. (NM-6; MILESTONES M20.B, M20.C; question 5.14)

### MISSING_SYNTAX: MS-4 to MS-30 (his; strictly in order; MS-4 has waited on his word since 10-04)

- **His plan.** MS-4: *"When the window is declared, I don't think the window should open, and the interpreter, if the
  file is ran without .open but has .window.new() it should report you forgot open"*. `.open()` exists; `new` still
  shows the window. Every program relying on that gains `.open()` (his window_and_extras.satl is his to change); prove
  it on a compositor. (MISSING_SYNTAX D1, MS-4; SATELLITE_WINDOW 2c; memory window-open-built-2026-10-03.md)
- **His plan.** MS-5 to MS-17, a code for each missing piece (blank file S030, corrected lines for S101/S150, S031
  missing return -- NEW_MILESTONES 5.3 waits on it -- S151-S156, S032, S033/S034, never-started threads): *"the
  missing syntax should apply to every single syntax element, capsule/object/variable"*. (MISSING_SYNTAX 8 Part B)
- **His plan.** MS-18 to MS-20, the prompt inside the window (`my_window.display(...)`, `my_window.input("> ")`):
  *"the prompt reads a line from the console built into the new window, almost everything is not built just yet"*.
  (Part C)
- **His plan.** MS-21 to MS-29, the tutorial (on by default) and its program, with file copy/rename/delete: *"copy a
  file, delete a file, rename a file, create a new file (we already have that) but we don't have a error command built
  for it"*. (Part D)
- **His plan.** MS-30, last: *"parsing input beyond "--help" is the final milestone of MISSING_SYNTAX.md"*. (MS-30,
  D14)

### Operators and binary

- **His plan (owed since 09-22).** `&& ||` for conditions, `& | ~ << >>` for bits, `.and() .or() .not() .shift_left(n)
  .shift_right(n)` (1 6 4 1 / 1 6 4 11 numbered), exclusive or only as `.xor()` (`^` is power): *"let's use c symbols
  as much as we can for this then, and also allow .and() .or() like we allow .power()"*. Only `!` runs; QUAD hit the
  missing `&&` on 10-03. (MILESTONES M19; DOC_QUESTIONS Q8; ERRORS3 #10; memory satellite-operator-spellings.md)
- **Owed with it.** The choices he never made (a bit operator ignores the sign; widths) get built as reversible
  defaults, reported as such; a bit question is still open in the binary_and_hexadecimal help. (memory
  satellite-16-bit-open-questions.md)
- **Plan, his to schedule.** 003 DESIGN 8.5's binary rulings: + on bit runs answers a bit run, the width grows to fit,
  the left operand's type wins, `!!` joins, indexing from the right. Today `b0101 + b0011` shows 8. (MILESTONES M20)

### Infinity, floats and the .satl track (SATELLITE_INFINITY.md Part 13)

- **Plan.** INF-3, next whenever the GUI is not the priority: + and - in the infinity family, * and / by numbers and
  percentages (refused by name today). When it lands, `infinity * -1` must equal `-infinity` (his Q23 ruling, 10-09).
  (SATELLITE_INFINITY INF-3, Q20; MILESTONES M11; DOC_QUESTIONS Q23)
- **Plan.** INF-4 to INF-9: products, powers, dividing by an infinity; infinite exponents and walks; the family words;
  .resize/.nines; the infinitely small (needs Q34); INF-9's counter and warning (nothing reads
  arguments.infinity.counter).
- **Plan.** FLT-3 (`inf * 0.5` is `(0.5 infinity)`, needs INF-3) and FLT-4 (`12.34.reverse()` is 34.12; S301 today);
  the float's fixed-width question (Q18) is open. SAT-1 to SAT-6, the .satl track, wait on Q12.

### Files (SATELLITE_FILE_OPERATIONS.md, Part 8 order)

- **Owed -- copy these out first.** The 10-07 file work never landed: its patches exist ONLY in
  /tmp/claude-1000/-home-madness-code-cxx-satellite/e99fe5f0-97ca-4af1-97db-ab6b131bb528/scratchpad/patches/ (job_a:
  FO-5, FO-8, FO-9; job_b: file race + hostile run; job_c: help pages), the 29 GB scratch once proposed for deleting.
  They need rebasing. His A-E were all yes (E: install when built and checked). (memory file-operations-2026-10-07.md)
- **His plan.** *"this is the SATELLITE programming language, and I wanted to finish SATELLITE_FILE_OPERATIONS.md"*.
  FO-6 first: the file race and hostile run -- no lock between runs, so two runs of a self-editing program can both
  read 7 and both write 8. (FO-6, Part 8; MILESTONES M15)
- **Plan.** FO-5 read-only open (word 1 8 7; the storyline generator's lock needs it); FO-8 binary files (needs R6);
  FO-9 no size limit on a file being changed (10 GB, memory not growing); FO-10 index_of/search given a list. Phase 2
  (byte values, lists, .write/.position/.read, three .replace forms) never started. (FO-5..FO-10)
- **Owed.** FO-7: help pages for the file words -- satellite.help has no satellite.file or satellite.variable.file
  topic (checked 10-10). SATELLITE_FILE_OPERATIONS.md is still its 09-18 text. (FO-7)

### Windows, the console, GTK and dependencies

- **His plan.** Console links: *"let's have it be my_console.link("greet", greet) we can just do colors and shadows of
  links using the same way we do it now"*. Feasible (VTE OSC 8); exit 64 fixed in BUILD 0007. Needs a new plan doc on
  his go; explain by scenario that a link names a CAPSULE, never text to run. (memory
  console-links-idea-2026-10-05.md)
- **Plan, needs a design talk.** Console tabs: "New tab" is greyed in the right-click menu. Program consoles
  (satellite.console.new) got none of the copy/paste work. (memory next-session-after-clear-2026-10-09.md)
- **Owed.** M36: console.input(prompt, target) and console.typed() refused S210; regions, line=/column= and the
  alternate screen never built; Ctrl-Z or an overflow on another thread leaves the terminal's colours changed (not
  run). (MILESTONES M36)
- **Owed.** His 10-06 opening note names things not built: `save`, `.sate` (dropped 10-03), some help topics,
  clickable red text (the links idea). (PROGRESS 2026-10-06)
- **His plan.** GTK-18, `satellite.terminal.new` (1 29 3): *"satellite.terminal to be a bash prompt"* -- VTE spawns a
  shell in the console; it hands a person a shell with the program's privileges. Only .foreground/.background exist.
  (GTK_AND_NO_DEPENDENCIES GTK-18; memory satellite-project-state.md)
- **His plan.** WIN-2: *"load as much as we can if that is possible without actually drawing the window"* -- GTK
  warmed on another thread before a window word is reached. Not built. (SATELLITE_WINDOW WIN-2)
- **Plan.** WIN-5: no cairo software fallback for a machine with no GL driver. WIN-10/DEP-9: Wayland only; X11 is a
  separate vendoring decision. GTK-11: save-file and choose-folder dialogs. (SATELLITE_WINDOW;
  GTK_AND_NO_DEPENDENCIES)
- **Owed.** WIN-1: where the spill goes, and whether it is removed at exit, never confirmed in code; the "no font is
  FATAL" crash never re-measured as asked. (SATELLITE_WINDOW WIN-1; DOC_SURGERY)
- **Plan.** DEP-8: the distribute package for a ~68 MB binary, shipped as .xz. (GTK DEP-8; memory
  satellite-project-state.md)
- **Owed.** libtiff (reached by satellite.window.picture) never proved with a .tif; pcre2, libgirepository and
  libcairo-script-interpreter are vendored and called by nobody. (GTK_AND_NO_DEPENDENCIES Part 00 Table B)

### Arguments (SATELLITE_ARGUMENTS.md)

- **His plan.** B4-B6, eighteen memory-unit spellings: *"we need all aliases to be "kilobyte" without the S so if we
  accidentally type in "kilobyte" or "megabyte" or "byte" it accepts it"*. (4B, 4C Phase B)
- **Plan.** B8 swap total/free/used; B12-B15 (memory.system.used, directory.history, system.stack,
  memory.system.reserved) and D4-D6, satl's own memory -- still refused (S110/S201). His words: *"this will be an
  entire 20 minute block just to build this into the interpreter, it will be alot of work"*. (4C Phases C-D;
  MILESTONES M34 Part 3 item 6)
- **His plan, milestoned last.** B16-B21, `satellite.variable.memory`, a literal like `1gb`: *"I know it's feature
  creep, but it's just too good of an idea to not use you know?"* (4B, Phase E, red note 8)
- **His plan, recorded only.** Red note 4, one master over every interpreter: *"split the interpreter someday into a
  global system wide machine"*, *"it's like, an anti-lock"*. (Part 5 red note 4)

### Errors, reports, debugging and help

- **His plan.** The full report R1-R20 (--report, ~/.satl/reports/, the frame stack, the last-known store, satl
  --report, satellite.report()): *"we can use this as a debug tool anyways, so it will facilitate development"*.
  Nothing exists yet. (SATELLITE_ERROR Parts 7-8)
- **Plan.** E14-E16 (lines around the failure; what 003 did; `satl --help <S-code>`), F6-F8 (feature register), G2-G6
  (coverage, watchpoints, timing per capsule, satellite.assert), H1-H9 -- H1 first: a source file changed since load
  prints the wrong line. (SATELLITE_ERROR Parts 5, 11, 12)
- **Owed.** "At or before line N": a refused write names a line thousands of bytes after the one that failed. (DESIGN
  8, 11)
- **Plan.** CONTAINERS step 3, refusals that show the right way into a declared container. His words: *"we can assume
  the declaration of the container is correct"*. Not started. (SCRATCH.md/CONTAINERS.md; memory
  containers-and-access-2026-09-26.md)
- **His plan.** M34 Part 4: *"we can combine them so access also displays whatever help has for that type"*; and *"so
  anytime we add a type, then that type's methods need to be registered within some directory structure"* -- help is
  still a second, hand-written list; satellite.help inside a program is S210. (MILESTONES M34 Part 4)
- **Owed.** M18: re-read ERROR.md's recursion-depth entry now that `if` exists. (MILESTONES M18)
- **His plan, queued.** ERRORS4: 85 pages of bugs (78 still there on build 0054), untouched: *"there's still... 200
  errors to fix or something like that, when this is done"*. Run each page's program on today's satl first.
  (SCRATCH.md/ERRORS4; memory which-milestone-2026-09-30.md, doc-surgery-2026-10-07.md)

### Threads

- **Plan.** THREADS T3: a window used from a program's thread (S727 refuses it) -- a design of its own. (THREADS.md
  T3)
- **Plan.** satellite.library is not writable at run time (S250), so nothing there can be shared or locked.
  (THREADS.md T2)
- **Owed.** S725 and S726 have no fixture or check.sh row; the two-thread locked file (1,000 of 1,000 lines) is in no
  test. (MILESTONES M38, M39)

### Speed

- **His plan.** Warm threads that get work -- combine marks batches, a parked thread takes one, M12, M30's lookahead:
  *"As the interpreter runs, it continues to look for spots where it can unmark things, where it can turn code into
  batches"*. Today the 24 threads convert lines and park. (MILESTONES M12, M30; PLAN M2, M7; DESIGN 13)
- **His plan.** M35 "Optimizing The Interpreter": *"we are going to rebuild the interpreter as we rebuilt the features
  to be a switch-hierarchy of everything with pathways, so that the entire interpreter is a switch-hierarchy of
  different cases and different switches"*. Only the feature register is built that way. (MILESTONES M35)
- **Owed.** On 09-23 satl was behind CPython on all 11 time_test programs: name search 28.6%, value copies 27%. He no
  longer trusts my timings -- give him the commands. (CPU-PLAN CPU-6; SATELLITE_ERROR F5b; memory
  satellite-threads-t1-2026-09-23.md)
- **Owed.** String `==` still goes through `char_traits<char16_t>::compare` (satellite_string.cpp:283); memcmp was
  8.5x faster on a 29,696-character string. A one-line change. (CPU-PLAN "Strings")
- **Owed.** String throughput at corpus scale (QUAD's real work) has never been measured. Cold and isolated; give him
  the command. (memory satellite-target-is-quad-ai.md)
- **His plan.** FAST_PRINTING retune, *"we can retune it when we're done"*: a 1 MiB value kept until its slot is
  reused (200 x 1 MiB peaks at 440 MB); +4-8% on short lines. The queue was the retune, then ERRORS4. (FAST_PRINTING
  item 3)
- **Owed.** FAST_PRINTING step 5 leftovers: satellite.console.new still writes its pty; Ctrl-C does not let go of what
  already waits on the desk. (FAST_PRINTING step 5)
- **His plan, someday.** SECRET.md's bypass and the mini interpreter: *"someday we can do the whole 3.5 ns thing"*.
  (memory display-cost-is-word-family-checks.md)
- **His plan.** A cycle collector as its own milestone (his 09-12 decision; trial deletion, no stop-the-world). The
  leak is by design until then -- his own comment in his program: *"It is a cycle and never freed"*. The help page
  does not mention it. (memory satellite-m26-decisions.md, satellite-has-pointers-already.md; DESIGN 1.1, 12;
  SPACESUITS.md)
- **Owed.** Spacesuits: a per-call-site cache (a method call 1.01 us vs 0.71 us), 003's D19 Burial queue, and the S240
  caret pointing at the callee's header. (memory spacesuits-built-2026-09-22.md)
- **Owed.** M37: znver6 is never chosen; Intel SDE not installed; measure the builds cold before adding more.
  CPU-PLAN: a check.sh row for the Core 2 instruction floor, and a PROGRESS row for CPU-7. (memory
  satellite-arguments-and-cpus-2026-09-23.md; CPU-PLAN)

### The language itself

- **Plan.** M10, `satellite.cxx() { C++ }` -- not started (no words.tsv row). (MILESTONES M10; PLAN M10; POLYMORPH/M6)
- **His plan.** M13, *"find and add at location of find"*: today `w.find("world").add("!")` answers `6!` and leaves w
  alone. Open: does a position survive a conversion? (MILESTONES M13)
- **His plan.** M20.G finally for if, while and for (his number to mint) -- *"switch will also have
  satellite.statement.finally(optional_condition) if no condition is given, then the condition is true..."* -- and
  M20.H switch. (MILESTONES M20.G, M20.H; PLAN red note 1)
- **Plan.** The other M35: a spacesuit takes a built type as its supertype; satl's refusal names it. (MILESTONES M35;
  SAT-1)
- **Plan.** LANGUAGE_IN_SATELLITE, after L1: L3 switch, L4 numbers out of text, L7 calc.satl, L8 tiny.satl, L10 speed
  against C++ and Python, L11 satellite in satellite. (SCRATCH.md/LANGUAGE_IN_SATELLITE.md; memory
  error-sweep-2026-09-25.md)
- **Plan.** Kinds satl called not built on 10-03: variant, date, duration, network, expression, capsule; "info" too,
  though satellite.variable.info is built -- check which word it means. (memory window-race-2026-10-03.md)
- **Plan (decided 09-16), not built.** The 8-bit string, ASCII only. (PROGRESS sections 2 and 5)
- **Plan.** A spacesuit joined to a string is refused, since DESIGN §12 defers a printable to_string (003's rule; 004
  not checked). (memory satellite-conversion-set.md)
- **Plan.** The converter from registry codes back to .satl: row_as_bits() exists, nothing calls it. (MILESTONES
  M1.5-M3.6; PLAN M1.5)
- **Waits for him to ask.** satellite.directory.current() (1 18 2) and .exists(d) (1 18 3). (MILESTONES M0.6; PLAN
  D0.6.6)
- **Owed.** M23 #1: `satellite_number(unsigned long long, bool)` is still not `explicit` (satellite_number.hpp:46).
  (memory which-milestone-2026-09-30.md)
- **His order.** After the file work: advanced tutorials for the website. EXTENSIONS.md (extending built-in types) is
  parked with 8 open questions. (memory file-model-2026-09-24.md)

### Reviews, tests and measurements

- **Owed.** Nobody has tried to break satellite_number and satellite_string (the four reviewers died on a session
  limit); ERROR.md §5 still says "not yet reviewed". INF-2 was never reviewed either. (PROGRESS 5; ERROR.md 5;
  SATELLITE_WINDOW Part 3)
- **Owed.** One fresh review of the 09-26 merge 8923ad3 (M16 + errors-2026-09-26, both ending "UNFINISHED"), skipped
  for tokens; also check M16 warns where a number is taken as text. (memory
  everything-on-github-for-cloud-2026-09-26.md, containers-and-access-2026-09-26.md; FAST_PRINTING "Also owed")
- **Owed, maybe.** A third review of the lock (8f8804e): a third reader was still running when it landed; THREADS.md
  records none. (memory satellite-threads-t1-2026-09-23.md)
- **Owed.** satellite.test's loss chain: 178 of 2,436 changed copies still print 547311173, unsorted (col_worth_negate
  and X_map_op27_minus_ulp look real; the 16 official loss tests may share them). Stopped on his word.
  (SCRATCH.md/SATELLITE_TEST.md; memory next-session-after-clear-2026-10-06.md)
- **Owed.** Prove `.every`/`.key`/`.clicked`/`.closed` on a reopened window (blocked by exit 64, fixed 10-09; never
  run). `==` on two windows never tested. (memory window-open-built-2026-10-03.md; ERRORS2 #14; NEW_ERROR_LIST B6)
- **Owed.** Security rows (DESIGN 9): machine-code bytes as a name, string or number; a very long current directory;
  an empty name. (DESIGN 9)
- **Owed.** The directory listing's 100,000-entry race against C++ readdir was never run; it sorts by bytes, not 003's
  character table. (MILESTONES M0.6; PLAN M0.6; PROGRESS section 1)

### Documents and the website

- **Owed.** The document update: about 38 docs left, in batches (an editor on a scratch copy, then one fresh checker;
  snapshot, copy in, patch). Check SATELLITE_FILE_OPERATIONS.md and ERRORS4/README.md first (no verdict yet). His
  rulings: old text to history/<same path>; INCOMPLETE.md stays; ERRORS4/ untouched. (memory
  doc-update-plan-2026-10-09.md; DOC_SURGERY)
- **Owed.** DOC_QUESTIONS lists about 30 places in other docs that still hold questions he has ruled; its own first
  line ("Nothing here has been decided") is stale. DOC_SURGERY's verdicts say a plain make installs nothing (wrong).
- **Owed, publish only on his go.** satellite.foundation is stale since 09-26 (54 of 327 examples failing on 10-07;
  post-strings says M16 is refused); nothing built since is on it. Follow SCRATCH.md/PUBLISH.md; he edits on the site,
  so compare first. He asked for a gap between a code box and its output. (PUBLISH.md; memory
  satellite-foundation-website.md)
- **Offered.** Self-pingbacks (a guess about the 09-24 outage): close pings on my 34 posts, and a no-self-pings plugin
  for him to upload. (memory satellite-foundation-website.md)

### Machines, GitHub and other projects

- **His to run.** GitHub main is c55de31 (no vendor/pcg-cpp), so a fresh clone cannot build. The upload copy is in
  /tmp/claude-1000/-home-madness-code-cxx-satellite/9a28c15f-efaf-4507-992f-43e5b9cabcac/scratchpad/upload/satellite
  (/tmp can vanish): re-sync it, scan for secrets, then he pushes from the copy, no --force. His words: *"but don't
  put old_versions, take out the folder old_versions"*. (memory github-deploy-key-2026-10-09.md)
- **Owed, after his push.** Mom's machine: rerun the toolkit's `all`; only step 9 (satellite) is left. The command is
  in memory next-session-after-clear-2026-10-09-late.md.
- **His to run.** Rotate the passwords of my login on mom's machine and the Dell (my rotate was refused). The PUBLIC
  toolkit repo still holds her machine's details (no password): rewrite its history, and put its machines and
  test-runs folders in .gitignore. (memory
  claude-code-accounts-2026-10-08.md, revision-10-and-toolkit-main-2026-10-09.md)
- **Owed.** satellite_backup cannot reach mom's machine: brackets for an IPv6 host, the password satellite_ssh keeps,
  an ssh `terminal`. The Dell has never run the toolkit chain (his 10-08 rule), and may lack disk for clang. (memory
  claude-code-accounts-2026-10-08.md, satellite-toolkit-2026-10-08.md)
- **Owed.** Tell him this workstation's ~/opt/pypy3 has the SSL crash the toolkit fixed on 10-09. (memory
  next-session-after-clear-2026-10-09-late.md)
- **Plan.** QUAD: still missing bzip2, zstd, JSON-string decoding and tar readers (QUAD/MISSING.md; keep its row
  numbers); next steps never started (the Common Crawl downloader, word vectors, cutting the Wikipedia dumps).
  (DOC_SURGERY; memory quad-test-2026-10-03.md)
- **Offered.** Commit the middle-band device to the scraper's own git (ask first; he said no to the two filters).
  llama.cpp's Vulkan build needs his sudo for the packages. (memory quad-test-2026-10-03.md,
  llama-cpp-on-the-w6600-2026-10-03.md)

### Housekeeping

- **Owed.** The 16:47 snapshot (backups/2026-10-10-before-open-items-docs/) has no patch yet -- write it when this
  write-down is finished. (backups folder, checked 10-10)
- **Offered.** Cut the 91 probe entries two agents put in his ~/.satl/satellite.log on 10-01 -- check for his newer
  entries and back up first. (memory running-programs-talk-2026-10-01.md)
- **Offered.** Delete the old .sate files (6 in ~/code/satl, 3,845 in the repo, 2,251 elsewhere); none has been
  written or read since build 0146. Back up first. (memory sate-dropped-2026-10-03.md)
- **His call, no ruling.** satellite/satl/satl_file.cpp's check_satl, compile_satl and run_calls are called by
  nothing, and check_satl decides "runnable" by the same three codes as file_can_run. Delete or keep? (PROGRESS 6.5;
  PLAN)

### Probably settled -- check before acting

- Vendor every dependency (09-20): vendor/new holds the release tarballs, built with nofallback; build/satl still has
  7 NEEDED libraries (libstdc++, libgcc_s, libwayland-client/-egl, libc, libm, ld-linux).
- DEP-2 (words link into satl): every word is compiled in since 10-07; NEEDED is still 7, not 6.
- DEP-7 (old zlib 1.2.11): vendor/new has zlib 1.3.2, libpng 1.6.58, freetype 2.14.3; SATELLITE_WINDOW Part 3 still
  says not done.
- libxkbcommon's checksum: vendor/new has a SHA256SUMS line; whether it was checked against a distro package is
  unrecorded.
- `.pointer()` as a weak word: built 10-02 (builds 0143-0145).
- Nested globals (satellite.library.a.b): 004's tests already use that shape.
- check.sh 28/32 on purpose: done 09-16 (PROGRESS 6.6).
- The distribute package's .tar.gz: 003's writes .tar.xz now; 004 has no distribute/ (DEP-8 above is what is left).
- Porting off Linux: its premise (satl-term) is gone; no Windows/macOS plan exists (see WIN-10).
- Satellite Orbit's result type: built 08-25; Orbit never came into 003 or 004.
- The selector one hop, ACCESS_PLAN A2/A4, bool to_string: all 003; 004 resolves a method by the value's kind (not
  run).
- PLAN M4's race (`i = i + 1` vs a C++ long long): satellite.test.speed has a C++ while-loop twin (`speed_while`).
- .builtin random: shelved at his word; the folder deleted 10-06 (copy in backups/2026-10-06-builtin-folder/).
- B11 arguments.directory: words_004.tsv has its rows (1 14 1 1 12, 1 14 3 12).
- D1-D9: D1-D3 built (B7), D7-D9 answered by arguments.threads; D4-D6 are in the Arguments bullet above.
- The CPU kernel plan CPU-1..CPU-5: he scrapped it on 09-23 (CPU-PLAN.md header).
- FAST_PRINTING step 6: built with RUNNING_PROGRAMS on 10-01; that file's START HERE is stale.
- A C++ race that includes the compile step: on 09-26 he said *"we gave up on racing compiled C++"*.
- RUNNING_PROGRAMS.md's open questions: its top now holds his rulings and its not-built list.
- Includes typed at the prompt: blocks, capsules and spacesuits came 09-25; includes are not mentioned since.
- Help topics briefed wrong on 09-22: 044534e landed all 27 that day; there are 35 now.
- Replace 003 with 004: done in practice (004 installs to ~/.satl; 003 kept in ~/.satl-003); his view is not recorded.

## 7. Doc fixes owed, and state still to act on

Merged from the 16:47 sweep (its doc-fix and state items, 86 read).
- Every doc fix was checked read-only against the tree on 2026-10-10 at about 19:10. Line numbers are as of then.
- State on this machine was checked the same way. Mom's machine, GitHub and the toolkit repo are as the notes left them.

### 7.1 Doc fixes -- one doc at a time

- **PROGRESS.md**
  - Header (lines 3-12) says "Checked against satl BUILD 0058", "BUILD 0001, built and installed", and that section 1
    "runs to 2026-10-07". What is true: build/satl is BUILD 0017, not installed; ~/.satl/satl is BUILD 0007; section 1 runs to 2026-10-10.
  - Lines 17-18 say "NEW_MILESTONES.md ... NM-1 to NM-6". It now runs to NM-8.
  - Section 1's 2026-10-10 entries are not newest first. The time review (BUILD 0010, labelled "later") sits last.
    The right order is NM-8 (0015-0017), the sections review (0014), sections (0012), the time review (0010), then time (0009).
  - The time review entry gives no check.sh total. It was 1321 (time-and-sleep memory note).
  - The sections entry names A-C as his to confirm. G (an unsized widget fills its section) is his too.
  - The config row (line 249) says "the installed satl says REVISION 09". Both are REVISION 10 now: installed 0007, build/satl 0017.
  - Section 5 (lines 478-483) says the 23 string-method libraries are "Still owed". That is CLOSED: the 32-bit ports are deleted,
    and every string method runs its number (BUILD 0016). strings/ itself still holds 5 files. (found while merging)
  - Section 1 has no row for running programs (satellite.variable.program / .bash, built 2026-10-01). (DOC_SURGERY)
  - Mark the 10-09 rulings: 6.7 item 1 and 6.8 item 2 (Q24); section 1's satellite.statement.for row (Q20);
    section 4's leading-slash item and 6.5's owed item 1 (Q19). (DOC_QUESTIONS "Other documents")
  - D11.1 disagrees with SATELLITE_INFINITY. This one is his to schedule. (SATELLITE_INFINITY Part 13)

- **NEW_MILESTONES.md**
  - The header (lines 3-11) says "Checked against BUILD 0058", and its list of waiting milestones stops at NM-6.
    NM-7 (sections, 0014) and NM-8 (strings 0016, lists 0017) are built but not installed.
  - Section 2's table stops at NM-6. Add rows for NM-7 and NM-8.
  - Section 3 item 4 (line 187) says "the exit-64 fix, still open". It was built 10-09 and is installed in 0007.
    NM-1 still needs program_quit() put back between files.
  - The NM-7 heading (line 485) says "A-C and H his to confirm". It leaves out G, which line 517 records only as "my pick".
  - Section 5 ("Open -- his") lists only the NM-1 to NM-6 questions. Add NM-7's A-C, G and H, and NM-8's `.satellite.` questions.
  - The letters of NM-8's `.satellite.` questions disagree with memory satellite-prefix-for-builtins-2026-10-10.md.
    Here they run A-G (D is "the arguments variable too"); the note runs A-F. Use the letters he was actually asked. (found while merging)

- **MILESTONES.md**
  - The top (lines 14-15) says "NEW_MILESTONES.md (2026-10-06, NM-1 to NM-6)". It is NM-1 to NM-8 now; NM-7 and NM-8 are built, not installed.
  - Lines 198, 720 and 1340 say "548 today". words/words.tsv has 558 rows now: the time words came first, then 4 string and 2 list numbers on 10-10.
  - M32 (line 852) records only BUILD 0009. Add BUILD 0010's review fixes: a sleep lets go of its line's
    object locks, a time is a satellite.library value, and a literal sleep it cannot take is refused before the run.
  - M34 Part 4 (line 1105) says satellite.help() lists 31 topics. It lists 34 since 10-10 (random, time, variable.time).
  - M34 --rebuild item 3 (line 995) counts arguments.magic. Update it once that row is taken out (Q18).
  - M27 (lines 751-753): #22 and #23 are the two strings/ harness files deleted on 10-10. #24 stays. (found while merging)
  - M21 (lines 652-670) ends "Nothing is deleted yet". The 32-bit words and both harness files went on 10-10. (found while merging)
  - Mark the 10-09 rulings: M20.A item 1 (Q6/Q20), M17 (Q7/Q24), M19 and M24 (Q8), M5's 2nd bullet (Q12), M29 (Q17),
    and the satellite.<word> bullet under "Found 2026-10-07 to 2026-10-09" (Q25). (DOC_QUESTIONS)
  - M11 ("the default is 4096", "Neither row is added yet", "DISPLAYED rounded to 32"), D11.1 and M20 ("non-whole division")
    disagree with SATELLITE_INFINITY. This is his to schedule. (SATELLITE_INFINITY Part 13)

- **README.md**
  - Line 3 says "Checked against satl BUILD 0058 on 2026-10-09".
  - The revision 10 paragraph (lines 52-61) and "Where it is today" (lines 105-124) stop at 10-07.
    - Missing from 10-09, installed in 0007: copy/paste and the right-click menu, the subtitle, 800x600,
      `.widgets.displays.file/.input`, `.directory`, `.listen`, and the exit-64 fix.
    - Missing from 10-10, only in build/satl 0017: time + sleep, sections (NM-7), and every string and list method run by its number (NM-8).
  - Line 130 says "NM-1 to NM-6". It is NM-1 to NM-8 now.
  - Say that scroll.horizontal is off unless written (Q9).
  - When Q3's licences land, "27 licences ... 26 other projects" becomes 32 and 31.
  - Line 132's "85 bug pages ... (78 still there on build 0054)" has not been re-checked.

- **SATELLITE_WINDOW.md** (untouched since 10-09 14:27)
  - It has nothing on the 10-09 widgets (`.widgets.displays.file/.input`, `win.name`, `.directory`, `.listen`) or on the 10-10 sections.
    GTK_AND_NO_DEPENDENCIES.md (~2493-2498) and the satellite.window help page already have them.
  - Line 1056 says "`.append()` places a button BY ITS CENTRE". Since 0014, a window laid out in sections refuses `.append`,
    and a window with an appended piece refuses a section. This is question H.
  - "WHAT COULD NOT BE SHOWN" (line 897) says every window capsule exits 64. That was fixed on 10-09 (BUILD 0007).

- **SATELLITE_ARGUMENTS.md**
  - Line 433 says "arguments.threads_startup (1,024)". Warm threads have been cores x 2 since 575191d (24 here). (warm-threads note)
  - The Part 4G scroll.horizontal row (line 885) gives no default. It is off unless written (Q9).
  - Line 879 says object_bytes_max and file_size_max_bytes are read "by nothing yet". Record Q11 there: reading them is owed work.
  - Part 4G's examples (lines 902 and 906) use arguments.magic. Change them when that row goes (Q18).

- **SATELLITE_FILE_OPERATIONS.md** (still the 2026-09-18 text)
  - It never got his 2026-09-24 words (.write / .position / .read, the three .replace forms) or his 10-07 A-E "yes".
  - Part 4's R1 still says no to seek+write, though his .position answer says yes.
  - Part 7 (line 567) says f.path answers the path as written. It answers the absolute path. (SATELLITE_TEST.md:109)
  - Part 2 (lines 127-128) calls "program root first" the include rule. The ruling says it is not about include (Q19).
  - It has no doc-surgery verdict yet.

- **SATELLITE_ERROR.md**: Parts 2-4 describe satl before the renumber. Examples: "Everything else still reports the old way"
  (line 69), "Nothing fills them in yet" (line 97), the four-digit S07xx table, and "SEVERITY IN THE NUMBER ITSELF. Not built" (line 296).
  The renumber, an S-code for every refusal, and the caret are all built. (DOC_SURGERY: "UPDATE (high)")

- **SATELLITE_INFINITY.md**: record the Q23 ruling in Part 12's row Q20 (line 1332) and in Part 3 (lines 411-418, "Q20 asks him to confirm it").

- **GTK_AND_NO_DEPENDENCIES.md**
  - GTK-17's "The rows" (line 2372) says "true unless written". scroll.horizontal is off unless written (Q9).
  - Record Q3 in DEP-6 (from about line 617) and in Table A's DEP-6 row (about line 103):
    VTE, lz4, simdutf, fmt and fast_float get licences, and DEP-6's relink duty covers VTE.

- **PLAN.md** (untouched since 09-18)
  - The opening and line 10 still lead with `.satc/.satb/.sati` and "M0.5 -> M0.6 and M0.7".
    The 09-17 rework further down says those are gone, and M0.7 and satl-term went on 09-22.
  - M0.8 (blocks, kept variables, `run <file>` at the prompt) is built but not marked BUILT.
  - M1's stale-input guard and M3.6's .sate converter died with .sate on 10-03.
  - M0.6 (line 226) gives M21 as a reason. M21 was dropped on 10-09 (Q16).
  - M0's table shows threads_startup 256. It is cores x 2 now.
  - "Decisions waiting on you" (lines 1104-1105) lists `& | ^ << >> !! ~` and unknown escapes. Those were ruled on 09-22 and 09-25 (Q8).
  - M11's satellite.infinity.new() and red note 9 ("satellite_float is arm 12") disagree with SATELLITE_INFINITY. His to schedule.

- **DESIGN.md**: section 10 (the infinity `.new()` spelling and layout) disagrees with SATELLITE_INFINITY. His to schedule. (Part 13)

- **ERROR.md**
  - Section 3's items 22 and 23 (lines 52-56) are about the two strings/ harness files deleted on 10-10. Item 24 stays. (found while merging)
  - The "n = 1 & 2" entry (lines 281-282) says those rows are QUESTION. They are ruled (Q8).
  - Item 15 (line 41, and lines 129-131 under "Fixed"): the refusal stays at code 13 (Q14).

- **REGISTRY.satellite**
  - Lines 67-70 say "nineteen" rows are QUESTION. 23 rows carry it today, and 15 will once the eight Q8 rows are marked.
  - Mark the eight LOGIC AND BITS rows (263-271) ANSWERED with his 09-22 words, then regenerate token_codes.hpp (Q8).
  - The 0x0B method-token rows (115 of them) have no header of their own.

- **Help pages (satellite.help/)**
  - satellite.console (lines 76-81) and satellite.window (line 62) say sideways scrolling is on unless written.
    It is off unless written (Q9).
  - satellite.help (lines 31-36) says access needs "a register of every name in use". Access reads the name where it is now (Q22).
  - satellite.include (lines 7-8) is wrong twice. A leading slash is RELATIVE (his 09-16 ruling).
    A relative include is relative to the including file's folder. **His page: the edit is his** (help-writers #5).
  - satellite.spacesuit says nothing of these by-design limits (SPACESUITS.md):
    - one constructor per spacesuit;
    - a constructor is public even inside satellite.protected;
    - a subtype with its own constructor cannot hand its supertype arguments (M35);
    - the reference-cycle leak.
  - There is no topic for running programs (satellite.variable.program / .bash). Only RUNNING_PROGRAMS.md's table describes them.

- **Comments in the source, and check.sh**
  - satellite/structured-library.cpp:507 says "THE CHECKER IS OWED". The checker runs.
  - satellite/threads/startup_threads.hpp:13 says "Nothing hands them work yet". cascade_convert hands them every line.
  - satellite/machine/thread_stop.hpp:28 says "Set once, never cleared". display_overrun_let_go clears it,
    and since 10-09 structured-library.cpp clears it before each window capsule.
  - These "DESIGN §5.6" / "§7.2" citations mean 003's DESIGN: bytecode_registry.hpp:132, bytecode_registry.cpp:417 and :965, library_values.hpp:20.
  - satellite-numbers/directory_words.hpp:37 says "MILESTONES M21's question". M21 was dropped (Q16).
  - make_support/048-link.mk:14 names build_libraries.py, which was deleted on 10-07.
  - check.sh's comment at line 2951 and label at line 2983 say both scroll rows are "true unless written" (Q9).

- **SCRATCH.md/OPEN_ITEMS_2026-10-10.md**: section 0 says BUILD 0014 and 1330/1330. It is BUILD 0017 and 1336/1336 now.
  F in section 1 still names 0014. Section 0's backups list lacks -before-string-numbers/ and -before-list-numbers/. (found while merging)

- **SCRATCH.md/DOC_QUESTIONS_2026-10-09.md**: line 5 still says "Nothing here has been decided". Mark Q1-Q26 ruled on 10-09,
  along with their defect bullets.

- **SCRATCH.md/DOC_SURGERY.md**
  - Mark the 10-09 rulings here:
    - the README edit plan, items 3-4 (~line 1022; Q1 delete, Q2 keep as an aim), and "Wrong today" (1017-1018);
    - the DEP-6 gap (302, Q3);
    - the DESIGN entry (762-793, Q10-Q13);
    - lines 805 and 817 (Q26/Q25), 378 (Q15) and 980 (Q19).
  - Its PROGRESS section has five slips:
    - SATC.md does exist: it is 003's, in old_versions/second_satellite/.
    - check_satl still sits uncalled at satl_file.cpp:139-158.
    - file_calls.cpp:131-139 does probe FILE paths; only includes don't.
    - The families hold 191 tokens, not 210.
    - There are 84 machine codes, not 89.
  - Its DESIGN entry (line 774) repeats "nothing hands them work".

- **SCRATCH.md/FAST_PRINTING.md**
  - Lines 11 and 69 say "Step 6 is next". RUNNING_PROGRAMS built it in substance (program_watch.cpp).
  - Lines 114 and 556 say "satl starts no programs today". It has since build 0128, from prompt_run.cpp and console_menu.cpp:94.
  - Line 73 says "prove-console.sh: 5 of its 6 stages". All six pass since the exit-64 fix (BUILD 0007).

- **SCRATCH.md/ERRORS2.md**: lines 25-26 say "M21 moves the string libraries off satellite_string32". M21 was dropped on 10-09,
  and those libraries were deleted on 10-10.

- **Small READMEs** (from DOC_SURGERY)
  - utility/linking/README.md names a make target that does not exist. build/link_test was never built, and its gperf confirmation is owed.
  - time_test/README.md: check.sh does not run the races it claims. Three of its files are unexplained, and official_test/speed_test is not mentioned.
  - experiments/nested_include/README.md: the experiment no longer runs. A bare deep() is refused since Revision 07, and its runner is gone.
  - licenses/README.md:
    - its counts are two eras old;
    - the FreeType credit is now in --version and --help;
    - line 161's SATELLITE_WINDOW.md:557-560 reference moved (now 1141-1142, and still wrong);
    - record Q3.

- **POLYMORPH/M7.md**: line 38 says "words.def has 342 rows"; words.tsv has 558, and all of them are compiled into satl.
  **Ask him first.** On 09-14 he refused this edit mid-discussion:
  *"why are you asking me to edit M7.md ??? we are having a conversation about all of this stuff until we nail it all down."*

- **Memory notes** (~/.claude/projects/-home-madness-code-cxx-satellite/memory/)
  - MEMORY.md has three stale lines:
    - The 10-09-late line's "his A-F open" means the 10-09 sketch questions (`.listen()` live ... `satellite.directory.open`).
      Those are not today's A-H, and not the `.satellite.` A-F. Say which set it is.
    - The satellite-prefix line says strings are done and the "next type his call". Lists are done too (BUILD 0017).
    - "Help writers' findings -- 13 bugs, none fixed" is wrong: most are fixed. Still open: #5, the number half of #9, #10, #12, #13.
  - time-and-sleep-2026-10-10.md: its description and OWED NEXT say BUILD 0010, check.sh 1321, and "F = install 0010".
    Its work is in build/satl 0017 now, and check.sh is 1336/1336.
  - window-sections-design-2026-10-10.md:
    - line 26 says "~12:30", but 0014 was linked at 12:04;
    - lines 54-58 (NEXT/STATE) are from before the build ("F install 0012", "NO code edited").
  - satellite-prefix-for-builtins-2026-10-10.md: its description says "NOTHING built". Strings and lists are built (0016, 0017).
  - satellite-004-layout.md still says every word is its own .so and check.sh has 223 rows. The words are compiled into satl since 10-07.
  - missing-syntax-plan-2026-10-03.md:20 says "keep `argument_1` answering". MS-1 refuses it, and quad_test.satl now reads x.argument3.
  - These descriptions lag their own bodies:
    - satellite-test-design-2026-10-06 ("NOT BUILT")
    - loss-and-speed-tests-2026-10-06 ("IN FLIGHT")
    - satellite-legal-built-2026-10-06 ("0047 NOT installed")
    - file-operations-2026-10-07 ("IN FLIGHT")
    - file-model-2026-09-24 ("building")
    - console-text-shadow-2026-10-04 ("white->background")
    - next-session-after-clear-2026-10-09 and -09-late ("not installed")
    - revision-10-and-toolkit-main-2026-10-09 ("NOT installed")
    - doc-update-plan-2026-10-09 ("running")
    - satellite-toolkit-2026-10-08 ("main file NOT started")
    - no-window-link-fix-2026-10-08 ("waits")
    - builtin-random-plan-2026-10-06 (shelved, deleted)

- **~/code/py/satellite_toolkit/NEXT_SESSION.md** (and PLAN.md, both 10-09 11:06)
  - It still opens "start here after the /clear of 2026-10-08".
  - It is missing the main-file and installer merge, the 15 review fixes, the clang BOLT/orc-rt fix, the pypy SSL fix,
    mom's three runs, and the 13 open questions.

### 7.2 State still to act on

- **CLOSED 21:11 -- built, not installed:** BUILD 0018 is installed (section 0).
- **The installer runs `satl --rebuild` and hello_world in HIS home:** copy ~/.satl/config.ini before an install (on
  21:11 it came out unchanged). Builds install now, his word: `make INSTALL_AFTER_BUILD=yes`. (arguments-table-and-first-start-2026-10-03.md)
- **Keep these backups too**, besides section 0's:
  - backups/2026-10-10-before-string-numbers/ (string-numbers.patch, plus satl-build-0014 as the timing baseline);
  - -before-list-numbers/ (list-numbers.patch, plus satl-build-0016).
- **His push is owed:** mom's machine has everything but satellite.
  - The upload copy staged for that push is in /tmp/.../9a28c15f-.../scratchpad/upload/satellite.
    It is the 10-09 morning tree (REVISION 10, BUILD 0001), not today's.
  - The toolkit history-rewrite push is also his.
  - Pushes go from a copy with GIT_SSH_COMMAND, never with git in the tree. (next-session-after-clear-2026-10-09-late.md)
- **GitHub -- ask him:** e63226e was pushed from this checkout on 10-08 at 20:12, and the 10-09 editors found the repo public.
  The push likely used ~/.ssh/satellite_deploy_ed25519, a key no note records. Was that him?
  CLAUDE.md says the repo is private: check live before saying either. (github-deploy-key-2026-10-09.md; DOC_QUESTIONS defects)
- **Mom's machine:** kernel 7.2.9-elkt is installed but is not the default boot entry; she still runs 6.12.0-211.61.1.el10_2.
  Making it the default and rebooting is his call. (Her BIOS 1402 predates Intel's 13th/14th-gen fixes.) (moms-machine-2026-10-08.md)
- **Toolkit work is uncommitted** in ~/code/py/satellite_toolkit:
  - the main file (4,574 lines), the installer (3,010), the review fixes, and the clang BOLT/orc-rt and pypy SSL fixes;
  - 1001 tests passed;
  - commit or push only if he asks;
  - ~/code/py/satellite_toolkit.before-2026-10-09 (11 GB) is kept aside. (revision-10-and-toolkit-main-2026-10-09.md)
- **This machine's ~/opt/clang-23.1.3 has no BOLT layout**, from the builder bug that has since been fixed. It was not rebuilt.
  Tell him; a rebuild is his call. (next-session-after-clear-2026-10-09-late.md)
- **File operations never landed:**
  - The 10-07 rewrite (FO-5..FO-10, his 09-24 words, his A-E "yes") left no patch in backups/2026-10-07-before-file-operations/.
    The only copy of its patches is in /tmp (e99fe5f0).
  - His F-H, asked on 10-07, are unanswered.
  - His words: *"this is the SATELLITE programming language, and I wanted to finish SATELLITE_FILE_OPERATIONS.md"*. (file-operations-2026-10-07.md)
- **/tmp holds ~50 GB of older sessions' scratch. Ask before deleting, and copy out what matters first:**
  - e99fe5f0 (29 GB): the only copy of the 10-07 file-operations patches;
  - 9a28c15f: the upload copy, moms_all.log and toolkit copies;
  - f00b04df: the PGO patch;
  - 544744b4: 7.4 GB.
  (next-session-after-clear-2026-10-09.md)
- **Needs his sudo, on this machine:**
  - threads-max and max_map_count are back at 505,689 and 1,048,576 (read 10-10).
    His 4,194,304 was never written to /etc/sysctl.d. (PROGRESS section 2)
  - /opt/rocm/lib is not in ldconfig. The fix is /etc/ld.so.conf.d/rocm.conf plus ldconfig. (llama-cpp-on-the-w6600-2026-10-03.md)
  - 003's leftovers are still in /usr/local/share (satellite, doc/satellite, man1/satl.1.gz, satl-term.1.gz, org.satellite.terminal.desktop).
    `sudo sh satellite_enterprise/install.sh --system` takes them over. Until then `man satl` describes a program that is gone.
    (satl-install-locations.md)
  - The oneAPI dnf repo's GPG key is broken, so every dnf query needs --disablerepo=oneAPI. (satellite-build-deps-almalinux.md)
- **His own files -- never edit them; tell him:**
  - satellite.variable.info/satellite.directory.list.satl (his 09-25 draft) needs four fixes: arguments.cwd -> arguments.directory,
    .string on free()'s float, the closing quote on line 9, and satellite.return(satellite). (listing-files-and-sub-2026-09-25.md)
  - ~/code/satl/mythkeepers/probes/ (4 files) still use argument_N. MS-1 refuses it (S201, naming argument(N+2)). (missing-syntax-plan-2026-10-03.md)
  - These of his programs stopped on 004 at the last sweep (10-02):
    - #000000 colours;
    - satellite.system.memory.main;
    - .length(), satellite.variable.variant, .modulus;
    - a map field and a bare cross-file call;
    - the C++ constructor spelling, refused on purpose.
    They have not been swept since. (pointer-reference-built-2026-10-02.md)
  - satellite_server_source_update/ in the tree has been empty since 10-08. Deleting it is his.
    His backup PULL test (does a server copy build?) was never reported. (server-upload-2026-10-07.md)
  - Leave these as they are: examples/complete_test.satl, help.txt, capsule_calling.satl, function_calling.cpp (and its binary),
    and experiments/arguments_and_config/main.cpp. (next-session-after-clear-2026-09-22c/d.md)
- **Keep ~/opt/gcc-17:** satl links its libstdc++. (containers-and-access-2026-09-26.md)
- **`satl hello.satl` run from a shell flashes a console that closes when the program ends cleanly.**
  It was flagged on 10-05 and not changed; his call. (next-session-after-clear-2026-10-05.md)
- **ERRORS4 has 85 bug pages, and 78 were still there on build 0054** (3 fixed, 4 changed).
  - His 10-09 ruling: leave the tickets alone.
  - Before fixing one, run its program on today's satl.
  - His words: *"there's still... 200 errors to fix or something like that, when this is done"*. (ERRORS4/README.md; DOC_SURGERY)
- **MISSING_SYNTAX:** MS-1 to MS-3 and Part 4G are built; MS-4 to MS-30 are not.
  MS-4 waits on his word, and NEW_MILESTONES is his current plan. Nothing reads arguments.missing yet (that is MS-5).
- **Other docs keep their own open lists, not merged here:**
  - SCRATCH.md: FAST_PRINTING, CONTAINERS, MAP_SORT, SPACESUITS, NEW_ERROR_LIST, ERRORS2, ERRORS3;
  - SATELLITE_INFINITY Parts 12-13; SATELLITE_FILE_OPERATIONS Part 8; THREADS (T2, T3);
  - SATELLITE_ARGUMENTS (Phases C/D, 4B); SATELLITE_ERROR Parts 8 and 12;
  - GTK DEP-1..DEP-9; ERROR.md sections 3, 5 and 6; POLYMORPH/M1-M7.

### 7.3 Probably settled -- check before acting

- His source-editor sketch's line notes (old names, `.open()` twice, a missing `)`): he changed it again at 17:10.
  Everything after `.open()` is now commented out. Its unbuilt words are already in section 2.
- The full-size icon AVIF cannot be decoded here. That only matters for icon work; he would export a PNG.
- ~/.satl/satl has no gio custom icon. Add one only if he asks.
- The icons-and-satl-mime-type branch was never merged. That is moot: there has been no git since 10-05.
- Root-owned files from a root make on 10-07: done. Nothing in the tree is owned by anyone else, and make refuses root.
- The no-window link stubs were never applied, by his ruling: *"there is no no-window build"*.
- His speed test and race.cpp are out of step. He was told on 10-06, and he wants no speed work: *"it's definitely fast enough"*.
- His 1-trillion speed standard (~12 days) is a standing fact; satellite.test.speed() runs ~60 s instead.
- DOC_QUESTIONS Q1-Q26 were all ruled *"looks good"* on 10-09. The work they call for is in 7.1.
- NEW_ERROR_LIST: A1-A10 and B1-B5 are built (checked on 0054). Only choices he can overrule remain.
- PLAN.md's .satc/.satb/.sati pipeline is gone by its own 09-17 rework. Only the opening lines are left (in 7.1).
- The installer comment that systemd puts ~/.local/bin on PATH is fixed in 004. Only 003's frozen copy keeps it.
- MISSING_SYNTAX's "quad_test uses argument_1" no longer holds: quad_test.satl reads x.argument3.
- Older notes' items that later notes closed were checked on 10-10: Revision 07, spacesuits, DEP-5, unknown escapes, errors.md,
  the 13700K microcode and about 15 more. The full list is in the sweep journal's "memory 09-22..10-03" result.
