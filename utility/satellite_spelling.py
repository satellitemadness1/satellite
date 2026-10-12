#!/usr/bin/env python3
"""satellite_spelling.py -- writes programs in the `.satellite.` spelling (NEW_MILESTONES.md NM-8).

The author, 2026-10-10: "we need to place everything that isn't under the name "satellite" ... under the
name "satellite"" -- `.find("str")` becomes `.satellite.find()` -- each type as soon as its methods have
numbers, the old spelling refused (his D and E of 21:15), every call of a chain (his B of 22:20), his own
spacesuit's capsules as he writes them (his C). So a string's 24 methods and a list's 24 -- the ones that
run their numbers -- are written `s.satellite.find("x")` and `l.satellite.append(3)`, and everything else
keeps its `.` until its type is numbered.

    python3 utility/satellite_spelling.py FILE...              says what it would change, and changes nothing
    python3 utility/satellite_spelling.py --write FILE...      changes the files
    python3 utility/satellite_spelling.py --only=option,header,place FILE...
                                                               makes only those kinds of change (period, option,
                                                               header, place) -- his P of 2026-10-12: B's options and
                                                               K's headers and places, without the periods
    python3 utility/satellite_spelling.py --text FILE...       reads FILE as text that QUOTES programs (a .md, a
                                                               help page) instead of as one program; a .sh is read
                                                               for what it hands satl: its 'single quotes' and its
                                                               here-documents, never one fed to python
    python3 utility/satellite_spelling.py --apply LOG [FROM TO] [--write]
                                                               the places a RECORDING satl wrote down as it ran
                                                               (SATL_RESPELL=LOG). That switch was a night's tool for
                                                               the conversion of 2026-10-10 and is NOT in satl: build
                                                               it back in from method_periods.hpp's history to use this.

WHAT A RECEIVER IS, IT WORKS OUT AS THE CHECKER DOES, AND FURTHER: a name by the line that declared it
(satellite.variable.string s, a capsule's parameters, a for's name, a spacesuit's fields), a literal by its
kind, a chain by what each method answers, an item by the list's <type>, a capsule's call by what its
satellite.return lines hand back. A receiver it cannot work out is left alone and LISTED, with its line --
the walker refuses it when it runs, naming the new spelling, so nothing is ever changed wrongly in silence.

THE METHOD LISTS ARE READ FROM THE C++ (string_calls.cpp's string_number_of, container_calls.cpp's
list_number_of and every other type's switch named in NUMBER_SWITCHES, token_codes.hpp's method_code_of), so
when a type is numbered next, this converts it with a row in NUMBER_SWITCHES below -- and what its methods
answer in answer_of, so a chain on it is followed.

THE BARE NAME `arguments`, WHERE IT IS THE RUN'S OWN, IS WRITTEN satellite.arguments (2026-10-11: "arguments
becomes satellite.arguments but the arguments like .username .machine don't get the word "satellite""): a name
of the program's own called arguments -- satellite.main's parameter -- stays as written, and its rows are never
respelled. A recording satl writes such a place down as "prefix".

DISPLAY'S AND INPUT'S NAMED OPTIONS GO UNDER SATELLITE (the author's B, 2026-10-11 ~18:20, "yes"): inside
satellite.console.display(...) and satellite.console.input(...), end= foreground= background= bold= italic= are
written satellite.end= satellite.foreground= and so on, and the bare ones are refused. Only an argument of the
call itself is one -- never one of a capsule's call inside it, and never text in quotes.

THE ARGUMENTS' TWO OLDER SPELLINGS ARE WRITTEN AS THEY ARE NOW (his K, the same hour, "yes"): a read of
satellite.library.main.arguments.<row> is satellite.arguments.<row>, and the header
satellite.main(satellite.container.list<satellite.variable.string> arguments) is
satellite.main(satellite.variable.arguments arguments) -- the name main gives it stays.

A WORD THAT IS A VALUE ENDS WHERE THE LEXER ENDS IT (the sweep's E13, 2026-10-11): the longest run that is a
word, so the names after satellite.bool.true, satellite.console.width, satellite.library.<value> and
satellite.arguments are what its value answers -- satellite.library.span.satellite.abs() -- and the arguments'
rows stay as written: satellite.arguments.username.satellite.upper().

HIS SPACESUITS ARE HIS WHEREVER THEY ARE WRITTEN (the sweep's E15): satellite.class is satellite.spacesuit, a
spacesuit of a file the program includes is read from that file, and a protected capsule is his only where his
own capsules can reach it. A parent this cannot read makes the call "not worked out" -- never respelled.

TEXT THAT QUOTES PROGRAMS IS PROSE AS WELL (the sweep's E16): a `.` with a blank after it is a full stop, and
one with nothing before it that could be a receiver -- a blank, a backtick -- names a word. A spelling the text
talks about -- "is refused", "is written", or set beside its .satellite. spelling -- is listed, never respelled;
so is the arguments' older place, which text quotes as history or to test its refusal.

Strings in quotes and // comments are never changed: a program's text is its own.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TREE = os.path.normpath(os.path.join(HERE, ".."))


# ---------------------------------------------------------------- what the C++ says

def read(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def method_words():
    """{spelling: token name} -- every spelling of every method, from method_code_of."""
    text = read(os.path.join(TREE, "satellite/bytecode/token_codes.hpp"))
    body = text[text.index("method_code_of(std::string_view spelling)"):]
    body = body[:body.index("return 0;")]
    return dict(re.findall(r'spelling == "(\w+)"\) return (\w+);', body))


def numbered(path, function):
    """The token names a switch in `function` gives a number: `case token::find_token: return ...`."""
    text = read(os.path.join(TREE, path))
    body = text[text.index("token::Code " + function + "(token::Code method)"):]
    body = body[:body.index("default:")]
    return set(re.findall(r"case token::(\w+):", body))


def word_paths():
    """EVERY WORD OF THE LANGUAGE, as the lexer matches a dotted run against them (bytecode_registry.cpp, "LONGEST
    FIRST, and only a whole path counts"): words.tsv's spellings without their (...) shape, and aliases.tsv's
    second spellings -- satellite.class among them."""
    paths = set()
    for row in read(os.path.join(TREE, "words/words.tsv")).splitlines():
        parts = row.split("\t")
        if len(parts) > 1 and parts[1].startswith("satellite"):
            paths.add(parts[1].split("(")[0])
    for row in read(os.path.join(TREE, "words/aliases.tsv")).splitlines():
        if row.startswith("satellite") and "\t" in row:
            paths.add(row.split("\t")[0].split("(")[0])
    return paths


# EVERY NUMBERED TYPE'S SWITCH (NM-8): the kind this reader calls it, the file, and the function that says
# which method runs which number. A type numbered next is one more row here -- its methods are then written
# `.satellite.` on every receiver this reader can type as that kind.
NUMBER_SWITCHES = [
    ("string", "satellite/bytecode/string_calls.cpp", "string_number_of"),
    ("list", "satellite/bytecode/container_calls.cpp", "list_number_of"),
    ("file", "satellite/bytecode/file_calls.cpp", "file_number_of"),
    ("thread", "satellite/bytecode/thread_calls.cpp", "thread_number_of"),
    # A BASH LINE IS A PROGRAM (KIND_OF_TYPE below): one switch says which words both have.
    ("program", "satellite/bytecode/program_calls.cpp", "program_number_of"),
    # AN OBJECT'S FIVE WORDS (2026-10-11): pointer, reference, ok, lock and unlock on an object of a
    # spacesuit -- never a capsule of his of the same name, which keeps its `.` (Reader.declares).
    ("object", "satellite/bytecode/pointer_calls.cpp", "object_number_of"),
    # .center() AFTER satellite.console.display(...) (2026-10-11): a word's call, typed "display" below.
    ("display", "satellite/bytecode/console_calls.cpp", "display_number_of"),
    ("window", "satellite/bytecode/window_methods.cpp", "window_number_of"),
    # THE OTHER SCALARS (2026-10-11): one switch a type, the eight of them in one file.
    ("float", "satellite/bytecode/scalar_calls.cpp", "float_number_of"),
    ("hex", "satellite/bytecode/scalar_calls.cpp", "hexadecimal_number_of"),
    ("binary", "satellite/bytecode/scalar_calls.cpp", "binary_number_of"),
    ("color", "satellite/bytecode/scalar_calls.cpp", "color_number_of"),
    ("fraction", "satellite/bytecode/scalar_calls.cpp", "fraction_number_of"),
    ("percentage", "satellite/bytecode/scalar_calls.cpp", "percentage_number_of"),
    ("bool", "satellite/bytecode/scalar_calls.cpp", "bool_number_of"),
    ("time", "satellite/bytecode/scalar_calls.cpp", "time_number_of"),
    ("number", "satellite/bytecode/number_calls.cpp", "number_number_of"),
    ("infinity", "satellite/bytecode/number_calls.cpp", "infinity_number_of"),
    ("map", "satellite/bytecode/container_calls.cpp", "map_number_of"),
]

METHODS = method_words()
UNDER = {kind: numbered(path, function) for kind, path, function in NUMBER_SWITCHES}
# A WINDOW'S WORDS OF PARTS (the author's final syntax, 2026-10-10 ~21:15): widgets.display.file and
# scroll.vertical are each one word, `.satellite.` once before it -- so the first part takes it, as a numbered
# method does, and answer_of says the parts after it are the word's (window_calls.hpp).
UNDER["window"] |= {"widgets_token", "scroll_token"}
WINDOW_PART_HEADS = {"widgets_token", "scroll_token"}
# AN INFINITY'S PERIOD IS ITS OWN WORDS AND THE NUMBER'S (method_periods.hpp's number_a_method_runs): its name
# may hold a plain number (SATELLITE_INFINITY.md Q24), and the checker judges the name where the walker
# judges the value.
UNDER["infinity"] |= UNDER["number"]
STRING_NUMBERED = UNDER["string"]
LIST_NUMBERED = UNDER["list"]
FILE_NUMBERED = UNDER["file"]

# WHAT A NUMBER'S WORDS ANSWER WHEN IT IS A NUMBER (answer_of, below).
NUMBER_ANSWERS_A_NUMBER = {"to_number_token", "reverse_token", "abs_token", "floor_token", "ceil_token", "round_token",
                           "truncate_token", "max_token", "min_token", "clamp_token", "digits_token", "bytes_token",
                           "shift_left_method_token", "shift_right_method_token"}
# THE ARGUMENTS VARIABLE'S METHODS ARE A MAP'S (2026-10-11), and its rows are data, never methods: args.size is
# args.satellite.size, args.username stays. Its .get and .set are refused whichever period they are written with
# (main_arguments.hpp), so they are not respelled.
UNDER["args"] = UNDER["map"] - {"get_token", "set_token"}

WORD_PATHS = word_paths()
# THE ARGUMENTS' ROWS satl NAMES ITSELF (words.tsv's satellite.library.arguments.<row>): after a row, a name that
# goes on with one of these is the row's -- satellite.arguments.session.directory -- and never a method.
ROW_PATHS = {path[len("satellite.library.arguments."):] for path in WORD_PATHS
             if path.startswith("satellite.library.arguments.")}

# DISPLAY'S AND INPUT'S NAMED OPTIONS (console_calls.cpp's kDisplayOptions and kInputOptions), written under
# satellite since the author's B of 2026-10-11 ~18:20 -- satellite.end="!", satellite.foreground=xFF8800 -- and
# only the ones each word takes: an option the word has not got is refused whichever way it is spelled.
OPTIONS = {"satellite.console.display": {"end", "foreground", "background", "bold", "italic"},
           "satellite.console.input": {"foreground", "background", "bold", "italic"}}

# THE ARGUMENTS' OLDER PLACE (main_arguments.hpp, "where they lived until 2026-10-03") and main's older header
# type: his K of 2026-10-11, "yes" -- each is written as it is now, then refused.
OLD_ARGUMENTS = "satellite.library.main.arguments"
OLD_MAIN_TYPE = ("list", ("string",))

# A WORD THAT IS A VALUE WITH NO BRACKETS, and the kind it is (expression.cpp: the bool words and the console's
# facts): what follows it in the same run is that value's method (the author's G, 2026-10-11, BUILD 0038:
# satellite.bool.true.satellite.string). A bare satellite.infinity is no value ("there is no value here to work
# with", BUILD 0039); its call is, in WORD_ANSWERS.
VALUE_WORDS = {"satellite.bool.true": ("bool",), "satellite.bool.false": ("bool",),
               "satellite.console.width": ("number",), "satellite.console.height": ("number",)}

# A METHOD NO OTHER TYPE HAS: on a receiver this cannot work out, it says the receiver.
ONLY_A_STRINGS = {"upper_token", "lower_token", "starts_with_token", "ends_with_token", "substring_token",
                  "split_token", "trim_token", "at_token", "resolved_token", "foreground_token"}
ONLY_A_LISTS = {"by_name_token", "by_value_token", "sum_token", "max_token", "min_token", "reserve_token"}
ONLY_A_MAPS = {"keys_token", "values_token"}     # a list is refused them: it already IS its items

# EVERY WORD THAT NAMES A TYPE, and the kind it is (type_shape.hpp's kind_of_type_word). A kind not
# under satellite is still worth knowing: it is what keeps `n.string` as it is.
KIND_OF_TYPE = {
    "satellite.variable.string": "string", "satellite.variable.number": "number",
    "satellite.variable.binary": "binary", "satellite.variable.bin": "binary",
    "satellite.variable.percentage": "percentage", "satellite.variable.percent": "percentage",
    "satellite.variable.file": "file", "satellite.variable.bool": "bool",
    "satellite.variable.infinity": "infinity", "satellite.container.list": "list",
    "satellite.container.index": "map", "satellite.container.map": "map",
    "satellite.variable.window": "window", "satellite.variable.float": "float",
    "satellite.variable.double": "float", "satellite.variable.hex": "hex",
    "satellite.variable.hexadecimal": "hex", "satellite.variable.color": "color",
    "satellite.variable.colour": "color", "satellite.variable.fraction": "fraction",
    "satellite.variable.time": "time", "satellite.variable.thread": "thread",
    "satellite.variable.program": "program", "satellite.variable.bash": "program",
    "satellite.variable.arguments": "args", "satellite.variable.info": "list",
    "satellite.container.multiple": "multiple",
}

# WHAT A WORD'S CALL ANSWERS, for the words a chain is written on. Anything else is not worked out.
WORD_ANSWERS = {
    "satellite.file.open": ("file",), "satellite.file.new": ("file",),
    "satellite.container.list": ("list", ("unknown",)), "satellite.container.map": ("map", ("unknown",), ("unknown",)),
    "satellite.console.input": ("string",), "satellite.directory.list": ("list", ("string",)),
    "satellite.directory.current": ("string",), "satellite.window.new": ("window",),
    "satellite.thread.new": ("thread",), "satellite.variable.info": ("list", ("map", ("string",), ("unknown",))),
    # what a display's call is followed by: only its .center() (console_calls.cpp's display_number_of)
    "satellite.console.display": ("display",),
    "satellite.infinity": ("infinity",),
    # A QUESTION ANSWERS A BOOL, AND THE FREE SPACE A FLOAT (file_calls.cpp, expression.cpp's
    # call_directory_word, info_calls.cpp -- each run on BUILD 0037, 2026-10-11): satellite.file.exists(p).string
    # is a bool's .string.
    "satellite.file.exists": ("bool",), "satellite.file.clear": ("bool",),
    "satellite.directory.change": ("bool",), "satellite.directory.free": ("float",),
}
# EVERY WORD THAT MAKES A WINDOW OR A PIECE OF ONE (window_calls.cpp's kWords) answers a window value.
for _piece in ("button", "label", "text_box", "text_area", "checkbox", "switch", "slider", "number_box", "progress",
               "choice", "row", "column", "grid", "picture", "scroll", "frame", "split", "menu", "canvas", "tabs",
               "one_of"):
    WORD_ANSWERS["satellite.window." + _piece] = ("window",)
WORD_ANSWERS["satellite.console.new"] = ("window",)
INFO = ("list", ("map", ("string",), ("unknown",)))      # satellite.variable.info: a list of maps (info_calls.hpp)


def word_answer(word):
    if word in WORD_ANSWERS:
        return WORD_ANSWERS[word]
    if word.startswith("satellite.info."):
        return INFO
    if word.startswith("satellite.random."):
        return ("number",)
    if word.startswith("satellite.time."):
        return ("time",)
    return UNKNOWN


# ---------------------------------------------------------------- the characters

class Token:
    __slots__ = ("kind", "text", "start", "end")

    def __init__(self, kind, text, start, end):
        self.kind, self.text, self.start, self.end = kind, text, start, end

    def __repr__(self):
        return f"{self.kind}:{self.text!r}@{self.start}"


def tokens_of(text, quoted=False):
    """The program's tokens -- as bytecode_registry.cpp's lexer splits them, closely enough to find a
    method and its receiver. `quoted`: the text QUOTES programs (a shell script, a document), so `\\n`
    outside a string ends a line and `\\"` is a quote, as a printf or an echo writes them."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in " \t\r":
            i += 1
            continue
        if c == "\n":
            out.append(Token("nl", "\n", i, i + 1))
            i += 1
            continue
        if quoted and c == "\\" and i + 1 < n and text[i + 1] == "n":
            out.append(Token("nl", "\n", i, i + 2))
            i += 2
            continue
        if quoted and c == "\\" and i + 1 < n and text[i + 1] in "trabfv0":
            i += 2                       # \t, a printf's or a grep's: a blank, never a name's first letter
            continue
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            if quoted:
                k = text.find("\\n", i, j)
                j = j if k < 0 else k
            out.append(Token("comment", text[i:j], i, j))
            i = j
            continue
        if c == '"' or (quoted and c == "\\" and i + 1 < n and text[i + 1] == '"'):
            j = i + (2 if c == "\\" else 1)
            while j < n and text[j] != "\n":
                if quoted and text.startswith('\\"', j):
                    j += 2
                    break
                if text[j] == '"':
                    j += 1
                    break
                j += 2 if text[j] == "\\" and j + 1 < n else 1
            out.append(Token("str", text[i:j], i, j))
            i = j
            continue
        if c.isdigit():
            j = i
            while j < n and (text[j].isdigit() or (text[j] == "." and j + 1 < n and text[j + 1].isdigit())):
                j += 1
            kind = "float" if "." in text[i:j] else "number"
            if j < n and text[j] == "%":
                kind, j = "percentage", j + 1
            elif j < n and (text[j] == "s" or text.startswith("ms", j) or text.startswith("ns", j)):
                unit = 1 if text[j] == "s" else 2
                if j + unit >= n or not (text[j + unit].isalnum() or text[j + unit] == "_"):
                    kind, j = "time", j + unit
            out.append(Token("num", text[i:j], i, j))
            out[-1].kind = "num:" + kind
            i = j
            continue
        if c.isalpha() or c == "_":
            j = i
            while j < n and (text[j].isalnum() or text[j] == "_"):
                j += 1
            out.append(Token("ident", text[i:j], i, j))
            i = j
            continue
        # A FULL STOP, IN TEXT THAT QUOTES PROGRAMS (the sweep's E16, 2026-10-11): a `.` with a blank after it ends
        # a sentence -- "them. satellite.terminal.foreground" is a word after it, never a method -- and one with a
        # blank before it has no receiver: "until .foreground() or" names a word, and is no call to respell.
        if quoted and c == "." and (i + 1 >= n or text[i + 1] in " \t\r\n" or text.startswith("\\n", i + 1) or
                                    i == 0 or text[i - 1] in " \t\r\n"):
            out.append(Token("stop", c, i, i + 1))
            i += 1
            continue
        out.append(Token("punct", c, i, i + 1))
        i += 1
    return out


# ---------------------------------------------------------------- kinds

UNKNOWN = ("unknown",)


def item_of(kind):
    if kind[0] == "list":
        return kind[1] if len(kind) > 1 else UNKNOWN
    if kind[0] == "map":
        return kind[2] if len(kind) > 2 else UNKNOWN
    if kind[0] in ("string", "file"):
        return ("string",)
    return UNKNOWN


# WHAT ANSWERS THE WINDOW ITSELF (method_periods.hpp's kind_a_method_answers): every doing, and the three makers.
WINDOW_DOINGS = {"append_token", "open_token", "close_token", "focus_token", "press_token", "resize_token",
                 "colour_token", "background_token", "font_token", "every_token", "message_token", "ask_token",
                 "choose_a_file_token", "menu_token", "item_token", "separator_token", "line_token", "box_token",
                 "circle_token", "write_token", "arc_token", "clear_token", "save_token", "display_token",
                 "home_token", "listen_token", "section_token", "widgets_display_file_token",
                 "widgets_display_input_token"}
# THE 18 WORDS WHOSE ANSWER THE BRACKETS DECIDE (the sweep's B5 and E14, 2026-10-11): with something in its
# brackets each sets a thing and answers the window -- w.satellite.title("t").satellite.focus() -- and read bare it
# answers what it holds: text, a bool or a number. A value is a slider's number or a bar's percentage, so it is
# not worked out; scroll.vertical and scroll.horizontal are a part's (answer_of's "part").
WINDOW_READS = {"title_token": ("string",), "text_token": ("string",), "chosen_token": ("string",),
                "path_token": ("string",), "pressed_token": ("string",), "changed_token": ("string",),
                "closed_token": ("string",), "key_token": ("string",), "clicked_token": ("string",),
                "typed_token": ("string",), "directory_token": ("string",), "on_token": ("bool",),
                "fullscreen_token": ("bool",), "outline_token": ("bool",), "thickness_token": ("number",),
                "value_token": UNKNOWN}


def answer_of(kind, method, given=False):
    """What `method` (a token name) answers on a receiver of `kind`. `given`: whether the call was written with
    something in its brackets, for the one word whose answer it decides (a colour's .transparency)."""
    k = kind[0]
    if k == "string":
        if method in ("size_token", "find_token", "to_number_token"):
            return ("number",)
        if method in ("empty_token", "contains_token", "starts_with_token", "ends_with_token"):
            return ("bool",)
        if method == "split_token":
            return ("list", ("string",))
        if method in STRING_NUMBERED:
            return ("string",)
        return UNKNOWN
    if k == "list":
        item = kind[1] if len(kind) > 1 else UNKNOWN
        if method in ("size_token", "index_of_token", "search_token"):
            return ("number",)
        if method in ("contains_token", "empty_token"):
            return ("bool",)
        if method in ("first_token", "last_token", "max_token", "min_token"):
            return item
        if method == "sum_token":
            return item if item != UNKNOWN else ("number",)
        if method in ("join_token", "to_string_token"):
            return ("string",)
        if method in ("sort_token", "by_name_token", "by_value_token", "reverse_token", "append_token",
                      "clear_token", "insert_token", "truncate_token", "reserve_token", "remove_token",
                      "remove_at_token", "remove_first_token", "remove_last_token"):
            return kind                  # each answers the list it changed
        return UNKNOWN
    if k == "args":                      # the arguments variable: a map of rows filed under their names
        kind = ("map", ("string",), UNKNOWN)
        k = "map"
    if k == "map":
        key = kind[1] if len(kind) > 1 else UNKNOWN
        if method == "keys_token":
            return ("list", key)
        if method == "values_token":
            return ("list", kind[2] if len(kind) > 2 else UNKNOWN)
        if method in ("size_token",):
            return ("number",)
        if method in ("contains_token", "empty_token"):
            return ("bool",)
        if method in ("sort_token", "clear_token", "remove_token", "remove_first_token", "remove_last_token"):
            return kind                  # each answers the map
        if method == "reverse_token":
            return ("list", key)         # the keys back to front, as a list (1 4 1 14)
        if method in ("first_token", "last_token"):
            return key                   # a KEY, not a value (1 4 1 11 and 12)
        if method == "get_token":
            return kind[2] if len(kind) > 2 else UNKNOWN
        return UNKNOWN
    # THE OTHER SCALARS (scalar_calls.hpp), as their numbers' files answer (2026-10-11): a hex's and a binary's
    # .add IS +, which answers a number (x00FF.add(x01) is 256); a colour's .transparency reads a number and
    # .transparency(n) answers the colour it changed; a fraction's two numbers are a number when whole and a
    # float when not, which only the value says -- so what follows them is the walker's to judge.
    if k in ("hex", "binary") and method == "add_token":
        return ("number",)
    if k == "color" and method == "transparency_token":
        return kind if given else ("number",)
    if k == "fraction" and method in ("numerator_token", "denominator_token"):
        return UNKNOWN
    if k == "bool" and method == "add_token":
        return ("string",)               # + joins a bool only with text (his A, 2026-10-11; method_periods.hpp)
    if k in ("number", "infinity"):
        # A NUMBER'S WORDS (number_calls.cpp) -- and an infinity's, which are the number's and its own -- as
        # method_periods.hpp's kind_a_method_answers says them. .add, .power and .modulus answer what + ^ and %
        # do (5 + "x" is text, 2 ^ -1 a float), so they are not worked out.
        if method in ("to_string_token", "to_binary_token", "to_hexadecimal_token"):
            return ("string",)
        if method in NUMBER_ANSWERS_A_NUMBER:
            return ("number",)
        return UNKNOWN
    if k in ("number", "float", "binary", "hex", "fraction", "color", "percentage", "infinity", "time", "bool"):
        if method in ("to_string_token", "to_binary_token", "to_hexadecimal_token"):
            return ("string",)
        if method in ("to_number_token", "width_token", "numerator_token", "denominator_token",
                      "transparency_token"):
            return ("number",)
        if method in ("reverse_token", "add_token", "power_of_token"):
            return kind
        return UNKNOWN
    if k in ("program", "thread"):
        # start() answers what it started, so `one_second.start().join()` is followed to its join; a thread's
        # stop() answers the thread too. A thread's join() and wait() answer what its capsule handed back.
        if method == "start_token" or (k == "thread" and method == "stop_token"):
            return kind
        if k == "thread":
            return UNKNOWN
        if method in ("join_token", "code_token", "return_token", "end_token", "exit_token", "quit_token",
                      "shutdown_token"):
            return ("number",)
        if method in ("ok_token", "pass_token"):
            return ("bool",)
        if method == "error_text_token":
            return ("string",)
        return UNKNOWN
    if k == "object":
        # the five words every object has (pointer_calls.hpp): pointer() and reference() answer an object
        # of the same spacesuit, ok() a bool, lock() and unlock() nothing
        if method in ("pointer_token", "reference_token"):
            return kind
        if method == "ok_token":
            return ("bool",)
        return UNKNOWN
    if k == "window":
        if method in ("width_token", "height_token", "across_token", "down_token", "columns_token", "rows_token"):
            return ("number",)
        if method == "ok_token":
            return ("bool",)
        if method == "answer_token":
            return ("string",)
        if method in WINDOW_PART_HEADS:
            return ("part", method)      # the head of a word of parts: what follows it is the word's, never a method
        if method in WINDOW_DOINGS:
            return kind                  # a doing answers the window, a maker the piece it made
        if method in WINDOW_READS:
            return kind if given else WINDOW_READS[method]
        return UNKNOWN
    if k == "part":
        if kind[1] == "widgets_token" and method in ("display_token", "displays_token"):
            return ("part", "display")
        if kind[1] == "display" and method in ("file_piece_token", "input_piece_token"):
            return ("window",)
        if kind[1] == "scroll_token" and method in ("vertical_token", "horizontal_token"):
            return ("window",) if given else ("bool",)
        return UNKNOWN
    if k == "file":
        if method in ("read_all_token", "first_token", "last_token", "path_token", "error_text_token"):
            return ("string",)
        if method in ("size_token", "index_of_token", "search_token"):
            return ("number",)
        # A FILE'S CHANGES ANSWER WHETHER THEY LANDED, never the file (the walker: f.append("a").append("b") is
        # a bool's second link), as its questions do; its lock answers nothing.
        if method in ("lock_token", "unlock_token"):
            return UNKNOWN
        if method in FILE_NUMBERED:
            return ("bool",)
        return UNKNOWN
    return UNKNOWN


# ---------------------------------------------------------------- the reader

class Scope:
    def __init__(self, to_parent=False, suit=None, capsule=None):
        self.names = {}
        self.to_parent = to_parent       # a spacesuit's protected/public block: its fields are the suit's
        self.suit = suit
        self.capsule = capsule


SUIT_WORDS = ("spacesuit", "class")     # satellite.class is satellite.spacesuit's second spelling (aliases.tsv)


def include_path(written, including):
    """WHERE satellite.include(written) READS ITS FILE, by include_shape.cpp's rules: `.satl` added when it is not
    written, ~/ the user's home, /root/x the machine's /x, and anything else beside the including file."""
    path = written if written.endswith(".satl") else written + ".satl"
    if path.startswith("/root/") and len(path) > len("/root/"):
        return path[len("/root"):]
    if path.startswith("~/"):
        return os.path.expanduser(path)
    if path.startswith(os.path.expanduser("~") + "/"):
        return path
    return os.path.normpath(os.path.join(os.path.dirname(including), path.lstrip("/")))


def stem_of(written):
    """The name an included file is read through (include_shape.cpp's stem_of): narration for ../narration/narration."""
    name = written.rsplit("/", 1)[-1]
    return name[:-len(".satl")] if name.endswith(".satl") else name


INCLUDED = {}                           # a path -> the Reader of a file a program includes, None if unreadable


def included(path):
    """THE FILE AT `path`, READ FOR WHAT IT DECLARES -- its spacesuits, their capsules and its satellite.library
    values -- never walked or changed: a program's spacesuit may extend one of another file's (unit.unit)."""
    if path not in INCLUDED:
        INCLUDED[path] = None           # before reading: a file that includes itself, round about, reads as unread
        try:
            INCLUDED[path] = Reader(read(path), False, {}, path)
        except (OSError, UnicodeDecodeError):
            pass
    return INCLUDED[path]


class Reader:
    """One pass over one text: what each name is, and every method written with a bare `.` whose
    receiver is a string or a list."""

    def __init__(self, text, quoted, returns, path=None, around=None):
        self.text = text
        self.toks = [t for t in tokens_of(text, quoted) if t.kind != "comment"]
        self.quoted = quoted
        self.path = os.path.abspath(path) if path and not quoted else None
        self.around = around             # a shell script's: an offset here -> (the script's text, the offset there)
        self.returns = returns           # (suit or None, capsule) -> kind, from the pass before
        self.found = {}                  # this pass's satellite.return kinds
        self.scopes = [Scope()]
        self.last_seen = {}              # the nearest declaration of each name, scope or not
        self.suits = {self.toks[k + 2].text for k in range(len(self.toks) - 2)
                      if self.toks[k].text in SUIT_WORDS and self.is_punct(k - 1, ".")
                      and self.tok(k - 2).text == "satellite" and self.toks[k + 2].kind == "ident"
                      and self.toks[k + 1].kind == "nl"} | \
                     {self.toks[k + 1].text for k in range(len(self.toks) - 1)
                      if self.toks[k].text in SUIT_WORDS and self.is_punct(k - 1, ".")
                      and self.tok(k - 2).text == "satellite" and self.toks[k + 1].kind == "ident"}
        self.pending = []                # declarations in a header's ( ), for the { that follows
        self.holds = {}                  # what a multiple holds NOW, line by line: `x = x.string` makes it text
        self.after_line = None           # (name, kind) a multiple takes when this line ends
        self.inserts = []                # character offsets where `satellite.` goes
        self.options = []                # character offsets of a display's or an input's option (his B)
        self.replaced = []               # (start, end, the new text, what) -- his K's two older spellings
        self.unknown = []                # (offset, method spelling) not worked out
        self.match = self.brackets()
        self.values = {}                 # a run's start -> (where its value word ends, its kind), worked out once
        self.names_arguments = None      # text that quotes programs: whether it declares a name `arguments`
        # EVERY SPACESUIT'S OWN CAPSULES, AND WHAT IT EXTENDS: after a bare `.`, a name his spacesuit -- or
        # one it extends -- declares is HIS capsule, one of the five words every object has too, and keeps
        # its `.` (his C of 2026-10-10; satellite/bytecode/pointer_calls.hpp).
        self.suit_capsules, self.suit_supers = self.capsules_of_suits()
        self.includes = self.includes_of()
        self.library_values = {}
        self.library_values = self.library_values_of()
        self.values = {}                 # worked out again now that the library values are known

    def capsules_of_suits(self):
        """{spacesuit: {capsule: whether it is public}}, and {spacesuit: [what it extends, as written]}. A capsule
        is public only in satellite.public: 003 made one written outside every section protected
        (capsule_scopes.cpp's declare_capsule)."""
        bodies, supers = [], {}
        for k, t in enumerate(self.toks):
            if t.kind != "ident" or t.text not in SUIT_WORDS or not self.is_punct(k - 1, ".") or \
                    self.tok(k - 2).text != "satellite":
                continue
            n = k + 1
            while self.tok(n).kind == "nl":
                n += 1
            if self.tok(n).kind != "ident":
                continue
            name, j = self.tok(n).text, n + 1
            if self.is_punct(j, "(") and j in self.match:
                # A PARENT IS A NAME, OR A FILE'S AND A NAME: satellite.spacesuit alien(unit.unit)
                written, supers[name] = "", []
                for x in range(j + 1, self.match[j] + 1):
                    if x == self.match[j] or self.is_punct(x, ","):
                        if written:
                            supers[name].append(written)
                        written = ""
                    elif self.tok(x).kind == "ident" or self.is_punct(x, "."):
                        written += self.tok(x).text
                j = self.match[j] + 1
            while self.tok(j).kind == "nl":
                j += 1
            if self.is_punct(j, "{") and j in self.match:
                bodies.append((name, j, self.match[j]))
        capsules, public = {}, []
        for k, t in enumerate(self.toks):
            if t.text == "public" and self.is_punct(k - 1, ".") and self.tok(k - 2).text == "satellite":
                j = k + 1
                while self.tok(j).kind == "nl":
                    j += 1
                if self.is_punct(j, "{") and j in self.match:
                    public.append((j, self.match[j]))
        for name, a, b in bodies:
            inner = [(x, y) for (_, x, y) in bodies if a < x and y < b]
            own = capsules.setdefault(name, {})
            for k in range(a, b):
                if self.tok(k).text == "capsule" and self.is_punct(k - 1, ".") and self.tok(k - 2).text == "satellite" \
                        and self.tok(k + 1).kind == "ident" and not any(x < k < y for x, y in inner):
                    own[self.tok(k + 1).text] = any(x < k < y for x, y in public)
        return capsules, supers

    def includes_of(self):
        """THE FILES THIS ONE INCLUDES, by the name each is read through: {stem: path}. Only a file has them --
        text that quotes programs is read alone."""
        found = {}
        if self.path is None:
            return found
        for k, t in enumerate(self.toks):
            if t.text != "include" or not self.is_punct(k - 1, ".") or self.tok(k - 2).text != "satellite" or \
                    not self.is_punct(k + 1, "(") or k + 1 not in self.match:
                continue
            written = self.text[self.tok(k + 1).end:self.tok(self.match[k + 1]).start].strip()
            if written.startswith('"') and written.endswith('"') and len(written) > 1:
                written = written[1:-1]
            if written and written != "satellite" and "(" not in written:
                found[stem_of(written)] = include_path(written, self.path)
        return found

    def library_values_of(self):
        """WHAT EACH satellite.library VALUE OF THIS FILE IS -- satellite.library.span = 25, a number -- by what a
        line gives it; not worked out when two lines give it different kinds."""
        values = {}
        for k, t in enumerate(self.toks):
            if t.text != "satellite" or t.kind != "ident" or not (k == 0 or self.tok(k - 1).kind == "nl" or
                                                                  self.is_punct(k - 1, "{")):
                continue
            if self.tok(k + 2).text != "library" or not self.is_punct(k + 1, ".") or not self.is_punct(k + 3, ".") or \
                    self.tok(k + 4).kind != "ident" or not self.is_punct(k + 5, "=") or self.is_punct(k + 6, "="):
                continue
            name = self.tok(k + 4).text
            if "satellite.library." + name in WORD_PATHS:
                continue
            end = k + 6
            while end < len(self.toks) and self.toks[end].kind != "nl":
                end += 1
            kind = self.kind_between(k + 6, end - 1, 0)
            values[name] = kind if values.get(name, kind) == kind else UNKNOWN
        return values

    # -------------------------------------------------------- his spacesuits

    def suit_key(self, written):
        """THE SPACESUIT A NAME WRITTEN IN THIS FILE IS: (its file, its name) -- narration.narrator is narrator in
        the file included as narration -- or None when this cannot read it: a file it cannot open, or a name with
        no body anywhere it can see."""
        if "." not in written:
            return (self.path, written) if written in self.suit_capsules else None
        stem, name = written.split(".", 1)
        other = included(self.includes[stem]) if stem in self.includes else None
        return (other.path, name) if other is not None and name in other.suit_capsules else None

    def reader_of(self, key):
        return self if key[0] == self.path else included(key[0])

    def parents_of(self, key):
        owner = self.reader_of(key)
        return [owner.suit_key(each) for each in owner.suit_supers.get(key[1], ())] if owner else [None]

    def extends(self, suit, other):
        """Whether spacesuit `suit` is `other`, or extends it."""
        seen, todo = set(), [suit]
        while todo:
            each = todo.pop()
            if each is None or each in seen:
                continue
            if each == other:
                return True
            seen.add(each)
            todo.extend(self.parents_of(each))
        return False

    def declares(self, suit, name):
        """WHETHER `x.name` ON AN OBJECT OF `suit` RUNS HIS CAPSULE, as CapsuleTable::declares says it
        (suit_reach.cpp, BUILD 0034): the first capsule of that name -- or of a method word's other spelling, as
        to_number is number -- on the spacesuit and then on what it extends, most derived first; one in
        satellite.protected only from inside a capsule of that spacesuit, or of one that extends it. True or
        False -- or None when a spacesuit on the way cannot be read, and so the answer could be either."""
        method = METHODS.get(name)
        here = self.current_suit()
        caller = (self.path, here) if here else None
        unread, seen, todo = False, set(), [suit]
        while todo:
            each = todo.pop(0)
            if each is None:
                unread = True
                continue
            if each in seen:
                continue
            seen.add(each)
            owner = self.reader_of(each)
            capsules = owner.suit_capsules.get(each[1], {}) if owner else {}
            found = name if name in capsules else \
                next((c for c in capsules if method is not None and METHODS.get(c) == method), None)
            if found is not None:
                if capsules[found] or (caller is not None and self.extends(caller, each)):
                    return True
                return None if unread else False
            todo.extend(self.parents_of(each))
        return None if unread else False

    # -------------------------------------------------------- brackets

    def brackets(self):
        match, stack = {}, []
        for i, t in enumerate(self.toks):
            if t.kind != "punct":
                continue
            if t.text in "([{":
                stack.append(i)
            elif t.text in ")]}":
                while stack:
                    j = stack.pop()
                    if self.toks[j].text == {")": "(", "]": "[", "}": "{"}[t.text]:
                        match[i], match[j] = j, i
                        break
        return match

    def tok(self, i):
        return self.toks[i] if 0 <= i < len(self.toks) else Token("end", "", 0, 0)

    def is_punct(self, i, ch):
        t = self.tok(i)
        return t.kind == "punct" and t.text == ch

    # -------------------------------------------------------- words

    def word_ending_at(self, i):
        """(start index, "satellite.a.b") when tokens start..i spell a satellite word, else None."""
        parts, j = [], i
        while self.tok(j).kind == "ident":
            parts.append(self.tok(j).text)
            if self.is_punct(j - 1, ".") and self.tok(j - 2).kind == "ident":
                j -= 2
                continue
            break
        parts.reverse()
        if parts and parts[0] == "satellite" and not self.is_punct(j - 1, "."):
            return j, ".".join(parts)
        return None

    def word_starting_at(self, i):
        """(end index, "satellite.a.b") for the dotted word from i, else None."""
        if self.tok(i).text != "satellite" or self.tok(i).kind != "ident" or self.is_punct(i - 1, "."):
            return None
        parts, j = ["satellite"], i
        while self.is_punct(j + 1, ".") and self.tok(j + 2).kind == "ident":
            parts.append(self.tok(j + 2).text)
            j += 2
        return j, ".".join(parts)

    def value_word(self, i):
        """A WORD THAT IS A VALUE, WHERE IT ENDS AND WHAT IT IS (the sweep's E13): (the index of its last name,
        its kind) for the run from `satellite` at i, up to a `.satellite.` already written -- the arguments (the
        run's own, or at their older place, his K), satellite.library.arguments, a satellite.library value of this
        file or of a file it includes, a bool word, a console fact -- or None for a run that is a word of some
        other kind, or no word at all. The lexer reads the longest run that is a word, so in
        satellite.library.span.abs() the word is satellite.library.span and .abs() its value's method."""
        if i in self.values:
            return self.values[i]
        names = [i]
        while self.is_punct(names[-1] + 1, ".") and self.tok(names[-1] + 2).kind == "ident" and \
                self.tok(names[-1] + 2).text != "satellite":
            names.append(names[-1] + 2)
        spelled = [self.tok(k).text for k in names]
        found = None
        if spelled[:2] == ["satellite", "arguments"]:
            found = names[1], ("args",)
        elif spelled[:4] == ["satellite", "library", "main", "arguments"]:
            found = names[3], ("args",)
        elif spelled[:3] == ["satellite", "library", "arguments"]:
            found = names[2], ("row", "")
        elif spelled[:2] == ["satellite", "library"] and len(spelled) > 2 and \
                "satellite.library." + spelled[2] not in WORD_PATHS:
            # A VALUE OF THIS FILE'S, OR OF A FILE IT INCLUDES -- satellite.library.combine_settings.default_threads
            # -- or one this cannot see: what follows that is listed, never respelled.
            if spelled[2] in self.library_values:
                found = names[2], self.library_values[spelled[2]]
            elif spelled[2] in self.includes and len(spelled) > 3:
                other = included(self.includes[spelled[2]])
                found = names[3], other.library_values.get(spelled[3], UNKNOWN) if other else UNKNOWN
            else:
                found = names[2], ("library",)
        else:
            for n in range(len(spelled), 0, -1):
                path = ".".join(spelled[:n])
                if path in WORD_PATHS:
                    found = (names[n - 1], VALUE_WORDS[path]) if path in VALUE_WORDS else None
                    break
        self.values[i] = found
        return found

    def value_ending_at(self, i):
        """The kind of the value word whose last name is token i, else None."""
        j = i
        while self.is_punct(j - 1, ".") and self.tok(j - 2).kind == "ident":
            j -= 2
            if self.tok(j).text == "satellite" and not self.is_punct(j - 1, "."):
                found = self.value_word(j)
                return found[1] if found and found[0] == i else None
        return None

    def type_at(self, i):
        """(end index, kind) for a type written from i -- satellite.container.list<...> -- else None."""
        found = self.word_starting_at(i)
        if found is None:
            return None
        end, word = found
        if word not in KIND_OF_TYPE:
            return None
        kind = KIND_OF_TYPE[word]
        params = []
        if self.is_punct(end + 1, "<"):
            depth, j, start = 0, end + 1, end + 2
            while j < len(self.toks):
                if self.is_punct(j, "<"):
                    depth += 1
                elif self.is_punct(j, ">"):
                    depth -= 1
                    if depth == 0:
                        break
                elif self.is_punct(j, ",") and depth == 1:
                    params.append(self.param_kind(start))
                    start = j + 1
                j += 1
            params.append(self.param_kind(start))
            end = j
        if word == "satellite.variable.info":
            return end, INFO
        if kind == "list":
            return end, ("list", params[0] if params else UNKNOWN)
        if kind == "map":
            return end, ("map", params[0] if params else UNKNOWN, params[1] if len(params) > 1 else UNKNOWN)
        if kind == "multiple":
            return end, ("multiple",) + tuple(params)
        return end, (kind,)

    def param_kind(self, i):
        found = self.type_at(i)
        if found:
            return found[1]
        written = self.suit_written_at(i)
        if written:
            return ("object", self.suit_key(written[1]))
        return UNKNOWN

    def suit_written_at(self, i):
        """(end index, name) for a spacesuit named from i -- greeter, or narration.narrator of a file this one
        includes -- else None."""
        t = self.tok(i)
        if t.kind != "ident" or self.is_punct(i - 1, "."):
            return None
        if t.text in self.suits:
            return i, t.text
        if t.text in self.includes and self.is_punct(i + 1, ".") and self.tok(i + 2).kind == "ident":
            return i + 2, t.text + "." + self.tok(i + 2).text
        return None

    # -------------------------------------------------------- names

    def declare(self, name, kind, in_header):
        self.last_seen[name] = kind
        if in_header:
            self.pending.append((name, kind))
            return
        scope = self.scopes[-1]
        while scope.to_parent and len(self.scopes) > 1:
            scope = self.scopes[self.scopes.index(scope) - 1]
        scope.names[name] = kind

    def lookup(self, name):
        kind = self.declared_as(name)
        # A MULTIPLE IS WHAT IT WAS LAST GIVEN, as the lines run in order (the loss tests' x is a number,
        # then text, then a list ...). Unknown once anything not worked out is put into it.
        if kind[0] == "multiple" and name in self.holds:
            return self.holds[name]
        return kind

    def declared_as(self, name):
        # TEXT THAT QUOTES PROGRAMS HAS NO SCOPES WORTH THE NAME -- a shell's braces are not a program's --
        # so there the nearest declaration above is the one.
        if self.quoted and name in self.last_seen:
            return self.last_seen[name]
        for scope in reversed(self.scopes):
            if name in scope.names:
                return scope.names[name]
        if name == "arguments":
            return ("args",)
        return self.last_seen.get(name, UNKNOWN)

    def current_suit(self):
        for scope in reversed(self.scopes):
            if scope.suit:
                return scope.suit
        return None

    def current_capsule(self):
        for scope in reversed(self.scopes):
            if scope.capsule:
                return scope.capsule
        return None

    # -------------------------------------------------------- what an expression is

    def kind_ending_at(self, i, depth=0):
        """The kind of the expression whose last token is i."""
        if depth > 60:
            return UNKNOWN
        t = self.tok(i)
        if t.kind == "str":
            return ("string",)
        if t.kind.startswith("num:"):
            # 1/3 -- A FRACTION: two numbers and a touching slash (fraction_values.cpp), so a method written after
            # it is the fraction's, as the checker reads it (2026-10-11).
            if (self.is_punct(i - 1, "/") and self.tok(i - 2).kind.startswith("num:") and
                    self.tok(i - 2).end == self.tok(i - 1).start and self.tok(i - 1).end == t.start):
                return ("fraction",)
            return (t.kind[4:],)
        if t.kind == "ident":
            # A WORD THAT IS A VALUE (value_word): satellite.arguments -- THE RUN'S OWN ARGUMENTS (2026-10-11), the
            # arguments variable, as main's is -- satellite.bool.true, satellite.library.span ... asked before the
            # period, which every word's last name has before it.
            value = self.value_ending_at(i)
            if value is not None:
                return value
            if self.is_punct(i - 1, "."):
                return self.after_a_period(i, depth)
            if self.word_ending_at(i):
                return UNKNOWN
            if re.fullmatch(r"b[01]+", t.text):
                return ("binary",)
            if re.fullmatch(r"x[0-9A-Fa-f]+", t.text):
                return ("hex",)
            return self.lookup(t.text)
        if t.kind == "punct" and t.text == ")" and i in self.match:
            j = self.match[i]
            before = self.tok(j - 1)
            if before.kind == "ident":
                word = self.word_ending_at(j - 1)
                if word:
                    # A THREAD KNOWS ITS CAPSULE: satellite.thread.new(worker(7))'s join() answers what worker
                    # hands back
                    if word[1] == "satellite.thread.new" and self.tok(j + 1).kind == "ident" and \
                            self.is_punct(j + 2, "("):
                        return ("thread", self.tok(j + 1).text)
                    return word_answer(word[1])
                if self.is_punct(j - 2, "."):
                    return self.after_a_period(j - 1, depth)
                return self.returns.get((None, before.text), self.returned_by_any(before.text))
            return self.kind_between(j + 1, i - 1, depth)
        if t.kind == "punct" and t.text == "]" and i in self.match:
            return item_of(self.kind_ending_at(self.match[i] - 1, depth + 1))
        if t.kind == "punct" and t.text == "}" and i in self.match:
            j = self.match[i]
            inside = [k for k in range(j + 1, i) if self.is_punct(k, ":")]
            if inside:
                return ("map", UNKNOWN, UNKNOWN)
            first_end = self.operand_end(j + 1, i - 1)
            return ("list", self.kind_ending_at(first_end, depth + 1) if first_end is not None else UNKNOWN)
        return UNKNOWN

    def after_a_period(self, i, depth):
        """The kind of `receiver.name` or `receiver.name(...)`, i on the name."""
        name = self.tok(i).text
        under = self.tok(i - 2).text == "satellite" and self.is_punct(i - 3, ".")
        if under:
            receiver = self.kind_ending_at(i - 4, depth + 1)          # .satellite.name
        else:
            receiver = self.kind_ending_at(i - 2, depth + 1)
        # THE ARGUMENTS' ROWS ARE DATA (main_arguments.hpp's past_the_argument_names): after the arguments a name
        # that is no map's word is a row, and after a row a name goes on with it when satl names such a row --
        # arguments.session.directory -- or when it is no method at all. A method after a row is a method of what
        # the row holds, which this does not know; after a satellite.library name it cannot see, nothing is known.
        if not under and receiver[0] == "args" and METHODS.get(name) not in UNDER["map"]:
            return ("row", name)
        if not under and receiver[0] == "row" and \
                ((receiver[1] + "." if receiver[1] else "") + name in ROW_PATHS or name not in METHODS):
            return ("row", (receiver[1] + "." if receiver[1] else "") + name)
        if receiver[0] == "library":
            return UNKNOWN
        if receiver[0] == "row":
            receiver = UNKNOWN
        if name in METHODS and receiver[0] != "object":
            method = METHODS[name]
            # A THREAD'S join() AND wait() ANSWER WHAT ITS CAPSULE HANDS BACK (thread_calls.hpp)
            if receiver[0] == "thread" and len(receiver) > 1 and method in ("join_token", "wait_method_token"):
                return self.returns.get((None, receiver[1]), self.returned_by_any(receiver[1]))
            if receiver == UNKNOWN and method in ONLY_A_STRINGS:
                receiver = ("string",)
            elif receiver == UNKNOWN and method in ONLY_A_LISTS:
                receiver = ("list", UNKNOWN)
            elif receiver == UNKNOWN and method in ONLY_A_MAPS:
                receiver = ("map", UNKNOWN, UNKNOWN)
            given = self.is_punct(i + 1, "(") and self.match.get(i + 1, i + 2) != i + 2
            return answer_of(receiver, method, given)
        if receiver[0] == "object":
            # ONE OF THE FIVE, unless it is a capsule of his written with its `.` -- or one a spacesuit this
            # cannot read might declare
            if METHODS.get(name) in UNDER["object"]:
                his = False if under else self.declares(receiver[1], name)
                if his is False:
                    return answer_of(receiver, METHODS[name])
                if his is None:
                    return UNKNOWN
            if receiver[1] is None or receiver[1][0] != self.path:
                return UNKNOWN
            return self.returns.get((receiver[1][1], name), UNKNOWN)
        # A WIDGET BY ITS NAME after a window (the author, 2026-10-09): main_window.files_display is a window value.
        if receiver[0] == "window":
            return ("window",)
        # A CAPSULE IN A SPACE OR A FILE -- `tools.triple(7)` -- by its name, when every capsule of that
        # name hands back the same kind.
        if receiver == UNKNOWN and self.is_punct(i + 1, "("):
            return self.returned_by_any(name)
        return UNKNOWN

    def returned_by_any(self, name):
        kinds = {kind for (suit, capsule), kind in self.returns.items() if capsule == name}
        return kinds.pop() if len(kinds) == 1 else UNKNOWN

    def operand_end(self, a, b):
        """The last token of the first operand in a..b: up to a top-level operator or comma."""
        if a > b:
            return None
        k = a
        if self.is_punct(k, "-") or self.is_punct(k, "!"):
            k += 1
        last = None
        while k <= b:
            t = self.tok(k)
            if t.kind == "punct" and t.text in "([{" and k in self.match:
                last = self.match[k]
                k = last + 1
                continue
            if t.kind == "punct" and t.text in ",+-*/%^=<>&|:;!?":
                break
            if t.kind == "nl":
                break
            last = k
            k += 1
        return last

    def plainly_text(self, end):
        """Whether the operand ending at `end` is text in quotes that reads as no number -- "x", never "2" or
        "-2.5" (satellite_object.cpp's number_in_text: a minus, digits, and a point and digits)."""
        t = self.tok(end)
        return t.kind == "str" and len(t.text) > 1 and \
            not re.fullmatch(r"-?[0-9]+(\.[0-9]+)?", t.text[1:-1])

    def kind_between(self, a, b, depth):
        """The kind of the expression in a..b: a comparison is a bool; a sum that starts with text, or whose
        every text is plainly text, is text; anything else is its first operand."""
        k, operands, compares, power = a, [], False, False
        while k <= b:
            end = self.operand_end(k, b)
            if end is None:
                break
            operands.append(end)
            k = end + 1
            while k <= b and self.tok(k).kind == "punct" and self.tok(k).text in "+-*/%^=<>&|!":
                if self.tok(k).text in "=<>&|!":
                    compares = True
                if self.tok(k).text == "^" or (self.tok(k).text == "*" and self.is_punct(k + 1, "*")):
                    power = True
                k += 1
        if compares:
            return ("bool",)
        kinds = [self.kind_ending_at(e, depth + 1) for e in operands]
        # A NUMBER MEETS TEXT BY WHAT THE TEXT SAYS (satellite_object.cpp, the author 2026-09-25: "we need 4 + "2"
        # to return the number 6"): 5 + "2" is the number 7 and 5 + "x" the text "5x", while "5" + 2 is "52". So a
        # sum with text in it is text when it starts with text, or when every text in it is plainly text; a text
        # that may read as a number, after something that may be a number, is not worked out.
        if any(kd == ("string",) for kd in kinds):
            if kinds[0] == ("string",) or \
                    all(self.plainly_text(e) for e, kd in zip(operands, kinds) if kd == ("string",)):
                return ("string",)
            return UNKNOWN
        # ARITHMETIC ON A HEX OR A BINARY ANSWERS A NUMBER: both are read by their worth (object_hexadecimal.cpp),
        # so (mask + 1) and (b1000 + b0101) are numbers, never the hex or the binary they start with (2026-10-11).
        if len(operands) > 1 and kinds[0][0] in ("hex", "binary"):
            return ("number",)
        # A NUMBER'S ARITHMETIC (NM-8): a number's words are under satellite and a float's and a fraction's are
        # not yet, so a sum starting with a number is a number only when it can be nothing else -- every operand
        # a number, or a percentage, binary or hex it meets by worth, and no power in it (2 ^ -1 is the float
        # 0.5). A float anywhere makes a float; anything else is not worked out, and the walker judges it.
        if len(kinds) > 1 and kinds[0] == ("number",):
            if ("float",) in kinds:
                return ("float",)
            if not power and all(kd[0] in ("number", "percentage", "binary", "hex") for kd in kinds):
                return ("number",)
            return UNKNOWN
        return kinds[0] if kinds else UNKNOWN

    # -------------------------------------------------------- the walk

    def walk(self):
        toks, i = self.toks, 0
        header_depth = None          # paren depth of a header whose names go to the next {
        paren = 0
        next_block = None            # what the next { opens: ("protected"|"suit"|"capsule", name)
        while i < len(toks):
            t = toks[i]
            if t.kind == "nl" and self.after_line is not None:
                self.holds[self.after_line[0]] = self.after_line[1]
                self.after_line = None
            if (t.kind == "ident" and self.tok(i + 1).text == "=" and self.tok(i + 2).text != "=" and
                    (self.tok(i - 1).kind == "nl" or self.is_punct(i - 1, "{") or i == 0) and
                    self.declared_as(t.text)[0] == "multiple"):
                end = i + 2
                while end < len(toks) and toks[end].kind != "nl":
                    end += 1
                self.after_line = (t.text, self.kind_between(i + 2, end - 1, 0))
            if t.kind == "punct" and t.text == "(":
                paren += 1
            elif t.kind == "punct" and t.text == ")":
                paren = max(0, paren - 1)
                if header_depth is not None and paren < header_depth:
                    header_depth = None
            elif t.kind == "punct" and t.text == "{" and not self.a_literal_brace(i):
                kind, name = next_block or (None, None)
                scope = Scope(to_parent=kind == "protected",
                              suit=name if kind == "suit" else None,
                              capsule=(self.current_suit(), name) if kind == "capsule" else None)
                for pname, pkind in self.pending:
                    scope.names[pname] = pkind
                self.pending = []
                self.scopes.append(scope)
                next_block = None
            elif t.kind == "punct" and t.text == "}" and not self.a_literal_brace(self.match.get(i, -1)):
                if len(self.scopes) > 1:
                    self.scopes.pop()
            elif t.kind == "ident":
                word = self.word_starting_at(i)
                if word:
                    end, spelled = word
                    if spelled in ("satellite.protected", "satellite.public", "satellite.private"):
                        next_block = ("protected", None)
                    elif spelled in ("satellite.spacesuit", "satellite.class") and toks[end + 1:end + 2] and \
                            toks[end + 1].kind == "ident":
                        self.suits.add(toks[end + 1].text)
                        next_block = ("suit", toks[end + 1].text)
                    elif spelled in OPTIONS and self.is_punct(end + 1, "("):
                        self.options_in(end + 1, OPTIONS[spelled])
                    elif spelled == "satellite.capsule":
                        named = self.word_starting_at(end + 1)
                        capsule = named[1] if named else self.tok(end + 1).text
                        next_block = ("capsule", capsule)
                        header_depth = paren + 1
                    elif spelled.startswith("satellite.statement."):
                        header_depth = paren + 1
                    elif spelled == "satellite.return" and self.is_punct(end + 1, "(") and end + 1 in self.match:
                        close = self.match[end + 1]
                        kind = self.kind_between(end + 2, close - 1, 0)
                        key = self.current_capsule()
                        if key:
                            old = self.found.get(key)
                            self.found[key] = kind if old in (None, kind) else UNKNOWN
                    typed = self.type_at(i)
                    if typed:
                        end2, kind = typed
                        if self.tok(end2 + 1).kind == "ident" and not self.word_starting_at(end2 + 1):
                            name = self.tok(end2 + 1).text
                            capsule = self.current_capsule()
                            if (kind == OLD_MAIN_TYPE and next_block and next_block[0] == "capsule"
                                    and next_block[1] == "satellite.main"):
                                kind = ("args",)       # main's arguments are the arguments variable
                                # HIS K: main's older header, satellite.main(satellite.container.list<
                                # satellite.variable.string> arguments), is satellite.main(satellite.variable.arguments
                                # arguments) -- the type only, and the name main gives it stays
                                if spelled == "satellite.container.list" and self.is_punct(i - 1, "(") and \
                                        self.tok(i - 2).text == "main":
                                    self.replaced.append((t.start, self.tok(end2).end, "satellite.variable.arguments",
                                                          "header"))
                            elif kind == ("thread",) and self.is_punct(end2 + 2, "="):
                                kind = self.thread_given(end2 + 3) or kind
                            self.declare(name, kind, header_depth is not None and paren >= header_depth)
                            if kind[0] == "multiple" and self.is_punct(end2 + 2, "="):
                                end = end2 + 3
                                while end < len(toks) and toks[end].kind != "nl":
                                    end += 1
                                self.after_line = (name, self.kind_between(end2 + 3, end - 1, 0))
                            i = end2 + 2
                            continue
                    # HIS K: a read of satellite.library.main.arguments.<row> is satellite.arguments.<row> -- in a
                    # program. In text that quotes programs the old place is history or a row that tests its
                    # refusal (check.sh's switch_refused), so there it is listed and never respelled.
                    if spelled == OLD_ARGUMENTS or spelled.startswith(OLD_ARGUMENTS + "."):
                        if self.quoted:
                            self.unknown.append((self.tok(i + 6).end, spelled))
                        else:
                            self.replaced.append((t.start, self.tok(i + 6).end, "satellite.arguments", "place"))
                    # A WORD THAT IS A VALUE ENDS BEFORE ITS RUN DOES (value_word): the names after it are its
                    # value's methods, or the arguments' rows, and each period among them is read as any other
                    value = self.value_word(i)
                    if value and value[0] < end:
                        i = value[0] + 1
                        continue
                    i = end + 1
                    continue
                if t.text == "arguments":
                    runs = self.the_runs_bare_name(i)
                    if runs:
                        self.inserts.append(t.start)       # arguments -> satellite.arguments
                    elif runs is None:
                        self.unknown.append((t.end, "arguments"))
                written = self.suit_written_at(i)
                if written and self.tok(written[0] + 1).kind == "ident":
                    self.declare(self.tok(written[0] + 1).text, ("object", self.suit_key(written[1])),
                                 header_depth is not None and paren >= header_depth)
                    i = written[0] + 1
                    continue
            elif t.kind == "punct" and t.text == ".":
                self.a_period(i)
            i += 1

    def options_in(self, open_at, names):
        """HIS B: DISPLAY'S AND INPUT'S NAMED OPTIONS, UNDER SATELLITE -- each argument of the call whose `(` is at
        `open_at` that starts `name =`, never `==`, with a name the word takes, as console_calls.cpp's
        options_written_in reads them: at the call's own depth, so a capsule's call inside it keeps its own."""
        close = self.match.get(open_at)
        if close is None:
            return
        k, starts = open_at + 1, True
        while k < close:
            t = self.tok(k)
            if t.kind == "nl":
                k += 1
                continue
            if starts and t.kind == "ident" and t.text in names and self.is_punct(k + 1, "=") and \
                    not self.is_punct(k + 2, "="):
                self.options.append(t.start)
            starts = self.is_punct(k, ",")
            k = self.match[k] + 1 if t.kind == "punct" and t.text in "([{" and k in self.match else k + 1

    def thread_given(self, k):
        """("thread", its capsule) when the tokens from k are satellite.thread.new(capsule(...)), else None."""
        word = self.word_starting_at(k)
        if word and word[1] == "satellite.thread.new" and self.is_punct(word[0] + 1, "(") and \
                self.tok(word[0] + 2).kind == "ident" and self.is_punct(word[0] + 3, "("):
            return ("thread", self.tok(word[0] + 2).text)
        return None

    def talked_about(self, last, spelled, old):
        """IN TEXT THAT QUOTES PROGRAMS, A SPELLING TALKED ABOUT RATHER THAN WRITTEN (the sweep's E16): `old`, which
        ends at token `last` -- with its brackets, when they follow -- is followed by "is refused" or "is
        written", or stands within a few lines of `spelled`, the spelling it would become: `age.size is
        refused`, `host.lock() calls your spacesuit's lock(), and host.satellite.lock() the language's` -- or of
        a sentence that says the same word's new spelling is written or the old refused, or that names `old`
        itself: check.sh's rows of a refusal quote one ('2.5.number|that float.number is written that
        float.satellite.number now'). Such a sentence says what the old spelling does, and respelling it would
        change what it says. In a shell script the lines around it are the script's own, quotes and all."""
        k = last + 1
        if self.is_punct(k, "(") and k in self.match:
            k = self.match[k] + 1
        while self.is_punct(k, ","):
            k += 1
        if self.tok(k).text in ("is", "are", "was", "were") and self.tok(k + 1).text in ("refused", "written"):
            return True
        text, at = self.around(self.tok(last).start) if self.around else (self.text, self.tok(last).start)
        a = b = at
        for _ in range(3):               # two lines above it, and four below: a row's sentence follows its program
            a = max(text.rfind("\n", 0, a), 0)
        for _ in range(5):
            found = text.find("\n", b + 1)
            b = len(text) if found < 0 else found
        near = text[a:b]
        word = spelled.rsplit(".", 1)[-1]
        return spelled in near or re.search(re.escape(old) + r"(\(\))? (is|are|was|were) ", near) is not None or \
            (("is written" in near or "refused" in near) and
             re.search(r"\.satellite\." + re.escape(word) + r"\b", near) is not None)

    def the_runs_bare_name(self, i):
        """THE BARE NAME `arguments` WHERE IT IS THE RUN'S OWN (2026-10-11: "arguments becomes satellite.arguments"):
        not after a period, not a capsule's call, and no name of the body called that -- satellite.main's
        parameter `arguments` is main's own and stays as written. In text that QUOTES programs only where a row
        or a key follows it pressed against it -- arguments.username, arguments["x"] -- so prose that says
        "the arguments" is never touched; and where that text declares a name `arguments` anywhere, the place is
        listed (None), never respelled -- a page about main's own parameter names its rows that way."""
        if self.is_punct(i - 1, "."):
            return False
        after = self.tok(i + 1)
        if after.kind == "punct" and after.text == "(":
            return False
        before = self.tok(i - 1)
        if before.kind == "ident" or (before.kind == "punct" and before.text in ">"):
            return False                     # the name a declaration gives: satellite.variable.arguments arguments
        pressed = after.kind == "punct" and after.start == self.tok(i).end and after.text in ".[" and \
            (after.text == "[" or self.tok(i + 2).kind == "ident" and self.tok(i + 2).start == after.end)
        if self.quoted:
            if not pressed or "arguments" in self.last_seen:
                return False
            if self.names_arguments is None:
                self.names_arguments = any(t.text == "arguments" and (self.is_punct(k - 1, ">") or (
                    self.word_ending_at(k - 1) or (0, ""))[1] in KIND_OF_TYPE) for k, t in enumerate(self.toks))
            return None if self.names_arguments else True
        if "arguments" in (name for name, _ in self.pending):
            return False
        return not any("arguments" in scope.names for scope in self.scopes)

    def a_literal_brace(self, i):
        """Whether the { at i opens a list or a map rather than a body."""
        if i < 0:
            return False
        before = self.tok(i - 1)
        return before.kind == "punct" and before.text in "=(,{:+[" or before.kind == "str"

    def a_period(self, i):
        if self.tok(i - 1).text == "satellite" and self.is_punct(i - 2, "."):
            return                       # .satellite.find -- written already
        if self.tok(i - 1).text == "satellite" and self.tok(i - 1).kind == "ident":
            return                       # satellite.return(...) -- a word of the language, never a value's method
        before = self.tok(i - 1)
        if not (before.kind in ("ident", "str") or before.kind.startswith("num:") or
                (before.kind == "punct" and before.text in ")]}")):
            return                       # nothing that could be a receiver: "`.sum`" in prose names a word
        name_tok = self.tok(i + 1)
        if name_tok.kind != "ident" or name_tok.text not in METHODS:
            return
        if name_tok.text == "satellite":
            return
        method = METHODS[name_tok.text]
        if not any(method in methods for methods in UNDER.values()):
            return
        receiver = self.kind_ending_at(i - 1)
        if receiver[0] == "row":
            if (receiver[1] + "." if receiver[1] else "") + name_tok.text in ROW_PATHS:
                return                   # arguments.session.directory -- a row satl names, data
            receiver = UNKNOWN           # a method of what the row holds
        elif receiver[0] == "library":
            self.unknown.append((self.tok(i).end, "." + name_tok.text))
            return                       # a satellite.library name this cannot see: its value, or a file's name
        if receiver == UNKNOWN or receiver[0] == "multiple":
            if method in ONLY_A_STRINGS:
                receiver = ("string",)
            elif method in ONLY_A_LISTS:
                receiver = ("list", UNKNOWN)
            elif method in ONLY_A_MAPS:
                receiver = ("map", UNKNOWN, UNKNOWN)
            elif method == "sort_token" and self.is_punct(i + 2, "(") and (i + 2) in self.match:
                after = self.match[i + 2]
                if self.is_punct(after + 1, ".") and self.tok(after + 2).text in ("by_name", "by_value"):
                    receiver = ("list", UNKNOWN)
        if receiver[0] in UNDER:
            if method in UNDER[receiver[0]]:
                his = self.declares(receiver[1], name_tok.text) if receiver[0] == "object" else False
                if his:
                    return               # HIS OWN CAPSULE of that name keeps its `.` (his C)
                if his is None or (self.quoted and self.talked_about(
                        i + 1, before.text + ".satellite." + name_tok.text, before.text + "." + name_tok.text)):
                    self.unknown.append((self.tok(i).end, "." + name_tok.text))
                    return               # a spacesuit it cannot read, or a sentence about the old spelling
                self.inserts.append(self.tok(i).end)
        elif receiver == UNKNOWN or receiver[0] == "multiple":
            self.unknown.append((self.tok(i).end, "." + name_tok.text))


# WHAT A CHANGE IS, as main() counts them: a method's period, a named option (his B), main's older header and the
# arguments' older place (his K).
SAID = {"period": "to .satellite.", "option": "named options under satellite",
        "header": "main headers to satellite.variable.arguments",
        "place": "satellite.library.main.arguments to satellite.arguments"}


def edit(text, changes):
    for start, end, new, _ in sorted(changes, reverse=True):
        text = text[:start] + new + text[end:]
    return text


def convert(text, quoted, path=None, around=None):
    """(the new text, its changes as (start, end, new text, what), what is not worked out as (offset, spelling)).
    `path` is the file the text is, so its satellite.include lines can be read; `around`, a shell script's text
    around an offset (convert_shell)."""
    returns = {}
    for _ in range(3):                   # what capsules return, worked out from their satellite.return lines
        reader = Reader(text, quoted, returns, path, around)
        reader.walk()
        if reader.found == returns:
            break
        returns = reader.found
    reader = Reader(text, quoted, returns, path, around)
    reader.walk()
    changes = sorted({(at, at, "satellite.", "period") for at in reader.inserts} |
                     {(at, at, "satellite.", "option") for at in reader.options} | set(reader.replaced))
    return edit(text, changes), changes, reader.unknown


def shell_regions(text):
    """THE TEXT A SHELL SCRIPT HANDS satl, as (start, end) spans: what is inside 'single quotes' -- a
    printf's program, an expected sentence -- and the body of a here-document, except one fed to python.
    A bash "double-quoted" string and a # comment are the script's own and are left alone; a $( ... )
    inside double quotes is a command of its own, read again by these rules, so the single-quoted
    arguments a helper is given inside "$(helper '...' '...')" are found."""
    spans, n = [], len(text)
    stack = ["top"]                     # "top", "sub" (inside $( ), "dq" (inside "...")
    heredocs = []                       # (word, fed to python) waiting for the end of their line
    i = 0
    while i < n:
        c, state = text[i], stack[-1]
        if c == "\n" and heredocs and state != "dq":
            body = i + 1
            for word, python in heredocs:
                end = body
                while end < n:
                    stop = text.find("\n", end)
                    stop = n if stop < 0 else stop
                    if text[end:stop].strip() == word:
                        break
                    end = stop + 1
                if not python:
                    spans.append((body, end))
                stop = text.find("\n", end)
                body = n if stop < 0 else stop + 1
            heredocs = []
            i = body
            continue
        if c == "\\":
            i += 2
            continue
        if state == "dq":
            if c == '"':
                stack.pop()
            elif text.startswith("$(", i) and not text.startswith("$((", i):
                stack.append("sub")
                i += 2
                continue
            i += 1
            continue
        # top or sub
        if c == "#" and (i == 0 or text[i - 1] in " \t\n;("):
            j = text.find("\n", i)
            i = n if j < 0 else j
            continue
        if c == '"':
            stack.append("dq")
        elif c == "'":
            j = text.find("'", i + 1)
            j = n if j < 0 else j
            line_start = text.rfind("\n", 0, i) + 1
            if not re.search(r"python3?\s+(-\w\s+)*-c\s*$", text[line_start:i]):
                spans.append((i + 1, j))                 # python -c '...' is python's, not satl's
            i = j + 1
            continue
        elif text.startswith("$((", i):
            j = text.find("))", i)
            i = n if j < 0 else j + 2
            continue
        elif text.startswith("$(", i):
            stack.append("sub")
            i += 2
            continue
        elif c == "(" and state == "sub":
            stack.append("sub")
        elif c == ")" and state == "sub":
            stack.pop()
        elif text.startswith("<<", i) and not text.startswith("<<<", i):
            m = re.match(r"<<-?\s*(['\"]?)(\w+)\1", text[i:])
            if m:
                line_start = text.rfind("\n", 0, i) + 1
                head = text[line_start:i]
                python = bool(re.search(r"python|\.py\b", head, re.I)) or "PY" in m.group(2).upper()
                heredocs.append((m.group(2), python))           # python's own text, or a .py being written
                i += m.end()
                continue
        i += 1
    return spans


def convert_shell(text):
    """convert() over a shell script's spans, joined into one text so a name declared in one program is
    known in the sentence quoted after it; the changes are carried back to the script's own offsets."""
    spans = shell_regions(text)
    joined, starts = "", []
    for a, b in spans:
        starts.append((len(joined), a, b - a))
        joined += text[a:b] + "\n"
    def back(at):
        for virtual, real, length in starts:
            if virtual <= at <= virtual + length:
                return real + (at - virtual)
        return None
    _, changes, unknown = convert(joined, True, around=lambda at: (text, back(at) if back(at) is not None else 0))
    real = sorted({(back(a), back(b), new, what) for a, b, new, what in changes
                   if back(a) is not None and back(b) is not None})
    return edit(text, real), real, [(back(at), word) for at, word in unknown if back(at) is not None]


def line_of(text, at):
    return text.count("\n", 0, at) + 1


def apply_log(log, root_from, root_to, write):
    """THE WALKER'S OWN ANSWER (SATL_RESPELL=<log>, a satl built with method_periods.hpp's recording): each
    line is a file, a line, the column of a method's name and "insert" or "remove". The programs were run
    in a copy -- they write files beside themselves -- so a path under `root_from` is changed under
    `root_to`. Each change is made only where the text there is what the line says."""
    places = {}
    with open(log, encoding="utf-8") as handle:
        for row in handle:
            parts = row.rstrip("\n").split("\t")
            if len(parts) != 4:
                continue
            path, line, column, what = parts[0], int(parts[1]), int(parts[2]), parts[3]
            path = os.path.abspath(path)
            if root_from and path.startswith(root_from):
                path = root_to + path[len(root_from):]
            places.setdefault(path, set()).add((line, column, what))
    made = refused = 0
    for path, changes in sorted(places.items()):
        if not os.path.exists(path):
            print(f"{path}: not there -- {len(changes)} left")
            refused += len(changes)
            continue
        lines = read(path).split("\n")
        for line, column, what in sorted(changes, reverse=True):
            text = lines[line - 1] if 0 < line <= len(lines) else ""
            name = re.match(r"[A-Za-z_]\w*", text[column:])
            if what == "insert" and name and text[column - 1:column] == "." and name.group(0) in METHODS:
                lines[line - 1] = text[:column] + "satellite." + text[column:]
                made += 1
            elif what == "remove" and name and text[column - 11:column] == ".satellite.":
                lines[line - 1] = text[:column - 10] + text[column:]
                made += 1
            elif what == "prefix" and name and name.group(0) == "arguments" and \
                    not text[:column].endswith(".") and not text[:column].endswith("satellite."):
                lines[line - 1] = text[:column] + "satellite." + text[column:]    # the run's own: satellite.arguments
                made += 1
            else:
                print(f"{path}:{line}:{column}: {what} -- the text there is not a method after a period: {text.strip()[:120]}")
                refused += 1
        if write:
            with open(path, "w", encoding="utf-8") as handle:
                handle.write("\n".join(lines))
        print(f"{path}: {len(changes)} from the walker")
    print(f"{'made' if write else 'would make'} {made}; {refused} not made")


def main(argv):
    if "--apply" in argv:
        rest = [a for a in argv if not a.startswith("--")]
        if len(rest) not in (1, 3):
            sys.exit("--apply LOG [FROM TO] [--write]")
        apply_log(rest[0], os.path.abspath(rest[1]) if len(rest) == 3 else "",
                  os.path.abspath(rest[2]) if len(rest) == 3 else "", "--write" in argv)
        return
    write = "--write" in argv
    quoted_all = "--text" in argv
    only = set(SAID)
    for a in argv:
        if a.startswith("--only="):
            only = set(a[len("--only="):].split(","))
            if not only <= set(SAID):
                sys.exit(f"--only takes {', '.join(SAID)} -- not {', '.join(sorted(only - set(SAID)))}")
    paths = [a for a in argv if not a.startswith("--")]
    if not paths:
        sys.exit(__doc__)
    counts, unknown_total = dict.fromkeys(SAID, 0), 0
    for path in paths:
        text = read(path)
        quoted = quoted_all or not path.endswith(".satl")
        out, changes, unknown = convert_shell(text) if path.endswith(".sh") else convert(text, quoted, path)
        if only != set(SAID):
            changes = [change for change in changes if change[3] in only]
            out = edit(text, changes)
            if "period" not in only:       # a period not worked out is left alone with the rest of them
                unknown = [(at, word) for at, word in unknown if not word.startswith(".")]
        unknown_total += len(unknown)
        mine = [what for _, _, _, what in changes]
        for what in SAID:
            counts[what] += mine.count(what)
        if changes:
            print(f"{path}: " + ", ".join(f"{mine.count(what)} {said}" for what, said in SAID.items() if what in mine))
        for at, word in unknown:
            print(f"{path}:{line_of(text, at)}: {word} -- not worked out, left as it is: "
                  f"{text.splitlines()[line_of(text, at) - 1].strip()[:140]}")
        if write and out != text:
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(out)
    print(f"{'changed' if write else 'would change'} {sum(counts.values())} in {len(paths)} files (" +
          ", ".join(f"{counts[what]} {said}" for what, said in SAID.items()) + f"); {unknown_total} not worked out")


if __name__ == "__main__":
    main(sys.argv[1:])
