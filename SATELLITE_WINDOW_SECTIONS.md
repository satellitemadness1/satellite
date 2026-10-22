# SATELLITE_WINDOW_SECTIONS — a window divided into named sections, and what each one shows

Written 2026-10-20 from the author's message that day, from the rule the author added in a second
message (the whole window must be sectioned off), and from a third message that added a button and a
progress bar. Sections are built (NM-7, 2026-10-10). This plan changes how they are written, lets a
fraction size them, makes their names names, adds that rule, and adds the displays that go in them.
Checked against the working tree on 2026-10-20 (build/satl prints BUILD 0066; HEAD b48fb5d56 says
0058), and reviewed the same day.

---

## 1. The author's design

```
satellite.variable.percent fifty_percent = 50%
satellite.variable.window main_window = satellite.window.new("window title", "window secondary text", 800x600)

main_window.satellite.window.section("section_name_here", fifty_percent, 100%)

section_name_here.satellite.window.display.file("my_file_list_name")
section_name_here.satellite.window.display.text("str1", "str2") // this displays 2 strings within that section, as a text display
section.satellite.window.display.image("filename.ext") // this displays an image in that section
section.satellite.window.display.console()
.display.text.input()
.display.text.view() // (view is for a text_view in GTK)
```

> *"so this gives a section of the window called "section_name_here" so we can type this:
> section_name_here.satellite.window.display.file("my_file_list_name")"*

> *"then there is a source code view, and the "console" is just the satellite.console.new()
> object"*

> *"the sections can be divided off by satellite's numbering system -- a percent, a fraction, a
> pixel, so we have from 0% to 100% and from 0/1 to 1/1 in fractions, and from 0pixels to the
> width/height of the window"*

> *"provide errors when 100% of the "window" has NOT been named, so if the code doesn't use 100% of
> the window in sections -- provide an error saying that you need to section off the entire window"*

> *"display.button("text", capsule_to_run) display.progress(1-100) and this takes a value so you
> pick a name for it... ".satellite.window.display.progress("name_of_display", 1-100) // so this
> displays from 1 - 100 and then we update it with name_of_display(1-100)"*

The same lines as one program, under the rules below. The example's one section is half the window,
so a second section names the other half. In this plan a section shows one display (Section 6 asks
about that), so the text goes in the second section:

```
satellite.variable.percent fifty_percent = 50%
satellite.variable.window main_window = satellite.window.new("window title", "window secondary text", 800x600)

main_window.satellite.window.section("section_name_here", fifty_percent, 100%)
main_window.satellite.window.section("right_half", fifty_percent, 100%)

section_name_here.satellite.window.display.file("my_file_list_name")
right_half.satellite.window.display.text("str1", "str2")
```

What it shows: a window with "window title" over "window secondary text" in its bar, 800 by 600
pixels inside it. The left half lists the files of your home folder, folders first; the right half
shows "str1" with "str2" under it. Without the two `right_half` lines, satl refuses the program
before it runs:

```
S157: MISSING_SECTION
satl(check): in satellite.main, the right 50% of main_window, in the row of
section_name_here (400 of its 800 pixels), has no section -- you need to section off
the entire window
```

---

## 2. The rules

1. **A section is made with `.satellite.window.section`, on a window or on a section.**

   ```
   main_window.satellite.window.section("top", 100%, 50%)
   top.satellite.window.section("inner", 50%, 100%)
   ```

   It takes the section's name in quotes, its width and its height, and answers the section it
   made. Sections are laid out left to right in the order they are made, and one that does not fit
   starts a new row under the tallest before it, as today. What fits is decided at the window's own
   size (rule 3), in the window and in each of its sections, and decided again when the program
   gives the window a new size with `.satellite.resize`. When a person resizes the window, or it
   fills the screen, the rows stay as they are: a share stays a share, and a row that is then wider
   than the window is cut off at its right edge. Today a row is decided again at every size.

2. **A size is a percent, a fraction or a number of pixels.** A width and a height may be of
   different kinds: `section("top", 1/2, 250)`.

   | kind | written | above | at most |
   |---|---|---|---|
   | percent | `50%`, a `satellite.variable.percent`, `100% / 3` | 0% | 100% |
   | fraction | `1/2`, a `satellite.variable.fraction`, `1 / 3` | 0/1 | 1/1 |
   | pixels | `250`, a `satellite.variable.number` | 0 | the width or height of what holds it |

   A percent or a fraction is a share of what holds the section: the window, or the section it is
   made in. Pixels are measured against what holds them in whole pixels, as it is drawn at the
   window's own size, which is what its `.satellite.width` and `.satellite.height` answer while the
   window is that size. A size outside its range is refused with S410 NOT_A_POSITION (exit 19). A
   value that is not a size at all, such as a string, a float or a bool, is refused with S301
   TYPES_DO_NOT_MEET (exit 27). Both are refused as they are today. A size of 0 stays refused,
   because a section with no room shows nothing, and so does a share finer than a millionth of what
   holds it (under 0.0001%, or a fraction under 1/1000000). A share past 100% is refused now; today
   one may reach 3276700%.

3. **A window's size is its inside.** `satellite.window.new(…, 800x600)` gives its sections 800 by
   600 pixels. The 600 is under its title bar and its menu, not around them. The window's
   `.satellite.width` and `.satellite.height` answer 800 and 600, and `.satellite.resize(1024, 768)`
   asks for an inside that size. This holds for every window, a console too, so the help page's
   "400, 300 is the middle of an 800 by 600 window" becomes true for appended pieces as well.

   Today the title bar is inside the 600. A window with a subtitle, as in the author's example,
   gives a 100% section 800 by 553, because its bar is satl's own and GTK always draws that inside
   the window. A window or a console without a subtitle has GTK's own shorter bar, about 37 pixels,
   or no bar inside it on a compositor that draws its own title bars.

