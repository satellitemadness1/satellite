# OPEN ITEMS -- 2026-10-11: what is open, and what closed today

The day before's file, SCRATCH.md/OPEN_ITEMS_2026-10-10.md, still holds everything older: its section 1's unanswered
letters (listed in 1b below), and sections 4-7 (the 688-item sweep of 10-10). Nothing there is closed by this file
unless section 3 says so.

## 0. Where things stand (~14:00, 2026-10-11)

- **BUILD 0031 INSTALLED** (~/.satl/satl and the word satl), commit c57f59b2 on milestones-install-and-no-console-handover,
  uploaded to repo-9. BUILD 0030 = commit 20119d5f, repo-8. ./check.sh on 0030: 1349 passed, 0 failed (0031 changed only
  build_stack.py, 050-build.mk, commit_and_upload.sh and a help page, none of which check.sh runs).
- **GITHUB'S MAIN IS 7671402 = BUILD 0031 + today's docs (commit 9903bc3b)** -- one snapshot commit, as on 10-09 (his
  rule then: *"put everything onto main, just erase the current main and replace it"*, *"take out the folder
  old_versions"*). Repo public. Before it: 4258c2ae (BUILD 0030), pushed at ~13:10 and proven from github.com; the 10-09
  main (c55de31) is mirrored in ~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-11-github-main-before-replace/
  (no new mirror for 4258c2ae -- his word, ~14:05). BUILD 0031's snapshot was proven by a fresh clone (13:49); 7671402
  differs from it only in four docs; a clone from github.com PROVED it at 14:05 (plain make, one question, exit 0) -- its log:
  /tmp/claude-1000/-home-madness-code-cxx-satellite/c4e4a254-1700-4b7a-82bf-29c82c49d9ac/scratchpad/fromgh2/make.log ("<<< make exited 0 >>>" at its end = proven).
- **HOW A PUSH TO MAIN IS MADE** (done twice today: 4258c2ae, then 7671402): `git clone --no-hardlinks` of the tree
  into the scratchpad; `rm -rf .git old_versions`; `git init -b main`; user jsadlier / satellitemadness1@gmail.com;
  `git add -f -A .` (FORCE: vendor's upstream .gitignore files hide files the branch tracks -- glib's gvdb among
  them); check the file list equals `git ls-tree -r --name-only <commit>` minus old_versions; one commit; a secret
  scan (only glib's own test certificates hit); a FRESH CLONE of that snapshot built with plain make at a
  pseudo-terminal under a scratch HOME (about 6 minutes; scratchpad tools/pty_make.py answers the question); then
  `GIT_SSH_COMMAND="ssh -i ~/.ssh/satellite_deploy_2026-10-09_ed25519 -o IdentitiesOnly=yes" git push --force
  git@github.com:satellitemadness1/satellite.git main:main`; then clone from https://github.com/satellitemadness1/satellite
  and build that too. A COPY IS NOT A CLONE: a copy keeps file times and git does not -- that is how 10-10's proof
  missed the gperf stop.
- **HIS ~/code/satl IS IN THE NEW SPELLING** (NEW_MILESTONES.md NM-8). Backup and patch:
  backups/2026-10-11-before-his-programs-spelling/ (code-satl.tar.zst = the folder before, his-programs-spelling.patch).
- **HIS OLD CHECKOUT ~/source-opt/satellite** holds the 10-09 history (c55de31): `git pull` cannot reach the new main
  (each snapshot is a commit of its own). A fresh clone is the way: `git clone https://github.com/satellitemadness1/satellite
  ~/source-opt/satellite-new && cd ~/source-opt/satellite-new && make`. Told him in the chat.
- Today's backups, in ~/.claude/projects/-home-madness-code-cxx-satellite/backups/: 2026-10-11-before-build-question-and-help
  (tree.tar.zst, config.ini, satl-build-0029, vendor/'s scripts, build-question-and-help.patch),
  -github-main-before-replace, -before-his-programs-spelling, -before-review-fixes (review-fixes.patch), -before-docs.
- ANOTHER CLAUDE SESSION works in ~/code/satl/infinity_data3 (its scratchpad /tmp/claude-1000/-home-madness-code-satl-
  infinity-data3/, a satl-0030 copy running list256.satl at 13:34). It had already respelled infinity_data_main.satl;
  this session did not touch that folder.

## 0a. IN PROGRESS SINCE ~14:20 (after a /clear) -- NM-8 FOR EVERY TYPE, SEVEN AGENTS AT ONCE -- START HERE

**WHERE NM-8 STANDS NOW: /home/madness/.nm8/NEXT_SESSION.md, its top section (BUILD 0039 installed at ~19:35; his
O, P, Q waiting; N next). EVERYTHING OPEN, NUMBERED: SCRATCH.md/ALL_OPEN_2026-10-11.md (339 items). Read those
first; what follows in 0a is the history of the day.** His infinity_data3 file was CHANGED at his ask at ~16:41
("just display more so I can see that it's doing something") -- it ran, but its first line came after ~13 minutes
(512 satellite.random.normal draws per piece); now 593 lines, md5 45ff5929, knob satellite.library.show_every = 4;
backup + patch in ~/.claude/projects/-home-madness-code-satl-infinity-data3/backups/2026-10-11-before-more-display/.

**THE CURRENT TASK, HIS ASK OF ~15:00 (it decides the order of merging):** *"so this doesn't work:
/home/madness/code/satl/infinity_data3/infinity_data_main.satl and I think it's because the word "satellite" isnt put
in front of everything, can you do the work necessary to make that file run without changing that file? If it has an
error of course, don't change the interpreter to work around any errors, obviously if the file has an error we would
need to change the file, not the interpreter"* -- then *"we need to run clear before we do anything"*.
- HIS FILE IS NEVER EDITED (a Gemini session works in that folder too). md5 at 15:00: 95631e07dd98d514a89042084be81226,
  578 lines; a byte copy for runs: /home/madness/.nm8/id3/copy_of_main.satl (run copies there -- the program writes
  infinity_data_done.txt beside itself, and runs for hours by design: 8.2 billion units, satellite.random.normal 1-2 s a
  draw). BUILD 0032 stops at line 143 (S110 "data_type.satellite.to_string is written data_type.to_string -- a number's
  words are not under satellite yet").
- IT NEEDS FIVE FAMILIES, so MERGE THESE FIRST: numbers (`x.satellite.to_string()` on numbers, a library value
  `satellite.library.units.satellite.to_string()`, a capsule's answer `space.call_piece_count().satellite.to_string()`),
  scalars (`depth.satellite.string` on a float), objects (`progress.satellite.lock()`, `space.satellite.lock()`,
  `maker.satellite.pointer()`, and the thread handoff `satellite.thread.new(units_worker(progress,
  block.satellite.pointer(), step, units))`), threads (`pool[k].satellite.start()` / `.satellite.join()` on a
  list<satellite.variable.thread>), files (`done.satellite.append(line)`, `saved = done.satellite.close` -- close bare,
  answering a bool). Strings and lists (`characters.satellite.size`, `.satellite.at(..)`, `.satellite.append`) already
  work. Maps and the window are not used -- merge them after.
- ABOUT TO RUN when he stopped for the /clear (he declined it; ask before running it): the recording satl
  (/home/madness/.nm8/rec/build/satl = BUILD 0032 + recording.patch) on the copy for 60 s with SATL_RESPELL -- it accepts
  the new spelling on every kind (recording it) and runs the old code, so any error it stops on that is NOT a period is
  a candidate error IN HIS FILE (or NM-8 work, e.g. the handoff reading `.satellite.pointer()` as text). Errors in his
  file: report them with the fix -- the file changes, not the interpreter (his words).
- THEN: once the five are merged and built, run his file itself (a minute, timeout, scratch HOME) and show him the
  first lines it prints.
- TOOLS: /home/madness/.nm8/tools/merge_family.py FAMILY_COPY TARGET [--write] (three-way merge against 09c2defa;
  resolves both-sides-appended and both-sides-respelled-one-line conflicts; skips the build number and the generated
  files -- regenerate them: words/make_words.py, satellite/bytecode/make_word_codes.py, make_token_codes.py,
  satellite/satellite_test/make_test_programs.py; REGISTRY.satellite's `free` marker row is fixed by hand);
  /home/madness/.nm8/tools/race.py OLD_SATL OLD_DIR NEW_SATL NEW_DIR PROGRAM... (alternating runs, best of 5, outputs
  compared; the bar is +5%; only on a quiet machine).

- HIS ANSWERS to section 1's A-C of ~14:30: *"A. yes B. yes C. yes"* -- NM-8 first, then the install questions; the
  bashrc alias defaults to [n] when satl was linked into the system's bin and [y] when it was not; the link goes in
  /usr/local/bin, not /bin.
- BACKUP: ~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-11-before-nm8-every-type/ (tree.tar.zst,
  config.ini, satl-build-0031).
- BUILD 0032 = THE FOUNDATION, INSTALLED, commit 09c2defa, uploaded repo-11; check.sh green (in a copy: 1348 + the pty
  row, which fails only from a long path). No behaviour change: method_periods.hpp `number_a_method_runs` is a switch,
  one case a type, asked by the walker and the checker's literal rule; `period_written_wrong(receiver, word, Kind,
  under)` says "has no" for any numbered kind and lists the numbered kinds itself; `kind_a_method_answers` one case a
  kind; number_row.hpp `MethodCall method_call` (the ListCall shape) appended last, hidden from calls by name in
  function_table.cpp; utility/satellite_spelling.py reads `NUMBER_SWITCHES`.
- THE WORKFLOW nm8-every-type (run wf_231e2d88-8a5, task w97meun3n): seven agents, each in a copy-on-write copy
  /home/madness/.nm8/<family> made from 09c2defa (`cp -a --reflink=always`; short paths so the pty row fits), each with
  its own HOME <family>-home, building with `make -j6 INSTALL_AFTER_BUILD=no`, running check.sh in its copy:
  numbers (+infinity), scalars (float hex binary color fraction percentage bool time), maps (+ the arguments variable
  and the bare name arguments -> satellite.arguments), files, threads (+programs, bash), window (his final syntax),
  objects (+ .center()). Their brief: /home/madness/.nm8/BRIEF.md (the decisions per family are in the workflow script:
  ~/.claude/projects/-home-madness-code-cxx-satellite/d489d5fb-4e0e-4e80-a250-a331a488596a/workflows/scripts/
  nm8-every-type-wf_231e2d88-8a5.js). Each writes /home/madness/.nm8/<family>-REPORT.md when done. Token codes given
  out so no two take one: numbers 0x0B7A-0x0B86, scalars 0x0B87-8A, maps 0x0B8B-8E (get, set), files 0x0B8F-92,
  threads 0x0B93-96, window 0x0B97-0xBA6, objects 0x0BA7-AA.
- THE RECORDING SWITCH (SATL_RESPELL=<log>: a period written the other way is written down, not refused, and
  utility/satellite_spelling.py --apply makes the changes) is /home/madness/.nm8/recording.patch -- tested, NEVER in the
  tree (built only in copies).
- INTEGRATION, ONE FAMILY AT A TIME IN HIS ORDER (numbers, scalars, maps, files, threads, window, objects): three-way
  merge of the family's copy (base 09c2defa) into the tree, generated files regenerated, conflicts on a program line
  that both sides respelled resolved by taking both insertions; checked in a copy first (build + check.sh + the
  converter + the recording satl), timed old vs new on a quiet machine (at most 5% slower), ONE fresh reader's review,
  then built in the tree (install + commit). Then his ~/code/satl converted (backup first; three Gemini sessions work in
  crack_energy_distribution, infinity_data3 and negatively_signed_infinity_data1 -- ask before touching those), the
  .md docs, then TASK 2 below with his A-C.

## 0b. THE NEXT TASKS, IN HIS WORDS (written at "context is red", ~14:30) -- start here after /clear

**TASK 1 -- NM-8 FOR EVERY TYPE LEFT** (his ~13:50: *"we still need to get the word satellite onto everything, before
we do anything... .pointer, numbers floats, everything builtin needs the word satellite still"*; his A-C of ~14:05 in
section 3 and NEW_MILESTONES.md NM-8 "EVERY TYPE LEFT"). DONE ~14:10 -- THE MAP IS SCRATCH.md/NM8_INVENTORY_2026-10-11/ (inventory.tsv: 231 words, one row each, with
its dispatch file:line, its row if any, what it answers, uses in the tree and in his programs; machinery.md: what each
family needs; summary.txt). It came from a read-only workflow of 7
readers mapping every built-in word still written with a bare period, by type (numbers, other scalars, containers +
the arguments variable, files, threads/programs, window, objects + a cross-check of all 139 method spellings). Its
results land in /home/madness/.claude/projects/-home-madness-code-cxx-satellite/c4e4a254-1700-4b7a-82bf-29c82c49d9ac/subagents/workflows/wf_9550492d-1a9/journal.jsonl (one "type":"result" line per reader);
/tmp/claude-1000/-home-madness-code-cxx-satellite/c4e4a254-1700-4b7a-82bf-29c82c49d9ac/scratchpad/tools/inventory_table.py JOURNAL OUT_DIR turns them into inventory.tsv + machinery.md + a per-type summary (if
the scratchpad is gone, re-run the workflow: script nm8-inventory-2026-10-11 under this session's workflows/scripts).
TEMPLATES for each type: backups/2026-10-10-before-string-numbers/string-numbers.patch,
backups/2026-10-10-before-list-numbers/list-numbers.patch, backups/2026-10-10-before-satellite-spelling/
satellite-spelling.patch, and `git diff d68e4d51 542d65dd` (the spelling) + 7e3f5e85 (its review fixes). PLAN given to
him: numbers first, then floats and the other scalars, maps, files, threads/programs, the window, time; each type a
number for every word (the call runs it), then `.satellite.`, then every program, test, help page and his ~/code/satl
converted (utility/satellite_spelling.py: KIND_OF_TYPE + UNDER), check.sh green, at most 5% slower, one fresh reader.

**TASK 2 -- THE INSTALL QUESTIONS** (his words, ~14:30, after a pull from GitHub): *"a pull off github did not ask
whether I wanted to install the icons and the app onto my system, and this question needs to run something different
on debian/ubuntu than it does for enterprise linux, and it needs to install the icons for satl and .satl files, and it
needs to ask if the user wants to do every single thing, with the default answer being [y] for each thing... We need to
ask if the user wants to install .satl to /home/username/.satl and then ask to add a symlink so it's on the users linux
console and put it into /bin, then it needs to ask if they want to install a copy to their system's apps (different for
debian, different for enterprise linux) then does the user want to install the icons for satl exe, then does the user
want to install icons for .satl files with [y] being selected by default, and if these things have already been
installed, we need to record if they have already been installed into /home/username/.satl/icons.ini will have like 3
lines in it, we need to check whether these icons and file ext .satl has already been installed... then we need to ask
if the user wants to put alias satl = /home/username/.satl/satl into their bashrc file -- if the user didn't install a
copy into their .bin, then this question needs to be asked with the default answer being [n]"*.
As I read it, a plain make ends with these, one at a time, each [Y/n] unless said: (1) install satl into ~/.satl;
(2) a link in /bin so `satl` works at the console; (3) a copy in the system's apps -- Debian/Ubuntu one way, Enterprise
Linux another; (4) the icons for the satl program; (5) the icons for .satl files; (6) ~/.satl/icons.ini (about 3 lines)
records what is installed, and is read first so nothing is asked or done twice; (7) `alias satl=~/.satl/satl` in
~/.bashrc, default [n] (his condition -- question B). TODAY: one question, "Install it into ~/.satl? [Y/n]"
(050-build.mk; commit and upload too in his checkout); install.sh's 070-desktop.sh writes the launcher
~/.local/share/applications/org.satellite.terminal.desktop; memory: desktop-icon-install-for-satellite (read the XML's
comments first), satl-binaries-carry-a-gio-icon, local-bin-not-on-session-path. The minute's wait is bash's read -t.

## 1. Questions waiting on the author ("yes to all" works)

ANSWERED ~16:53: *"A. nothing further with infinity_data for right now, B. not sure about this one, don't know what you
mean, C. you decide, D. yes E. let's just build whatever doesn't yet have satellite in front of it that is built
in..."* -- A: his file stays as it is. B: kept (a note of mine). C: deleted, after cmp showed it identical to the
installed satl. D (the objects review's M1/M2): CLOSED -- BUILD 0034, 00f1c2d2, repo-13; check.sh +11 rows, 1459/0.
E (OPEN): every built-in still without satellite, swept and then built -- /home/madness/.nm8/NEXT_SESSION.md.
ASKED ~16:45 (after his "stop stop" and *"just needed you to display a few variables, nothing else"*) -- about the
infinity_data3 change of ~16:41:
- A. Cut his file back to just the display lines, dropping the `satellite.library.show_every` knob? (no -- it works)
- B. Take the pointer at the top of section 0a back out? (no)
- C. Delete the satl test copy /tmp/claude-1000/-home-madness-code-cxx-satellite/5db33c05-dcac-4f03-9d0d-60d5e815dfda/
  scratchpad/id3run/satl-0033 (his rule: no saved copies of a built satl)? (yes)

ASKED ~14:30 -- ANSWERED after the /clear, "A. yes B. yes C. yes" (section 0a).

NOT ASKED YET -- written here at ~14:00, then he turned the day to NM-8 (section 3); ask them when NM-8 allows:

- A. GitHub's main from now on: every build also replaces main with its snapshot (untested), or only when he says
  "push" (tested first, about 7 minutes)? (only when he says?)
- B. 46 of his ~92 programs still stop on OLDER problems before they run: C++-style constructors (`view_object()`,
  `view_type()`, `view_link()`, `darkening()` -- satellite's is satellite.constructor), `#000000` colours (004 reads
  x000000), `\u` and `\x` escapes, voice.satl changing satellite.library.voice_quiet inside a capsule (S250), list types
  written `view_object` for `view_object.view_object` (S201), a number's `.floor` / `.modulus` and a list's `.length`
  (not words in 004), `for (i = 0; ...)` with no declaration, `arguments.get(...)`, `arguments.length(...)`, and the
  3 mythkeepers probes' `args.argument_1` (argument3 now). Fix them in his programs, one family at a time, global_view
  first? (yes?)

## 1b. Still open from 10-10 (SCRATCH.md/OPEN_ITEMS_2026-10-10.md section 1 has the words)

- Sections and time, letters of ~16:00: A (sections fill left to right and wrap), B (a % stays a % on resize), C
  (sections hold sections), D (`ms` = millisecond), E (a bare sleep(5) refused), G (a widget with no size fills its
  section), H (sections OR `.append(piece, x, y)`, not both). F was answered.
- The `.satellite.` spelling, ~17:20: D (`args.satellite.username`), E (words in quotes stay), F (symbols stay), G
  (names like `size` and `section` become free for his own things). A-C were answered.
- NM-8's next type is the window (his E of 21:15: *"yes we do this before we do anything further"*), with his final
  window syntax (memory: his-editor-syntax-final-2026-10-10) -- NOT BUILT.

## 2. Gaps left open today, on purpose

- His programs: 30,740 of the 52,797 places sit in files no checker reads whole, because the programs that include
  them stop first on an older problem (question B). 93% are `.append` on a list declared in the same file; a fresh
  reader checked 258 of the other 2,080 (one per file and word) and found all right. A receiver neither the converter
  nor the checker could type is refused when it RUNS, with a sentence naming the new spelling.
- The 43 places the tool could not work out stay bare, rightly (line_creator's `object_types` and infinity_data2's
  `definitions` are declared nowhere; journal's `.join()` is a program's; mythkeepers' `home.add` and
  `squares[key].add/.remove` are his spacesuits' own capsules).
- Older, from 10-10's review, not fixed: INSTALL_AFTER_BUILD=yes in the ENVIRONMENT runs the installer twice.
- The fresh reader's note, not a defect: satellite_backup's own `folder` line is read the same way in both places now.

## 3. CLOSED today (his answers, and what was finished)

- **~14:30, after the /clear: his "A. yes B. yes C. yes"** (section 0a). **BUILD 0032 BUILT, INSTALLED, COMMITTED
  09c2defa, UPLOADED repo-11** -- NM-8's foundation for every type, no behaviour change (PROGRESS.md section 1, first
  entry; NEW_MILESTONES.md NM-8). Backup before it: backups/2026-10-11-before-nm8-every-type/.

- **~13:50, HIS NEW ORDER OF WORK: *"we still need to get the word satellite onto everything, before we do
  anything... .pointer, numbers floats, everything builtin needs the word satellite still"*** -- NM-8 for every type
  left (NEW_MILESTONES.md NM-8, "EVERY TYPE LEFT"). Asked A-C at ~13:55; HIS ANSWERS ~14:05: *"a. yes b. arguments
  becomes satellite.arguments but the arguments like .username .machine don't get the word "satellite", so we don't
  type satellite.arguments.satellite.username, we just enter satellite.arguments.username C. yes"* -- A: `.pointer()`
  and `.reference()` become `.satellite.pointer()` / `.satellite.reference()`; B: the bare name `arguments` becomes
  `satellite.arguments`, its rows written straight after it (`satellite.arguments.username`), its words under
  satellite like every type's; C: operators, literal endings (`+ == 5ms 50%`) and words in quotes (`"text"`) stay.
  And GitHub: *"we update github main with this, and since we've already backed up main only 1 commit ago, we don't
  need to backup main again... at least not right this second"* -- main updated with BUILD 0031 and today's docs,
  no new mirror.

- **His answers to 10-10's ~23:55 questions: *"A. ask to install, but what is going to be asked when someone pulls
  this from the git repo?? B. yes C. yes D. yes"***:
  - A -- the question stays, and ANSWERED: someone who pulls (a fresh clone, plain make) waits about 6 minutes while
    the vendored stack and satl build, and is asked one thing -- "Install it into ~/.satl? [Y/n]". Their copy never
    commits or uploads (commit_and_upload.sh: only the folder this machine's satellite_backup profile backs up).
    Proven by real clones (PROGRESS.md section 1, first entry). Until today a clone was asked about the commit and
    the upload, committed in its own git, and failed its make where git had no name.
  - B -- DONE: NEW_MILESTONES.md NM-8, "HIS OWN PROGRAMS".
  - C -- DONE: the list page (a[0] is S413, past the end S501) and satellite.container's (main's arguments run).
  - D -- left as the record.
- ***"when I pull from github it fails almost instantly..."*** and ***"plain make on the github repo fails instantly
  for the same reason it failed before"*** -- FIXED: GitHub had the 10-09 snapshot (nothing pushed since); and the
  branch itself, cloned fresh, stopped at gperf (git keeps no file times) -- vendor/build_stack.py's
  even_out_checkout_times. His choice when asked: **"Replace main, like 10-09"**; and ***"re-commit until plain make
  works from main"*** -- DONE: main = BUILD 0030 (4258c2ae), and a clone from github.com built with plain make.
- **The review (three fresh readers, one per change)** -- A: two LOW (his pcre2 tarball, written in 4 seconds, would
  have been re-timed; under dash the question did not wait) -- FIXED in 0031, plus the profile's folder line read as
  satellite_backup reads it. C: two LOW in the new help sentence -- FIXED. B: 258 of 258 sampled places right; ONE
  MEDIUM -- safe_recovery's narration strings quote its own code lines, so they had to be respelled with them (774
  places), and breach_walk's pinned copy of column_count's body -- FIXED.
