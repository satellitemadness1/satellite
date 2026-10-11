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
list_number_of, token_codes.hpp's method_code_of), so when a type is numbered next, this converts it with
no change here but a line in KIND_OF_TYPE below.

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


METHODS = method_words()
STRING_NUMBERED = numbered("satellite/bytecode/string_calls.cpp", "string_number_of")
LIST_NUMBERED = numbered("satellite/bytecode/container_calls.cpp", "list_number_of")
UNDER = {"string": STRING_NUMBERED, "list": LIST_NUMBERED}

# A METHOD NO OTHER TYPE HAS: on a receiver this cannot work out, it says the receiver.
ONLY_A_STRINGS = {"upper_token", "lower_token", "starts_with_token", "ends_with_token", "substring_token",
                  "split_token", "trim_token", "at_token", "resolved_token", "foreground_token"}
ONLY_A_LISTS = {"by_name_token", "by_value_token", "sum_token", "max_token", "min_token", "reserve_token"}

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
}
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


def answer_of(kind, method):
    """What `method` (a token name) answers on a receiver of `kind`."""
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
    if k == "map":
        if method == "keys_token":
            return ("list", kind[1] if len(kind) > 1 else UNKNOWN)
        if method == "values_token":
            return ("list", kind[2] if len(kind) > 2 else UNKNOWN)
        if method in ("size_token",):
            return ("number",)
        if method in ("contains_token", "empty_token"):
            return ("bool",)
        if method in ("sort_token", "reverse_token", "clear_token", "remove_token"):
            return kind
        if method in ("first_token", "last_token"):
            return kind[2] if len(kind) > 2 else UNKNOWN
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
        if method in ("code_token", "return_token", "end_token", "exit_token", "quit_token", "shutdown_token"):
            return ("number",)
        if method == "ok_token":
            return ("bool",)
        if method == "error_text_token":
            return ("string",)
        return UNKNOWN
    if k == "file":
        if method in ("read_all_token", "first_token", "last_token", "path_token", "error_text_token"):
            return ("string",)
        if method in ("size_token", "index_of_token", "search_token"):
            return ("number",)
        if method in ("contains_token", "ok_token", "exists_token", "empty_token"):
            return ("bool",)
        return kind
    return UNKNOWN


# ---------------------------------------------------------------- the reader

class Scope:
    def __init__(self, to_parent=False, suit=None, capsule=None):
        self.names = {}
        self.to_parent = to_parent       # a spacesuit's protected/public block: its fields are the suit's
        self.suit = suit
        self.capsule = capsule