4. **A section's name is a name.** The section line declares its name in the capsule that wrote it,
   as a `satellite.variable.window` line would, so the lines after it can write the name bare:

   ```
   main_window.satellite.window.section("top", 100%, 50%)
   top.satellite.window.display.file("files")
   ```

   - The name must be one a variable could have, and not one of the language's own words. Before the
     run satl refuses a name that is already a name in that capsule (a variable, a parameter, a
     field, a capsule, a namespace, an included file, another section or display), `satellite`, a
     name shaped like a hex or binary number (`xff`, `b01`), and a method's own word (`text`,
     `file`, `section`, `window`, …). So two sections of one name in one capsule are refused, even
     in two windows or in two different sections. Today both are allowed.
   - Declaring the name again is refused as a name declared twice, with a sentence of its own.
     `satellite.variable.window top = main_window.satellite.window.section("top", 100%, 50%)` is told
     `top is declared by its section line already -- write top, bare, where the section is wanted`
     (the wording is the author's to change).
   - It ends where a declaration ends: at the `}` of the block it was made in.
   - `main_window.top` still reaches it, there and in any capsule given the window (`its_window.top`).
     There are no globals, so that is how another capsule reaches it.
   - A file display's or a progress display's name, `display.file("files")` or
     `display.progress("bar", 1)`, is a name in the same way.

5. **The whole window is sectioned off.** A window that has a section has all of it in sections.
   Laid out at its own size (rule 3):

   - each row of sections is as wide as the window,
   - every section in a row is as tall as the row, and
   - the rows together are as tall as the window,

   and no section reaches past the window's edge. The rows are the layout's own rows (rule 1).

   - **Worked out exactly** from the sizes the program wrote, with one tolerance: under half a pixel
     at the window's own size, short or over. So three `100% / 3` sections, a hair under 100%, are
     whole, and so are three `1/3`, which are exactly 1/1, and six `100% / 6`, a hair over. Three
     `33.3%` of 800 pixels are 0.8 short, and are not.
   - **Drawn as judged.** Every edge, across a row and down the rows, is at the pixel nearest its
     running total, so a window judged whole is drawn whole. Today each share is rounded down on
     its own: three thirds of 800 pixels leave 2 empty, and the help page's own two halves of 553
     leave 1.
   - **At the run, whatever the checker found.** Every open window that has a section is judged
     when main returns. After each capsule the press loop runs, whatever queued it (a press, a key,
     a timer, a typed line or any other event), every window whose sections or size that capsule
     changed is judged. A window closed by then is not judged. A main that never returns is judged
     only by the checker.
   - **Before the run as well, when the checker can see the whole window**, and then it refuses one
     that is not whole. It can see the whole window when all of these hold:
     - the window is declared in a capsule from `satellite.window.new`, with its size written as
       numbers;
     - its name is never given another window there, and is used only to write its own lines: never
       handed to a capsule, put in a list or a field, or given to another name;
     - it is not resized or closed there;
     - its section lines run straight down that capsule, in no block;
     - each size is a literal, or literals joined by an operator (worked out with the run's own
       arithmetic, as read_library_values works out a `satellite.library` value), or a
       `satellite.library` value, or a name declared there with one of those and never given
       another value there.

     The author's example is judged before it runs: `fifty_percent` is 50%, because nothing changes
     it.
   - **Only a window with a section is judged.** A window of appended pieces, a console, and a window
     with nothing in it are as they are today.
   - **A section's own sections are not judged.** The section is named, so all of it counts.
   - **A sectioned window holds nothing but sections**: a display goes in a section (rule 7), and
     nothing is `.append`-ed into it (as today).
   - **A person's resize is not judged, and nor is `.satellite.fullscreen`.** Shares follow the window
     and pixels do not, so a window sectioned in pixels shows an empty strip when a person makes it
     bigger.
   - **The refusal is new**: S157 MISSING_SECTION, machine code 85, exit 85, in the MISSING family
     beside S156. It names the window and the first place, top left first, as a share of the window
     and in its pixels. The share is a percent, whatever kind the sizes were written in, to two
     places, with more only where they are needed to show a digit that is not 0. The pixels are the
     place as drawn. The three gaps end with the author's words, and the two places past an edge with
     their own:

     ```
     the right 25% of main_window, in the row of left and middle (200 of its 800 pixels), has no
       section -- you need to section off the entire window
     the 50% of main_window under right (300 of its 600 pixels) has no section -- you need to
       section off the entire window
     the bottom 50% of main_window (300 of its 600 pixels) has no section -- you need to section off
       the entire window
     extra reaches past main_window's bottom edge -- its rows add up to more than the window, and a
       section past the window is never seen
     wide reaches past main_window's right edge -- it is wider than the window, and a section past
       the window is never seen
     ```

     The right edge is passed only after a `.satellite.resize` makes the window narrower than a
     section given in pixels.

6. **A display fills the section it is made in.**

   ```
   name.satellite.window.display.file("a_name")      the files of a folder
   name.satellite.window.display.text("str1", ...)   lines of text
   name.satellite.window.display.image("a.png")      a picture
   name.satellite.window.display.console()           a console of the program's own
   name.satellite.window.display.text.input()        a one-line text input
   name.satellite.window.display.text.view()         a text view
   name.satellite.window.display.source.view()       a text view for satellite source
   name.satellite.window.display.button("OK", go)    a button that runs a capsule
   name.satellite.window.display.progress("bar", 0)  a progress bar, set with bar(75)
   ```

   - A display takes no size: its section is its size. A section shows one display or holds
     sections, not both.
   - Each display word answers the display it made. A program that needs a display later holds it in
     a name: `satellite.variable.window out = right.satellite.window.display.console()`.
   - **file**: the file display built on 2026-10-09: a folder's files, folders first, starting in
     your home folder, and `.satellite.directory("/tmp/")` as today. Its name is a name (rule 4).
   - **text**: each string on a line of its own, from the section's top left corner. It takes at
     least one string. `.satellite.text` reads the lines as one string with a newline between them,
     and `.satellite.text("x")` replaces them. It is a GtkLabel, as `satellite.window.label` makes.
   - **image**: the picture in the file, as big as the section lets it be, keeping its shape. It
     reads what `satellite.window.picture` reads, through the same code: PNG, JPEG, TIFF, GIF and
     ANI (their first frame), BMP, ICO, TGA, PNM, XPM and XBM. Its path is read the way
     `satellite.file` reads a path. A file that is not there is refused when the line runs (S510
     FILE_NOT_THERE, exit 39), and one that is not a picture with S513. `.satellite.path` reads and
     changes it, as a picture's does.
   - **console**: what `satellite.console.new` makes, without a window of its own. It has the
     console's words: `.satellite.display("words")`, `.typed`, `.clear()`, `.home()`, `.columns`,
     `.rows`, `.colour`, `.background`, `.font` and `.scroll.vertical` / `.horizontal`. It has none
     of a window's words, and it closes with its window. It talks to the program, not to a shell, and
     `satellite.console.display` still writes to satl's own console, never into this one. It takes
     the keyboard when it is clicked, not when it is made.
   - **text.input**: today's `widgets.display.input`, under its new name. `.satellite.text`,
     `.listen(capsule)` and `.changed(capsule)` work as today.
   - **text.view**: a text view a person can type in, which scrolls when its text is longer than the
     section. `.satellite.text` reads and sets the whole text, and `.changed(capsule)` works.
   - **source.view**: a text view, as `text.view` is, whose satellite source is coloured by satl's
     own lexer as it is typed: the language's words (`satellite.` words and every `.satellite.`
     word), text in quotes, numbers of every kind, and comments, each in a colour of its own.
     Proposed colours, the author's to change: words #0000FF, text #A31515, numbers #098658,
     comments #008000.
   - **button**: the text on it, then the capsule a press runs, as the author wrote it (Section 6
     asks about the order). The capsule may declare nothing, the button, or the button and its
     window, as a `.satellite.pressed` capsule may, and it runs as a press runs today: after main
     returns, in the order the presses came. `.satellite.text` reads and changes its text,
     `.satellite.press()` presses it, and `.satellite.pressed(other)` gives it another capsule, as on
     a `satellite.window.button`.
   - **progress**: its name in quotes, which is a name as a section's is (rule 4), then how far along
     it is, a whole number from 0 to 100. Calling its name sets it (rule 9). `.satellite.value` reads
     it as a percentage, as today's progress bar answers. GTK draws it as a bar across the middle of
     its section.

7. **Where a display goes.** A display goes in a section. Three of today's places stay, under the
   new names:
   - a file display or a text input made on a window with no sections goes across its top;
   - a text input made in a file display goes across the file display's top, starting with its
     folder;
   - a file display made in a file display goes across its top too.

   A window shows displays across its top or has sections, not both, and a section shows one display
   or holds sections, not both (rule 6). The new displays (text, image, console, text.view,
   source.view, button, progress) go only in a section. Each of these is refused before the run where
   the checker can see the lines, and otherwise when the line runs.

8. **The old spellings are refused, and the sentence names the new** (the author's ruling D of
   2026-10-10):

   | written today | written now |
   |---|---|
   | `w.satellite.section("top", 100%, 50%)` | `w.satellite.window.section("top", 100%, 50%)` |
   | `.satellite.widgets.display.file("files")` | `.satellite.window.display.file("files")` |
   | `.satellite.widgets.display.input("folder")` | `.satellite.window.display.text.input()` |
   | `.file("files", 100%, 100%)`, a display given a size | a section of that size, with the display in it |

   Their rows stay in words.tsv (1 6 18 49 1 1, 49 1 2 and 52): taking a row out would move the code
   of every later word (make_words.py:16-37). A `.satellite.window.` that is not one of the new words
   is refused with `window is the start of a word: .satellite.window.section(...) or
   .satellite.window.display.file(...)`. An object's own capsule named `section` still runs.

9. **A progress display is set by calling its name.** `bar(75)`, on a line of its own, sets the
   progress display named bar to 75 out of 100.

   - It takes a whole number from 0, empty, to 100, full. A number past 100 or below 0 is refused
     with S410, and a value that is not a whole number (a float, a fraction, a percentage, text)
     with S301. Its sentences name what it takes: `bar(75)`, or `bar.satellite.value(75%)` for a
     percentage.
   - It answers nothing, so it is a line of its own, as `args.x(6)` is. Inside an expression it is
     refused before the run.
   - Another capsule, given the window, sets it with today's word through the window:
     `its_window.right.bar.satellite.value(75%)`. Whether the call goes through the window too is a
     question in Section 6.
   - A progress display's name is refused before the run when a spacesuit's supertype has a capsule
     of that name, because the call would reach that capsule.
   - Only a progress display's name is called this way. Any other name followed by brackets is a
     capsule call, as today.

---

## 3. What satl has today

