# LINK_PLAN — links and colours on a string

Rewritten 2026-10-18 from the author's message that day. It replaces the 2026-10-17 plan (twelve
words, milestones TXT-0 to TXT-7), which the author scrapped as too big: *"we have blown this way
out of proportion really"*. Three words now, and nothing else. Checked against the working tree on
2026-10-18 (build/satl prints BUILD 0065; HEAD b48fb5d56 says 0058).

---

## 1. The author's design

```
satellite.variable.string my_string = "hello, some text"
my_string.satellite.text.link("some", capsule_name)
my_string.satellite.text.foreground(x000000) // now the foreground is 000000
my_string.satellite.text.background(xcccccc) // now the background is cccccc

// now that we have added what we want to add, we can print the string,

satellite.console.display(my_string)
```

> *"Look at how they do links on the web currently, they change the color to blue and that's about
> all that they do"*

> *"the syntax something("substr", capsule_name) if "substr" exists then it goes through, and then
> if substr doesn't exist, it doesn't go through... let's just leave it at that, where substr can
> be a string object itself"*

> *"make sure that everything is behind object_name.satellite.text.option(substr, capsule_name)
> with substr being optional and that is definitely enough, we don't need more than that"*

What the program shows: "hello, some text" in black on #cccccc, with "some" in blue. Clicking
"some" runs capsule_name.

---

## 2. The rules

1. **Three words, on a string's name.**

   ```
   name.satellite.text.link(capsule)          name.satellite.text.link(piece, capsule)
   name.satellite.text.foreground(color)      name.satellite.text.foreground(piece, color)
   name.satellite.text.background(color)      name.satellite.text.background(piece, color)
   ```

   Nothing else is under `.text`, and there is no `satellite.text.…` with nothing in front of it.
   A colour is what `.satellite.foreground(c)` takes today: a solid satellite.variable.color or six
   hex digits (colour_of, console_style.cpp:164-187).

2. **One option per line: each word changes its string, the way `.append` does.** On a line of
   its own it changes the name in front of it. Inside an expression it changes the name too and
   answers it, as `display(s.satellite.append("d"))` does today. A literal in front,
   `"x".satellite.text.link(go)`, has no name to change and is refused before the run.

3. **The piece is optional and comes first.** It is a literal or a string variable, and the option
   goes on the first place it appears, the place `.find` answers. If the string does not have it,
   the line does not go through: it is refused when it runs, as `.find` refuses (S420
   TEXT_NOT_FOUND, exit 15). An empty piece is refused as `.replace` refuses one (S421
   EMPTY_SEARCH_TEXT).

4. **Options pile up.** Each line puts its option on the characters it covers and leaves the rest
   as they were. A later colour on the same characters replaces the earlier one there; a later
   link replaces the earlier link there.

5. **A link is blue, like a web link**: #0000EE, whatever foreground its characters were given.
   Its background is drawn as set. In satl's console VTE adds, by itself, a dashed underline under
   every link (solid while the pointer is on it) and a hand pointer (vte.cc:9368-9369,
   vtedefines.hh:77-80). satl draws no underline of its own.

6. **The options sit beside the characters, not in them.** `.size`, `.find`, `==` and every other
   method see only the characters, so a linked "OK" `==` "OK". `+` and `.append` keep each part's
   options where its characters land. Every other method that makes a string from a string
   (`.substring`, `.upper`, `.replace`, `.split`, `[n]`, …) answers plain characters. Whole copies
   keep them: `t = s`, a capsule's parameter, a list item.

7. **Only a terminal draws them.** Into a pipe or a file a string is its characters. Under NO_COLOR
   it is drawn with no colours, and a link keeps VTE's underline. The link itself (OSC 8) is
   written only into satl's consoles; on any other terminal a link is blue text.

8. **A click runs the capsule the way a button's press runs one**: by the key worked out when
   `.link` ran (capsule_key.hpp), on the program's thread, in the order clicked. A click made while
   main is still running waits until main returns, as a press does. The capsule takes no
   parameters.

9. **Only a link this run wrote can be clicked.** A link's address holds a number chosen at random
   when the run starts. A link in text from anywhere else (a file, typed input, another program's
   output) looks like a link in VTE and does nothing when clicked.

10. **The old spellings are refused, and the sentence names the new.** `s.satellite.foreground(c)`
    and `s.satellite.background(c)` (1 6 1 24, 1 6 1 25). Their rows stay in words.tsv: taking a
    row out would move the code of every later word (make_words.py:29-38). Not touched:
    `satellite.console.foreground` / `.background`, `satellite.terminal.foreground` /
    `.background`, and display's and input's own options (`satellite.foreground=` and the rest).

---

## 3. What satl has today