class Reader:
    """One pass over one text: what each name is, and every method written with a bare `.` whose
    receiver is a string or a list."""

    def __init__(self, text, quoted, returns):
        self.text = text
        self.toks = [t for t in tokens_of(text, quoted) if t.kind != "comment"]
        self.quoted = quoted
        self.returns = returns           # (suit or None, capsule) -> kind, from the pass before
        self.found = {}                  # this pass's satellite.return kinds
        self.scopes = [Scope()]
        self.last_seen = {}              # the nearest declaration of each name, scope or not
        self.suits = {self.toks[k + 2].text for k in range(len(self.toks) - 2)
                      if self.toks[k].text == "spacesuit" and self.is_punct(k - 1, ".")
                      and self.tok(k - 2).text == "satellite" and self.toks[k + 2].kind == "ident"
                      and self.toks[k + 1].kind == "nl"} | \
                     {self.toks[k + 1].text for k in range(len(self.toks) - 1)
                      if self.toks[k].text == "spacesuit" and self.is_punct(k - 1, ".")
                      and self.tok(k - 2).text == "satellite" and self.toks[k + 1].kind == "ident"}
        self.pending = []                # declarations in a header's ( ), for the { that follows
        self.holds = {}                  # what a multiple holds NOW, line by line: `x = x.string` makes it text
        self.after_line = None           # (name, kind) a multiple takes when this line ends
        self.inserts = []                # character offsets where `satellite.` goes
        self.unknown = []                # (offset, method spelling) not worked out
        self.match = self.brackets()

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
        if self.tok(i).kind == "ident" and self.tok(i).text in self.suits:
            return ("object", self.tok(i).text)
        return UNKNOWN

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
            return (t.kind[4:],)
        if t.kind == "ident":
            if self.is_punct(i - 1, "."):
                return self.after_a_period(i, depth)
            word = self.word_ending_at(i)
            if word:
                return ("bool",) if word[1] in ("satellite.bool.true", "satellite.bool.false") else UNKNOWN
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
        if self.tok(i - 2).text == "satellite" and self.is_punct(i - 3, "."):
            receiver = self.kind_ending_at(i - 4, depth + 1)          # .satellite.name
        else:
            receiver = self.kind_ending_at(i - 2, depth + 1)
        if name in METHODS and receiver[0] != "object" and receiver[0] != "args":
            method = METHODS[name]
            if receiver == UNKNOWN and method in ONLY_A_STRINGS:
                receiver = ("string",)
            elif receiver == UNKNOWN and method in ONLY_A_LISTS:
                receiver = ("list", UNKNOWN)
            return answer_of(receiver, method)
        if receiver[0] == "object":
            return self.returns.get((receiver[1], name), UNKNOWN)
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

    def kind_between(self, a, b, depth):
        """The kind of the expression in a..b: a comparison is a bool; a sum with text in it is text;
        anything else is its first operand."""
        k, operands, compares = a, [], False
        while k <= b:
            end = self.operand_end(k, b)
            if end is None:
                break
            operands.append(end)
            k = end + 1
            while k <= b and self.tok(k).kind == "punct" and self.tok(k).text in "+-*/%^=<>&|!":
                if self.tok(k).text in "=<>&|!":
                    compares = True
                k += 1
        if compares:
            return ("bool",)
        kinds = [self.kind_ending_at(e, depth + 1) for e in operands]
        if any(kd == ("string",) for kd in kinds):
            return ("string",)
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
                    elif spelled == "satellite.spacesuit" and toks[end + 1:end + 2] and toks[end + 1].kind == "ident":
                        self.suits.add(toks[end + 1].text)
                        next_block = ("suit", toks[end + 1].text)
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
                            if (kind == ("list", ("string",)) and next_block and next_block[0] == "capsule"
                                    and next_block[1] == "satellite.main"):
                                kind = ("args",)       # main's arguments are the arguments variable
                            self.declare(name, kind, header_depth is not None and paren >= header_depth)
                            if kind[0] == "multiple" and self.is_punct(end2 + 2, "="):
                                end = end2 + 3
                                while end < len(toks) and toks[end].kind != "nl":
                                    end += 1
                                self.after_line = (name, self.kind_between(end2 + 3, end - 1, 0))
                            i = end2 + 2
                            continue
                    i = end + 1
                    continue
                if t.text in self.suits and self.tok(i + 1).kind == "ident" and not self.is_punct(i - 1, "."):
                    self.declare(self.tok(i + 1).text, ("object", t.text),
                                 header_depth is not None and paren >= header_depth)
            elif t.kind == "punct" and t.text == ".":
                self.a_period(i)
            i += 1

    def a_literal_brace(self, i):
        """Whether the { at i opens a list or a map rather than a body."""
        if i < 0:
            return False
        before = self.tok(i - 1)
        return before.kind == "punct" and before.text in "=(,{:+[" or before.kind == "str"

    def a_period(self, i):
        if self.tok(i - 1).text == "satellite" and self.is_punct(i - 2, "."):
            return                       # .satellite.find -- written already
        name_tok = self.tok(i + 1)
        if name_tok.kind != "ident" or name_tok.text not in METHODS:
            return
        if name_tok.text == "satellite":
            return
        method = METHODS[name_tok.text]
        if method not in STRING_NUMBERED and method not in LIST_NUMBERED:
            return
        receiver = self.kind_ending_at(i - 1)
        if receiver == UNKNOWN or receiver[0] == "multiple":
            if method in ONLY_A_STRINGS:
                receiver = ("string",)
            elif method in ONLY_A_LISTS:
                receiver = ("list", UNKNOWN)
            elif method == "sort_token" and self.is_punct(i + 2, "(") and (i + 2) in self.match:
                after = self.match[i + 2]
                if self.is_punct(after + 1, ".") and self.tok(after + 2).text in ("by_name", "by_value"):
                    receiver = ("list", UNKNOWN)
        if receiver[0] in UNDER:
            if method in UNDER[receiver[0]]:
                self.inserts.append(self.tok(i).end)
        elif receiver == UNKNOWN or receiver[0] == "multiple":
            self.unknown.append((self.tok(i).end, name_tok.text))


def convert(text, quoted):
    returns = {}
    for _ in range(3):                   # what capsules return, worked out from their satellite.return lines
        reader = Reader(text, quoted, returns)
        reader.walk()
        if reader.found == returns:
            break
        returns = reader.found
    reader = Reader(text, quoted, returns)
    reader.walk()
    out = text
    for at in sorted(set(reader.inserts), reverse=True):
        out = out[:at] + "satellite." + out[at:]
    return out, sorted(set(reader.inserts)), reader.unknown


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
    _, inserts, unknown = convert(joined, True)

    def back(at):
        for virtual, real, length in starts:
            if virtual <= at <= virtual + length:
                return real + (at - virtual)
        return None
    real_inserts = sorted({back(at) for at in inserts if back(at) is not None})
    out = text
    for at in reversed(real_inserts):
        out = out[:at] + "satellite." + out[at:]
    return out, real_inserts, [(back(at), word) for at, word in unknown if back(at) is not None]


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
    paths = [a for a in argv if not a.startswith("--")]
    if not paths:
        sys.exit(__doc__)
    total = unknown_total = 0
    for path in paths:
        text = read(path)
        quoted = quoted_all or not path.endswith(".satl")
        out, inserts, unknown = convert_shell(text) if path.endswith(".sh") else convert(text, quoted)
        total += len(inserts)
        unknown_total += len(unknown)
        if inserts:
            print(f"{path}: {len(inserts)} to .satellite.")
        for at, word in unknown:
            print(f"{path}:{line_of(text, at)}: .{word} -- not worked out, left as it is: "
                  f"{text.splitlines()[line_of(text, at) - 1].strip()[:140]}")
        if write and out != text:
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(out)
    print(f"{'changed' if write else 'would change'} {total} in {len(paths)} files; {unknown_total} not worked out")


if __name__ == "__main__":
    main(sys.argv[1:])