- **A section** is one word, 1 6 18 52 `satellite.variable.window.section(name, width, height)`
  (words/words.tsv:641), written `.satellite.section` (section_token 0x0B79, REGISTRY.satellite:446).
  It runs through satellite-numbers/words/1.6.18.52.cpp, a_piece_made (window_methods.cpp:186-205)
  and window_widget_made (window_widgets.cpp:310-427), and answers the section. No word can be
  numbered under 1 6 18 52, an argument form; the next free number under the window is 1 6 18 54
  (words/satellite_words.hpp's next-free rows).
- **The requested spellings are refused today.** `.satellite.window.section(` finds no method
  spelling, falls to the word reader, which takes `satellite.window` (1 27, the makers' family), and
  the checker answers with a generic S110: "main_window is followed by something that is not a
  method call" (bytecode_registry.cpp:540-608; probed). A bare `top.` is S201 NAME_NOT_DECLARED. The
  lexer already reads the longest dotted spelling after `.satellite.` (bytecode_registry.cpp:531-566),
  as it reads widgets.display.file (0x0B97). So new spellings need rows, not a lexer change.
  widgets.display itself has no spelling: the lexer reads widgets, then the console's display, and
  the parts rule takes that display as the family only before .file or .input
  (window_shapes.cpp:173-192).
- **The checker judges a section's shape only**: three arguments, a quoted name, and a name a widget
  may have (section_written_wrong, program_check.cpp:685-706, asked at 1136-1137, 2360-2365 and
  2433-2438; a_widget_name_fits, window_shapes.cpp:249-268). It judges no size: 0%, 150%, 1/2,
  "wide" and a percent name all pass it (probed). Its section names are one set for the whole
  program, not one per window (program_check.cpp:4212-4297). It keeps a name's type, never its value
  (program_check.cpp:6-28; satellite_legal.hpp:101-104). The only values it works out before the run
  are satellite.library values (read_library_values, library_values.cpp:393-421).
- **Sizes at the run** (extent_of, window_readers.cpp:97-129): a percent, whether a literal, a name or
  `100% / 3`, is a share, kept in millionths and cut down (100% / 3 is 333333, :72-91). A
  satellite.variable.number is pixels. A fraction is refused with S301 (place_of, :170-193). At or
  below 0%, past 3276700%, finer than 0.0001%, and pixels outside 1 to 32767, are S410, exit 19.
- **Names at the run** are kept in the list of the one window or section a widget was made in
  (satellite_window.hpp:401-411). `main_window.top` searches main_window's list (window_widget_named,
  window_widgets.cpp:452-465). A second "top" in one owner is refused when its line runs (:353-364).
  The same name in two windows, or under two sections, is allowed, and a section and a display share
  their owner's list.
- **Declared names** belong to one capsule, and there are no globals (program_check.cpp:26-28,
  4298-4353). With arguments.access on, a name ends at its block's `}` (close_the_block, :359-391). A
  new name is refused if it already names a capsule, namespace, included file, field or variable
  there (a_name_it_may_take, :1803-1826; declared twice, :1830-1844). No word declares a variable
  today, but the section word's function is handed the running capsule's table
  (ExpressionContext::variables, expression.hpp:57-63; VariableTable::declare, value.hpp:142-158).
  The checker can declare only in check_statement, because names_in_statement takes a const
  DeclaredNames& (program_check.cpp:2031-2036). A widget name may be `satellite`, `xff` or `b01`,
  which no line can then reach (probed). `satellite.variable.window top = main_window.top` works now.
- **The layout** (an_area_is_laid_out, window_widgets.cpp:174-205, in an anonymous namespace in a file
  compiled only with GTK) goes left to right in the order made. A new row starts under the tallest
  when the next would pass the right edge at the width it is given, and a shorter section leaves a
  hole under it that nothing fills. A child is never given less than its GTK minimum (:188-194), and
  what passes the edge is cut off (GTK_OVERFLOW_HIDDEN, :234). Each share is rounded down at every
  allocation (pixels_of, :158-164): three 333333-millionth sections of 800 pixels are 266 each and
  leave 2, and two 50% of 553 leave 1. Nothing anywhere adds sizes up or knows what is covered.
- **100% is the sections' area**: the window under its header bar, its menu bar
  (window_menu_bar.cpp:109-110) and a strip of widgets made on it with no size
  (a_windows_widgets_box, window_widgets.cpp:279-306). satl sets only the GtkWindow's default size
  (satellite_window.cpp:200), and GTK 4.24 puts a client-side title bar inside it
  (gtkwindow.c:4719-4771). An 800x600 window with a subtitle gives its sections 800 by 553
  (PROGRESS.md:217-218; prove-sections.sh:241-242), while its `.satellite.height` answers 600
  (window_size_of, satellite_window.cpp:636-672). A subtitle makes satl put up its own bar, which
  forces a client-side title bar on any compositor (gtk_window_set_titlebar, gtkwindow.c:3123-3138,
  3159-3200). With no subtitle, GTK makes its own default-decoration bar when the window is
  realized, only if the compositor does not draw title bars itself (gtkwindow.c:3976-4007,
  4490-4503). The by-hand proofs click at points worked out for an 800x600 window cornered at
  (240, 100) on mutter's 1280x800 monitor (press-a-button.sh:216-224;
  prove-canvas-tabs-menus.sh:321-325).
- **Numbers**: a percent is a whole number of 10^-32 percents (satellite_percentage.hpp), and a
  division rounds half away from zero. So three `100% / 3` are 99.99999999999999999999999999999999%
  and six `100% / 6` are 100.00000000000000000000000000000002%, and neither is `== 100%`. Three `1/3`
  are 3/3, which is `== 1/1`: a fraction is two floats, compared by cross-multiplying
  (object_fraction.cpp:85-109). A percent meeting a fraction is S210, not built, and nothing turns
  one into the other. `800x600` is two arguments, not a value (bytecode_registry.cpp:505-517). There
  is no pixel suffix: `250px` is S201.
- **Two displays**: `widgets.display.file(name)`, 1 6 18 49 1 1 (0x0B97), and
  `widgets.display.input(name)`, 1 6 18 49 1 2 (0x0B98), each taking 1 or 3 arguments. A file display
  is a GtkListView over a GtkDirectoryList, folders first, starting in $HOME (a_file_display_of,
  window_widgets.cpp:113-142). An input is a GtkEntry, of the piece kind text_box. In a section a
  display given no size is 100% by 100% (:180-187). On a window it goes across the top with no size,
  or into the sections' area with one (:393-405). Every maker assumes a name:
  - a_piece_made reads the first argument as the name (window_methods.cpp:186-190);
  - window_widget_made refuses an empty or bad name (:351; window_shapes.cpp:254) and a duplicate
    (:353-364), and files each piece under its name (:421, :425);
  - a window with more pieces than named widgets counts as holding appended pieces
    (window_holds_appended_pieces, :437-449). That refusal is asked first (:340-345).
- **The pieces nearest the new displays** are made by `satellite.window.*` and placed only with
  `.satellite.append`, which refuses a section (satellite_window.cpp:281-287) and a sectioned window
  (:401-409):
  - a label is a GtkLabel of one string, where "\n" breaks the line (window_pieces.cpp:50);
  - a text area is a GtkTextView asking for 300x150, in no scroller (:65-77);
  - a picture is gdk_texture_new_from_filename, then gtk_picture_new_for_paintable (:334, :372). Its
    path is used as written, relative to satl's working folder, not resolved as satellite.file's
    paths are (file_calls.cpp:185-230). Measured: it loads PNG, JPEG, TIFF, GIF, BMP, ICO, TGA, ANI,
    PNM, XPM and XBM, and not SVG or WebP.
