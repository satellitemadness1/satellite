# satellite-004 — SATELLITE_FILE_OPERATIONS

Reading and writing files: every word, what each one does, how it is built, and
the milestones that build it. Written 2026-09-18 from the author's brief (Part 1)
for the dynamic storyline generator in `/home/madness/code/satl/secure_environment`,
the first program that needs files in 004.

**This is MILESTONES.md M15** (*"satellite_file, and `file_object.seek()`"*),
grown into its own file the way SATELLITE_ARGUMENTS.md grew out of its brief.

**Built from the secure_environment session, on the author's word** (2026-09-18):
*"we will even follow the milestones to build the file operations from this
directory into the interpreter that we need."* And on why a word that does not
exist yet is fair to ask for: *"we are bending the interpreter into what we need
for this, slightly bending it, and trying to also keep a general purpose
programming language at the same time, so if there doesn't exist something yet,
we just haven't built it, it's not off the table."*

**THE MODEL IN ONE LINE — the author's:** *"we'll build it so you can iterate
over the lines as if they were objects ... As if each line were a list of
objects."* **A text file is a `satellite.container.list` of lines that lives on
the disk.** Every list word that means something for lines works on a file,
spelled the same and meaning the same, so learning the list teaches you the file.

**Status (2026-09-18, later the same day):** the author ruled **lines count from
1** (*"all line counts all start at 1, so that is how we will build file
indexing"*) and asked for the build: *"you can write that part of the satellite
interpreter now"*, together with **a program that edits its own running source**
(Part 3.8). FO-1 to FO-4 are being built; Part 7 says what is built and checked.
Part 3 is written counting from 1 throughout.

---

# Part 1 — the brief, as the author wrote it

The file part of the 2026-09-18 brief, word for word. The whole brief, including
the story it is for, is Part 1 of
`/home/madness/code/satl/secure_environment/DYNAMIC_STORYLINE_GENERATOR.md`.

> so for example, to access a binary file in satellite, we type this:
>
> satellite.variable.file my_file = satellite.file.new("/path/dir/filename.se", "text") and "binary" for a binary file... this writes a text file, and you use this syntax to write a line,
>
> my_file.write("line_1") will append a line automatically in satellite.
>
> then I enter this:
>
> my_file.write("line_2") so now there are two lines in my file,
>
> my_file.seek("line_2") will take you to a line, and then,
>
> my_file.write("line_2") will write on that line and push all of the lines down one line, so it will replace and then push down the "line_2" so now it says "line_2" twice, so I have to use...
>
> my_file.seek("line_2") (position the write line at the first occurrence of "line_2"
>
> my_file.replace("line_2", "line_3") and that will replace it, but you have to "seek" that position, and satellite does everything for you, so I didn't type that explanation of satellite correctly, append mode .append("string") will append to the bottom of a file, and "write" will write a line at the position that you used .seek to go to. THere is also my_file.search("string") will position the writer in the middle of a file, I think I will create satellite with this syntax, now that I think about it, i'll build this syntax:
>
> my_file.seek("an entire line") will highlight that entire line,
>
> my_file.replace("what you want to replace," "with what you want to replace it with here")
>
> so there will be .replace, .write (to write a line at a certain line, after using .seek to go to that line,) and .seek("anywhere") .seek will take you to a specific position in a file, or you can .seek("entire_line") well, let's use seek(line_number) and or and how and or and how should we create the interpreter for this project? You tell me the functions that you want to have for file writing, and then i'll build it that way, something like this is my proposal, now your turn to give me your proposal/changes to mine/keep mine exactly/or you create the file operations for the interpreter, then we'll build it all into milestones INSIDE of the directory /home/madness/code/cxx/satellite and we'll build into the document SATELLITE_FILE_OPERATIONS.md and in that file will be literally everything we need, and ALL FROM THIS DIRECTORY... so we shall do all of this from here, we will even follow the milestones to build the file operations from this directory into the interpreter that we need

**And the four notes the author added the same morning, while this file was
being written:**

> .append is a satellite word for like, "add to list" you write list.append(object) and it adds that to the list, but for a file, it will append a line to the end of the file, just like it was a list, that is how we'll build our file opeartions, we'll build it so you can iterate over the lines as if they were objects,
>
> that is the best way to build the file operations, you know?
>
> As if each line were a list of objects,
>
> so we will do .replace(line_number, "string") or .replace will optionally take this: .replace("string", "with string") so it will have two syntaxes
>
> this is possible with C++

---

# Part 2 — what the tree already has

Measured 2026-09-18 against `build/satl`, **VERSION 004 REVISION 04 BUILD 0142**.
The probe programs are in `secure_environment/tools/probes_004/`.

**Every file word is numbered and none is built.** From `words/words.tsv`:

| number | word | 003's meaning (HELP.md `## file`, M19) |
|---|---|---|
| `1 8 1` | `satellite.file.new(path)` | makes a file that is **not** there; refuses to clobber |
| `1 8 2` | `satellite.file.open(path, mode)` | mode `"read"` `"write"` `"append"` `"read_append"` |
| `1 8 3` | `satellite.file.clear(path)` | empties a file by name, leaves it there |
| `1 8 4` | `satellite.file.new(path, mode)` | `new` with a mode |
| `1 8 5` | `satellite.file.exists(path)` | a directory answers false |
| `1 6 2 1` | `satellite.variable.file.new` | numbered and deliberately empty |
| `1 6 2 2` | `…file.open` | reopen a closed handle |
| `1 6 2 3` | `…file.read_line` | the next line, or nothing at the end |
| `1 6 2 4` | `…file.write_line(s)` | the text and a newline |
| `1 6 2 5` | `…file.read_all` | the whole file as one string |
| `1 6 2 6` | `…file.close` | answers whether the writes landed |
| `1 6 2 7` | `…file.exists` | whether this handle's path is still there |
| `1 6 2 8` | `…file.ok` | whether the file is open |
| `1 6 2 9` | `…file.path` | the path it was opened on |
| `1 6 2 10` | `…file.error` | why the last thing failed, in the machine's words |
| `1 6 2 11` | `…file.write(x)` | exactly the bytes, no newline |

**Four facts that decide the order of work:**

1. **A method call alone on a line is refused.** `s.append("_and_more")` gives
   S0401, code 13: *"s is followed by something that is not = , and there is no
   statement of that shape"* (probe `p6_method_statement.satl`). Every line of
   the author's syntax (`my_file.append("line_1")`) has this shape. **So it is
   the first thing built: FO-1.**
2. **`satellite.variable.file` cannot be declared.** The declaration branch of
   `check_statement` accepts number, string, binary and percentage only (M8
   quotes the message). Nor can `satellite.variable.bool`, which is what most
   file words answer, so FO-1 opens bool as well.
3. **The list words are numbered and not built either** (M14):
   `satellite.container.list.append` `1 4 2 1` through `.search(pattern)`
   `1 4 2 26`. **The file words take the list's spellings** (Part 3), so whatever
   M14 decides about a list (index syntax, where counting starts) the file has to
   match. When both are built, one reading of `f[n]` should serve both.
4. **The directory words are the pattern to copy.** `satellite-numbers/directory_words.hpp`
   is one header shared by three one-word libraries. They answer a value and
   never stop the program, refuse a path holding a NUL (`path_holds_a_nul` 31),
   and open through one descriptor so a path deeper than PATH_MAX still works.
   Every rule there applies here.

**Also in place:** the author's path rule for includes (2026-09-16, PROGRESS §4):
*"program root first, then filesystem root when it's not found"*. The author's own
first program already relies on it for files:
`// load the first story from /story/story_one/story_one.ds.se`. **Machine codes:
the next free one is 39.** **Object arms:** binary is 7 and percentage is 8, so
the file is 9. M15 warns this will be the first arm that is not `const`, because
a file is not a value.

---

# Part 3 — the proposal

## 3.1 What a file is

- **A text file is a list of lines.** Line `n` is a `satellite.variable.string`.
  The newline that ends a line belongs to the file and is not part of the line's
  string.
- **A handle is a reference.** Two names for one open file are two names for one
  list, as in 003 and as with a spacesuit. Closing the file through one name
  closes it for both.
- **Nothing a file does stops the program, with one exception.** Every word
  answers a value. When something fails, the handle keeps the reason, and `ok`
  and `error` report it. This is 003's rule and the directory words'. **The
  exception is reading a line that cannot be read, which stops the program:**
  `f[n]`, `f.first` or `f.last` past the end (`line_past_the_end`, 47), on a
  closed file (`file_not_open`, 45) or on a binary one (`file_has_no_lines`, 46),
  as a list index past the end does. **So does any question about the lines of
  a file that is not open** -- `size`, `empty`, `index_of`, `search`,
  `contains`, `read_all` -- since the only answers it could give are made up
  (the review, 2026-09-18). The words that change a file still answer false,
  and `ok`, `error`, `path`, `exists`, `open`, `close` and `save` still answer.
- **A line number below zero is refused** (`not_a_position`, 19, S0905): lines
  count from 1, and nothing counts below zero.
- **A save that fails when a body ends is said**, and the run exits 44
  (`file_unwritable`, S0904) instead of 0. The interpreter closes every file a
  body held when the body ends, main's included. Asking where a
  line is (`index_of`, `search`) is a question, and a question with no answer
  must not stop the program (the `find` lesson, S0716). Reading a line that is
  not there is a mistake in the program, and giving back a made-up answer would
  be *"an answer that is wrong and does not say so"*. A misuse the checker can
  see before the run is refused before the run: a line word on a number, or
  `f.replace(1)` with one argument.
- **Changes are on the disk when the program ends, or at `save` or `close`, and
  never half.** A save writes a new copy beside the file and renames it over the
  old one, so a crash or Ctrl-C leaves either the old file or the new one, never
  a mixture. **`append` is the fast path:** nothing before the end moves, so the
  line goes to the disk through a buffer and needs no rewrite (DESIGN §1.1: the
  likely scenario is the fast one). **The interpreter saves and closes every
  file still open when the program ends.** A user who forgets `close` loses
  nothing: *"satellite does everything for you."*

## 3.2 The words, by name — `satellite.file`

**As built, these words have no library** (`satellite/bytecode/file_calls.hpp`
says why): each one answers a *handle*, and a library's scenarios can only take
values and answer machine codes. So they belong to the object model, the way
`satellite.variable.number`'s declaration belongs to the walker. **This is a
departure from DESIGN §3.4, and it is the author's to rule on (R10).**

| word | number | what it does | answers |
|---|---|---|---|
| `satellite.file.new(path)` | `1 8 1` | makes a **text** file that is not there, open and empty. **Never clobbers:** if anything is at the path, the handle is not ok and says so, and the file is untouched | a file |
| `satellite.file.new(path, kind)` | `1 8 4` | the same, with the kind named: `"text"` or `"binary"`. **Re-meant:** 003's second argument was an access mode | a file |
| `satellite.file.open(path)` | `1 8 6` **new** | opens a **text** file that **is** there | a file |
| `satellite.file.open(path, kind)` | `1 8 2` | the same, kind named. **Never creates:** a missing file gives a handle that is not ok, so a misspelt story path cannot quietly become an empty story. **Re-meant**, mode → kind | a file |
| `satellite.file.clear(path)` | `1 8 3` | empties a file by name and leaves it there | bool |
| `satellite.file.exists(path)` | `1 8 5` | whether a file is there; a directory answers false | bool |

The author's first line is exactly the proposal:
`satellite.variable.file my_file = satellite.file.new("/path/dir/filename.se", "text")`.

## 3.3 The words, through the handle — `satellite.variable.file`

Every row marked **list** is spelled as `satellite.container.list` spells it and
means the same thing, done to lines. **The "number" column is superseded, as
built:** in 004 a method is a REGISTRY token and not a word-table row (`find`
and `replace` already were), so the file's methods are the `[METHOD]` rows
`0x0B07` to `0x0B1D` of `REGISTRY.satellite`. `words.tsv` gained exactly one
row, `satellite.file.open(path)` `1 8 6`, code 4464, and no other code moved.

| word | number | what it does | answers | from |
|---|---|---|---|---|
| `f.append(x)` | `1 6 2 12` | adds `x` as a new last line | bool | **the author**; list |
| `f.insert(n, x)` | `1 6 2 13` | puts `x` in as line `n`, and line `n` and everything below it move down one. **This is the author's `seek` + `write`** | bool | list |
| `f[n]` | *(syntax)* | line `n`, as a string, counting from 1. A line that cannot be read stops the program (3.1) | string | list |
| `f.replace(n, x)` | `1 6 2 14` | line `n` becomes `x` | bool | **the author** |
| `f.replace(a, b)` | `1 6 2 15` | text `a` becomes `b`: the first occurrence in the file, looking from line 0 down, and inside one line only (R4). **Which shape runs depends on what kind the first argument is:** a number means a line, a string means text | bool | **the author** |
| `f.index_of(x)` | `1 6 2 16` | the number of the first line that **is exactly** `x`. **The author's `seek("an entire line")`.** When no line matches, the answer is `0`, which is never a line (R3) | number | list |
| `f.search(x)` | `1 6 2 17` | the number of the first line that **contains** `x` anywhere. Or `0`. **The author's `search`.** To find where in the line, use `f[n].find(x)` | number | **the author**; list |
| `f.contains(x)` | `1 6 2 18` | whether any line **is exactly** `x` | bool | list |
| `f.remove_at(n)` | `1 6 2 19` | takes line `n` out; the lines below move up one | bool | list |
| `f.remove(x)` | `1 6 2 20` | takes out the first line that is exactly `x` | bool | list |
| `f.remove_first()` `f.remove_last()` | `1 6 2 21` `22` | as a list's | bool | list |
| `f.truncate(n)` | `1 6 2 23` | keeps the first `n` lines | bool | list |
| `f.clear` | `1 6 2 24` | no lines; the file stays | bool | list |
| `f.size` | `1 6 2 25` | how many lines | number | list |
| `f.empty` | `1 6 2 26` | whether there are no lines | bool | list |
| `f.first` `f.last` | `1 6 2 27` `28` | the first or the last line | string | list |
| `f.save` | `1 6 2 29` | puts every change on the disk now, whole or not at all | bool | new |
| `f.read_all` | `1 6 2 5` | the whole file as one string, newlines included, **with every change not yet saved** | string | 003, kept |
| `f.close` | `1 6 2 6` | saves, then closes. `false` means the changes did not land, and `error` says why | bool | 003, kept |
| `f.open` | `1 6 2 2` | opens a closed handle again | bool | 003, kept |
| `f.ok` `f.error` `f.path` `f.exists` | `1 6 2 8` `10` `9` `7` | as in 003 | | 003, kept |

**Walking a file is walking a list**, with the `for` that runs today:

```satellite
satellite.variable.file story = satellite.file.open("/story/story_one/story_one.ds.se", "text")
satellite.statement.for(satellite.variable.number i = 1; i < story.size + 1; i + 1)
{
    satellite.variable.string line = story[i]
    satellite.console.display(line)
}
story.close()
```

**The author's example, in these words.** The file after each line:

| statement | the file afterwards |
|---|---|
| `satellite.variable.file my_file = satellite.file.new("/demo/demo.se", "text")` | *(empty)* |
| `my_file.append("line_1")` | `line_1` |
| `my_file.append("line_2")` | `line_1` `line_2` |
| `my_file.insert(2, "line_2")` | `line_1` `line_2` `line_2` |
| `my_file.replace(2, "line_3")` | `line_1` `line_3` `line_2` |
| `my_file.replace("line_2", "line_4")` | `line_1` `line_3` `line_4` |

## 3.4 What changes from the author's first version, and why

| the author wrote | proposed | why |
|---|---|---|
| `seek(x)` then `write(x)` | `insert(n, x)` | **no hidden cursor.** A position the program cannot see is state behind the user's back, and it makes the same line do different things depending on what ran before it. A line number is visible and cannot drift. The author's own later note moves the same way: `.replace(line_number, "string")` |
| `seek("an entire line")` | `index_of(x)` | the list's word for exactly this |
| `write("line_1")` appends when nothing was sought | `append(x)` | the author's later ruling: *"append mode .append("string") will append to the bottom of a file"* |
| `search("string")` "positions the writer in the middle" | `search(x)` answers the line; `f[n].find(x)` the place in it | the same information, with no cursor |
| `write_line(s)` `write(x)` `read_line` (003's, numbered) | **left empty** | they are cursor words. A number is never reused (DESIGN §3.2), so each one stays numbered with no library behind it, as `1 6 2 1` already is. **If the author wants `seek` and `write` as well, they go on top of this model as two more words (R1). They add to it and replace nothing** |

## 3.5 Text and binary

- **`"text"`: UTF-8 on the disk, satellite strings in memory** (16 bits a
  character, PROGRESS §2). The decode is strict. A file that is not UTF-8 opens
  as a handle that is not ok, and `error` names the line and the byte. Lines end
  at `\n`, and a `\r` just before it is part of the ending. **The handle writes
  endings the way the file had them**, so a `\r\n` file stays `\r\n`: nothing is
  changed behind the user's back. A file whose lines end in `\r` alone is refused
  by name, because 004 already misreads those (M26 #13). A byte-order mark is
  kept and is not part of line 0. When a last line has no `\n`, `append` ends it
  first. A string holding `\n` becomes that many lines when it is appended or
  inserted (R5).
- **`"binary"`: bytes, not lines.** A binary file has no lines, so the words for
  lines answer false or `""` and `error` says why. What a binary handle takes
  and gives is FO-8's to design. The obvious answer is
  `satellite.variable.binary`, which already keeps its width, taken in whole
  bytes. core 001 of the storyline generator writes text only.

## 3.6 Paths

**The author's include rule, used for files** (R2). A path without a leading
`/` is relative to the folder of the `.satl` that contains the call, the same way
an include is. A path with a leading `/` is looked up **under the program's own
folder first, then at the filesystem root**. That is how
`/story/story_one/story_one.ds.se` in the author's file reaches the program's own
`story/` folder. For `new`, which makes something, the folder that already holds
the parent directory wins, and when neither holds it the handle is not ok. A
path holding a NUL is refused (`path_holds_a_nul`, 31).

## 3.7 How it is built

- **`satellite/satellite_variable_file/satellite_file.hpp`:** the resolved path,
  the kind, open or not, the last machine code and the reason, the descriptor,
  the append buffer, **a line index** (where each line starts, built the first
  time a line is asked for by number), and **the lines themselves** (loaded at
  the first change that is not an append).
- **Arm 9 of `satelliteObject`:** `std::shared_ptr<satellite_file>`, the reference
  M15 predicted.
- **`satellite-numbers/file_words.hpp`**, plus one folder a word, which is
  `directory_words.hpp`'s shape. New rows go in through `words/words_004.tsv`, as
  `satellite.variable.percentage` did.
- **The fast path is `append` with nothing waiting to be saved:** encode once into
  the handle's buffer, and write it out when the buffer fills, before anything
  reads the file, and at save or close. The bar is **×1.05 of C++ writing the same
  lines** (DESIGN §1.1), raced as `display` was.
- **The edit path** loads the lines, edits them in memory, and `save` writes
  `<file>.saving.<pid>` beside the file, `fsync`s it and renames it over the
  original. **This path holds the whole file in memory, which DESIGN §1.2 ("no
  built-in limits") does not allow in the end.** FO-9 replaces it with a
  streamed rewrite: the index plus a list of changes, and never the whole file
  at once. It is staged this way so the storyline generator can start sooner,
  and the limit is written down here, not hidden.
- **Machine codes, added to `machine_codes.hpp` before any is used:**
  39 `file_not_found`, 40 `file_already_there`, 41 `not_a_file`,
  42 `file_unreadable`, 43 `file_not_text`, 44 `file_unwritable`,
  45 `file_not_open`, 46 `file_has_no_lines` (a line word on a binary file),
  47 `line_past_the_end`. The handle holds them, and `error` says them in words.

## 3.8 A program that edits its own source while it runs

**The author, 2026-09-18:** *"we have to build it so that it can edit .satl files
that are currently running, because we have to change the "ds_build" number
automatically every single time the file is ran ... it has to grab a copy of the
file, leave the file alone, and run the copy while having the ability to edit
it's own source."*

**Most of this was already true, and the rest is one change.** 004 reads every
`.satl` of a program **once, before anything runs** (`load_program`), turns it
into bytecode in memory, and runs only that. So a program can open its own
`.satl` with the file words and rewrite it, and nothing it is running changes.
**The one place that re-read a file was the error report**, which reopened the
`.satl` to quote the failing line. After a self-edit, that would quote the new
line under the old line's caret. **Reports now quote the copy that was loaded**
(`loaded_sources()` in `satellite/machine/source_position.hpp`). The storyline
generator's version, as `tests/file_self_edit.satl` runs it:

```satellite
satellite.variable.number ds_build = 1
satellite.variable.number next = ds_build + 1
satellite.variable.file me = satellite.file.open("file_self_edit.satl")
satellite.variable.number at = me.search("satellite.variable.number ds_build = ")
me.replace(at, "    satellite.variable.number ds_build = " + next.string)
me.close()
satellite.console.display("RUN " + ds_build.string)
```

The first run says `RUN 1` and leaves `= 2` in the file; the next says `RUN 2`.
The save is whole or not at all (3.1), so an editor with the file open sees one
change, never half of one.

---

# Part 4 — the rulings, the author's

| # | question | recommended |
|---|---|---|
| ~~**R0**~~ | ~~Where counting starts~~ **ANSWERED 2026-09-18: from 1.** The author: *"all line counts all start at 1, so that is how we will build file indexing..."*. Claude had recommended 0; Part 3 is rewritten from 1 | — |
| **R1** | Keep `seek` and `write` as well, on top of the list words? | no. The list words say the same thing with nothing hidden |
| **R2** | The include path rule, used for files (3.6)? | yes, and the author's own `/story/...` already assumes it |
| **R3** | `index_of` / `search` with no match: a number, or refused as string `find` is (S0716)? | **Built as `0`**, which R0 made possible: 0 is never a line, so it cannot be mistaken for one, and `> 0` tests it. A file search that stops the program would be 003's `find` lesson again. `f.contains(x)` answers the yes/no question |
| **R4** | `replace(a, b)`: the first occurrence, or every one? | the first, matching the author's first brief (*"position the write line at the first occurrence"*). If every one is wanted, `replace_all(a, b)` would be a new word, `1 6 2 30` |
| **R5** | A newline inside `append` or `insert`: several lines, or refused? | several lines, because *"satellite does everything for you"* |
| **R6** | What a binary file takes and gives | `satellite.variable.binary` in whole bytes; decided at FO-8 |
| **R7** | **A read-only open, for the storyline generator's lock:** `satellite.file.open(path, kind, "read")`, word `1 8 7`, where every changing word answers false and the file is never rewritten | **yes.** The generator can then never change the author's story files, even by a mistake in the program |
| **R8** | A sandbox, so a program can touch only files under its own folder? | not now. DESIGN §9's hostile paths are refused by name, and a sandbox is a separate decision |
| **R9** | M15's `seek("str")`, `seek("entire_file")`, `seek("collection_of_str")` | the first is `index_of`/`search`; the second is `read_all`; the third is `index_of` given a list, which waits on M14 |
| **R10** | The file words as the object model's, with no library (3.2) | keep them there until a handle can cross a library boundary; that question goes to the author with M8's spacesuits, which have the same shape |

---

# Part 5 — the milestones

Each milestone gets a **Build**, a **Done when**, and **Entries**: the inputs that
could break it. Every entry list also takes in DESIGN §9's hostile inputs: names
and bytes that are not text, machine code given as text, a directory given as
the file, a symlink, an unreadable file, and a very long path. The tests are
`tests/file_*.satl` with their `.sate`, in `./check.sh`, and a milestone is done
when `make test` is green. **A fresh reader tries to break each milestone before
it is marked done**, as with `for` (PROGRESS §1).

## FO-1 — a method call is a statement, and a bool can be declared — **BUILT 2026-09-18** (Part 7)

**Needs no ruling, and is useful beyond files.**
**Build:** the statement shape `<declared name>.<method>(<arguments>)` standing
alone on a line: the checker accepts it, the walker runs it and throws the answer
away, and `--repl` runs the same shape. Also `satellite.variable.bool` as a fifth
declarable type, with `satellite.bool.true` / `.false`.
**Done when** probe `p6_method_statement.satl` gets past line 6, and a method
whose answer is used (`if (s.contains("x"))`) still runs exactly as before.
**Entries:** a method on an undeclared name; a method the name's kind does not
have; two methods chained as a statement; the shape inside `for`, `while` and
`if` bodies; a bool declared from a number (refused); a bool shown by `display`.

## FO-2 — the file type, and the words by name — **BUILT 2026-09-18** (Part 7)

**Needs R2 and R7.**
**Build:** `satellite.variable.file` declarable; arm 9; `satellite_file.hpp`;
`file_words.hpp`. The words: `satellite.file.exists`, `new(path)`,
`new(path, kind)`, `open(path)`, `open(path, kind)`, `clear`, plus
`open(path, kind, "read")` if R7 is yes. On the handle: `ok`, `error`, `path`,
`exists`, `open`, `close`. Machine codes 39–45. `"binary"` gives a handle that
is not ok and says binary is FO-8. The interpreter closes every open file when
the program ends.
**Done when** a program makes a file, closes it, opens it again, asks all four
questions, and every refusal below gives a handle that is not ok, with the
reason, and the program still going.
**Entries:** `new` over an existing file (**compare the bytes before and after:
untouched**); `open` of a missing file (**nothing is created**); a directory; a
symlink to a file and to a directory; a NUL in the path; a path 5,000 bytes
deep; a file with no read permission; a folder with no write permission; two
names for one handle, closed through one of them; `close` twice; a leading `/`
that exists only under the program root, only at the filesystem root, and in
both.

## FO-3 — the list of lines: append, read, walk — **BUILT 2026-09-18** (Part 7)

**The storyline generator's first need.** It writes its record and reads its
story files with nothing past this milestone.
**Needs R0, R3 and R5.**
**Build:** `append`, `f[n]`, `size`, `empty`, `first`, `last`, `read_all`,
`index_of`, `search`, `contains`, the line index, the append fast path, and
code 47.
**Done when** Part 3.3's walking example prints every line of a file; the race
shows `append` of 1,000,000 lines within ×1.05 of C++ `write`; and a 1 GB file
walked by `f[n]` keeps memory flat, because the index holds positions, not text.
**Entries:** a file that is not UTF-8 (the line and the byte are named); endings
of `\r\n`, `\r` alone, and a mix of the two; a byte-order mark; a NUL inside a
line; a last line with no `\n`, then `append`; an empty file; a single line of
10 MB; `f[-1]`, `f[size]`, `f[1.5]`; `index_of("")`; `search` for a wide
character, which must never match half of one (the rule `string_minus` already
follows); `append` to a file that another program changed after it was opened.

## FO-4 — changing lines: insert, replace, remove, save — **BUILT 2026-09-18** (Part 7)

**Needs R4.**
**Build:** `insert`, both `replace` shapes chosen by the first argument's kind,
`remove_at`, `remove`, `remove_first`, `remove_last`, `truncate`, `clear`,
`save`, and the atomic save (write beside, `fsync`, rename).
**Done when** Part 3.3's example table leaves exactly `line_1` `line_3` `line_4`
on the disk (compared against a fixture); a `kill -9` during `save` leaves the
old file or the new one and nothing else (run 100 times); and `read_all` before
`save` shows the changes.
**Entries:** `insert(size, x)` (the same as append); `insert(0, x)`;
`insert(size + 1, x)` (past the end: false); `remove_at` of the last line;
`replace(a, b)` where `a` is not in the file; `replace(a, b)` where `b` contains
`a` (runs once, does not loop); `replace(n, x)` with `x` holding `\n` (R5);
`replace(1)` (refused by the checker); two handles on one path, both saving (the
second save wins, and HELP says so); `save` into a folder that became read-only
(false, a reason, and the old file intact); a 100 MB file changed at line 0 (the
time recorded; the limit is FO-9's).

## FO-5 — read-only (R7)

**Build:** `open(path, kind, "read")`, word `1 8 7`: every changing word answers
false with the reason *"opened to read"*, and `save` and `close` never write.
Merged into FO-2 if the author prefers.
**Done when** every changing word has been tried on a read-only handle and the
file's bytes and modification time are unchanged.

## FO-6 — the race and the hostile run

**Build:** `make race` rows for `append`, `f[n]` and `read_all` against the same
work in C++, plus DESIGN §9's inputs thrown at every file word.
**Done when** each race is within ×1.05, or ERROR.md holds the number and the
reason; nothing hostile crashes or is taken as anything but data; and a fresh
reader's findings are fixed.

## FO-7 — the help paragraphs

Every word gets its `satellite.help` text, written in 003 HELP.md's style
(*"a file that would not open is a value, not an error"*). It waits on M34
(`satellite.help`), and Part 3's tables are the source for it.

## FO-8 — binary files

**Needs R6.** **Build:** the binary kind: a byte position, and taking and giving
whole bytes. **Entries:** a binary value that is not a whole number of bytes; a
line word on a binary handle (`file_has_no_lines` 46); a text file opened as
binary and a binary file opened as text.

## FO-9 — no limit on a file being changed

DESIGN §1.2. **Build:** replace FO-4's whole file in memory with the line index
plus a list of changes, applied as a streamed rewrite on save.
**Done when** a 10 GB file with one line inserted at line 0 saves using memory
that does not grow with the file, and FO-4's tests all still pass unchanged.

## FO-10 — `index_of` and `search` given a list

M15's *"collection_of_str"*: the first line that matches any string in a list.
**Waits on M14.**

---

# Part 6 — what the storyline generator needs, in order

| generator step (`DYNAMIC_STORYLINE_GENERATOR.md`) | file milestone |
|---|---|
| reading a story file (`/story/story_one/story_one.ds.se`) | FO-1, FO-2, FO-3 |
| writing a run's record | FO-3 (`append`) |
| the lock never changing a story file | FO-5 |
| core 002 reading core 001's record, and correcting it | FO-4 |
| a record past a few GB, edited | FO-9 |

---

# Part 7 — what is built and checked (2026-09-18, BUILD 0145)

**FO-1 to FO-4, and 3.8.** `make test`: **242 passed, 0 failed**, with the
install checks' 14.

**COMMITTED 2026-09-18 AS `1547deb`.** This line said "Nothing is committed; the
author commits" while the change sat in a working tree that TWO sessions were
sharing — 22 files and 706 insertions that existed nowhere but the working tree,
with another session committing over the top of it all night. One `git add -A`
would have swept it into an unrelated commit. Nothing was edited, only saved;
`git reset --soft HEAD~1` returns it untouched.

**Re-verified through the real paths before it was committed**, rather than taken
on the note's word: `make` 0 errors and 0 warnings, `./check.sh` **242 passed, 0
failed**, `build/file_cases` **222 ok, 0 failed**.

**A fresh reader tried to break it** and found five wrong answers and a set of
shapes the check let through. All are fixed and the worst are pinned in
check.sh (`tests/file_word_chain.satl`, `file_not_open.satl`,
`file_wrong_count.satl`, `file_unsaved.satl`):
- a method after a word's call (`satellite.file.open("t.se").append("x")`) was
  dropped without a word; the walker now reads a word statement to its end;
- a failed save at the end of a run was silent; it is said, exit 44;
- a relative path after `satellite.directory.change` went to the wrong folder;
  folders are anchored to where satl started;
- `f.truncate(-1)` answered true; a negative line number is refused;
- a file that is not open answered `size` 0 and `search` 0; it now stops;
- the CHECK now refuses, before anything runs: a wrong argument count to a file
  word or method, a library word given two arguments (the lexer's new counting
  had turned `satellite.variable.string.replace("a", "b")` into a run-time
  refusal), a file method on a string or number, a string method on a file,
  and a method-call statement that does not end at its line's end
  (`f.size = 3`, `f.`, `f[`);
- refusals point at their own line (a word statement's and an if's or while's
  fell back to the NEXT line); `f.error` and `f.path` never stop a program;
  `f == g` compares two files by identity.

**Not fixed, and older than this:** a nesting about 20,000 deep crashes the
walker's C stack (exit 139), with `((...))` as with `f[f[...]]` -- the depth
MILESTONES already records as accepted.

| piece | files | checked by |
|---|---|---|
| **A method call as a statement**, and `satellite.variable.bool` / `.file` declared | `program_check.cpp` (the name branch; the declaration branch), `program_walk.cpp` (`run_statements`, `run_assignment`) | `tests/method_statement.satl`; every `tests/file_*.satl` |
| **Word and method calls take several arguments**, and a word call is lexed to the row with that many parameters | `bytecode_registry.cpp` (`arguments_in`, `shaped_word_code`), `expression.cpp` (`call_word`, `call_method`) | `satellite.file.new(path, "text")` in `tests/file_list.satl`; every existing check unchanged |
| **The 23 method names**, `[METHOD]` rows `0x0B07`-`0x0B1D`, and `satellite.file.open(path)` `1 8 6` (code 4464, the only row `words.tsv` gained) | `REGISTRY.satellite` → `token_codes.hpp`; `words/words_004.tsv` → `words.tsv` → `word_codes.hpp` | `check_make_words` (14 cases) |
| **The handle**, arm 9 of `satelliteObject` (`FileHandle`, a `std::shared_ptr`) | `satellite_object.hpp/.cpp` | identity equality; `kind_name` "a file" |
| **`satellite_file`**, the list of lines on the disk: 1-based, strict UTF-8, each line's own ending, the byte-order mark, the append fast path (64 KiB streaming), the whole-or-nothing save (write beside, fsync, keep mode and owner, rename, fsync the folder), `realpath` so a symlink stays one | `satellite/satellite_variable_file/satellite_file.hpp/.cpp` | `build/file_cases`: **222 cases**, clean under ASan and UBSan, and nine deliberately broken copies each turned their cases red |
| **The words and methods in a program**, `f[n]`, the path rule (3.6) | `satellite/bytecode/file_calls.hpp/.cpp`, `expression.cpp` | `tests/file_list.satl` (the author's example, walked with `for` and `f[i]`), `tests/file_past_the_end.satl` (exit 47) |
| **A program edits its own source** (3.8): reports quote the loaded copy | `source_position.hpp` (`loaded_sources`), `program_walk.cpp` (`load_program`) | `tests/file_self_edit.satl` (RUN 1, then RUN 2, then the file says 3), `tests/file_snapshot.satl` (rewrites line 10, fails on it, the report quotes the loaded line) |

**Choices made where Part 3 was silent** (the file class, 2026-09-18):
- a `\r` not followed by `\n` in text given to `append`, `insert` or `replace` is
  refused (`file_not_text`), so a handle never writes a file it would refuse to
  open;
- a handle keeps the ABSOLUTE path it opened, so `satellite.directory.change`
  after an open cannot send a save to a different file; `path` still answers the
  path as written;
- a full rewrite first checks the file itself may be written, so renaming over
  a read-only file cannot get around its permissions;
- a failed append write cuts the file back to its old size, so a retry cannot
  write the same bytes twice;
- `new` over a FIFO, device or socket is `not_a_file`; over a dangling symlink,
  `file_already_there`.

**Open, and the author's:**
- **R10**: the file words have no library (3.2).
- **`append` answers true when the line is held but the automatic 64 KiB write
  failed.** The failure is in `error`. The API has no third answer for "held,
  not on the disk yet"; `save` and `close` do report it.
- **`open` (the method) after a `new` that was refused** because a file was
  there opens that existing file. Nothing is clobbered, but a "new" handle ends
  up on an old file.
- **A leftover `.<name>.saving.<pid>`** from a crashed run with the same process
  id blocks full rewrites of that file until it is removed. The error names it.
- **Two runs of a self-editing program at once** can both read 7 and both
  write 8; there is no file lock yet.
- **Not built:** FO-5 (read-only open), FO-6 (the race), FO-7 (help), FO-8
  (binary), FO-9 (no limit on an edited file), FO-10.

---

# Part 8 — what is accomplished, and what is next

Written 2026-09-18 from the author's own handoff note, so the two do not have to
be read side by side.

## Accomplished (`1547deb`)

| | |
|---|---|
| **the type** | `satellite.variable.file` — a text file as a list of lines, **counting from 1** |
| **the words** | `satellite.file.new(path)` / `new(path, "text")`, `open(path)` / `open(path, "text")`, `exists(path)`, `clear(path)`. **`open(path)` is the one row `words.tsv` gained**: word `1 8 6`, code 4464 |
| **the methods** | 23: `append insert replace index_of search contains remove_at remove remove_first remove_last truncate clear size empty first last save read_all close open ok error path exists`. `replace` takes `(n, text)` or `(old, new)` |
| **reading** | `f[n]` reads line n |
| **writing** | saves are **whole or not at all**; appends stream to disk; files close when a body ends; a failed save exits **44** |
| **the language** | a method call stands alone on a line; `satellite.variable.bool` and `.file` declare; word and method calls take several arguments and the lexer picks the row by argument count; a method can follow a word's call |
| **running the copy** | reports quote the LOADED copy, so a program can rewrite its own `.satl` while it runs |
| **the checker** | wrong argument counts, a file method on the wrong type, and `f.size = 3` lines are refused **before anything runs** |
| **the numbers** | machine codes **39–47**; report codes **S0901–S0905** |

## Next, in the order the milestones give

1. **FO-5 — a read-only open.** The smallest, and the one a program that only
   reads wants.
2. **FO-6 — the race.** Two runs of a self-editing program can both read 7 and
   both write 8; **there is no file lock yet**, and this is the open item most
   likely to bite a real program rather than a test.
3. **FO-7 — help.** The file words are not in `help_lines/` yet, and HELP.md is
   generated from it.
4. **FO-8 — binary.** The brief asks for `"binary"` beside `"text"` and only
   text is built.
5. **FO-9, FO-10** — no limit on an edited file, and the last of the ten.
6. **R10, still the author's** — the file words have no library (3.2).

## The four small ones already named, kept here so they are not lost

- `append` answers true when the line is held and the automatic 64 KiB write
  failed; the failure is in `error`, and the API has no third answer for "held,
  not on the disk yet".
- `open` (the method) after a `new` that was refused opens that existing file —
  nothing is clobbered, but a "new" handle ends up on an old one.
- A leftover `.<name>.saving.<pid>` from a crashed run with the same process id
  blocks full rewrites until removed. The error names it.
- A nesting about 20,000 deep still crashes the walker's C stack (exit 139) —
  **older than this work**, and the depth MILESTONES already records as accepted.

