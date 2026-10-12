# NM-8 inventory, 2026-10-11 -- how each family dispatches today, and what numbering it needs

Seven read-only readers (workflow nm8-inventory-2026-10-11, run wf_9550492d-1a9); one row per word in inventory.tsv.

## threads-programs

HOW IT DISPATCHES TODAY. No number runs for this family.

Walker (expression.cpp call_method):
- The period check is at :480-487. It works out its_number INLINE, for strings and lists only. It does NOT call method_periods.hpp's number_a_method_runs, so that rule has a second copy here.
- After the check, the thread arm (:685-693) calls thread_calls.cpp:329 call_thread_method: start :346, stop :348, join and wait :352.
- The program arm (:698-706) calls program_calls.cpp:461 call_program_method: start :480, ok :509, error :511, end/exit/quit/shutdown :513, pass :515, join/code/return :517.

Checker (program_check.cpp):
- method_on_a_name :1218 asks number_a_method_runs. That answers 0 for kind thread (18) and kind program (19). type_shape.hpp:122-124 maps 1 6 13 to thread, and both 1 6 23 and 1 6 24 to program.
- method_judged then checks a thread at :906-918 and a program or bash line at :923-948.

What a program sees today (probed on BUILD 0031): t.satellite.start() and p.satellite.join(), and every other word of the family, are S110 '... is written t.start -- a thread's words are not under satellite yet'. The checker says it for a declared name and for a list<satellite.variable.thread> item. The walker says it for a later link of a chain.

STEP 1, THE NUMBERS, IS ALREADY DONE. This is unlike the map, file, number and window. words/words.tsv already has:
- 1 6 13 1-4: start and join (003's), stop and wait (004's).
- 1 6 23 1-12 and 1 6 24 1-12 (2026-10-01).
- word_codes.hpp codes 4275/4276/4531/4532, 4545-4556 and 4559-4570, and fixed_code works for them.
Nothing needs a new number. Steps 2 and 3 are missing.