- **A console** (satellite.console.new, 1 5 10) is a frame: console_new (window_console.cpp:517-538)
  calls frame_new and a_terminal_to_type_in (:342-431), which appends a VteTerminal in a scroller to
  the frame's column (:396), on a pty whose other end satl holds.
  - It takes the keyboard when it is made (:430).
  - is_a_window() is true for it (satellite_window.hpp:160), and 24 places ask it. Six of them then
    hand its widget to a gtk_window_* call (satellite_window.cpp:541, 557, 580, 612, 677, 692).
  - Its words test for the console kind at a_console_that_is_open (window_console.cpp:61-77),
    `.clear()` (satellite-numbers/words/1.6.18.41.cpp:38), a bare `.typed` (1.6.18.44.cpp:39), the
    scroll words (satellite_window.cpp:176-186), the look (window_look.cpp:168, 211) and the `.text`
    refusals (window_asks.cpp:137, 205). Nothing can hold it or be held by it
    (window_widgets.cpp:315-318).
  - It is cleaned up from its window's destroy (the_console_went_away, window_console.cpp:189-215,
    connected at :429). By then GTK has disposed every widget inside the window, the terminal too
    (window_desk.cpp:243-249; gtkwindow.c:2716-2719), so that cleanup makes no vte_ call. The desk's
    let_go_of_every_piece (window_desk.cpp:197-228) runs from the same destroy.

  VTE 0.84.1 sizes itself to what it is given, at least 2 cells by 1 (vte.cc:8419-8565). A real
  shell is GTK-18 (GTK_AND_NO_DEPENDENCIES.md:2391-2417), not built.
- **A source view**: GtkSourceView is not vendored. It would bring libxml2, which is not vendored
  either, and a satellite .lang file to write. GtkTextView, its tags and its gutter are in the
  carried GTK 4.24. satl's lexer, tokenise_one_line (bytecode_registry.cpp:388-728), gives each
  code and the byte offset where its token began, never where it ends (bytecode_registry.hpp:146-152).
  Byte offsets are what gtk_text_buffer_get_iter_at_line_index takes. A fraction is three codes: a
  number, fraction_token 0x0309 and a number (token_codes.hpp:71). The author's sketches spelled a
  source view `main_window.widgets.text.source("source_display")` (2026-10-09,
  satellite_source_editor/source_editor_main.satl:20) and
  `widget.source_view("satellite_source_view1", 100x100)` (2026-10-12,
  ~/code/satl/source_editor/source_editor-001/satellite_source_editor_main.satl:8).
- **When the window is handed over**: main returns, programs and threads close, and the press loop
  starts (structured-library.cpp:552-620; windows_run_until_they_are_closed, window_run.cpp:71-114).
  The loop runs every queued event's capsule on the interpreter's thread. The window has been on the
  screen since `new` (satellite_window.cpp:226); MS-4 would change that, and waits on the author. A
  window or section keeps no line number (satellite_window.hpp:410-422). A refusal at that point
  takes every window down (close_the_desk_now, window_desk.cpp:433-449, through
  close_the_program_windows_now, :401-427).
- **What builds without GTK**: satl must still build with neither gtk4 nor vte (047-window.mk:24-28).
  The window files are compiled only with GTK (:330-361). The checker and the window's readers and
  shapes are compiled in every build (040-sources.mk:134, 155-156).
- **Codes**: the next machine code is 85; 84 is time_below_zero (machine_codes.hpp:186). The MISSING
  family, S150 to S156 (s_codes.hpp:88-118), is proven by build/missing_cases
  (satellite/machine/missing_cases.cpp), with one check.sh row a code (check.sh:216-229).
- **Tests**: check.sh never gets past `satellite.window.new`: with no screen, S730 stops the run at
  the first window word (headless(), check.sh:2526-2527). So it proves only what the checker
  refuses. Its section rows are 2624-2646, and 8476-8477 counts the words under 1 6 18 and their
  files. build/count_cases is a check.sh harness that links the lexer (065-tests.mk:63-65, 76-80).
  prove-sections.sh runs 17 checks on a headless mutter, by hand, and mutter is not installed on
  this machine. No satellite_test program uses a window, and none should, because their child satl
  keeps DISPLAY.
- **What writes sections**:
  - check.sh, prove-sections.sh, and the window help page
    (satellite.help/satellite.window/help_text.txt:44-55, 142-154);
  - the author's satellite_source_editor/source_editor_main.satl, whose one section is 100% by
    250 pixels;
  - none of the 602 files under ~/code/satl.

  utility/satellite_spelling.py cannot respell a word already under satellite (:1363-1364) and does
  not see dotted spellings (:85-90). Run on a file in the new spelling, it writes
  `main_window.satellite.window.satellite.section(` (probed).
- **A button** (satellite.window.button, 1 27 2) is a GtkButton (window_pieces.cpp:49) made as a free
  piece, so nothing can put one in a section today. `.satellite.pressed(capsule)` (1 6 18 6) keeps
  the capsule's key and connects "clicked" once (window_answers.cpp:271-298). A click is queued
  (the_desk_saw_something, window_desk.cpp:355-380) and run after main returns
  (structured-library.cpp:620-652). The capsule may declare nothing, the piece, or the piece and its
  window (a_press_can_run, program_check.cpp:1781-1801).
- **A capsule's name comes first** in every window word that takes one. That rule was set when
  GTK-11 was built on 2026-09-21, not by the author: GTK-11's plan had written
  `satellite.window.ask("delete it?", when_answered)` (GTK_AND_NO_DEPENDENCIES.md:1418), and the
  build turned it round (window_shapes.cpp:284-295). The checker looks for it straight after `(`
  (program_check.cpp:2116-2152, 2290-2291), and the walker only as the first argument
  (expression.cpp:555). Written last, it is refused today (probed). LINK_PLAN's LINK-2 plans to read
  one from the last argument (LINK_PLAN.md:239-243).
- **A progress bar** (satellite.window.progress(), 1 27 10) is a GtkProgressBar asking for 300 pixels
  of width (window_pieces.cpp:243-244). `.satellite.value(50%)` sets it, and takes only a percentage:
  a plain number is refused with S301 (1.6.18.10.cpp:59-71), and past 100% is drawn full
  (window_state.cpp:163-171). The percentage rule was a choice made in GTK-4 on 2026-09-21, not the
  author's ruling (GTK_AND_NO_DEPENDENCIES.md:1005-1012, 1034-1037).
- **A name followed by `(`** is always a capsule call, in the checker (check_statement,
  program_check.cpp:3763-3772; names_in_statement, 2482-2520) and in the walker
  (program_walk.cpp:2230-2246; expression.cpp:1727-1738). A progress bar's name called, `bar(75)`, is
  "no capsule named bar" today (probed). The nearest precedent is the arguments' row call,
  `args.x(6)`, which is the same line as `args.x = 6` and a line of its own
  (satellite.help/arguments/help_text.txt:79-81; program_check.cpp:2521-2532). A variable may share
  its name with a capsule of a spacesuit's supertype, which the call then reaches
  (capsule_reach.cpp:207-232 against :62-81; probed). An included file's capsule is reached only as
  `other.bar(...)` (:117-126).
- **NM-7's A, B, C and H** were built as proposed on 2026-10-10 and are still recorded as unanswered
  (NEW_MILESTONES.md:503-506, 525-528): A is rows, B is shares that stay shares, C is sections in
  sections, and H is sections or `.append`. This plan builds on them as they are. The author's
  `display.button` (2026-10-20) is H's first way for a button: a button made in a section. H's other
  way, `.append` over the sections, is still the author's.

---

## 4. The milestones

### SEC-0 — the new spellings, and a section's name is a name

No picture changes: sections, the file display and the text input are written the new way and are
reached by their names.

- REGISTRY.satellite gains these method spellings, and make_token_codes.py regenerates
  token_codes.hpp:
  - `window`, a part that runs nothing, as `widgets` is (0x0B73);
  - `window.display`, the first dotted spelling that is a part, since widgets.display has none of
    its own;
  - `window.section`, `window.display.file` and `window.display.text.input`.

  a_part (window_shapes.cpp:173-178) learns `window` and `window.display`, and
  word_in_parts_written_wrong names the words each one begins. `window.display.text` gets no
  spelling until SEC-3 makes it run. The lexer reads `window.display.text.input` whole, and before
  SEC-3 a `.satellite.window.display.text(` is read as the part `window.display` followed by
  `.text`, which the parts rule answers. The codes come from the method family's free row (0x0B9B
  up). LINK_PLAN draws on the same row, so the codes are whichever are next when this lands.