- **A string** is a satellite_string: `narrow16_`, `wide_count_` and an atomic `bookmark_`, so
  its copy and move are written by hand (satellite_string.hpp:99-131, 206-210). `+` is
  `out = left; out.append(right)` (string_and_string_add.hpp). `==` compares characters
  (satellite_string.cpp:277).
- **A string's colour is codes inside its text** (coloured_text, console_style.cpp:270-275, called
  from string_coloured, console_calls.cpp:564-587). Measured on BUILD 0065: a green "OK" has
  `.size` 22, `.find("K")` 17 and `== "OK"` false; `.substring(1, 3)` is ESC[3; `.upper()` turns
  the codes into ESC[…M, which VTE reads as *delete lines*; written to a file, the codes go with
  it. Rule 6 ends all of that.
- **Only `.append` and `.clear` change their string** (changes_a_string, string_calls.cpp:143-146),
  on a line of their own or inside an expression (expression.cpp:595-605, 925-935). Any other
  string method alone on a line does nothing, silently: `s.satellite.foreground(x00FF00)` leaves
  s as it was and exits 0.
- **`text` is already a word**: a window's `w.satellite.text` (1 6 18 8, window_methods.cpp:51).
  `s.satellite.text.link(…)` is refused today: "a string has no text" (method_periods.hpp:319-321).
  A longer path after `.satellite.` is one method token, as `widgets.display.file` is
  (REGISTRY.satellite:460-463, make_token_codes.py).
- **A string method takes exactly one count of arguments** (string_method_arity,
  string_calls.cpp:122-141), in the checker and in the walker. A window word with two counts is
  the precedent: window_method_also_takes (program_check.cpp:1122-1131).
- **A capsule's name comes first** in every window word that takes one (GTK-11,
  window_shapes.cpp:284-298). The checker looks for it at the first argument
  (program_check.cpp:2116-2153) and the walker reads it there (expression.cpp:545-566).
- **A button's press** is pushed by the desk onto a queue (the_desk_saw_something,
  window_desk.cpp:355-380) and run on the program's thread after main returns
  (windows_run_until_they_are_closed, structured-library.cpp:620-654; run_capsule,
  program_walk.cpp:2580-2594). A pressed capsule may declare nothing, the piece, or the piece and
  its window (a_press_can_run, program_check.cpp:1781-1800).
- **satl's console after main returns**: the press loop waits only for the program's windows, not
  for satl's console (a_program_window_is_open, window_desk.cpp:56-65); a clean run closes the
  console at once (console_launch.cpp:425-427); closing it while a run is going is a hangup
  (window_console.cpp:182-214).
- **VTE 0.84.1** (vendor/vte): allow-hyperlink is off by default, and an OSC 8 link written while
  it is off is dropped for good (vteseq.cc:1728-1729). With it on, VTE draws the underline and the
  hand pointer itself, never follows a link, and `vte_terminal_check_hyperlink_at(terminal, x, y)`
  answers the link under a point (vtegtk.cc:3804-3834, GTK4 only). VTE's own click gesture claims
  every left press to start a selection (vte.cc:7374-7410). satl calls none of the hyperlink
  functions today.
- **How a string reaches the console**: display feeds satl's console with vte_terminal_feed
  (console_feed.cpp:278). Colour codes are written when stdout is a terminal and NO_COLOR is not
  set (console_style.cpp:189-202); for_the_screen strips only SGR codes (204-231). A string's
  characters reach VTE unfiltered, ESC included, whether they came from a file, from input, or a
  raw ESC byte in a literal (measured). A `satellite.console.new` console's display writes the raw
  string to its own pty (window_console.cpp:540-548).
- **The console's default colours** are black on light blue #90D5FF (console_shadow.hpp:15-21).
  #0000EE on #90D5FF is 5.9:1.

---

## 4. The milestones

### LINK-0 — a string keeps its options beside its characters

Nothing new to write in a program yet: the two colour words that exist today move onto the new
storage, and everything after this builds on it. No picture changes.

- `satellite_string` gains `runs_`: a `std::shared_ptr<const std::vector<Run>>`, null for a plain
  string. A Run is a first and last character (counted as satellite_string counts them), an
  optional foreground, an optional background, and a capsule key (empty: not a link). Runs never
  overlap and are kept in order.
- The four hand-written copy and move members carry `runs_`; a move leaves the source null.
- `append` moves the other string's runs along by this string's size and keeps them, `s.append(s)`
  included, so `+` keeps both sides'. `from_utf8`, `clear` and `substring` (in place too) leave no
  runs. Nothing else has to change for rule 6: every other method already builds its answer from
  characters.
