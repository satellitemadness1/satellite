# OPEN ITEMS -- 2026-10-11: what is open, and what closed today

The day before's file, SCRATCH.md/OPEN_ITEMS_2026-10-10.md, still holds everything older: its section 1's unanswered
letters (listed in 1b below), and sections 4-7 (the 688-item sweep of 10-10). Nothing there is closed by this file
unless section 3 says so.

## 0. Where things stand (~14:00, 2026-10-11)

- **BUILD 0031 INSTALLED** (~/.satl/satl and the word satl), commit c57f59b2 on milestones-install-and-no-console-handover,
  uploaded to repo-9. BUILD 0030 = commit 20119d5f, repo-8. ./check.sh on 0030: 1349 passed, 0 failed (0031 changed only
  build_stack.py, 050-build.mk, commit_and_upload.sh and a help page, none of which check.sh runs).
- **GITHUB'S MAIN IS BUILD 0030 (4258c2ae)** -- one snapshot commit, as on 10-09 (his rule then: *"put everything onto main, just
  erase the current main and replace it"*, *"take out the folder old_versions"*). Repo public. The 10-09 main (c55de31)
  is mirrored in ~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-11-github-main-before-replace/.
  A clone of it builds with plain make (PROGRESS.md section 1, first entry). BUILD 0031's snapshot (the review's
  fixes; scratchpad gh2/satellite, commit 5332163) was made and a fresh clone of it built (exit 0, 13:49) -- NOT
  PUSHED: at ~13:50 he said *"we still need to get the word satellite onto everything, before we do anything..."*.
- **HOW A PUSH TO MAIN IS MADE** (done once today, 4258c2ae; 0031's tested, held): `git clone --no-hardlinks` of the tree
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

## 1. Questions waiting on the author ("yes to all" works)

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