STEP 2: CODE INTO NUMBER FILES, run by number, as strings and lists were.
- Files: satellite-numbers/words/1.6.13.{1-4}.cpp, 1.6.23.{1-12}.cpp and 1.6.24.{1-12}.cpp, 28 in all. The alias files and the bash files can be describe_ functions that point at one shared body.
- Each file also needs a declaration in word_table.hpp and a row in word_table.cpp, named exactly as words.tsv spells it, with its '()', '(how)' or '(text)'. Add each to WORD_SOURCES in make_support/040-sources.mk.
- Append a new scenario to the end of Scenarios in number_row.hpp (e.g. thread_call and program_call). StringCall's shape fits as it is: home is always nullptr, and no TypeShape is needed.
- function_table.cpp:30 must keep the new rows out of the by-name view, as it does list_call rows (the list step's review). Today satellite.variable.program.start() written as a call is S210 'has no library built for it yet' (probed).
- thread_calls.cpp: add thread_number_of(token::Code method) (start→1 6 13 1, join→2, stop→3, wait→4) with a static_assert like every_list_word_numbered, and run_thread_number, which does call_thread_method's refusals (:333-345) first.
- program_calls.cpp: add program_number_of(token::Code method) (start→1, ok→2, error→3, join→4, code→5, return→6, end→8, exit→9, quit→10, shutdown→11, pass→12) and run_program_number, which does the refusals at :465-479 and the S741 check at :496-508 first.
- Keep the exact signature 'token::Code X(token::Code method)'. utility/satellite_spelling.py's numbered() parses it.
- The bodies are in anonymous namespaces today and must move into a parts namespace, like string_method_parts and container_parts.
  - Thread: start, join, reap, Launch, run_the_thread and Running/running(). close_every_thread still needs running().
  - Program: start, join, end, pass, never_ran, close_the_input and a_string. They depend on remember, forget, keep_watcher and join_finished_watchers (program_stop/program_watch), programs_watched and the console hand-over.

STEP 3: THE SPELLING.
- method_periods.hpp:40 number_a_method_runs: add the thread and program kinds. Also add them to the walker's inline copy at expression.cpp:480-482.
- method_periods.hpp:51 kind_a_method_answers needs these answers, so chain_periods can judge a later link before anything runs:
  - thread: start and stop answer the thread; join and wait answer how_many_kinds.
  - program: start answers the program; ok and pass a boolean; error a string; join, code, return, end, exit, quit and shutdown a number.
- period_written_wrong :116 has a 'has no X' branch for strings and lists only, so a thread and a program need it too. Today t.satellite.size says 'is written t.size', and t.size is then refused again. Its tail at :118, 'a string's and a list's are', grows.
- Sentences to change:
  - thread_methods_are() and program_methods_are().
  - Every hand-built 'name + ".start()"' or '"." + spelling' sentence: thread_calls.cpp :142,173,190-191,197,209,217,254,334,338 and program_calls.cpp :61-62,246,457-458,472,489-490,502-503.
  - The S720-S726 and S740-S742 texts in machine/s_codes.hpp :365-424.
  - The S742 notes in program_stop.cpp:272,275 ('write p.join() -- or .code() or .return()').
- Converter (utility/satellite_spelling.py): add 'thread' and 'program' to UNDER. Its answer_of for thread and program lacks start (answers the receiver's kind), stop, program join (number) and pass (bool). Without them, u.start().wait() and his journal.satl:12 one_second.start().join() are not converted.
- Expectations to update:
  - check.sh :6095, 6125, 6128, 6164 and 6189, plus the description lines.
  - satellite.test's manifest.tsv:153 phrase 'p.start(loud) -- start() takes nothing, or'. Then make_test_programs.py regenerates test_programs.cpp, which check.sh compares byte for byte.
- No satellite.help page exists for thread, program or bash. Only satellite.time's '.stop()' prose and satellite.delete's prose mention them.

WHAT IS HARDER THAN STRINGS AND LISTS:
(a) Program and bash are ONE Kind (satelliteObject::program) with TWO number sets. A program name may hold a bash line and a bash name a program (program_calls.hpp:70-71), so which number runs is only known from the value's satellite_program::bash at run time. The checker only needs 'has a number', which is the same for both.
(b) start() and start(how) are two numbers on one token (1 6 23 1 and 7), chosen by the argument count.
(c) The aliases are separate tokens with separate numbers that run one body: wait=join, join=code=return, end=exit=quit=shutdown. Strings folded their aliases in the lexer (to_number→number, and 1.6.1.13 kept a file that runs nothing). Here the tokens were split on purpose (the review of steps 2-5), so that spacesuit names code, return, quit and shutdown stay separate names.
(d) Shared spellings:
- join is also a list's, already .satellite.join.
- ok and error are also a file's (1.6.2.8/10); ok is also a pointer's .ok() and a window's .ok.
- Those keep '.' until their own types are numbered, so one word takes two periods depending on the receiver.
- Spacesuit fields and capsules named end, exit, code, quit, shutdown or return (tests/program_end_names.satl) keep '.' for good (his C).
- Receivers the converter cannot see: untyped list items (workers[1].join()) and multiples need the walker's recording switch (not in satl now) or hand edits.
(e) Side effects come before a run-time refusal. chain_periods stops after t.start() today, so t.start().satellite.join() was refused only as it ran, after the thread had really started (probed: the line before it printed, then 'satl(run)' S110). Strings and lists were pure values, so this never mattered for them.
(f) Order-sensitive state: a process-wide registry, thread_local stop_of_this_thread, object-lock waits (let_go_while_waiting, start_waiting_for, take_back_after_waiting, S728), 003's join-once rule (D5/D9), and the program watcher with the console lock. Moving the bodies must keep every order.
(g) No method needs the name's slot (home), and every one needs its brackets.
(h) Speed should stay within his 5% bar, since the time is pthread_create and fork/exec, but it was not measured.
(i) satellite.thread.new (1 23 1) is already a satellite word; nothing to do there.
(j) His newest program already expects threads under satellite: infinity_data3/infinity_data_main.satl:303,308,426,431 writes pool[k].satellite.start() and .satellite.join() on a list<satellite.variable.thread>. That is refused today (probed with my own copy of the same shape).

UNKNOWNS: 1. Whether a bash line should run its own numbers (1.6.24.n) or the program's (1.6.23.n), with the 1.6.24 rows as second names like 1.6.1.13 to_number. I found no ruling on this. STEP 5 made a bash line "a program".
2. Whether start("hide") should run 1.6.23.7 (start(how)) or the same number as start().
3. Whether each alias (wait; code and return; exit, quit and shutdown) should run its own number with a shared body, or map to its main word's number with the alias file left empty. Both choices fit some part of the existing precedent.
4. changes_receiver: I set it true when the call changes the shared thread or program object (started, joined, stopped, input), and false for ok and error. None of them replaces the value held under the name.
5. The use counts are rough regex counts. Tree and his counts are taken on names declared satellite.variable.thread, program or bash (including capsule parameters, for headers and list items), plus a hand tally from check.sh. A bash line held by a capsule parameter typed program is counted under program. Bare counts include files, pointers, windows and lists. I did not separate check.sh's python (sys.exit, p.wait()) from satellite text beyond reading the lines.
6. Speed against his 5% bar was not measured.
7. I did not run any of his programs. His infinity_data3 lines were confirmed refused only through my own copy of the same shape.
8. satellite/satellite_test/test_programs.cpp holds about 57 start and 49 join, but it is generated from programs/, which is already counted.

## containers

TODAY. Walker: expression.cpp call_method (376). its_number (480-482) is set only for a string or a list, so an index gets 0, and a `.satellite.` written on one is refused naming the `.` (483). The container branch (605) sends an index to container_calls.cpp call_container_method (525). That function refuses a wrong count or missing brackets. It refuses the position words (540: append insert remove_at truncate index_of search) and index_refuses (550: sum max min join reserve by_name by_value). Then it runs a switch: clear, remove(k), remove_first and remove_last change the map (586-642); size, empty, first/last (keys), keys/values, contains (hash), sort and reverse answer something new.

On the arguments variable straight on its name, contains and remove pass their key through an_argument_key first (609-612).

get and set have NO TOKEN. one_operand matches them by NAME (1617-1652), only straight on a name that holds an index and only after a plain `.`.

Checker: method_on_a_name (program_check.cpp:1218). kind_of_type_word gives index for 1 4 1, 1 4 5 and 1 6 21, and number_a_method_runs(index) is 0 (method_periods.hpp:40). method_judged's container block is at 949. get/set are allowed by name at 1780 (map, index or multiple only). The bare `arguments` refuses the changing words at 1863. chain_periods follows kind_a_method_answers (method_periods.hpp:74-81).

MULTIPLE (1 4 6) has no words of its own (type_shape.hpp: 'multiple IS NOT A CONTAINER'). The checker never judges its period, because its kind is how_many_kinds; the walker judges by the value it holds. Probed: on a held list, m.satellite.size runs and m.size is refused at run. Nothing to number: it follows the map.

INFO (1 6 22) is a LIST of maps. Its own words are a list's, already 1 4 2 n and written .satellite. (probed: i.satellite.size). Its items are maps (i[1].size today). Their keys (name, size, size_type, line_count, permissions, type, bytes, files, sub, complete, approximate, owner, created, modified) are DATA, read only by brackets: i[1]["name"] works, i[1].name is S201 (probed). Nothing of its own to number.

ARGUMENTS (1 6 21) is an index value. Every map word is its method, and its rows are data (see the methods list).

NUMBERING NEEDED, as lists were done:
(a) Words. 1 4 1 already has 1 set(k,v), 2 get(k), 3 has(k) (this row serves contains, contain and has), 4 size, 5 empty, 6 clear, 7 remove(k), 8 keys and 9 values. 10 search(pattern) is 003's word, and 004 refuses .search on a map, so it would stay a number with no code, like list 1 4 2 4-7. Six words need new numbers: first, last, sort(), reverse, remove_first() and remove_last(), as 1 4 1 11-16. They are APPENDED to the end of words/words_004.tsv: words.tsv is generated by make_words.py, check.sh compares it byte for byte, and a code is 4097 + its row, so rows are only ever appended. satellite.container.index (1 4 5) needs no rows of its own: a value does not know which of the two words declared it (is_an_index_word), so the map's numbers serve it, the arguments variable and info items too.
(b) Files. 15 files satellite-numbers/words/1.4.1.<n>.cpp, each with describe_1_4_1_n. Each fills a new Scenarios field appended last in number_row.hpp (map_call; the ListCall signature fits). The code moves whole out of call_container_method's switch; get and set move out of expression.cpp's by-name block (get is index_into, set is write_through_index). Rows go into word_table.cpp and word_table.hpp, and names into WORD_SOURCES in make_support/040-sources.mk. function_table.cpp:30 must hide a map_call row from calls by name, as it hides list_call.
(c) container_calls.cpp: map_number_of(method), with a static_assert like every_list_word_numbered, and run_map_number. call_container_method keeps only an index's refusals.
(d) method_periods.hpp: number_a_method_runs gets index -> map_number_of. kind_a_method_answers' index case must be FIXED: reverse answers a list, first and last a key. period_written_wrong's 'has no X' sentence must include a map, so m.satellite.append is told a map has no append rather than sent to write m.append.
(e) expression.cpp: its_number takes an index; the container branch runs run_map_number; the arguments key respelling (609) stays in front.
(f) get/set need TOKENS before `.satellite.get` can lex. That means two [METHOD] rows in REGISTRY.satellite plus make_token_codes.py (kTokenCount 202 -> 204), and container_arity get 1, set 2. The by-name blocks (expression.cpp:1617, program_check.cpp:1780) go away. main_arguments.cpp:355 must still keep them from being row writes. Then .get would work on items, chains and multiples too.
(g) The checker follows automatically through method_on_a_name, chain_periods, item_periods (map<k, map<..>> and list<map<..>> by shape) and the literal path. One exception: program_check.cpp:1864 tests token::method_token, and must take a_method_period(), or arguments.satellite.clear() slips past it.
(h) utility/satellite_spelling.py. UNDER gains 'map' and 'args'. The filter in a_period must take the map's set. after_a_period skips METHODS on args. answer_of('map') is wrong for first/last (it should give the key, kind[1]) and for reverse (it should give ('list', kind[1])). get and set must join METHODS.
My scratch copy, run dry (no tree file touched), found: tree 203 known places in 13 files (check.sh 53, full_text_5 51, loss_chain 35, map_loss 28, map help 11, speed_map_ops 9, multiple_loss 6, ...) plus 70 it could not work out; his programs 127 in 10 files (get 37, has 37, set 28, contains 14, size 6, keys 2, remove 2, clear 1) plus 1; arguments receivers tree 12, his 12.
After the programs: check.sh's sentences, satellite.test's programs (test_programs.cpp is regenerated), the satellite.container.map, satellite.container and satellite.access help pages, SATELLITE_ARGUMENTS.md, and the what_containers_have() and index_refuses() sentences.
(i) Speed: speed_map_ops.satl (twin speed_objects_twins.cpp:404) is the race for his 5% bar.

HARDER THAN STRINGS AND LISTS:
- get and set are matched by name in two places and have no token.
- One value kind sits under four type words. Numbering the map numbers the arguments variable's methods in the same step (args.size becomes args.satellite.size). Those must be kept apart from its rows, which are data and start only on token::method_token. Methods on `.satellite.` and rows on `.` never collide, unless his open D puts rows under satellite too.
- The map's words share every token with the list (and most with string or file) but mean different things: first/last answer a key, sort takes a word and answers a map, reverse answers a list, contains asks the hash. So no list number can be reused.
- Existing defects meet the work: the reverse mismatch; m.append(x) and m.string pass the checker and are refused only at run; and arguments.get("program") in his 4 russian programs is refused as 'no capsule named get'.

UNKNOWNS: 1. What a map's .reverse() should answer. The walker gives the keys reversed as a list; the checker's kind table (method_periods.hpp:79) and the converter assume the map. Both spellings of m.reverse().size are refused today. His to decide before its number's file is written.
2. Whether sort on a map is one number taking nothing, "key" or "value" (my reading), or two rows, sort() and sort(by), the way 004 numbered string() and string(x).
3. Whether the arguments variable's methods run the map's 1 4 1 n (my reading: one value kind, one set of numbers) or get their own under satellite.variable.arguments. 1 6 21 has no children. 003's satellite.container.arguments 1 4 3 1-10 (length, count, keys, to_string, lines, has(k), get(k), first, last, contains(x)) is a type 004 does not have.
4. His open D of 2026-10-10: do rows go under satellite too (args.satellite.username)? That is refused today as 'satellite.username is not a word'. Rows are data and nothing here settles it.
5. Two questions about his programs. Should .get run on the arguments variable (args.satellite.get("program") meaning args["program"])? His 4 russian programs stop at the checker on arguments.get("program") today. And is arguments.first() answering the first key, "argument1", what safe_recovery.satl wants? 003's 1 4 3 8 was the first argument. I did not run his programs.
6. Tokens for get/set (the system's way) or a by-name rule in the lexer. With tokens, the tree's two spacesuit capsules named get() (official_test/loss_test/object_loss.satl:254 and satellite/satellite_test/programs/loss/loss_chain.satl:1865) should still reach call_member, but that is untested.
7. Does 1 4 1 10 search(pattern) keep its number with no code? Does satellite.container.index get rows of its own (my reading: no)?
8. 70 places in the tree, and 1 in his, have receivers the converter could not work out. Only the walker finds those, at run time, as with the 122 for strings and lists.
9. The speed effect is not measured.
10. My use counts are greps plus a static reader. They count comments, strings and check.sh's expected sentences, and the args counts grep only the names args, arguments and argz.

## files

HOW FILES DISPATCH TODAY. A file value is satelliteObject::file, a FileHandle (shared_ptr<satellite_file>). It answers 26 words. Every one is a method TOKEN from method_code_of: 24 registry spellings plus the aliases contain and has. None is matched by name and none lacks a token. a_member_next (capsule_calls.hpp:57) gives a file no by-name members, and no file literal exists. So the file family has none of the number or map family's name-matching problem.
- Walker: expression.cpp call_method. At 480-482 the method's number is 0 for a file, so f.satellite.size is refused with "a file's words are not under satellite yet" (probed). The file branch is at 636-659. It runs lock and unlock itself (640-646), then takes ObjectHold(&file->lock, writing) and calls file_calls.cpp call_file_method (381-486). That function reads the count from file_method_arity (273-304), stops the eight questions on a handle that is not open (open_or_stop, 408-420), then runs one switch (424-481).
- Checker: program_check.cpp method_on_a_name (1218) asks number_a_method_runs. A satellite.variable.file name already has the kind file (type_shape.hpp:111), so the check gets the period right for free once the rule knows files. Item and chain links get it too, including fs[1].x on a list<file>. The file's argument counts are judged in method_judged at 1100-1121.
- A method on a word's answer, such as satellite.file.open(p).append(x), is judged only by the walker (probed: written with .satellite. it is refused when it runs, S110). f[n] is syntax (read_file_line, reached from expression.cpp:955) and needs no number.

WHAT NUMBERING THIS FAMILY NEEDS, done the way strings (1 6 1 n) and lists (1 4 2 n) were:
1. words/words_004.tsv: new rows under satellite.variable.file at the next free numbers, from 1 6 2 12. That is 17 rows: append, insert, replace, index_of, search, contains, remove_at, remove, remove_first, remove_last, truncate, clear, size, empty, first, last, save. It becomes 19 if lock and unlock are numbered, and one more if replace gets two rows. SATELLITE_FILE_OPERATIONS.md 3.3 proposed exactly 1 6 2 12-29 in that order (with replace as rows 14 and 15) and then marked the table "superseded, as built"; no row was ever added. make_words.py and then make_word_codes.py append the rows, and no existing code moves (a code is 4097 plus the row).
   Seven words already have 003 rows and codes: open 1 6 2 2 (4201), read_all 5 (4204), close 6 (4205), exists 7 (4206), ok 8 (4207), path 9 (4208), error 10 (4209). Their spellings have no brackets. 1 6 2 1 new, 3 read_line, 4 write_line(s) and 11 write(x) stay numbered and empty, per 3.4's ruling, as the list's 1 4 2 4-7 are.
2. satellite-numbers/number_row.hpp: a FileCall scenario, appended LAST in Scenarios. It takes receiver, arguments, had_parentheses, name and context. It needs no home, because as_file() const already hands back a changeable satellite_file*. function_table.cpp:30 must keep file_call rows out of rows_, as it does for list_call rows, so satellite.variable.file.read_all written as a word call is still refused.
3. 24 new files satellite-numbers/words/1.6.2.<n>.cpp (26 if lock and unlock are numbered). Each takes one case of call_file_method's switch whole, plus describe_1_6_2_<n>. Each also needs a row in word_table.cpp, a declaration in word_table.hpp and a name in WORD_SOURCES (make_support/040-sources.mk).
4. file_calls.cpp/.hpp:
   - file_number_of(token), a switch, plus a static_assert that every file word is numbered (the string_calls.cpp:148-154 pattern).
   - run_file_number, which keeps the count refusal and open_or_stop ahead of the number, as run_list_number keeps the count.
   - The anonymous helpers (text_of, number_of, a_number, a_string, a_string_always, a_line_or_stop, open_or_stop) moved into a named file_method_parts:: in the header.
   - call_file_method left holding only the refusal of a word a file does not have.
5. expression.cpp:480 adds (*live).is_file() ? file_number_of(method). The file branch runs run_file_number inside the ObjectHold.
6. method_periods.hpp:
   - number_a_method_runs adds the file case.
   - kind_a_method_answers gets a case O::file. size, index_of and search answer a number. contains, empty, ok, exists and every changing word answer a boolean. first, last, read_all, path and error answer a string. lock and unlock answer nothing. This lets the checker follow f.satellite.first.satellite.size; today f.first.size is refused only when it runs (probed).
   - period_written_wrong's "X has no Y" sentence and its "a string's and a list's are" ending should take the file.
7. Sentences: call_file_method builds what = name + "." + method on every call (file_calls.cpp:384), which needs .satellite.; satellite_object.cpp:799 also names read_all and path; check.sh rows that quote these change with them.
8. Conversion: utility/satellite_spelling.py already knows the file kind (KIND_OF_TYPE, WORD_ANSWERS for satellite.file.open/new, answer_of 'file'). It needs FILE_NUMBERED = numbered('satellite/bytecode/file_calls.cpp', 'file_number_of') added to UNDER. One defect to fix with it: its answer_of says the changing words (append, insert, replace, remove*, truncate, clear, save, close, open) answer the FILE, but the walker answers a bool (probed: f.append("a").append("b") is refused as "not built for a bool").
   - Places on a file receiver in the tree: 364 in 21 files. official_test/loss_test/file_loss.satl has 121, satellite_test loss/loss_chain.satl 112, full/full_objects_3.satl 60, check.sh 21 (its file programs are around lines 1960-2012, plus printf lines), tests/file_list.satl 18, and the rest 4 or fewer. satellite/satellite_test/test_programs.cpp is GENERATED from these programs and must be remade (make_test_programs.py).
   - His ~/code/satl: 137 places in 20 files: close 44, ok 32, error 22, read_all 17, size 9, append 8, clear 2, path 2, save 1. 4 of them are already written with .satellite.
   - No help page covers files (FO-7 is unbuilt). SATELLITE_FILE_OPERATIONS.md has about 45 places.
   - Count method: uses_tree and uses_his count uses ON A FILE RECEIVER, meaning the name's nearest earlier declaration in the same file is satellite.variable.file, or the receiver is a satellite.file.new/open(...) answer. Each word's notes also give the literal grep count.

HARDER THAN STRINGS AND LISTS:
(a) The lock. Every file call runs under ObjectHold, but lock and unlock toggle that hold, so they run before it and outside it. They are also every object's words (capsule_calls.cpp:229, program_check.cpp:1478/1727). Numbering them for a file alone splits one word across two spellings, unless objects' lock goes under satellite at the same time (infinity_data3 already writes progress.satellite.lock()). If they are left unnumbered, period_written_wrong must not tell f.satellite.lock() that "a file has no lock".
(b) replace has two shapes under one token: one number, or 3.3's two.
(c) Every token is shared: with strings and lists (already numbered), maps, windows (open, close, path, save, append, clear), programs (ok, error) and objects (ok, lock, unlock). Per-kind number switches keep them apart; the "so far whose" sentences need checking.
(d) Seven words carry 003 numbers and spellings; seventeen to nineteen need new rows.
(e) Eight questions stop the program on a handle that is not open; that guard has to stay ahead of the number.

EASIER: no word is matched by name or lacks a token. A file is a handle, so no method keeps the chain on the name (there is no changes_a_string to mirror). The speed bar should be easy: file I/O dominates, and the eager what-string at file_calls.cpp:384 is the same kind of cost the list path dropped when it got 10.7% faster.

DIRECTORY VALUES: satellite has no directory kind (satelliteObject::Kind has none), so there is nothing to number here.
- satellite.directory.change(d) answers a bool (1.18.1.cpp, expression.cpp:1876). free(d) answers a float in MB (info_calls.cpp:163). Methods on those belong to the bool and float families.
- current() (1 18 2) and exists(d) (1 18 3) have no library: S210 in a program (probed).
- list() (1 18 4), list(d) (1 18 5) and system() (1 18 6) read their names and are then refused in a program with a stale "no list type built yet (MILESTONES M14)" (expression.cpp:1879; on PROGRESS's owed list as ERRORS4/040). Once built they would answer a list, whose words are already numbered.
- satellite.info.file, satellite.info.directory and satellite.info.dir (1 30 1/2) answer a satellite.variable.info, a list of maps. i.satellite.size runs today; each entry's words (i[1].keys, .contains("size")) belong to the map family.
- The satellite.file.* words (1 8 1-6) and satellite.directory.* are already under satellite. The file words run by code compare in call_file_word (no library, per ruling R10) and need nothing for NM-8.

UNKNOWNS: For the author to decide:
1. replace on a file: one number (the string's rule, "one method, one number"), or the two rows SATELLITE_FILE_OPERATIONS.md 3.3 proposed (1 6 2 14 replace(n, x), 1 6 2 15 replace(a, b))? Two numbers would mean choosing the number from the argument's kind at run time.
2. lock and unlock: should a file's be numbered now, and should every object's lock and unlock go under satellite at the same time? His ~14:05 answer A named only .pointer() and .reference(). infinity_data3 already writes .satellite.lock() on objects.
3. Should the new rows follow 3.3's order (it yields the same numbers if added in that order), and should 003's empty rows 1 6 2 1/3/4/11 (new, read_line, write_line(s), write(x)) stay empty?
4. Who wrote infinity_data3's new spelling (done.satellite.append, done.satellite.close, x.satellite.lock())? NM-8 says he wrote it himself; SCRATCH.md/OPEN_ITEMS_2026-10-11.md says another Claude session respelled infinity_data_main.satl. So whether it reflects his own choice for files is not settled.
5. Does NM-8's "make it run those numbers" also mean turning the satellite.file.* words (1 8 x) into number files? Today they run by code compare with no library (ruling R10) and are already under satellite.

Limits of this report:
- The file-receiver counts are heuristic. They miss files reached through an object's field, a list of files, a multiple, an included file, or a word answer with nested brackets (full_objects_3.satl:205-206). A name declared twice in one file is resolved by its nearest earlier declaration.
- Nothing was built, timed or run through check.sh. The probes ran BUILD 0031 (~/.satl/satl) on scratch files only.

## window

TODAY. A window's methods are 58 method tokens. colour/color is the only alias. None is matched by NAME. The only by-name lookup after a window is the program's own widget or section name (expression.cpp:458-472 a_widget_by_name; window_widgets.cpp:452), and those stay bare under his rule. One type covers every piece: satellite.variable.window = 1 6 18 = satelliteObject::window (type_shape.hpp:116). The piece (window, button, canvas, console, menu, section, file display...) is only known at run time.

How a call runs today:
- Walker: expression.cpp:480-487 sets its_number to 0 for a window, so any `.satellite.` is refused. Then expression.cpp:712-726 calls call_window_method (window_methods.cpp:25), which asks answer_a_question (window_questions.cpp:28) for bare reads and questions first, then runs the switch.
- Shapes: the arity table is window_shapes.cpp:56-123, also_takes :125, capsule-name methods :208-232, widget chain order :161-174.
- Checker: method_on_a_name (program_check.cpp:1218) leads to method_judged's window block (:1003-1060). names_in_statement handles the widget chain, capsule names and .press (:1561-1622). The widget member chain is at :1745-1769, the widget-name pre-pass at :3376-3421.
- Words: 1 6 18 has NO child rows. satellite.window.* are 1 27 1-24 and satellite.console.new is 1 5 10, already under satellite. satellite-numbers/words/ has nothing for windows.

PROBED on the installed satl (BUILD 0030; scratch HOME, empty XDG_RUNTIME_DIR, headless):
- His line 8, main_window.satellite.section(...), is refused before the run: "main_window.satellite.section is written main_window.section -- a window's words are not under satellite yet". main_window.satellite.open() and c.satellite.display("x") on a console get the same refusal.
- His line 10 (.satellite.widgets.display.file) and plain .widgets.display.file are both refused at `.file`: ".file("a_name") makes a widget, and is written after .widgets.displays".
- satellite.window.button("x").satellite.title("t") and main_window.title("a").satellite.focus() PASS the checker. It does not follow a chain after a window word or past a window's first link; the walker refuses them when the line runs.

NUMBERING NEEDED, following strings and lists:
1. Rows appended to words/words_004.tsv under 1 6 18 (satellite.variable.window.<method>), first available. About 58 rows: words.tsv rows 559 on, codes 4655 on (lists ended at 4654). One number per token, as strings did: bare read and bracketed write in one function; colour/color one number with two spellings; append's 1 or 3 arguments, file/input's 1 or 3 and listen's 0 or 1 each inside one function.
2. A file satellite-numbers/words/1.6.18.<n>.cpp per number, holding a describe_1_6_18_<n> and the case body moved whole from window_methods.cpp / window_questions.cpp. Rows in word_table.hpp/.cpp, entries in WORD_SOURCES (make_support/040-sources.mk), and `WindowCall window_call` appended last to Scenarios in satellite-numbers/number_row.hpp. FunctionTable::operator[] must hide window_call rows as it hides list_call rows (function_table.cpp:30).
3. window_number_of(method), with a static_assert that every number exists, plus run_window_number. It keeps call_window_method's checks first (main-thread only, arity, closed handle, a doing written bare) and then calls functions.method(number)->scenarios.window_call.
4. method_periods.hpp: add the window kind to number_a_method_runs and a window case to kind_a_method_answers, and add windows to period_written_wrong's sentence. expression.cpp:480: add is_window() to its_number; expression.cpp:712: run the number. utility/satellite_spelling.py: add window to UNDER, a window case to answer_of, and every satellite.window.* word plus satellite.console.new to WORD_ANSWERS (only satellite.window.new is there now).

HARDER THAN STRINGS AND LISTS:
(a) MULTI-PART WORDS. His final syntax writes `.satellite.` ONCE: files_display.satellite.widgets.display.file(...). His B rule of 10-10 and method_periods.hpp put it on every link that runs a number. .widgets, .displays and .scroll answer the window itself so that the next word can follow, and .vertical/.horizontal follow .scroll. To write `.satellite.` once, widgets.display.file, widgets.display.input, scroll.vertical and scroll.horizontal must each be ONE dotted word: e.g. 1 6 18 n widgets, n 1 widgets.display, n 1 1 .file(name), n 1 2 .input(name); m scroll, m 1 vertical, m 2 horizontal. The lexer's `.satellite.` fold (bytecode_registry.cpp:528-545) reads ONE identifier today. It would have to read a dotted run longest-first, the way the word lexer does.
(b) `display` COLLIDES. His rename displays -> display makes `.display` both the console's .display("words") and the widget family's head. A dotted word fixes this in the lexer; otherwise the walker must tell them apart by what follows.
(c) THE ANSWER'S KIND DEPENDS ON THE BRACKETS AND THE PIECE. .title bare answers text, but .title("x") answers the window. .value answers a number on a slider and a percentage on a progress bar. .file, .input and .section answer the NEW widget. kind_a_method_answers takes only (kind, method), so chain_periods (program_check.cpp:1129) would also need to know whether `(` follows.
(d) THE CHECKER'S WINDOW-ONLY CODE COMPARES token::method_token DIRECTLY: program_check.cpp:1561 (widget chain order), :1749 and :1762 (member chain and .section after it), :3380 and :3382 (the pre-pass). Each must ask a_method_period() instead, or `.satellite.widgets` and `.satellite.section` slip past unchecked. The probe of his line 10 was only judged at `.file`. A window word's answer also needs a period check, like the literal check at :2172.
(e) A BUILD WITHOUT GTK. WORD_SOURCES are always compiled; GTK_SOURCES only when gtk4 is found (047-window.mk:279-326). A number file that calls window_set_* would not link without GTK, so each one needs the `#if SATELLITE_HAS_WINDOW` halves (window_readers.hpp:36), or run_window_number must answer no_window_here first. WINDOW_DEFINE reaches every object (060-compile.mk:25).
(f) SHARED TOKENS. Per-kind numbering handles append, clear and background: they already run string and list numbers (1 6 1 14/15/25, 1 4 2 1/11). But open, close, ok, path, save, clear and append are also a file's (1 6 2 n rows exist but nothing runs them); ok is also a program's and bash's; resize is an infinity's (not built); directory, home and scroll.vertical/horizontal are also arguments-variable rows. Plain greps are useless for these words.
(g) 11 methods take a capsule NAME read as written (expression.cpp:524, program_check.cpp:1575). This is keyed on the method code, so the period does not affect it; the number's function receives the resolved capsule key as text.
(h) a_widget_name_fits (window_shapes.cpp:195) refuses any method word as a widget name. Once bare window words are refused, those names could be freed: his G of 10-10, still open.
(i) Speed is not a concern: every window method crosses to the GTK thread, so one table lookup is noise.

WORK (receiver-aware counts):
- In the four directories named for this count, only examples/window.satl (3 uses) and tests/threads_window_on_a_thread.satl (0) use windows.
- check.sh: about 210 uses in its programs, many through widget_says, which declares main_window once (check.sh:2493). Plus about 200 receiver-less `.word` mentions in quoted expected sentences and comments (not all window words).
- Help pages: satellite.window 32 uses + 55 receiver-less mentions, satellite.console 19 + 21, satellite.variable.window 5 + 1.
- Outside those directories: satellite/satellite_variable_window/prove-*.sh and press-a-button.sh, about 189 uses (run by hand); gtkcar 2; experiments/window_test 3; his satellite_source_editor/source_editor_main.satl 5. Two of his five are already .satellite. (section, widgets). The file is HIS and is never edited.
- C++ message strings quoting window words: about 114 in window_*.cpp and satellite_variable_window/, about 8 in program_check.cpp. The walker also builds name + "." + method_name_of(method) at window_methods.cpp:35 and window_questions.cpp:31.
- Docs: SATELLITE_WINDOW.md about 70 receiver-less mentions, GTK_AND_NO_DEPENDENCIES.md about 224.
- His ~/code/satl: ONE window program, crack_energy_distribution/distributor_window.satl, 7 uses (append 4, changed, open, text).

HOW I COUNTED: uses_tree and uses_his count only calls on a window. A receiver counts if it is a name declared satellite.variable.window (per program in check.sh and the scripts, file-wide in help pages) or the answer of satellite.window.*() or satellite.console.new(), followed through widget names. The plain-grep totals are in the notes. Script: /tmp/claude-1000/-home-madness-code-cxx-satellite/c4e4a254-1700-4b7a-82bf-29c82c49d9ac/scratchpad/inv/window/count_window_words.py; output counts.json beside it.

UNKNOWNS: For him to decide:
1. One number per method token (as strings did), or one per shape: title read vs title("x") write, append(piece) vs append(piece, x, y), file/input with 1 or 3 arguments, listen() vs listen(c)? The words tsv split shapes for satellite.window.text_box (1 27 4 / 1 27 23).
2. Number the methods under the type (1 6 18 n) or under each piece's word (e.g. canvas 1 27 20 n, console 1 5 10 n)? number_a_method_runs and the checker only know the kind (window), not the piece, so I assumed 1 6 18.
3. `.satellite.` once or on every link for widgets.display.file and scroll.vertical? His final file reads as once. scroll.vertical has no ruling.
4. Line 14 of his final file, main_window.open(), is bare. By the rule it becomes main_window.satellite.open(); not settled with him.
5. displays -> display collides with the console's .display("words"). Is the old `displays` refused (his D), and does `display` become a dotted word?
6. Do links that run nothing (widgets, display, scroll) get numbers with no function, like 1 6 1 13? The rule's wording, "takes .satellite. when it runs a number", would need to become "when it is a numbered word".
7. His G (free method words as widget names) is still open.

Limits of what I measured:
- The counts are heuristic. They miss windows declared in an included file, items of a list of windows, and multiples. Scoping in check.sh is approximate. The plain-grep "words" figure only removes satellite.* word paths.
- The probes ran on the satl another session had just installed (13:54:44, reports BUILD 0030). The open-items file says 0031 was installed; I did not settle which binary it is.

Seen but outside this family, and also not under satellite: `.center()` after satellite.console.display(...) (expression.cpp:1977-1983, which only accepts a plain `.` before it). It belongs to the console statements family, not to window values.

Not read: the GTK side beyond the function locations, the prompt (session.cpp) handling of window words, and GTK_AND_NO_DEPENDENCIES.md Part 2G in detail.

## objects-and-everything

HOW IT DISPATCHES TODAY.

(1) OBJECTS. The five words are method codes (token_codes.hpp: pointer 0x0B6E, reference 0x0B6F, ok 0x0B1A, lock 0x0B5B, unlock 0x0B5C). So the lexer already makes x.satellite.pointer into satellite_method_token + pointer_token (bytecode_registry.cpp:527-545); no lexer work is needed.
- The walker sends any value that answers_as_an_object (user_defined or pointer, satellite_object.hpp:322) to call_member at expression.cpp:441. This happens BEFORE the period rule at :480-487, and :442-447 refuse every .satellite. on an object whatever the word.
- call_member (capsule_calls.cpp:185) runs pointer/reference/ok through every_object_method (pointer_calls.cpp:18): at :204 for a pointer, before its object is held, so a gone pointer still answers ok() false and pointer(); at :225 for an object. lock/unlock are at :229-237. Any other name is the spacesuit's own capsule.
- The checker: names_in_statement refuses .satellite. after a declared object name (program_check.cpp:1713-1716). member_of_an_object (:1458, the five at :1478-1492) and the multiple path (:1720-1736) accept the five with `.`. method_judged refuses pointer/reference on every other declared type (:884-891).
- The thread handoff satellite.thread.new(x.pointer()) is refused by the word's spelling: every_object_word (pointer_calls.hpp:40), at capsule_calls.cpp:139 and program_check.cpp:1959, through dotted_names_at, which reads text names.

(2) EVERY OTHER SCALAR. A string runs its own numbers (1 6 1 19-23/26). Every other kind runs ONE shared tail of call_method:
- float, hex, color and fraction ask their own *_values.cpp first (expression.cpp:665-680) and fall through.
- infinity is guarded (:741-753).
- then conversion_of (:349/:494) sends the conversions to object_convert.cpp (object_to_string :59, object_to_number :89, object_to_binary :99, object_to_hexadecimal :126) at :782-797.
- .add goes to Value::add (:806).
- .find goes to str_find_str (:805). It answers on no non-string: S301 at run, though the checker lets it through as of_a_string_or_number (program_check.cpp:862).
- .reverse() goes to reverse_of (:582-598, container_calls.cpp:323).

(3) .center()/.centre() is taken inside the display word's call (expression.cpp:1976-1983).

THE NUMBERING NEEDED, done the way strings and lists were.

(a) OBJECTS:
- Five new words.tsv rows. My reading is that the parent is satellite.spacesuit 1 10 (satellite.class is its alias in aliases.tsv), next free 1 10 1..5: satellite.spacesuit.pointer(), .reference(), .ok(), .lock(), .unlock().
- Five files, satellite-numbers/words/1.10.1.cpp to 1.10.5.cpp. They hold every_object_method's three branches and call_member's lock/unlock.
- A Scenarios slot appended last in number_row.hpp. StringCall's shape fits (receiver, home, arguments, had_parentheses, name, context).
- object_number_of(method) and run_object_number, called from call_member's three sites.
- method_periods.hpp: number_a_method_runs(user_defined or pointer, m) answers those numbers. kind_a_method_answers learns pointer->pointer, reference->object, ok->bool.
- period_written_wrong needs an object sentence for the five. Today's "an object's capsules are its spacesuit's own" is untrue of pointer.
- The walker's :441-447 and the checker's :1713-1716, :1478 and :1726 change from "never .satellite. on an object" to "the five only after .satellite., his capsules only after `.`".
- The thread handoff (capsule_calls.cpp:139, program_check.cpp:1959) needs to read the new period.
- utility/satellite_spelling.py: its Reader already types ('object', suit) receivers (a pointer is declared with its spacesuit), but UNDER has only string and list, and after_a_period skips objects. It needs UNDER['object'] = {pointer, reference, ok, lock, unlock}, for object receivers only: .ok on a file, program or window, and .lock on a file, keep `.` until those types are numbered.
- To respell: the satellite.spacesuit, satellite.delete and satellite.time help pages; THREADS.md; 13 lines of check.sh; 35 tree programs (28 in tests/: pointer_*, reference_*, threads_lock_*, delete_object; 6 in satellite.test; 1 in official_test).

(b) THE SCALARS NO NM-8 TYPE LISTS:
- bool: 1 6 6 has no children, so 1 6 6 1 satellite.variable.bool.string (to_string and str fold into it).
- percentage: 1 6 16 1 .string and 1 6 16 2 .add(x).
- binary: 003's rows exist (1 to_number, 3 to_string, 4 as_number, 6 to_hex, 8 string, 9 number, 10 binary, 11 hex). 8-11 would be the run numbers, like 1 6 1 19-22, with 3/1/4/6 as second names. It needs new rows for .add (13) and .reverse() (14).
- Each file calls the same hub (object_convert.cpp, Value::add, reverse_of), as 1.6.1.19.cpp calls object_to_string. The converter's KIND_OF_TYPE already knows bool, percentage and binary.

(c) CENTER: a row under the display word, e.g. 1 5 1 1 satellite.console.display.center, with centre as its second spelling.

WHAT IS HARDER THAN STRINGS AND LISTS.
1. Mixed periods on one kind. An object's period carries both the language's five words and his own capsules, which stay `.` (his C). So the rule becomes per word on one kind; strings and lists were all or nothing.
2. Shadowing. From outside, d.lock() and d.pointer() are always the language's, even when his spacesuit declares lock() or pointer(). His capsule runs only from inside (probe probes/own_lock.satl; ERRORS4 #005 is still open). After the change, bare d.lock() is either refused (his D) or his capsule (his C).
3. One token, many owners. Each receiver needs its own number, and the converter must type the receiver, never trust the spelling:
   - ok: file 1 6 2 8, program 1 6 23 2, bash 1 6 24 2, window, object.
   - lock/unlock: file, object.
   - the four conversions: every scalar.
   - add: string, number, binary, hex, percentage.
   - reverse: string, list, map, number, binary, hex.
4. The shared tail serves about 10 kinds. Splitting it per type must keep its order (the four types' own methods first, the infinity guard, .reverse() before the containers) and its refusal sentences.
5. A pointer answers ok() and pointer() before its object is held. The number files must keep that order.
6. The thread handoff reads names as text. satellite.thread.new(x.satellite.pointer()) would be told it "names a capsule and does not call it" instead of S721's sentence.
7. center is not on a value. x.satellite.center() today prints the line, then is refused at run as "a number's words".
8. .find on non-strings is a checker pass followed by a run refusal: a non-word to drop, not one to number.
9. No .type, .copy or .kind exists on any value, and .to_string is on every scalar but not on a list, map, file, thread, program, window or object. A list or map says S301 "a container has no to_string"; an object says "counter has no to_string".

CROSS-CHECK: 139 spellings, 122 codes. Every code is accepted by some type except:
- power_of/to_the_power_of/power. Infinity has it "numbered at INF-1, not built" (infinity_calls.cpp:49, S210), and a number refuses n.power(2) with "so far no type has it". bytecode/number_methods.cpp (power, modulus, max, min, abs, floor, ceil, round, truncate, sqrt, digits, bytes, shift_* matched by name) is DEAD: call_number_method has no caller.
- nines: infinity's, not built (INF-7).
- resize: a window's (infinity's is not built).
- center/centre: no value type's (the display word's).

Owners of the rest:
- string only (numbered): find, starts_with, ends_with, substring, split, trim, at, resolved, upper/uppercase/up, lower/lowercase, foreground.
- list only (numbered): by_name, by_value, sum, max, min, reserve.
- string, list, map and file: contains/contain/has, size, empty; clear also on a window.
- list, map and file: first, last, remove, remove_first, remove_last.
- list and file: insert, index_of, search, remove_at, truncate.
- list and map: sort.
- map: keys, values (and get/set by name).
- reverse: string, list, map, number, binary, hex.
- file: read_all, exists. File and window: save, close, open, path. File and program: error. String and file: replace.
- append: string, list, file, window.
- ok: file, program/bash, window, object. lock/unlock: file, object. pointer/reference: object only.
- to_string/str/string: string, number, binary, hex, float, color, fraction, percentage, bool, time, infinity.
- number/binary/hex spellings: string, number, binary, hex, color, float (float's binary and hex are not built).
- add: string, number, binary, hex, percentage.
- thread: start, stop, wait, join. program/bash: start, join, ok, error, pass, code, return, end, exit, quit, shutdown.
- color: transparency. fraction: numerator, denominator. hex: width (also a window's).
- window with its pieces, console, canvas and menu: 56 codes, focus through section, plus append, clear, save, close, open, ok, path, resize, width and background.
- Not period words: the console's foreground=/background=/bold=/italic=/end= are name=value arguments (console_calls.cpp:96-153).

TOOL FOR THE OTHER FAMILIES: /tmp/claude-1000/-home-madness-code-cxx-satellite/c4e4a254-1700-4b7a-82bf-29c82c49d9ac/scratchpad/inv/objects-and-everything/bin/census.py. It is a read-only copy of the converter's Reader; `python3 -B census.py <words...>` prints the use counts by receiver type for the tree and for his programs.

UNKNOWNS: 1. ok, lock and unlock on objects. His A of 2026-10-11 ~14:05 named only .pointer() and .reference(). "Everything builtin" and his own infinity_data3 (progress.satellite.lock()) point to yes, but he was never asked.
2. What bare x.lock(), x.pointer() or x.ok() means after the change when his spacesuit declares a capsule of that name: refused naming .satellite. (his D), or his own capsule (his C, which would also end ERRORS4 #005).
3. Where the five numbers live. Under satellite.spacesuit (1 10 1-5) is my reading only; words.tsv has nothing for them.
4. center/centre: how .satellite. is written after a word call (satellite.console.display(x).satellite.center()?), and whether it is numbered under 1 5 1.
5. Whether bool, percentage and binary belong to this family or to the number family. None of them is in NM-8's still-to-number list, so I listed their words here; binary's rows may duplicate the number family's.
6. Whether a number's .power()/.power_of() is to be built. He asked for it on 2026-09-16, and its code is dead in number_methods.cpp; no type answers it today.
7. The counts come from a copy of utility/satellite_spelling.py's reader. Receivers it cannot work out ('?') are counted to no type: for example 2 more .lock in the tree, and 4 .center plus 2 .centre with no value receiver. The raw grep figures are in the notes.
8. I did not run his programs. That infinity_data3's .satellite.lock() and .satellite.pointer() lines are refused by BUILD 0031 comes from my own probe of the same shape (probes/obj_sat.satl).
9. The checkout changed while I worked (another session committed 9903bc3b, BUILD 0031). I edited nothing in the tree. My probes wrote only to the scratch HOME.

## other-scalars

HOW THE FAMILY DISPATCHES TODAY. Nothing here runs a number yet, and there are two separate routes.
(1) Float, hex, color and fraction answer in their own files. At satellite/bytecode/expression.cpp:665-680 the walker calls float_method (float_values.cpp:105), hexadecimal_method (hexadecimal_values.cpp:186), color_method (color_values.cpp:177) or fraction_method (fraction_values.cpp:169); each one sets `answered`. Their checker halves are FINAL for a declared name (program_check.cpp:893 -> method_right_for :842): float_method_check :82, hexadecimal_method_check :145, color_method_check (color_check.cpp:238), fraction_method_check :145.
(2) Binary, percentage, bool and time have no file of their own and fall to the walker's shared tail:
- .reverse() at expression.cpp:582, which goes to container_calls.cpp:323 reverse_of.
- the four conversions at :782-797, through conversion_of :349 to object_convert.cpp's object_to_string :59, _number :89, _binary :99 and _hexadecimal :126.
- .find at :804 (str_find_str, always refused here).
- .add at :806 (satelliteObject::add, satellite_object.cpp:414).
For a name declared as one of these four types, the checker lets through exactly find, add and the four conversions (program_check.cpp:863-865, 1100-1103). It lets .reverse() through on every type (:881).

WHERE THE PERIOD RULE LIVES.
- The walker: expression.cpp:480 (its_number asks only string_number_of and list_number_of).
- method_periods.hpp:38 number_a_method_runs (string and list only).
- The checker's literal check, program_check.cpp:2172-2185. It is hard-coded: only a STRING literal's words are under satellite.
method_on_a_name :1218, item_periods :1175 and chain_periods :1129 already ask number_a_method_runs by kind, so they follow on their own once it knows the new kinds.

NOTHING IN THIS FAMILY IS MATCHED BY NAME. Every word is a method token from method_code_of. number_methods.cpp (abs floor ceil round sqrt digits bytes modulus shift_left shift_right power max min, all matched by name) is NOT compiled: it is not in make_support/040-sources.mk, and there is no number_methods.o under build/. So those names are refused on these types, and on numbers too: S110 for the bare names, S210 for the tokens power, max and min. NEW_MILESTONES NM-8's "matched by NAME today" describes dead code.

WHAT NUMBERING NEEDS, as strings (BUILD 0016) and lists (BUILD 0017) were done.

(a) Rows in words/words_004.tsv: appended, taking the next free number under each parent (make_words.py refuses a gap). Then make_words.py writes words.tsv and satellite_words.hpp, and bytecode/make_word_codes.py writes word_codes.hpp.
Rows that already exist:
- float 1 6 10 1-5: to_string, string, number, binary, hex.
- hex 1 6 11 1-10: to_number, width, to_string, digits, to_binary, hex(x), string, number, binary, hex.
- binary 1 6 5 1-12: to_number, width, to_string, as_number, digits, to_hex, binary(x), string, number, binary, hex, clear.
There are NO rows for color 1 6 19, fraction 1 6 20, percentage 1 6 16 or bool 1 6 6, and time 1 6 3 has only 1 6 3 0.
Next free numbers: 1 6 10 6, 1 6 11 11, 1 6 5 13, 1 6 19 1, 1 6 20 1, 1 6 16 1, 1 6 6 1, 1 6 3 1.
About 17 new rows cover what runs today:
- hex: add 1 6 11 11, reverse 1 6 11 12.
- binary: add 1 6 5 13, reverse 1 6 5 14.
- color: transparency (plus transparency(n) if it is two rows, as list .sort()/.sort(direction) are), string, number, binary, hex: 1 6 19 1-6.
- fraction: numerator, denominator, string: 1 6 20 1-3.
- percentage: string, add: 1 6 16 1-2.
- bool: string 1 6 6 1.
- time: string 1 6 3 1.
More rows are needed if the words that are refused only at run are to be numbered too.

(b) One file per number under satellite-numbers/words/ (1.6.10.n, 1.6.11.n, 1.6.5.n, 1.6.19.n, 1.6.20.n, 1.6.16.n, 1.6.6.n, 1.6.3.n .cpp, each with describe_1_6_x_n).
- Each file takes its code out of the four _values.cpp methods, object_convert or reverse_of.
- A second-name row only describes itself and runs nothing, as 1.6.1.13.cpp does.
- Each needs a row in satellite-numbers/word_table.cpp and a line in 040-sources.mk WORD_SOURCES.

(c) A new scenario appended last in satellite-numbers/number_row.hpp Scenarios. StringCall's shape (receiver, home, arguments, had_parentheses, name, context) fits every word; `home` is what color .transparency(n) writes through. function_table.cpp:30 must keep these rows out of the by-name view, as it does for list_call.

(d) Per-kind switches like string_number_of (string_calls.cpp:163), each with a static_assert(every_..._numbered). Also a run_<kind>_number like string_calls.cpp:198 that refuses a wrong count first.

(e) method_periods.hpp:
- number_a_method_runs gains floating, hexadecimal, color, fraction, binary, percentage, boolean and time.
- kind_a_method_answers (:54) must learn what each word answers:
  - a number: hex .width and .number, float .number, binary/hex .add.
  - a percentage: percentage .add.
  - the same kind: binary/hex .reverse.
  - color .transparency: a number when read, a color when set.
  - fraction .numerator/.denominator: a number or a float.
- period_written_wrong (:98) has to cover the new kinds: its sentence "a string's and a list's are" and its "X has no Y" branch.

(f) expression.cpp:
- :480 becomes number_a_method_runs((*live).kind(), method), with a branch that runs the number before :582, :665 and :764.
- Every sentence that quotes name + "." + spelling for these kinds must spell .satellite.: :589, :767, :772, :785, :790, :795, :801, :818, :824, :830.
- The same goes for float_values.cpp:89 and :113, kWhatAHexHas (hexadecimal_values.cpp:36), kWhatAColorHas (color_reading.hpp), fraction_values.cpp:150, and container_calls.cpp:405 and :413.

(g) program_check.cpp: the literal check at :2172-2185 must ask number_a_method_runs of the literal's kind (2.5, x1F, b1010, 50%, 250ms), and the sentences at :893 and :1100 need the new spelling.

(h) Conversion with utility/satellite_spelling.py.
- KIND_OF_TYPE (:79) already has every type here, and the script reads the numbered sets from the C++, so UNDER (:70) only needs the new switches.
- Its answer_of (:261) is wrong in three places for this family. It says .add answers the receiver's kind, but a binary's or a hex's .add answers a number. It says .transparency answers a number, but .transparency(n) answers a colour. It says .numerator/.denominator always answer a number.
- Places in the tree:
  - tests: the float, hex, color, fraction, binary, percentage and time tests.
  - loss tests: official_test/loss_test/{float,hex,hex_operations,color,fraction,binary,bool,percent}_loss.satl.
  - satellite/satellite_test/programs: full_numbers_2..8, 16 and 34, full_text_3, full_structure_3, full_objects_1..5, speed_bool/float/percent, and loss_chain.satl. loss_chain is generated by build.sh from the 16 loss tests ("do not edit it here").
  - check.sh: :1431 and :1665, and new rows like :7192 that check the words.tsv rows exist.
  - Help pages satellite.variable.{float,hex,color,fraction,binary,percent,time} and binary_and_hexadecimal; the examples must be re-run.
- His ~/code/satl: about 6 places (display_mock.satl:46; return_chance.satl:12,20; time_is_not_a_number.satl:112-113). His infinity_data3 already writes depth.satellite.string four times, which is refused today.

WHY THIS IS HARDER THAN STRINGS AND LISTS.
(1) The tokens are shared. to_string, to_number, to_binary, to_hexadecimal, add and reverse are one code on every kind, and the shared tail (expression.cpp:764-835) also serves numbers and infinities. Each kind needs its own switch, while the tail keeps serving the kinds that are not numbered yet. The number family (1 6 4) is a different job, so b.satellite.number.string mixes periods unless numbers move in the same step. His infinity_data3 already writes horizontal.satellite.to_string() on numbers next to depth.satellite.string on a float.
(2) Two dispatch routes have to be replaced: the four _values files, and the shared tail.
(3) Many words pass the checker and are refused only at run:
- float .binary, .hex and .reverse;
- percentage, bool and time .number, .binary, .hex, .find and .reverse;
- bool and time .add;
- binary .find;
- color and fraction .reverse.
Whether these get numbers decides the sentences, and whether the refusal moves to before the run.
(4) Some rows exist that run nothing:
- binary .width 1 6 5 2 and .clear 1 6 5 12 have tokens and are refused with S210; the sentence "so far no type has it" is stale.
- .digits (binary 1 6 5 5, hex 1 6 11 4) has no token at all, so it is refused with S110.
- Pairs of rows for one token: float 1 6 10 1/2; hex 1 6 11 3/7, 1/8 and 5/9; binary 1 6 5 3/8, 1/4/9 and 6/11. Hex's 1 6 11 5 to_binary is not even a spelling the lexer has. method_name_of(to_string_token) is "to_string", while strings ran their .string row (1 6 1 19).
(5) width_token and the transparency and colour tokens are shared with the window.
(6) color .transparency is one token with two shapes: reading it answers a number, and setting it changes the name and is refused on a copy.
(7) Literal receivers are uneven. The checker's literal check covers strings only. There is no colour literal: x00FF00.transparency is a hex's word, and refused. A fraction literal (1/3.numerator) and a bool are judged only at run, and satellite.bool.true.string does not parse at all.
(8) .pointer() and .reference(), which his request names, are refused on all eight of these types as "an object's", so they belong to the objects family. Note that b.satellite.pointer() on an object is refused today with a sentence that calls .pointer "an object's capsule", while his infinity_data3:421 already writes maker.satellite.pointer() (probed).

UNKNOWNS: (1) Whether the words that pass the checker but are refused only at run get numbers, and so take .satellite. The first group is float .binary, .hex and .reverse. The second is percentage, bool and time .number, .binary, .hex, .reverse and .find, plus bool and time .add, binary .find, and color and fraction .reverse. If they stay unnumbered, `p.satellite.number` is told "a percentage has no number" before the run, a change from today's S301 at run.
(2) Which row of each existing pair runs: to_string or string (1 6 10 1/2, 1 6 11 3/7, 1 6 5 3/8), and to_number, number or as_number. method_name_of says to_string; strings ran their .string row because they had no to_string row.
(3) Whether color .transparency is one row or two (read, and set(n)), and whether bare-shape rows such as 1 6 19 0 color() are wanted.
(4) Whether binary .width and .clear are built now or stay numbered-and-refused, and whether `digits` gets a token. 1 6 5 5 and 1 6 11 4 have none.
(5) Whether numbers (1 6 4, another family) are numbered in the same step. .number, .width, .numerator and .add answer numbers, so chains mix periods otherwise.
(6) The use counts are rough. Each use goes to the nearest declaration of its name above it, or to a literal written just before the period. Receivers in brackets, items and capsule answers are not attributed, and names reused in check.sh and loss_chain.satl may shift a few. Python's b.find() in check.sh:7586 was found and left out.
Counts across every receiver for the shared words: in the tree, .string 799, .number 126, .binary 66, .bin 22, .hex 46, .add 93, .find 31, .reverse 129, .width 21. In his programs, .to_string 471 (almost all numbers), .string 13, .add 6.
(7) I did not run his programs. Every behaviour above comes from my own probes against the installed BUILD 0031, with a scratch HOME under scratchpad/inv/other-scalars/ (probe.py, summarize.py, count3.py, summary.tsv). Nothing in the tree was edited.

## numbers

TODAY (BUILD 0031, probed with the installed satl under a scratch HOME): a number runs only the generic tail of expression.cpp call_method, which every number-like kind shares: the four conversions (conversion_of :349, run :782-797 -> satellite_object/object_convert.cpp), .add (:806-816 -> satelliteObject::add) and .reverse() (:582-598 -> reverse_of). .find gets past the checker and fails S301 at run. Every other number word is refused. The tokens power/power_of/to_the_power_of, max, min and truncate are S210 from method_judged (program_check.cpp:1100-1103). The eleven name-only words (abs floor ceil round sqrt digits modulus shift_left shift_right clamp, plus unnumbered bytes) are refused in three ways: S110 'not one of its methods' on a declared name (program_check.cpp:1773-1788; library values library_values.cpp:346-351), S110 'no capsule named abs' on an expression, and S201 on a literal (5.floor). NM-8's line 'its methods are matched by NAME today' is wrong. The only code that matches by name, bytecode/number_methods.cpp call_number_method, is in no make file and has no caller (unwired since 715ac3a on 2026-09-16; M17 was ruled 'finish it' on 2026-10-09). It is also stale: it matches power/max/min/truncate and the conversions by name, but the lexer now hands those over as tokens. Its power refuses 2 ^ -1, which ^ answers as 0.5. And it has no clamp. Infinity: power_of/resize/nines are refused by name before the run (infinity_calls.cpp:46-57, checker :897-903). .string answers. number/binary/hex/reverse/find are S301 and add is S210 (INF-3), all at run.

WHAT THE STRINGS/LISTS RECIPE NEEDS HERE:
(1) ROWS. Append to words/words_004.tsv (append-only, code = 4097 + row), then regenerate with words/make_words.py and satellite/bytecode/make_word_codes.py. 1 6 4 1-20 already exist from 003. New rows are needed for bytes, reverse and add, from the next free 1 6 4 21 (satellite_words.hpp:658). 1 6 4 6 to_string and 1 6 4 17 string are two rows for one token: pick the one that runs and make the other its second name, as 1 6 1 13 is for 1 6 1 20. 1 6 4 16 number(x) is a constructor, not a method. The infinity has nothing under 1 6 17, so new rows start at 1 6 17 1: string, power_of, resize, nines, and add/conversions/reverse if it is to own them.
(2) TOKENS FIRST. The lexer folds `.satellite.` into satellite_method_token only in front of a method code (bytecode_registry.cpp:528-544), and a method code is made only straight after a period (:608). So n.satellite.abs() lexes today as the word `satellite` plus a capsule call ('no capsule named abs'). The 11 name-only words each need a [METHOD] row in REGISTRY.satellite, starting at the free 0000101101111010 (:445). token_codes.hpp is then regenerated with satellite/bytecode/make_token_codes.py (method_code_of :389, is_method_code :535, method_name_of :260); check.sh re-runs both generators and compares. They then become method codes after any period, on every receiver. An object's capsule called round still works, because call_member takes any code. A multiple's capsule check at program_check.cpp:1726 lets only name_token, every_object_answers and lock/unlock through.
(3) FILES. satellite-numbers/words/1.6.4.<n>.cpp, about 21 of them: 18 numbered tokens plus bytes/reverse/add. Each describe_1_6_4_<n>() fills a new scenario appended LAST to number_row.hpp Scenarios (number_call; StringCall's signature at :112 is enough). Each also needs a declaration in word_table.hpp, a row in word_table.cpp and a line in 040-sources.mk WORD_SOURCES. function_table.cpp:26-29 must keep number_call rows out of rows_, as it does for list_call. Otherwise the word satellite.variable.number.abs(-5) stops being refused before the run. Probed: a string's word call reaches the run and fails there, while a list's is refused by the checker, which was the 0018 review fix.
(4) THE CALL. Add bytecode/number_calls.cpp/.hpp to replace the dead number_methods.* (INTERPRETER_SOURCES + HEADERS). It holds:
- number_number_of(), with a static_assert that every number exists (string_calls.cpp:150-154 pattern);
- number_method_arity: max/min/power/modulus/shift_* take 1, clamp 2, the rest 0. Not container_arity, where max/min take 0;
- run_number_number: refuse a wrong count, then call context.functions.method(n)->scenarios.number_call.
Walker: expression.cpp:480-482 adds is_number() (and infinity) to its_number. A branch like the string's (:564-576) goes before .reverse() (:582) and the shared tail (:766-835).
method_periods.hpp:
- number_a_method_runs (:40-45) adds number;
- kind_a_method_answers (:82-91): the new words answer a number; string/binary/hex answer a string; add stays unknown;
- period_written_wrong (:113-118) says "a string's and a list's are", and its 'has no X' branch must know a number.
Checker:
- method_judged :1100-1104 needs a number branch with its counts, like string_method_check;
- the literal period rule at :2173-2174 is hard-coded to string_token: ask number_a_method_runs for a whole-number literal;
- :2225-2239 refuses container words on a literal (3.max(5), a review's choice) and must let a number's own through;
- so_far_whose (file_calls.cpp:243-270) should learn 'a number has it'.
(5) CODE TO MOVE. Copy the live generic code per number, as strings did, and leave the other kinds on the shared tail: conversions via object_to_*, add via satelliteObject::add, reverse via reverse_of's number arm. Port number_methods.cpp's branches, but .power and .modulus must call satelliteObject::power/modulus (satellite_object.cpp:653/:632) as ^ and % do. clamp has to be written. sqrt stays a refusal.
(6) SPELLING. In utility/satellite_spelling.py, UNDER at :68-70 needs number, read from number_number_of, plus infinity; answer_of needs :261-270. It reads METHODS from method_code_of, so it can only see the name-only words after step 2. Tree, typed: 429 places in 57 files (satellite/ 319, mostly satellite.test programs; official_test 65; tests 22; check.sh 16; help 7). After that, satellite/satellite_test/make_test_programs.py must regenerate test_programs.cpp. The help pages satellite.variable.number, satellite.variable.infinity and binary_and_hexadecimal need it too (check_help_examples.py). His ~/code/satl, typed: 1,033 places in 63 files.
(7) REBASE:
- check.sh:405-418: five spellings on a number pinned S210 'no type has it'; nines and resize stay refused;
- check.sh :7442-7445: n.power(2);
- check.sh :7939-7941: n.satellite.string refused;
- satellite.test manifest.tsv:48 + full_numbers_13.satl: n.max(3) S210;
- the number help page's 'n.max(3) says no capsule named max', already stale (today it is S210 'so far it is a container's').

HARDER THAN STRINGS/LISTS:
(a) 11 words have no token, so the lexer cannot read .satellite. before them until they get one, and getting one affects every receiver.
(b) Shared words: string/number/binary/hex/add/reverse run one tail for float, hex, binary, fraction, color, percentage, time, bool and infinity. Numbering only the number gives n.satellite.string next to f.string, b.string and 50%.string.
(c) An infinity name holds a plain number (Q24; probed: satellite.variable.infinity i = 5, then i.hex answers 5). The loss tests do this: infinity_loss.satl and loss_chain.satl call x.add(...), .reverse() and .string on `satellite.variable.infinity x = given`. The checker judges by the declaration and the walker by the value. Number the number alone and they disagree: the checker wants `.`, the walker `.satellite.`. Either the infinity takes the number's words or an infinity name is judged at run, like a multiple. A multiple holding a number is already judged at run (probed).
(d) 003's rows are written as static functions: max(a, b), min(a, b), abs(a), clamp(a, low, high), power(a, b), modulus(a, b), truncate(a), sqrt(a). The receiver is `a`, so each row takes one argument more than the method, and the row names would mislead in refusals.
(e) Most of it has no live code to move, only dead and stale code.
(f) Number is the hottest type (every .string in a loop) against the 5% bar.
(g) Shared tokens have different counts per type: .resize is also the window's (built); max/min/truncate are the list's (numbered, 0 or 1 argument) and truncate the file's.
(h) .pointer/.reference are refused on a number ('an object's') and belong to the object family the author also named.

UNKNOWNS: Decisions for the author:
- to_string_token runs 1 6 4 6 (to_string) or 1 6 4 17 (string). The string precedent chose the token's first spelling; the conversion-set rows 17-20 mirror the string's 19-22.
- Does the number's power_of_token run 1 6 4 10? SATELLITE_INFINITY.md calls .power_of distinct from it.
- Which of 1 6 4 21-23 go to bytes, reverse and add.
- Does the infinity get numbers in the same step, and for which words: string; the unbuilt power_of/resize/nines; the refused number/binary/hex/reverse/find; add once INF-3 exists? How is an infinity name holding a number judged?
- His spellings that are not words: .plus(1) x16 (view_forge, global_view; reads as add) and .modulo(5) x1 (the_blue_moon). Aliases or refusals?
- floor/ceil/round/truncate answer the number unchanged (number_methods.cpp's reading of 003 DESIGN 8.6). His programs call .floor() 211 times and .round() 113 times.
- clamp's refusals: low > high, or a non-number.
- Should 003's static word calls (satellite.variable.number.max(3, 5)) ever run?
- n.find(x) passes the checker and fails at run. Refuse it before the run?
- His newest program infinity_data3/infinity_data_main.satl (modified 2026-10-11 13:50) already writes n.satellite.to_string(), obj.satellite.lock() and obj.satellite.pointer(). Today all are refused, and NM-8 says .pointer/.lock keep `.`. I did not settle whether he or the other session wrote them.

Count method:
- uses_tree/uses_his are typed counts. They come from the tree's own converter's reader (utility/satellite_spelling.py, copied to the scratchpad and run read-only), counting only bare `.word` in code whose receiver it types as a number or an infinity. That makes them a floor: receivers it cannot type are not counted (capsule answers, untyped items, multiples).
- A raw grep is the ceiling, and it is useless for .string/.number/.binary/.hex, because it matches satellite.variable.<type>.
- Tree help-page prose adds noise, e.g. '5.The'.
- I did not count test_programs/ or experiments/.

Small defects seen:
- A chain's refusal quotes the registry spelling. `n.string.size` is told 'n.to_string.size is written n.to_string.satellite.size' (link_spelled, expression.cpp:2370-2382), and numbers write .string far more often than .to_string.
- The walker's period sentence also uses the registry name, e.g. m.satellite.to_string for m.satellite.string.