- words/words_004.tsv gains these rows, and make_words.py and make_word_codes.py are run. The numbers
  are proposed; words.tsv is the author's.

  ```
  1 6 18 54          satellite.variable.window.window
  1 6 18 54 1        satellite.variable.window.window.section(name, width, height)
  1 6 18 54 2        satellite.variable.window.window.display
  1 6 18 54 2 1      satellite.variable.window.window.display.file(name)
  1 6 18 54 2 2      satellite.variable.window.window.display.text
  1 6 18 54 2 2 1    satellite.variable.window.window.display.text.input()
  ```

  1 6 18 54, 54 2 and 54 2 2 run nothing, as 49 and 49 1 run nothing today. Each word that runs gets
  its file in satellite-numbers/words/, taking over the code of 1.6.18.52,
  1.6.18.49.1.1 and 1.6.18.49.1.2. Each needs its row in word_table.cpp and .hpp and its name in
  WORD_SOURCES. These follow too:
  - window_number_of and its static_assert (window_methods.cpp:30-101);
  - window_method_arity (3, 1 and 0), makes_a_widget and kind_a_method_answers;
  - section_written_wrong's three callers, which test section_token (program_check.cpp:1136-1137,
    2360-2365, 2433-2438), test the new window.section code;
  - the checker's name pass (program_check.cpp:4212-4297);
  - check.sh:8476-8477's counts.
- A display with no name: `display.text.input()` is the first, and SEC-3 to SEC-6 make the rest the
  same way, all but the progress display, which has a name.
  - A maker beside a_piece_made reads no name and no size.
  - window_widget_made takes an empty name to mean none, asking neither a_widget_name_fits nor the
    duplicate check.
  - window_holds_appended_pieces counts only what `.satellite.append` placed, so a display never
    counts as appended. It can do that with a flag set in window_append, or by filing a nameless
    display in owner.widgets under "".
  - Rule 7's refusals are asked before the `.append` refusal (window_widgets.cpp:340).
- Rule 4, in the checker: check_statement declares the names that a section line and a
  `display.file` line make. It uses a_name_it_may_take's and declared twice's refusals, and two that
  a widget name never had: `satellite`, and the shape of a hex or binary number. declared twice
  (program_check.cpp:1830-1844) learns a name that a section or file display line declared, and says
  so (rule 4). S202's standing sentence (s_codes.hpp:176-180) names a section or display line among
  what declares a name.
- Rule 4, in the walker: each of those words declares its name in context.variables, holding
  Value::of_window of what it made. A block ends the name as it ends any name (run_block,
  program_walk.cpp:794-824).
- Rules 6 and 7 for the two displays that exist: `display.file` takes one argument and
  `display.text.input` takes none. Four things are refused:
  - a second display in a section;
  - a display in a section that holds sections;
  - a section in a section that shows a display;
  - a display across the top of a window that has sections, and a section in a window that shows
    one.

  The checker refuses each of them before the run where it can see the lines: straight down one
  capsule, in no block, on a name declared there and never given another value there. That is a
  section's or a display's name, declared by its own line, or a window's name declared from
  `satellite.window.new`, used only for its own lines and not closed there (rule 5).
  window_widget_made (window_widgets.cpp:310-427) refuses the rest when the line runs.
- Rule 8: the old spellings are refused before the run through a_word_in_parts and
  word_in_parts_written_wrong (window_shapes.cpp:187-241). Every window link asks them before the
  period rule (program_check.cpp:1331-1335, 1421-1424, 1530-1535, 1562-1565, 2105-2106;
  expression.cpp:478-486). The sentence has the shape of window_shapes.cpp:237-240's:
  `main_window.satellite.section is written main_window.satellite.window.section now -- a window's
  sections and displays are under satellite.window`.
- Every sentence that names today's spellings names the new ones:
  - program_check.cpp:2347-2348, 2362, 2415-2416 and 2435;
  - window_shapes.cpp:54-57, 221-222 and 229;
  - window_widgets.cpp:322, 329, 334 and 459-471;
  - satellite_window.cpp:285, window_answers.cpp:337 and window_asks.cpp:157;
  - the number files' own strings.
- Rewritten by hand: check.sh's rows (2603-2646, 8480-8522), the help page, and prove-sections.sh's
  six programs, including its sections.satl, whose left half holds a display and a section.
  utility/satellite_spelling.py learns the new spellings, so that a later run leaves them alone.
- The author's satellite_source_editor/source_editor_main.satl is the author's and is not edited.
  From SEC-0 on it is refused until it is rewritten; Section 6 has a version of it.
- satellite.help/satellite.window describes sections, their names and the displays the new way.

**Done when** check.sh passes with rows for:
- the author's percent, window, section and display.file lines, in the new spelling, passing the
  checker;
- a section and a file display reached by their bare names, and a section reached as `its_window.top`
  in a capsule given the window;
- each old spelling refused, naming the new, and a `.satellite.window.` that is no word refused;
- refused names: two sections called "top" in one capsule, a section named like a variable,
  `satellite`, `xff`, and a declaration of the name a section line has declared, with its own
  sentence;
- refused before the run: a display given a size, two displays in one section, a display in a
  section that holds sections, a section in a section that shows a display, a display across the
  top of a sectioned window, and a section in a window that shows a display across its top;
- an object's own `section` capsule still running;
- the generated headers, byte for byte.

prove-sections.sh, rewritten, passes on mutter (`sudo apt install mutter`). Its check 15 now makes
its input across the top of a sectioned window in a capsule given the window, where the checker
cannot see it, and shows it refused when its line runs (exit 13 and its sentence). One new check
does the same for a second display in a section.

### SEC-1 — fractions, the author's ranges, and a window's size is its inside

- extent_of (window_readers.cpp:97-129) takes a fraction. Every size keeps the exact value the
  program gave it (a percent's 10^-32 units, a fraction's two parts, or pixels) beside the millionths
  the layout uses (AnExtent, satellite_window.hpp:59-72). SEC-2 works from the exact value.
- Rule 2's ranges are judged when the line runs: a share above 0, no finer than a millionth, and at
  most 100% or 1/1, and pixels from 1 to what holds them. The S410 sentences say the range:
  `top's height is a share of what holds it, up to 100% -- all of it -- and was given 150%`.
- Before the run, the checker refuses a size written as a literal, for the first time:
  - with S410 (exit 19), a share at or below 0, finer than a millionth, or past 100% or 1/1, and
    pixels below 1 or past 32767;
  - with S301 (exit 27), a string, a float or a bool, as the run does, naming what a size takes.

  Pixels past what holds them are refused when the line runs. From SEC-2 on they are also refused
  before the run, once its pass knows the holder.
- Rule 3, measuring the bar GTK puts up and keeping GTK's own choice of decoration. A handler on the
  GtkWindow's realize runs after GTK has chosen its decoration and made its default bar, and before
  the window is shown (realize is RUN_FIRST, gtkwidget.c:1804-1810; gtkwindow.c:4021-4024). It
  finds the window's child with the titlebar class: satl's subtitle bar, GTK's default bar, or none
  where the compositor draws the title bar itself. It measures that child's natural height at the
  asked width with gtk_widget_measure. Then it asks for
  `gtk_window_set_default_size(asked width, asked height + that height)`.
  - `.resize` (satellite_window.cpp:609-634) adds the same measured bar.
  - A menu bar added later makes the window taller by its own height.
  - window_size_of (:636-672) answers the inside.
  - No window is given a bar it does not have today.
- press-a-button.sh, prove-canvas-tabs-menus.sh and prove-sections.sh aim their clicks for the taller
  window (mutter centres it, so its corner rises by about half its bar), or read the window's corner
  from mutter.
- The help pages for satellite.window and satellite.console say that a window's size is its inside.

**Done when** check.sh passes with rows for:
- passing the checker: `1/2`, `1 / 3`, a fraction name, and `1/2` beside `250`;
- refused before the run, each with its code: `0%`, `150%`, `3/2` and `0` (S410), and `"wide"` and
  `2.5` (S301).

On mutter, prove-sections.sh shows:
- a 100% by 100% section of an 800x600 window answering 800 and 600, and the window's own
  `.satellite.height` answering 600, with a subtitle and without one;
- a 1/2 section beside a 50% one, the same width;
- a section 600 pixels tall reaching the window's bottom edge.

press-a-button.sh, prove-canvas-tabs-menus.sh and prove-console.sh pass by hand on mutter.

### SEC-2 — the whole window is sectioned off