- One function, in console_style.cpp beside styled_line, draws a string: its characters, each
  run's SGR around it (38;2 and 48;2, as coloured_text writes them now), and after each run the
  line's own colour put back (the console's from-now-on colour, or display's
  `satellite.foreground=`), as styled_line's re-opening does today. display (display_plainly and
  display_with_options), input's prompt, and a `satellite.console.new` console's display all draw
  with it. Into a pipe or a file it writes the characters only.
- `"x".satellite.foreground(c)` and `.background(c)` (1 6 1 24, 25) answer a copy with a run
  instead of codes, on the characters that have no colour of that kind yet. That is today's
  re-opening of a colour inside a colour, kept so that every program using them looks the same
  until LINK-1 moves them. coloured_text retires.
- The tests that pin codes inside the text are rewritten to the new answers: check.sh:6069-6072
  (`.find` = 16, `== "OK"` false), full_text_2.satl:106-123, and the loss program
  (programs/loss/loss_chain.satl and official_test/loss_test/string_loss.satl, which cut the codes
  out by position); then test_programs.cpp is regenerated. The terminal bytes check.sh pins at
  5953-5997 should not change.
- satellite.help/satellite.variable.string (41-50), satellite.help/satellite.console (43-49) and
  console_style.hpp's design notes (38-57) stop saying the codes are in the string.

**Done when**, for `green = "OK".satellite.foreground(x00FF00)`: `.size` is 2, `.find("K")` is 2,
`green == "OK"`, `green.satellite.upper()` is a plain "OK", `"status: " + green` draws "OK" green
on a terminal, green written to a file is "OK", and check.sh passes.

### LINK-1 — `.text.foreground` and `.text.background`

- REGISTRY.satellite gains `text.link`, `text.foreground` and `text.background` as method
  spellings, as `widgets.display.file` is one, and token_codes.hpp is regenerated
  (make_token_codes.py).
- words/words_004.tsv gains these ten rows, and make_words.py and make_word_codes.py are run
  (DESIGN.md:205-207). The numbers are proposed; words.tsv is the author's.

  ```
  1 6 1 27        satellite.variable.string.text
  1 6 1 27 1      satellite.variable.string.text.link
  1 6 1 27 1 1    satellite.variable.string.text.link(capsule)
  1 6 1 27 1 2    satellite.variable.string.text.link(piece, capsule)
  1 6 1 27 2      satellite.variable.string.text.foreground
  1 6 1 27 2 1    satellite.variable.string.text.foreground(color)
  1 6 1 27 2 2    satellite.variable.string.text.foreground(piece, color)
  1 6 1 27 3      satellite.variable.string.text.background
  1 6 1 27 3 1    satellite.variable.string.text.background(color)
  1 6 1 27 3 2    satellite.variable.string.text.background(piece, color)
  ```

- string_number_of maps the tokens to the words. foreground and background each get a file in
  satellite-numbers/words/, as 1.6.1.24.cpp is one, with its row in word_table and WORD_SOURCES.
- The words change their string (rule 2): they join changes_a_string, and the list-item path that
  append uses (container_calls.hpp:171-185), so `l[1].satellite.text.foreground(c)` changes the
  item.
- The checker and the walker take one argument or two (rule 1), following
  window_method_also_takes. A colour is checked as colour_of checks one now. The piece is an
  ordinary value, a literal or a string variable.
- The piece (rule 3) is found as `.find` finds it (str_find_str), when the line runs. Not found is
  S420, exit 15: `my_string.satellite.text.foreground did not find "some"`. An empty literal piece
  is refused before the run, as `.replace("", …)` is (string_check.cpp:178-183).
- A literal in front of `.text` is refused before the run, on a line of its own or inside an
  expression. A line that begins with a literal is stepped over today
  (program_check.cpp:4026-4032), so this is a check of its own.
- The old spellings are refused before the run, naming the new (rule 10), in the shape of
  period_written_wrong's sentence (method_periods.hpp:299-312): `s.satellite.foreground is written
  s.satellite.text.foreground now -- a string's colours and links are under satellite.text`.
- Every program, test and help page with an old spelling is rewritten: tests/console_from_003.satl,
  full_text_2.satl, loss_chain.satl, string_loss.satl, check.sh,
  experiments/window_test/window_and_extras.satl, the satellite.variable.string help example, and
  README.md. Each `g = "OK".satellite.foreground(c)` becomes two lines: `g = "OK"`, then
  `g.satellite.text.foreground(c)`. check.sh:7357-7365 learns the new rows and files.
- satellite.help/satellite.variable.string describes foreground and background under `.text`.

**Done when** check.sh passes with rows for: a whole string, a piece, a piece held in a variable, a
later colour replacing an earlier one, a piece the string does not have (refused when it runs, exit
15), a literal in front (refused before the run), and each old spelling (refused, naming the new).

