# Questions from the document update, 2026-10-09

Five documents were rewritten on 2026-10-09 (README, PROGRESS, DESIGN, MILESTONES, NEW_MILESTONES), one
editor each, then one fresh checker each. These are the questions only you can answer. Each has the
editor's suggestion in it, so "yes to all" works. Nothing here has been decided: every one is still open
in its document.

## README.md

1. README's aims still list the .satc, .satb and .sati files you threw away on 2026-09-16 ('we threw away satc and satb in favor of all 16-bit'). I kept the bullet, marked 'a plan, never built'. Delete it? (I suggest yes.)
2. The 'Built for many threads' aim (warm threads finding work on their own) is not built: the threads start and wait. Keep it as an aim, marked 'not built yet'? (I suggest yes.)
3. VTE (the console) and lz4, simdutf, fmt and fast_float are compiled into satl, but satl --license does not list them (VTE is LGPL). Add their licences to licenses/ and to --license? (I suggest yes.)
4. The 003 tag and archive branch are only in this checkout's git; the public GitHub repository has only main. Keep README's line about them, as now worded? (I suggest yes.)
5. I added old_versions/prototype/ to 'The earlier satellites'. Keep it? (I suggest yes.)

## PROGRESS.md

6. A satellite.return inside a loop now ends the whole capsule (since break and continue, 2026-09-26). Is that your ruling? I suggest yes: it matches M20.E, where break is the word that leaves a loop.
7. number_methods.cpp has been unwired since 2026-09-16. Finish it (MILESTONES M17), not delete it? I suggest finish it: your .power() ruling waits on it and is still refused.
8. On 2026-09-22 you chose && || ! for conditions and & | ~ << >> for bits. May those 8 registry rows be marked ANSWERED with your words? I suggest yes; ! already works.
9. scroll.horizontal is OFF unless written (the first 2026-10-07 build had it on). Keep it off? I suggest yes: wide lines wrap and the console grows downward, as you asked.

## DESIGN.md

10. §3.3: a word built into satl can hold at most 16 numbers (kMaxDepth = 16), but standard 2 says no built-in limits. Keep 16 for now, since the deepest word has 6? (I suggest yes, and mark it as owed.)
11. §1.2: no config row holds the 27 nines, there is no digits, nesting or command-length row, and object_bytes_max and file_size_max_bytes are read by nothing. Keep standard 2 as written, and treat those rows as work still owed? (I suggest yes.)
12. §7: you wanted satellite.log in satellite-004/. Since M5 (2026-09-25) it goes to ~/.satl/satellite.log through arguments.log_path. Keep ~/.satl? (I suggest yes: one file a person, beside the installed satl.)
13. §6 question 2: may it be marked answered by your 2026-09-16 words 'convert every line of the program into 16-bits before a single line runs'? (I suggest yes. For now the question keeps its wording and your words are quoted under it.)
14. §11 defect 15: a string with no closing quote is refused before anything runs, but with code 13, not the 4 the defect asked for, and ERROR.md counts it fixed. Leave it at 13? (I suggest yes. The report now says the string is never closed.)

## MILESTONES.md