- **The judge** is one function over a window's own size and its sections' exact sizes. It lays them
  out by rule 1 at that size, with rule 5's tolerance, and answers "whole" or the first place without
  a section. Beside its verdict it gives the rows it chose and each section's drawn size, a
  section's own sections included, which it lays out but does not judge. It sits in a file compiled
  in every build, with no GTK in it: beside extent_of in bytecode/window_readers.cpp, or in a new
  INTERPRETER_SOURCES file. The checker, the run and an_area_is_laid_out all call it.
- an_area_is_laid_out (window_widgets.cpp:174-205) takes its rows from the judge's answer: a
  window's area its rows at the window's own size, and a section's area its rows at its own drawn
  size. The size they were chosen at is stored on the area, as kSizeKey is stored.
  `.satellite.resize` changes it, and a person's resize does not. Each edge, across a row and down
  the rows, goes at the pixel nearest the running total of the exact sizes at the size given. Today
  each share is rounded down on its own (pixels_of, :158-164).
- From SEC-2 on, rule 2's pixel range for a section made in a section is checked against that
  section's drawn size, from the judge.
- **At the run**, every open window that has a section is judged when main returns, between
  structured-library.cpp:576 and 620. After each capsule the press loop runs (window_run.cpp:100-108),
  every window whose sections or size that capsule changed is judged. Each section keeps the row and
  place of its line, so that the report points at the window's last section line.
- **Before the run**, the checker judges each window it can see whole (rule 5), and refuses one that
  is not whole. This is one pass over a capsule's rows once its names are known. It works out each
  size with the run's own arithmetic, as read_library_values does (library_values.cpp:393-421), from
  literals, satellite.library values, and names declared there with one of those and never given
  another value in that capsule.
- **The code** is machine code 85, missing_section, which is S157 MISSING_SECTION. It goes in:
  - machine_codes.hpp, and s_codes.hpp with its standing sentence: `a window laid out in sections is
    sectioned off whole -- across, each row's sections add up to all of its width and are all as tall
    as the row; down, its rows add up to all of its height; and no section passes its edge`;
  - missing_cases.cpp's register, and check.sh's MISSING row (216-229);
  - SATELLITE_ERROR.md Part 4 and its MISSING table (265-276), and SCRATCH.md/MISSING_SYNTAX.md.
- The tree's own windows are made whole, or expect S157:
  - prove-sections.sh's wrap.satl is still open when main returns, with its lower right 50% by 90%
    unsectioned, so it gets a fourth section there;
  - check.sh's rows that make part of a window (2628 and others).

**Done when** check.sh passes with rows for:
- the author's lines in Section 1 without the two right_half lines, refused before the run with
  Section 1's report, word for word, S157, exit 85;
- rule 5's three gaps and its bottom edge, refused before the run, each with S157, exit 85 and its
  sentence;
- whole windows passing the checker:
  - two 50% halves, three `100% / 3`, three `1/3` and six `100% / 6`;
  - `1/2` beside `50%`, and `400` beside `50%` in an 800-pixel-wide window;
  - `fifty_percent` twice;
  - a 250-pixel row over a 350-pixel row in an 800x600 window;
- a near miss refused before the run with its sentence. Three `33.3%` across 800 pixels give `the
  right 0.1% of main_window, in the row of a, b and c (1 of its 800 pixels), has no section -- you
  need to section off the entire window`;
- windows the checker cannot see passing it: one sized by a value the run works out, and one half
  made inside an `if`;
- the MISSING register's row for S157.

On mutter, prove-sections.sh shows:
- three `100% / 3` sections of an 800-pixel window are 267, 266 and 267 wide, and the last reaches
  800;
- two halves of a 600-pixel height are 300 and 300;
- a window whose second half is made inside an `if` that runs is whole and runs;
- one whose `if` does not run is refused when main returns, with exit 85 and the report at its last
  section line;
- a capsule run by `.clicked` that makes a window with one 50% section is refused with exit 85 when
  it returns;
- a `.satellite.resize` that makes a window narrower than a section given in pixels is refused at
  its right edge, and one that leaves a row of pixels short is refused as a gap.

### SEC-3 — `display.text` and `display.image`

- `display.text` needs a new shape of argument count, at least one. window_method_arity gives one
  count and window_method_also_takes one more (window_shapes.cpp:63-161), so the checker
  (program_check.cpp:1127-1132) and the walker (window_methods.cpp:144-149) learn it.
- `display.text` makes a label: its strings joined with newlines into one GtkLabel
  (window_pieces.cpp:50), set at the section's top left and not wrapped.
- `window.display.text` both runs and begins `.text.input` (and `.text.view` from SEC-4). It is the
  first method spelling that does both; the word satellite.console.display (1 5 1), with `.center`
  under it, is the precedent.
  - a_part (window_shapes.cpp:173-178) stays as it is. It takes one code with no context, so adding
    this one would refuse every right `display.text("str1")`.
  - a_word_in_parts (:187-192) takes `window.display.text` as a part only when a method period
    follows it, as its display_token clause already looks two codes on.
  - word_in_parts_written_wrong's begun-and-not-finished sentence (:226-230) then names the words it
    begins. A bracket after it means it is the word itself.
- `display.image` makes a picture through a_texture_from and gtk_picture_new_for_paintable
  (window_pieces.cpp:322-384). It never uses gtk_picture_new_for_filename, which sends a GIF, a BMP
  and the rest to GtkSvg and shows nothing (gdktextureutils.c:419-446). The path is resolved as
  satellite.file resolves one (file_calls.cpp:185-230).