### LINK-2 — `.text.link`, drawn

- `.text.link(capsule)` and `.text.link(piece, capsule)`. The capsule's name is read from the last
  argument, the one place it does not come first (Section 6), in the checker
  (program_check.cpp:2116-2153, 2290-2313) and the walker (expression.cpp:545-566). Its key is
  worked out as `.pressed(go)` works one out (capsule_key_at, capsule_reach.cpp:274-285). A name
  that is not a capsule, or a capsule that declares a parameter, is refused before the run.
- Drawing, in LINK-0's function: a linked run is blue, #0000EE, whatever its foreground (rule 5).
  Into satl's consoles it is wrapped in OSC 8, `ESC ] 8 ; ; satellite:<token>:<n> ESC \` before
  and `ESC ] 8 ; ; ESC \` after. token is the run's random number (rule 9). n is the capsule's
  number in the run's table of linked capsules, one number per capsule, kept behind a lock for the
  desk's thread. On another terminal the run is blue with no OSC 8; into a pipe or a file it is
  its characters.
- a_terminal_to_type_in (window_console.cpp:342-431) calls
  `vte_terminal_set_allow_hyperlink(terminal, TRUE)` before anything is written, for satl's own
  console and every `satellite.console.new` console.
- satellite.help/satellite.variable.string describes link.

**Done when** check.sh asserts that a linked string into a pipe is its characters, that `.link`
does not change `.size`, that on a pty (SATL_NO_WINDOW) a link is the blue SGR with no OSC 8, and
that a name that is not a capsule, or a capsule with a parameter, is refused; and by hand, in
satl's console a link shows blue with VTE's dashed underline and a hand pointer.

### LINK-3 — a click runs the capsule

- A GtkGestureClick for the primary button, in the capture phase, on the terminal, added in
  a_terminal_to_type_in. On a press it asks `vte_terminal_check_hyperlink_at(terminal, x, y)`. A
  `satellite:<token>:<n>` with this run's token and an n in the table claims the press, so VTE
  starts no selection, and is pushed onto the desk's queue as a button's press is
  (the_desk_saw_something). Anything else is left to VTE: selecting, pasting and the right-click
  menu work as now. That a capture-phase claim keeps the press from VTE comes from reading GTK 4.24
  (gtkmain.c:1992-1997); it is tried first.
- The capsule runs as a press runs (rule 8): windows_run_until_they_are_closed takes it and
  run_capsule runs it.
- After main returns, a run whose satl console has shown a link keeps waiting for clicks until the
  console is closed: the press loop counts that console (a_program_window_is_open skips it today),
  and satls_own_console_is_done does not close it after a clean run. Closing it then ends the run
  as closing the last window does, exit 0 or the code a capsule stopped on, instead of the hangup
  it is today (window_console.cpp:182-214).

**Done when**, by hand (check.sh cannot click): a program whose capsule writes a file writes it
when its link is clicked, twice for two clicks; a link in text read from a file does nothing when
clicked; dragging across ordinary text still selects it; and closing the console ends the run with
exit 0.

---

## 5. Not in this plan

- Everything the 2026-10-17 plan had beyond these three words: bold, italic, underline,
  strikethrough, dim, plain, size, weight, shadow, the console-wide `satellite.text.<word>`, and
  the "see here for more information" link under an error. Each is the author's to bring back, as
  a plan of its own.
- The prompt. A link displayed at the prompt is drawn; what a click does there is not decided
  here. This plan is for `satl file.satl` runs.
- A capsule that is handed the linked text. The capsule takes nothing (rule 8).

---

## 6. For the author

- **Where the capsule's name goes.** GTK-11 made it a LANGUAGE rule that a capsule's name comes
  first in the brackets, as in `.every(go, 1000)` and `.ask(when_answered, "delete it?")`: "one
  rule a person can hold in their head beats one line that reads slightly better"
  (window_shapes.cpp:284-298). `link("some", capsule_name)` puts it last. This plan writes it the
  author's way of 2026-10-18. Keeping GTK-11 instead makes the three words `link(capsule, "some")`,
  `foreground(x000000, "some")` and `background(xcccccc, "some")`. SATELLITE_WINDOW_SECTIONS.md
  (2026-10-20) asks the same about `display.button("text", capsule_to_run)`, so one answer settles
  both. That plan also found that the rule was a choice made when GTK-11 was built, not the
  author's ruling: GTK-11's own plan had written `ask("delete it?", when_answered)`.
- **Red or blue.** The prompt's start-up note says "RED TEXT IS CLICKABLE" (session.cpp:639),
  naming the console's links before they were built. This plan draws them blue, as the author said
  on 2026-10-18; the note changes with it, or the colour does.