15. Renumber the two M34s and the two M35s? I suggest no: satl's refusals, check.sh and the help text name M35 (supertype) and M34 (access), so all of those would change too.
16. M21: drop strings/ and the 23 old satellite_string32 words, which satl never calls? I suggest yes (the other choice is folding them into the tree's tests).
17. M29: drop 'convert while running', since your design that same night converts every line before any line runs? I suggest yes.
18. M33: drop the 'jump' step, together with arguments.magic, which nothing reads? I suggest yes.
19. Leading slash: keep include(/test) meaning ./test beside the file (what include does today, 'four spellings and no probing'), and treat your 'program root first, then filesystem root' answer as being about something else? I suggest yes.
20. M20.A item 1: is 'return leaves the whole capsule with its value' (taken from your 003 programs) your ruling? I suggest yes.
21. M20.F: is 'continue starts the loop's next turn' what you meant? I suggest yes.
22. M34: keep what the build chose (access reads the name where it is now; help() lists topics; access of an undeclared name is refused)? I suggest yes.
23. M11: confirm SATELLITE_INFINITY.md Q20, that -infinity and infinity * -1 are one value? I suggest yes.
24. M17: finish number_methods.cpp rather than delete it? I suggest finish: power, max, min, abs and the rest are numbered and refused today.

## NEW_MILESTONES.md

25. A line naming a satellite word that does not exist yet, like satellite.history, is refused with 'satellite is ... immutable', which gives the wrong reason. Should it say the word is not built yet instead? I suggest yes.
26. NM-2 and NM-2a's long as-built records (about 140 lines) are also in PROGRESS.md and the help pages. Leave them whole here while this is your live plan? I suggest yes.

## Defects the editors found while checking (no document recorded them)

- (README.md) fmt IS linked into satl: nm -C ~/.satl/satl shows 150 fmt::v symbols. DOC_SURGERY's licence-gap item (GTK_AND_NO_DEPENDENCIES.md DEP-6, licenses/README.md) left 'whether fmt is linked' unchecked; its raw grep found 0. So five carried projects (vte, lz4, simdutf, fmt, fast_float) are missing from licenses/ and satl --license, not four.
- (README.md) Not a satl defect, but no document or note records it: on 2026-10-08 at 20:12 a push from this checkout moved origin/main to e63226e. That is a single root commit, 'satellite 004 revision 09', by jsadlier, 2026-10-08 20:02 (reflog: 'update by push'). github.com/satellitemadness1/satellite is PUBLIC (API: private=false, created 2026-10-08T23:49Z, one branch 'main', no tags). HEAD is still 69b1d93. The lead's notes say the repository has been private and frozen with no git since 2026-10-05, so this is worth confirming with the author.
- (PROGRESS.md) satellite.variable.string(x) (word 1 6 1 18) is refused S210 'satellite.variable.string has no library built for it yet', exit 14, even when given a string. Its built-in function answers a string (satellite-numbers/words/1.6.1.18.cpp:19-26), but nothing reaches it. Probes p04c and p04e. Not in ERRORS4.
- (PROGRESS.md) S210's paragraph and its messages still speak of libraries ('the library behind it does not exist yet', 'has no library built for it yet'), though since 2026-10-07 no word is a library (machine/s_codes.hpp S210; probe p04b).
- (PROGRESS.md) arguments.scroll.horizontal is OFF unless written (arguments/arguments.cpp:328-330). check.sh:2857-2858's comment still says both scroll rows are 'true until a file's top line or the prompt says otherwise' (DOC_SURGERY records the GTK-17 text, not this).
- (PROGRESS.md) satellite/structured-library.cpp:506-518 still has the 2026-09-16 comment 'THE CHECKER IS OWED, AND IT IS THE NEXT PIECE ... four of check.sh's 32 now fail'. The checker exists: --debug prints 'program(checked)'.
- (PROGRESS.md) satellite/threads/startup_threads.hpp:13 says 'Nothing hands them work yet: that is PLAN M7'. In fact bytecode/cascade_convert.cpp:15-34 submits every line of a loading program to those warm threads, one chunk each. SCRATCH.md/DOC_SURGERY.md's DESIGN check repeats the stale comment as fact ('the start-up threads start and park and nothing hands them work').
- (PROGRESS.md) REGISTRY.satellite:67-70 still says 003's DESIGN refuses or leaves open 'nineteen' tokens, while 23 rows carry QUESTION. The '-- THE WORDS (0001000000000000 up)' header (line 320) sits above the 0x0B method-token rows (115 of them), which have no header of their own.
- (PROGRESS.md) DOC_SURGERY.md's PROGRESS section has five slips. (1) 'SATC.md does not exist': it is 003's, in old_versions/second_satellite/SATC.md. (2) 'satl_file.cpp decides nothing of the kind': check_satl is still at satl_file.cpp:139-158, uncalled. (3) Its leading-slash correction misses that file_calls.cpp:131-139 uses section 4's probing rule for FILE paths; only includes don't probe. (4) '210 tokens, 17 families': the eleven families hold 191; the rest are free and marker rows. (5) '89 machine codes': 84 (grep -c '=' over-counts).
- (DESIGN.md) satellite/threads/startup_threads.hpp:13 says 'Nothing hands them work yet: that is PLAN M7'. That is stale: bytecode/cascade_convert.cpp:34 has handed every file's line chunks (and every typed prompt line, satl/session.cpp:512) to the warm threads since 2026-09-16. SCRATCH.md/DOC_SURGERY.md's DESIGN entry repeated it as fact ('the start-up threads start and park and nothing hands them work').
- (DESIGN.md) satellite/structured-library.cpp:505-519 still says 'THE CHECKER IS OWED, AND IT IS THE NEXT PIECE ... four of check.sh's 32 now fail', directly above the check_program call at :536-537 that does that work. The comment is stale.
- (DESIGN.md) Three source comments cite 'DESIGN §5.6' with no '003' in front (bytecode_registry.hpp:132, bytecode_registry.cpp:378 and :896), and library_values.hpp:20 cites 'DESIGN §7.2'. This DESIGN.md has no such sections; they are 003's (old_versions/second_satellite/DESIGN.md §5.6 'The rest', §7.2 'The fix'). Every DESIGN section that 004 comments cite without '003' (§1, 1.1, 1.2, 3.2, 3.4, 4, 6, 7, 9, 12, 13) exists and was kept.
- (MILESTONES.md) A statement line 'satellite.<word>(x)' for a word that does not exist (satellite.history(x), satellite.cxx(), satellite.nonsense(x)) is refused S110 with 'satellite is the first legal object of every place in satellite.legal, and it is immutable -- nothing else may be named satellite, and it is never changed or deleted'. Inside an expression the same call says 'no capsule named history' (probes m20b_h3, m10_cxx).
- (MILESTONES.md) satellite.help/satellite.history/help_text.txt says reading arguments.history is refused S110 'there is no value here to work with'. On 0058 display(arguments.history) is S201 'arguments.history is not one of the arguments', exit 25 (probe m20b_history), although words.tsv:367 and :528 number it.
- (MILESTONES.md) satellite.help() at the prompt lists 31 topics, but satellite.help/ holds 32 topic folders: satellite.random has a folder and no line in help.txt.
- (MILESTONES.md) The numbered number methods refuse two different ways: n.power(3) is S210 not_built_yet, while n.abs, n.floor, n.digits and n.sqrt(4) are S110 'not one of its methods'. All are numbered 1 6 4 n (probes m17_ref_1..6).
- (MILESTONES.md) satellite.time() is S210 (exit 14) but satellite.time.now is S110 'there is no value here to work with' (exit 13); both words are numbered 1 9 n.
- (MILESTONES.md) make_support/055-cpus.mk folds btver1 into amdfam10 and c86-4g-m8 into c86-4g-m7, but the same clang 24 (built 2026-09-20) gives btver1 __SSSE3__ beyond amdfam10 and c86-4g-m8 __SHSTK__ beyond c86-4g-m7. So the '54 distinct sets' count is wrong (a reader re-derived 56). No build is wrong: those chips run the smaller build.
- (MILESTONES.md) build/cpu/<processor>/ is stale: build/cpu/haswell/satl --version is REVISION 08 BUILD 0024 (2026-09-23), with 62 .so libraries beside it that nothing reads.
- (MILESTONES.md) S725 THREAD_CANNOT_START and S726 THREAD_STOPPED have no test fixture and no check.sh row; the locked-file sharing result (1,000 of 1,000 lines) is in no test either.
- (MILESTONES.md) include_shape.cpp:148-170 follows 'four spellings and no probing' (written 2026-09-16 16:19). The decisions-table ruling 'program root first, then filesystem root', recorded 2026-09-17 00:05, was never applied to include.
- (NEW_MILESTONES.md) A line naming a satellite word with no words.tsv row is refused S110 with 'satellite is the first legal object of every place in satellite.legal, and it is immutable -- nothing else may be named satellite, and it is never changed or deleted', which is the wrong reason. Seen with bare satellite.history (p21a) and satellite.history(x) (p21b). Also, satellite.console.display(satellite.legal) is refused S201 'legal has no satellite.variable line declaring it', naming 'legal' alone (p16). DOC_SURGERY quotes this sentence as current behaviour but does not call it wrong.
- (NEW_MILESTONES.md) satellite/machine/thread_stop.hpp:25-29, the comment on program_quit(), says 'Set once, never cleared'. But display_overrun_let_go() (display/printing_satellite.cpp:844-845) clears it when the display's own overrun set it.

## Your answer, 2026-10-09: "looks good" -- every suggestion above is now your ruling

Each ruling is written into its document, beside the question, as "Ruled 2026-10-09". The work the rulings call
for is NOT done yet; it is listed here so none of it is lost.

### Work the rulings call for

- (README.md) Q3: add licenses/vte/, licenses/lz4/, licenses/simdutf/, licenses/fmt/ and licenses/fast_float/, each with a license.txt taken from its tarball in vendor/new/. VTE: its COPYING.README says which of COPYING.LGPL3, COPYING.GPL3 and COPYING.XTERM apply. lz4: lib/LICENSE (the library's text), not the top-level LICENSE or programs/COPYING (GPL, for its tools). simdutf: LICENSE-MIT and LICENSE-APACHE. fmt: LICENSE. fast_float: LICENSE-MIT, LICENSE-APACHE and LICENSE-BOOST. The wildcard licenses/*/license.txt in make_support/040-sources.mk then compiles them into satl --license through satellite/licenses/make_license_data.py. The counts in --version and --license are computed, not typed (satellite/version/title_lines.hpp:107, satellite/licenses/licenses.cpp:203).
- (README.md) Q3: regenerate licenses/THIRD-PARTY-NOTICES.txt with licenses/make_notices.sh, and add the five projects to licenses/README.md.
- (README.md) Q3: once that is built, update README.md: '27 licences ... the 26 other projects' becomes 32 and 31, and 'are not in that list yet' is no longer true.
- (README.md) Q1, Q2, Q4 and Q5 call for no code, help-text, registry or file work. Q1's deletion was an edit to README itself, and it is done in the copy.
- (PROGRESS.md) Q7: finish satellite/bytecode/number_methods.cpp/.hpp (MILESTONES.md M17): list it in make_support/040-sources.mk (it lists bytecode sources by name), and add the branch in one_operand (satellite/bytecode/expression.cpp) that reads name.name( off a declared number, so n.power(10) and the other 1 6 4 n methods answer instead of S210 or S110.
- (PROGRESS.md) Q7, M17's two gaps: add clamp (numbered 1 6 4 5, missing from number_methods.cpp), and give bytes a number in words/words_004.tsv (it is in the file but has no number; next free is 1 6 4 21).
- (PROGRESS.md) Q8: in REGISTRY.satellite, mark the eight rows ANSWERED with his 2026-09-22 words ('let's use c symbols as much as we can for this then, and also allow .and() .or() like we allow .power()'). They are lines 263-267 and 269-271: and_token, or_token, not_token, bit_and_token, bit_or_token, bit_not_token, shift_left_token, shift_right_token. Leave the ~ (home) literal row alone. Then regenerate satellite/bytecode/token_codes.hpp, which copies each row's text.
- (PROGRESS.md) Q8: REGISTRY.satellite:67-70 still says 'nineteen' rows are QUESTION. Once the eight are marked, 15 token rows carry QUESTION (23 today).
- (PROGRESS.md) Q8: change the refusal kNotReadToTheEnd in satellite/bytecode/program_walk.cpp:85-87. It says '& | << >> !! are undecided', but all five are decided: !! on 2026-09-09, the rest on 2026-09-22. SCRATCH.md/ERRORS4/039 already records the sentence as misleading.
- (PROGRESS.md) Q8: if that sentence changes, also change what quotes or expects it: satellite.help/binary_and_hexadecimal/help_text.txt:39-41, satellite/satellite_test/programs/manifest.tsv:60, satellite/satellite_test/test_programs.cpp:2725 and :2736, and satellite/satellite_test/programs/full/full_numbers_25.satl:5.
- (PROGRESS.md) Q8 (owed since 2026-09-22, not new): build && || & | ~ << >> and .and() .or() .xor(). Only ! works today (MILESTONES.md M19). One question must be settled first, and satellite.help/binary_and_hexadecimal/help_text.txt leaves it open: what a bit operator does with a binary's sign (-b0101).
- (PROGRESS.md) Q9: the comment at check.sh:2858 and the expectation label at check.sh:2890 still say both scroll rows are 'true unless written'. scroll.horizontal is off unless written.
- (PROGRESS.md) Q9: two help pages still say sideways scrolling is on unless written. satellite.help/satellite.console/help_text.txt:76-81 says 'it is 2048 characters wide, wraps nothing ... Each is on unless ... say false'. satellite.help/satellite.window/help_text.txt:31-33 says 'true unless config.ini or a line at the top of the file says false'. Both should say scroll.horizontal is off unless written.
- (DESIGN.md) Q10 (owed, no work now): take away the fixed 16 so a word may have any number of numbers. That is kMaxDepth = 16 in satellite-numbers/number_row.hpp:22 and the start-up refusal of a deeper word in satellite-numbers/call_number.satellite.cpp:93. Note that satellite/bytecode/word_codes.hpp:35 has its own kMaxDepth = 7 (and 255 at most for each number) for a word's 16-bit key; the way past it is word_number_token, which nothing writes yet.
- (DESIGN.md) Q11 (owed): add rows to satellite/config/satellite_config.hpp and satellite/arguments/argument_settings.hpp: one that holds the 27-nines default (999,999,999,999,999,999,999,999,999), and maxima for a whole number's digits, for nesting and for the length of a command. Give each its help line in satellite.help/arguments/help_text.txt and its row in SATELLITE_ARGUMENTS.md's settings table.
- (DESIGN.md) Q11 (owed): make something read arguments.object_bytes_max and arguments.file_size_max_bytes (satellite/config/satellite_config.hpp:93 and :102, satellite/arguments/argument_settings.hpp:81-82). Nothing reads them today.
- (DESIGN.md) Q12, Q13 and Q14 need no code work. arguments.log_path already says ~/.satl/satellite.log (satellite/config/satellite_config.hpp, satellite/machine/satellite_log.hpp). Every line is already converted before any line runs (satellite/bytecode/cascade_convert.hpp). check.sh:6686-6689 already holds an unclosed string at exit 13.
- (MILESTONES.md) Q16: delete strings/ and the 23 satellite_string32 word files satellite-numbers/words/1.6.1.0.cpp to 1.6.1.22.cpp. Also remove their 23 rows in satellite-numbers/word_table.cpp (lines 12-34) and their lines in make_support/040-sources.mk (WORD_SOURCES, and strings/string_method.hpp in the headers), make_support/030-directories.mk (STRINGS32) and make_support/065-tests.mk (make test's check_strings.py and check_string_methods.py, and the string_methods, string_cases and string_race targets).
- (MILESTONES.md) Q16: before touching the 23 rows 1 6 1 0 to 1 6 1 22 in words/words.tsv, check what reads them. They are frozen numbers, and the string methods M16 built run through satellite/bytecode/string_calls.cpp.
- (MILESTONES.md) Q16: fix two comments that name strings/ or M21: satellite-numbers/directory_words.hpp:37 ('MILESTONES M21's question') and check.sh:6864 (strings/check_string_methods.py).
- (MILESTONES.md) Q16: once strings/ is gone, remove #22 and #23 from MILESTONES.md M27 and ERROR.md section 3. Both are defects in the strings/ test harness.
- (MILESTONES.md) Q18: take out the arguments.magic row: satellite/config/satellite_config.hpp:59-68, satellite/arguments/argument_settings.hpp:75, satellite/arguments/arguments_cases.cpp:43, satellite.help/arguments/help_text.txt:15 and :17.
- (MILESTONES.md) Q18: first copy the author's words in satellite/config/satellite_config.hpp:59-60 into a document ('the magic is step, so it just jumps at that step, preparing every 5 lines by default'). No document holds them.
- (MILESTONES.md) Q18: check.sh uses arguments.magic as its sample number setting (lines 4665, 5343-5359, 5375), so move those rows to another setting. SATELLITE_ARGUMENTS.md Part 4G (lines 902 and 906) uses it in examples too.
- (MILESTONES.md) Q18: when the row is gone, update MILESTONES.md M34 (--rebuild) item 3 (line 976). It counts fourteen number rows, including arguments.magic (5), and its bit-pack option names magic.
- (MILESTONES.md) Q24: finish satellite/bytecode/number_methods.cpp/.hpp and wire it in. It is in no make file (make_support/040-sources.mk). Then n.power(3), n.abs, n.max, n.min and the rest of 1 6 4 n will answer. M17's two gaps go with it: clamp (1 6 4 5) is not in the file, and bytes has no row in words/words.tsv (next free 1 6 4 21).
- (MILESTONES.md) Q24: then change what expects today's refusals: check.sh:407-417 and :7306 (n.power), satellite/satellite_test/test_programs.cpp:2513-2517 (full_numbers_13.satl expects n.max(3) to be refused, S210), and satellite.help/satellite.variable.number/help_text.txt:35.
- (MILESTONES.md) Q23: when SATELLITE_INFINITY.md INF-3 is built (* by a number), infinity * -1 must give the same value as -infinity. Today it is refused, S210.
- (MILESTONES.md) Q22: satellite.help/satellite.help/help_text.txt:31-36 says satellite.access(a_name) will read a register of every name in use. Under the ruling, access reads the name where it is now, so reword that clause.
- (NEW_MILESTONES.md) Q25: change the refusal for a line naming a satellite word that does not exist yet (bare satellite.history and satellite.history(x), probes p21a/p21b) so it says the word is not built yet. Today it gives kSatelliteIsImmutable from satellite/bytecode/satellite_legal.cpp:10 (S110, exit 13). The ruling does not say which code or exit to use.
- (NEW_MILESTONES.md) Q25: add check.sh rows for the new refusal. The rows for naming something satellite (S202, exit 26) and for satellite.delete(satellite) (S110) keep the immutable sentence: check.sh:1315-1316 and satellite/satellite_test/test_programs.cpp full_structure_19 pin those.
- (NEW_MILESTONES.md) Q25: rewrite satellite.help/satellite.history/help_text.txt lines 8-10 to match the new refusal, then run the page's examples. Those lines already disagree with build 0058: they say bare satellite.history is refused S201 and satellite.history(x) S110 'no capsule named history', but build 0058 gives the immutable sentence for both.

### Other documents that hold the same questions (to update in the next batches)

- SCRATCH.md/DOC_QUESTIONS_2026-10-09.md, '## README.md', questions 1-5: mark each one answered 2026-10-09 ('D looks good'). Its opening line, 'Nothing here has been decided: every one is still open in its document', is no longer true.
- SCRATCH.md/DOC_SURGERY.md, '### README.md -- UPDATE (high)', the Edit plan at line 1022. Item (3), 'either mark .satc/.satb/.sati ... or delete -- the author decides', is ruled: delete (Q1). Item (4), 'add not yet: PLAN M7 or keep as an aim', is ruled: keep it as an aim, marked not built yet (Q2). The same two points are in its 'Wrong today' list at lines 1017-1018.
- GTK_AND_NO_DEPENDENCIES.md, DEP-6 (from line 617) and the DEP-6 row of Table A (line 103, 'the licence of every one of the 24'): record Q3. VTE, lz4, simdutf, fmt and fast_float get their licences in licenses/ and in satl --license. VTE is LGPL, so DEP-6's relink duty covers it too.
- licenses/README.md (for example '## Two things this folder does not yet settle', line 165): record Q3. Five carried projects (vte, lz4, simdutf, fmt, fast_float) have no folder yet, and each is to get one.
- SCRATCH.md/DOC_SURGERY.md, '### GTK_AND_NO_DEPENDENCIES.md -- GUT (medium)', line 302 (the DEP-6 gap, 'a live DEP-6 item nobody has written down'): record Q3.
- MILESTONES.md, M20.A, 'FOR YOU, ON THIS ONE', item 1 (lines 477-482): 'Whether that is your ruling -- return leaves the whole capsule, not only the loop ... -- is still yours to say'. Same as Q6. MILESTONES's own question 20 also covers the value a return hands back.
- MILESTONES.md, M17 (lines 407-418): 'Finish it or delete it. Not a fourth session. (... which one is the author's.)'. Same as Q7, and MILESTONES's own question 24.
- MILESTONES.md, M19 (lines 434-440) and M24 (lines 696-701): the registry rows of && || ! & | ~ << >> 'still say QUESTION' (Q8).
- REGISTRY.satellite, the LOGIC AND BITS family (lines 263-271) and the 'ROWS MARKED QUESTION ARE NOT SETTLED' header (lines 67-70) (Q8).
- PLAN.md, 'Decisions waiting on you, gathered from the rework' (lines 1104-1105): what & | ^ << >> !! ~ mean, 'the nineteen QUESTION rows, MILESTONES M24' (Q8).
- ERROR.md, the 'n = 1 & 2' entry (lines 281-282): 'they are the rows REGISTRY.satellite marks QUESTION, because §13 never decided what they mean' (Q8).
- GTK_AND_NO_DEPENDENCIES.md, GTK-17 'AS BUILT 2026-10-07', the 'The rows' bullet (lines 2371-2372): '(argument_settings.hpp, flags, true unless written)' (Q9).
- SATELLITE_ARGUMENTS.md, Part 4G settings table, the scroll.horizontal row (line 885). The scroll.vertical row says 'on unless written'; the horizontal row does not give its default (Q9).
- README.md, the revision 10 paragraph (lines 50-53). It does not say which scroll direction is on by default. It can now say scroll.horizontal is off unless written (Q9).
- SCRATCH.md/DOC_QUESTIONS_2026-10-09.md, '## PROGRESS.md', questions 6-9. The file still opens with 'Nothing here has been decided'. Mark these four ruled on 2026-10-09.
- MILESTONES.md, M5 'Chosen here, the author's to overrule', 2nd bullet 'the file is 003's, shared' (lines 144-145): Q12, the log stays at ~/.satl/satellite.log. The 1st bullet (every report goes in, not only warnings) stays his to overrule.
- ERROR.md, '## Fixed', the paragraph '1, 13, 14 and 15 -- checked 2026-09-25', item 15 (lines 129-131), and section '1. The prototype runner' item 15 (line 41): Q14, the refusal stays at code 13 (the defect asked for 4).
- SCRATCH.md/DOC_SUGERY.md is misspelled here -- see next line
- SCRATCH.md/DOC_SURGERY.md, its DESIGN.md section: line 770 (§3.3 kMaxDepth 'for the author to rule on') and line 793 ('his to rule on'), both Q10; lines 762 and 773 (§1.2 no 27-nines row, §4 no digits row), Q11; line 775 (§6 question 2), Q13; line 776 (§7 the log path), Q12.
- SATELLITE_ARGUMENTS.md, the settings table, line 879 (object_bytes_max, file_size_max_bytes 'by nothing yet'): Q11, reading them is work still owed.
- Related, but ruled separately: MILESTONES.md M29 'converting ahead WHILE running' (lines 761-772) is Q17. It rests on the same 2026-09-16 words as Q13, and the two rulings agree.
- SATELLITE_INFINITY.md: the Part 12 table, row Q20 (line 1332), and Part 3 'What it holds' (lines 411-418, 'Q20 asks him to confirm it'). Q23.
- PROGRESS.md: section 6.7 item 1 (line 681, number_methods.cpp 'IS WRITTEN AND NOT WIRED') and section 6.8 item 2 (line 783, 'Either finish it or delete it'). Q24, the same question as DOC_QUESTIONS Q7.
- PROGRESS.md: section 1, the satellite.statement.for row (line 179: 'Open, and the author's (M20.A)' ... 'that the author ruled it is not recorded'). Q20, the same question as DOC_QUESTIONS Q6.
- PROGRESS.md: section 4 'Open decisions (the author's)', the leading-slash item (lines 372-379), and section 6.5's owed list, item 1 (line 593). Q19.
- SATELLITE_FILE_OPERATIONS.md: Part 2 'what the tree already has' (lines 127-128) calls 'program root first, then filesystem root' the author's path rule for includes. The ruling says it is not about include. Q19.
- SCRATCH.md/DOC_SURGERY.md: line 980, its leading-slash correction (Q19), and line 378, 'leaves renumbering to the author' (Q15).
- PROGRESS.md: section 5 'Next' (lines 396-399) says the 23 string words are 'Still owed' and strings/ has not moved. Q16.
- PLAN.md: M0.6 (line 226) gives MILESTONES M21 as the reason the listing order is still the bytes'. M21 is now dropped. Q16.
- SCRATCH.md/ERRORS2.md: lines 25-26, 'M21 moves the string libraries off satellite_string32' (a scratch note). Q16.
- MILESTONES.md, '# Found 2026-10-07 to 2026-10-09', the bullet at lines 1320-1325 ('A line that is only satellite.<word>(x), for a word that does not exist ... is refused (S110) with a sentence about satellite itself'). This is the same defect as question 25. That list says 'what to do about each is the author's', so it should record the same ruling.
- SCRATCH.md/DOC_QUESTIONS_2026-10-09.md: questions 25 and 26 under '## NEW_MILESTONES.md' (lines 46-47), and the two defect bullets about the same refusal (line 63, '(MILESTONES.md) A statement line satellite.<word>(x)...'; line 72, '(NEW_MILESTONES.md) A line naming a satellite word with no words.tsv row...'). Its line 5 still says nothing there is decided.
- SCRATCH.md/DOC_SURGERY.md, '### NEW_MILESTONES.md -- UPDATE (high)': its **Why** paragraph (line 805) says NM-2's and NM-2a's AS BUILT blocks 'can be compressed', which question 26's ruling overrules while this is his live plan. Its **Edit plan** item 4 (line 817) calls the satellite.history refusal (question 25) 'a help-page fix'.