- A text or an image display made on anything but a section is refused, with a sentence that names a
  section: `main_window.satellite.window.display.text goes in a section --
  main_window.satellite.window.section("top", 100%, 100%)`. It is refused before the run where the
  checker can see its receiver (SEC-0's condition), and otherwise by window_widget_made when the line
  runs. SEC-0's four refusals cover the new displays too.
- REGISTRY gains `window.display.text`, which now runs, and `window.display.image`. Words:

  ```
  1 6 18 54 2 2 2    satellite.variable.window.window.display.text(words)
  1 6 18 54 2 3      satellite.variable.window.window.display.image(path)
  ```

- satellite.help/satellite.window describes `display.text` and `display.image`.

**Done when** check.sh passes with rows for:
- text with one string and with three, and an image, passing the checker;
- refused before the run: text with no string, an unfinished `display.text.` written with nothing
  after it, a text display made on a window with no sections, and one made in a file display.

On mutter, prove-sections.sh shows:
- the text display's `.satellite.text` reading "str1\nstr2";
- an image as wide or as tall as its section;
- an image file that is not there exiting 39.

### SEC-4 — `display.text.view` and `display.source.view`

- `display.text.view` is text_area's GtkTextView (window_pieces.cpp:65-77) inside a
  GtkScrolledWindow, with no 300x150 request, because the layout never gives a child less than its
  minimum. The handle keeps the text view beside the scroller, because window_asks.cpp:42 and 184
  and window_answers.cpp:81 cast the widget to one.
- `display.source.view` is a text view whose buffer has four tags. On every change, satl lexes each
  line that changed with tokenise_one_line and puts the tags on by byte offset
  (gtk_text_buffer_get_iter_at_line_index):
  - a word (word::is_word_code) and a method after `.satellite.` (token::is_method_code);
  - a string (0x0903);
  - a number of any kind: 0x0902, 0x090B, 0x090C, a binary, a hex, and a fraction's 0x0309 between
    its two numbers, so `1/2` is one run;
  - a comment (0x0803), to the end of its line.

  `.satellite.` itself, and the x of `800x600`, stay plain, as periods and commas do.
- A run ends where the next code with another offset begins, less the blanks before it. A comment's
  run, and an unclosed string's run, end at the line's end. Each line is coloured on its own, so a
  statement written across lines is coloured line by line.
- The colouring is one function from a line to its coloured runs, in bytecode/ beside the lexer, with
  no GTK in it. Its check is built for check.sh the way build/count_cases is, linking
  bytecode_registry.cpp.
- Each view made on anything but a section is refused, as SEC-3's displays are.
- REGISTRY gains `window.display.text.view` and `window.display.source.view`, and the parts rule
  names both among the words that `window.display` and `window.display.text` begin.
  `window.display.source` gets no spelling, as `window.display.text` had none before SEC-3, and its
  word row 1 6 18 54 2 4 runs nothing. Words:

  ```
  1 6 18 54 2 2 3    satellite.variable.window.window.display.text.view()
  1 6 18 54 2 4      satellite.variable.window.window.display.source
  1 6 18 54 2 4 1    satellite.variable.window.window.display.source.view()
  ```

- satellite.help/satellite.window describes `display.text.view` and `display.source.view`.

**Done when** check.sh passes with rows for:
- both views passing the checker;
- the colouring of `satellite.console.display("hi") // done`, of `1/2`, and of a line with an é
  inside its quotes, coming out at the right bytes.

On mutter, prove-sections.sh shows `.satellite.text("x")` and then `.satellite.text` reading "x", and
a text longer than its section leaving the view as tall as the section.

### SEC-5 — `display.console`

- It gets a new piece kind, a console in a section, with its row in kPieceNames
  (satellite_window.hpp:561-590). is_a_window() stays false for it, so no window word takes it for a
  frame. Every place that tests for the console kind accepts it, through one test meaning "a console
  of either kind":
  - a_console_that_is_open (window_console.cpp:61-77);
  - `.clear()` (satellite-numbers/words/1.6.18.41.cpp:38) and a bare `.typed` (1.6.18.44.cpp:39);
  - the scroll refusal and the scrolling it routes (satellite_window.cpp:176-186);
  - the look (window_look.cpp:168, 211).

  The `.text` refusals (window_asks.cpp:137, 205) refuse it as they refuse a console.
- a_terminal_to_type_in (window_console.cpp:342-431) makes its terminal for a window's column or for
  a section (gtk_widget_set_parent into the SatlArea, window_widgets.cpp:393-401). It keeps the
  spare-pty stopgap (:408-427), and takes the keyboard only for a console that is its own window
  (:430). VTE takes the keyboard by itself when the terminal is clicked.
- The section's maker opens the pty's slave as console_new does (window_console.cpp:528). With no
  pty or no slave, it takes the terminal out of the section and refuses the line with console_new's
  sentence, and the window and its other sections stay open.
- A console made on anything but a section is refused, as SEC-3's displays are.
  `satellite.console.new` still makes a console that is its own window.
- When its window closes, let_go_of_every_piece (window_desk.cpp:197-228) runs from the window's
  destroy, after GTK has disposed every widget in it. It touches only satl's own objects: it removes
  the `.typed` watch and the width timer with g_source_remove, lets go of satl's pty with
  g_object_unref, and sets terminal, scroller and vertical_bar to null. It makes no vte_ or gtk_
  call on any of them, as the_console_went_away makes none today (window_console.cpp:196-212).
- check.sh's pinned console lines (4104-4140) follow the change.
- REGISTRY gains `window.display.console`. Words:

  ```
  1 6 18 54 2 5      satellite.variable.window.window.display.console()
  ```

- satellite.help/satellite.window and satellite.help/satellite.console describe `display.console`.

**Done when** check.sh passes with a row for the console passing the checker.

By hand on mutter, as prove-console.sh does:
- a line displayed into it shows, `.clear()` empties it, and `.scroll.horizontal(0)` makes its
  columns follow the section's width;
- a line typed into it runs its `.typed` capsule, and a bare `.typed` then answers that line;
- with a text input in the next section: after a click on the input, a typed line does not run the
  console's `.typed`; after a click on the console, it does;
- with a second window keeping the run open, a person closes the window that holds the console. The
  run goes on with no crash and no Gtk-CRITICAL, the second window still answers, and closing it
  ends the run with exit 0.

### SEC-6 — `display.button` and `display.progress`

It comes after SEC-3, whose refusal of a display made outside a section it uses, and it can come
before SEC-4 and SEC-5. The word numbers below follow the milestones' order; built in another order,
each takes the next free number then.

- `display.button` makes a button: a GtkButton of the piece kind button, through SEC-0's maker of a
  display with no name. a_widget_for, which makes today's button, sits in an anonymous namespace
  (window_pieces.cpp:34-146), so its gtk_button_new_with_label call is shared or made again. The
  click is wired as window_pressed wires one (window_answers.cpp:271-298), with the key worked out as
  `.pressed(go)` works one out (capsule_key_at, capsule_reach.cpp:274-285).
- The capsule's name is read from the last argument. This holds only for the words that take the
  capsule last, the button and LINK-2's link: for them the checker's block, its name pass and the
  walker take the capsule at the last argument, and work out any argument before it as a value.
  Every other word that takes a capsule's name keeps it first, as today.
  - In the checker, the word joins window_method_takes_a_capsule_name (window_shapes.cpp:275-282).
    The block that wants the name at the first argument (program_check.cpp:2116-2152) looks at the
    last one instead. It also counts the word's two arguments there, because the count check at
    1127-1132 steps over every word that takes a capsule's name.
  - The name pass (2290-2327) knows the capsule's place from where the word was met, as it already
    remembers places (2050-2052), and never by reading backwards into a payload (the warning at
    2041-2048). a_press_can_run (1781-1801) judges what the capsule declares.
  - In the walker, expression.cpp:555 reads the name at the last argument for these words, where
    today it reads one only while no argument has been read.
  - LINK-2 plans the same reading for `.text.link(piece, capsule)` (LINK_PLAN.md:239-243). It is
    built once, for both. The sentences that say a capsule's name goes first then say where it goes:
    window_shapes.cpp:284-295, window_calls.hpp:72-89, GTK_AND_NO_DEPENDENCIES.md:1443-1450 and
    1751-1756, and satellite.help/satellite.window/help_text.txt:87.
  - If the author keeps GTK-11 (Section 6), the button is written `display.button(when_ok, "OK")`.
    It joins window_method_takes_a_capsule_name and window_method_takes_more_after_the_name, its two
    arguments are counted in the 2116 block, and a_press_can_run judges its capsule. The rest of this
    bullet is not needed.
- `display.progress` makes a progress bar: a GtkProgressBar of the piece kind progress, with no
  300-pixel request, because the layout never gives a child less than its minimum. It is made
  through a maker that reads a name and a value, and its first value is judged and set as rule 9
  judges and sets one.
- Its name is declared as SEC-0 declares a section's or a file display's: in check_statement, in
  declared twice's sentence, in context.variables holding Value::of_window, and in the checker's
  name pass (program_check.cpp:4213-4296). That pass takes the progress word's two arguments; today
  it allows a display word 1 or 3 (:4282-4283). The "no widget is named" sentences
  (program_check.cpp:2346, 2414; window_widgets.cpp:452-465) name display.progress among the words
  that make one.
- Rule 9, in the checker:
  - check_statement (program_check.cpp:3743-3800) asks whether a called name is a progress
    display's before it asks capsules.reach (3763-3772). That needs a new record, per capsule, of the
    names a progress display line declared, kept in Where beside DeclaredNames (169-195), because
    the checker keeps a name's type and never its value. The record ends where the name ends, in
    close_the_block (368-391) and in satellite.delete's handler (488-505) beside objects, lists and
    multiples, and when the name is given another value. check_statement reads it only for a name
    still declared.
  - It judges one argument and nothing after the `)`, and refuses a literal out of range or of the
    wrong kind.
  - names_in_statement's `(` arm (2482-2520) refuses the call inside an expression: `bar(75) changes
    the bar and answers nothing, so it is a line of its own`, as the arguments' row call is refused
    (2192-2198, 2521-2532).
- Rule 9, in the walker: run_statements' `(` arm (program_walk.cpp:2230-2246) asks variables.seen and
  the piece's kind before capsules.bare. Without that, the line would be stepped over with only a
  line on stderr (2234-2238). A name it has seen that holds a window of another kind is refused when
  its line runs, not handed to capsules.bare.
- A progress display's name is also refused when a spacesuit's supertype has a capsule of that name,
  which the call would reach: already_names does not walk the lineage that bare() walks
  (capsule_reach.cpp:207-232, against :62-81). An included file's capsule needs nothing, because a
  bare call reaches one only to be told `write other.bar(...)` (:117-126).
- The call runs a number of its own. It has no method token for window_number_of to map, so the
  walker names the number itself. Today's 1 6 18 10 takes only a percentage on a progress bar, and
  its sentences name `.satellite.value` (1.6.18.10.cpp:59-75).
- REGISTRY gains `window.display.button` and `window.display.progress`. Words:

  ```
  1 6 18 54 2 6      satellite.variable.window.window.display.button(text, capsule)
  1 6 18 54 2 7      satellite.variable.window.window.display.progress
  1 6 18 54 2 7 1    satellite.variable.window.window.display.progress(name, value)
  1 6 18 54 2 7 2    satellite.variable.window.window.display.progress.value(number)
  ```

  1 6 18 54 2 7 runs nothing, and 54 2 7 2 is run only by the call.
- satellite.help/satellite.window describes `display.button`, `display.progress` and `bar(75)`.

**Done when** check.sh passes with rows for:
- passing the checker: a button, one whose text is held in a name (`display.button(label, go)`), a
  progress display, `bar(75)`, and `its_window.right.bar.satellite.value(75%)` in a capsule given
  the window;
- refused before the run:
  - `display.button("OK")` and `display.button("OK", go, 1)`, and `display.button(go, "OK")` with a
    sentence that shows the button's own shape, `display.button("OK", go)`;
  - a button whose capsule is not there, and one whose capsule declares three windows;
  - `display.progress("bar", 150)` and `display.progress("bar", "x")`;
  - `bar(75)` inside an expression; `bar(150)`, `bar(-1)` and `bar("x")`; and `bar(75%)`, with its
    sentence naming `bar.satellite.value(75%)`;
  - a progress display named like a capsule of a spacesuit's supertype;
  - a button and a progress display made on a window with no sections;
- a window's name called, `w(50)`, still refused as "no capsule named w".

On mutter, prove-sections.sh shows:
- a real click on a button display runs its capsule, once a click;
- that capsule, given its window, calls `its_window.right.bar.satellite.value(50%)`, and
  `.satellite.value` then reads 50%;
- `bar(75)` in main, then `bar.satellite.value` reading 75%.

---

## 5. Not in this plan

- A shell prompt in a window: GTK-18, `satellite.terminal.new`. `display.console()` is the program's
  own console, which talks to the program.
- Line numbers in a source view, and colouring any language but satellite.
- SVG, WebP and moving pictures in `display.image`.
- The other `satellite.window` pieces as displays: a slider, a checkbox, a choice, a canvas and the
  rest. The author asked for the button and the progress bar.
- Keeping a window that is sectioned in pixels whole when a person resizes it.
- MS-4: a window that is not shown until `.satellite.open()`.
- Windows made at the prompt. This plan is for `satl file.satl` runs.

---

## 6. For the author

To answer before the milestone named:

- **Every window, or only a window with sections? (SEC-2)** The author's rule says the code must use
  100% of the window in sections. Taken word for word, it refuses every window that has no sections:
  windows of buttons placed with `.append`, consoles, and 5 of the 6 window programs in the tree.
  This plan judges only a window that has a section.
- **One display in a section, or several? (SEC-0)** The author's lines put a file display and a text
  display in `section_name_here`. This plan lets a section show one display, which fills it, and
  refuses a second; Section 1 moves the text to a second section. The other way: the displays made
  in one section share it top to bottom, in the order they are made, as the author ruled on
  2026-10-09 for a widget's own widgets.
- **A name for every display? (SEC-0)** `display.file("my_file_list_name")` names its display. The
  author's text, image, console, text.input and text.view take none. This plan writes them as the
  author did: each answers its display, and a program that needs it later holds it, as in
  `satellite.variable.window out = right.satellite.window.display.console()`. That takes today's
  name away from the text input (`widgets.display.input("folder")`). The other way is a name in
  quotes first in each, as in `display.console("output")`, reached bare like a section. The author's
  2026-10-12 sketch gave its source view a name in quotes first,
  `widget.source_view("satellite_source_view1", 100x100)`, and reached it through its window:
  `source_editor_window.satellite_source_view1.write(...)`. On 2026-10-20 the author gave the
  progress display a name in quotes, "this takes a value so you pick a name for it", and gave the
  button none; this plan follows that for both.
- **800x600 is the inside? (SEC-1)** For pixels to run "to the width/height of the window", the
  window's 600 has to be the sections' 600. This plan makes every window's size its inside. Every
  window keeps the title bar it has today and gets taller than today by that bar.
- **Rows as NM-7 built them? (SEC-2)** The whole-window rule follows NM-7's rows, which are A, still
  the author's to confirm. A left column beside two stacked sections is written with the two inside
  a right section:

  ```
  main_window.satellite.window.section("left", 50%, 100%)
  main_window.satellite.window.section("right", 50%, 100%)
  right.satellite.window.section("a", 100%, 50%)
  right.satellite.window.section("b", 100%, 50%)
  ```

  Three sections side by side and under, 50%x100% then 50%x50% then 50%x50%, are refused: the third
  starts a new row under the first, past the bottom. This plan also keeps a window's rows when a
  person resizes it, and chooses them again only when the program's `.satellite.resize` gives it a
  new size; today a row is decided again at every size. Say if sections should be laid out another
  way.
- **The source view's spelling (SEC-4).** This plan writes `display.source.view()`, from the author's
  2026-10-12 `source_view` and the author's "view is for a text_view in GTK". The author's 2026-10-09
  sketch wrote `widgets.text.source(...)`.
- **The capsule last, or first? (SEC-6)** The author wrote `display.button("text", capsule_to_run)`.
  Today a capsule's name comes first in every window word, as in `.every(go, 1000)` and
  `.ask(when_answered, "delete it?")`: "one rule a person can hold in their head beats one line that
  reads slightly better" (window_shapes.cpp:289-295). That rule was a choice made when GTK-13 and
  GTK-11 were built on 2026-09-21, not the author's ruling: their plans had written
  `every(1000, when_a_second_passes)` and `ask("delete it?", when_answered)`. The author has put the
  capsule last both times since, in `link("some", capsule_name)` and here. This plan writes it the
  author's way. Keeping the rule makes it `display.button(capsule_to_run, "text")`. LINK_PLAN's
  Section 6 asks the same about `link`, so one answer settles both.
- **A number or a percentage? (SEC-6)** The author wrote `display.progress("name_of_display",
  1-100)` and `name_of_display(1-100)`, plain numbers. Today's progress bar takes only a percentage,
  `.satellite.value(50%)`, and refuses 50; that was a choice made in GTK-4, not the author's ruling.
  This plan takes the author's number in the new display and its call, and leaves
  `.satellite.value(50%)` as it is. So after `bar(75)`, `bar.satellite.value` answers 75%, and
  comparing it with 75 is refused; it compares with 75%. And `bar.satellite.value(150%)` is still
  drawn full, where `bar(150)` is refused. The other way is a percentage everywhere: `bar(75%)`.
- **The call through a window too? (SEC-6)** The author wrote the bare name, `name_of_display(75)`.
  A capsule given the window, as a button's capsule is, reaches the display only through it
  (`its_window.right.bar`), because there are no globals. So it writes today's word,
  `its_window.right.bar.satellite.value(75%)`. This plan could let it write `its_window.right.bar(75)`
  as well. The author's 2026-10-09 lines `main_window.file_display(...)` had that shape, and that
  question (letter B, item 103) was never put to the author. Through a window the checker cannot
  know that the name is a progress display, so a wrong value there would be refused only when the
  line runs.

Taken without the author's word, so say if one is wrong:

- Rows that together pass the window's bottom edge are refused with S157, as gaps are: a section past
  the window is never seen.
- A section's own sections need not cover it.
- `display.text` takes any number of strings, one to a line. The other reading is two only: a
  heading over a line, as `new` takes a title and the words under it.
- A section of 0 is refused, as today.
- A progress display runs from 0, an empty bar, to 100. The author wrote 1-100.
- Only a progress display is set by calling its name, not a file display as in the author's
  2026-10-09 sketch (the question above).
- The refusal is S157 MISSING_SECTION, exit 85.
- `section` is one of the language's words, so no section can be named "section". The author's
  `section.satellite.window.display.image(...)` lines are read as "a section".
- The author's satellite_source_editor/source_editor_main.satl would be refused twice: once for the
  old spelling, and once because its one section is 250 of the window's 600 pixels. Here it is,
  whole, under these rules. The second section is the window's other 350 pixels, and what goes in it
  is the author's:

  ```
  satellite.variable.window main_window = satellite.window.new("SATELLITE SOURCE EDITOR", "VERSION 001 REVISION 01", 800x600)

  main_window.satellite.window.section("files_display", 100%, 250)
  main_window.satellite.window.section("the_rest", 100%, 350)

  files_display.satellite.window.display.file("list_of_files")

  main_window.satellite.open()
  ```
