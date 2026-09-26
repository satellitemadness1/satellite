#!/usr/bin/env python3
# utility/check_container_shapes.py <satl> <depth> [folder] -- EVERY CONTAINER SHAPE UP TO <depth>.
#
# The author, 2026-09-25 (SCRATCH.md/CONTAINERS.md, step 1): "for any combination of
# containers, lists, maps, multiples, for any combination of them ... first we have to
# accept any sort of crazy combination". So this writes every one of them rather than
# the few a person thinks of: list<T>, map<string, T>, index<number, T> and
# multiple<string, T>, nested to <depth>, over number, string, float and bool.
#
# FOR EACH SHAPE, ONE PROGRAM THAT MUST RUN (exit 0, and these lines, in order):
#     declared with a literal         `list<map<string, number>> x = {{"k": 1}}`
#     shown whole                     exactly the literal, back          {{"k": 1}}
#     its deepest item read           x[1]["k"]                          1
#     that item written, read again   x[1]["k"] = 2                      2
#     an empty one filled             y.append({"k": 1}) or y[7] = ...   then its deepest item
# AND TWO THAT MUST BE REFUSED (exit 27, types_do_not_meet): the same literal with a
# leaf of the wrong kind, and a wrong kind written into the deepest item. A shape that
# accepts everything accepts nothing in particular -- the declaration is the promise.
#
# map and index are one container (type_shape.hpp, 2026-09-26); string keys are
# written map and number keys index, so both spellings are walked at every depth.
#
# depth 2 is 84 shapes and runs in check.sh; depth 4 is 1364 and takes about 30 s.
# Each program runs with its own HOME, no window and no display (ERRORS2's rules).
import os, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

V = "satellite.variable."
C = "satellite.container."
# word, a literal, a second literal, how display shows the first, how it shows the second
LEAVES = {
    "number": ("1", "2", "1", "2"),
    "string": ('"a"', '"b"', "a", "b"),
    "float": ("1.5", "2.5", "1.5", "2.5"),
    "bool": ("satellite.bool.true", "satellite.bool.false", "true", "false"),
}
# the leaf written where another is declared: never one the declared kind converts from
# (a float name takes a whole number, so a number is not wrong for a float)
WRONG = {"number": ["bool", "string"], "string": ["number", "bool"],
         "float": ["bool", "string"], "bool": ["number", "string"]}
CONTAINERS = ("list", "map", "index", "multiple")


def shapes(depth):
    out = [("leaf", word) for word in LEAVES]
    if depth > 0:
        for inner in shapes(depth - 1):
            out += [(kind, inner) for kind in CONTAINERS]
    return out


def written(shape):
    kind, inner = shape
    if kind == "leaf": return V + inner
    if kind == "list": return C + "list<" + written(inner) + ">"
    if kind == "map": return C + "map<" + V + "string, " + written(inner) + ">"
    if kind == "index": return C + "index<" + V + "number, " + written(inner) + ">"
    return C + "multiple<" + V + "string, " + written(inner) + ">"


def the_leaf(shape):
    while shape[0] != "leaf":
        shape = shape[1]
    return shape[1]


# the literal holding `leaf` at the bottom, and the [ ] path down to it
def literal(shape, leaf, shown=False):
    kind, inner = shape
    if kind == "leaf":
        value = LEAVES[leaf][2] if shown else LEAVES[leaf][0]
        return ('"' + value + '"' if shown and leaf == "string" else value), ""
    text, path = literal(inner, leaf, shown)
    if kind == "list": return "{" + text + "}", "[1]" + path
    if kind == "map": return '{"k": ' + text + "}", '["k"]' + path
    if kind == "index": return "{7: " + text + "}", "[7]" + path
    return text, path                            # a multiple holds the value itself


def program(body):
    return ("satellite.include(satellite)\n\nsatellite.capsule satellite.main()\n{\n" +
            "\n".join("    " + line for line in body) + "\n    satellite.return(satellite)\n}\n")


def the_programs(whole):
    shape, leaf = whole, the_leaf(whole)
    text, path = literal(shape, leaf)
    shown, _ = literal(shape, leaf, shown=True)
    if not path:                                 # a leaf, or multiples of one: shown bare
        shown = LEAVES[leaf][2]
    body = [written(shape) + " x = " + text, "satellite.console.display(x)"]
    lines = [shown]
    if path:
        body += ["satellite.console.display(x" + path + ")",
                 "x" + path + " = " + LEAVES[leaf][1],
                 "satellite.console.display(x" + path + ")"]
        lines += [LEAVES[leaf][2], LEAVES[leaf][3]]
    kind, inner = shape
    if kind in ("list", "map", "index"):
        inner_text, inner_path = literal(inner, leaf)
        key = {"list": None, "map": '"k"', "index": "7"}[kind]
        body += [written(shape) + " y",
                 "y.append(" + inner_text + ")" if key is None else "y[" + key + "] = " + inner_text,
                 "satellite.console.display(y" + ("[1]" if key is None else "[" + key + "]") + inner_path + ")"]
        lines.append(LEAVES[leaf][2])
    runs = [(program(body), 0, lines)]

    if path:
        # what the deepest slot takes: its leaf, and a string for each multiple right above it
        takes, walk, down = {leaf}, [], whole
        while down[0] != "leaf":
            walk.append(down[0]); down = down[1]
        for kind in reversed(walk):
            if kind != "multiple": break
            takes.add("string")
        wrong = next(w for w in WRONG[leaf] if w not in takes)
        runs.append((program([written(whole) + " x = " + literal(whole, wrong)[0]]), 27, None))
        runs.append((program([written(whole) + " x = " + text, "x" + path + " = " + LEAVES[wrong][0]]), 27, None))
    return runs


# SATELLITE.ACCESS'S OWN LINES MUST RUN (step 2): access is asked about x, every line it
# prints is made into code -- a position letter becomes 1, a bare key 1, a quoted "key"
# stays the string it is -- and a program runs every line that fills a level, in order,
# then reads every line that reaches one. A line access shows that does not run is wrong.
LETTERS = ("n", "m", "p", "q", "r", "s", "t", "u", "v", "w")


def lines_to_code(printed):
    import re
    fills, reads = [], []
    for line in printed.split("\n"):
        if not line.startswith("  ") or line.startswith("   "):
            continue
        left = line[2:].split("  ")[0]
        if left == "value":
            continue
        code = re.sub(r"\[(" + "|".join(LETTERS) + r"|key[0-9]*)\]", "[1]", left)
        (fills if ".append(" in code or " = " in code else reads).append(code)
    return fills, reads


def access_programs(whole):
    text, path = literal(whole, the_leaf(whole))
    return program([written(whole) + " x = " + text, "satellite.access(x)"])


def run_text(satl, folder, name, text):
    path = os.path.join(folder, name + ".satl")
    with open(path, "w") as f:
        f.write(text)
    env = {k: v for k, v in os.environ.items()
           if k not in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY", "GDK_BACKEND")}
    env.update(HOME=os.path.join(folder, "home"), XDG_RUNTIME_DIR=os.path.join(folder, "xdg"),
               SATL_NO_WINDOW="1")
    try:
        done = subprocess.run([satl, path], capture_output=True, text=True, timeout=20, env=env)
        return path, done.returncode, done.stdout, done.stderr
    except subprocess.TimeoutExpired:
        return path, "timeout", "", ""


def access_runs(job):
    satl, folder, n, whole = job
    path, code, out, err = run_text(satl, folder, "a%05d_asked" % n, access_programs(whole))
    if code != 0 or not out.strip():
        return False, path, 0, code, None, [], err
    fills, reads = lines_to_code(out)
    text, _ = literal(whole, the_leaf(whole))
    body = [written(whole) + " x = " + text] + fills + ["satellite.console.display(" + r + ")" for r in reads]
    path, code, out, err = run_text(satl, folder, "a%05d_ran" % n, program(body))
    return code == 0, path, 0, code, None, [], err


def run(job):
    satl, folder, name, (text, wanted_code, wanted_lines) = job
    path = os.path.join(folder, name + ".satl")
    with open(path, "w") as f:
        f.write(text)
    env = {k: v for k, v in os.environ.items()
           if k not in ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY", "GDK_BACKEND")}
    env.update(HOME=os.path.join(folder, "home"), XDG_RUNTIME_DIR=os.path.join(folder, "xdg"),
               SATL_NO_WINDOW="1")
    try:
        done = subprocess.run([satl, path], capture_output=True, text=True, timeout=20, env=env)
        code, out, err = done.returncode, done.stdout, done.stderr
    except subprocess.TimeoutExpired:
        code, out, err = "timeout", "", ""
    got = [line for line in out.split("\n") if line]
    if wanted_lines is not None:
        got = got[-len(wanted_lines):]
    good = code == wanted_code and (wanted_lines is None or got == wanted_lines)
    return good, path, wanted_code, code, wanted_lines, got, err


def main():
    if len(sys.argv) < 3:
        sys.exit("usage: check_container_shapes.py <satl> <depth> [folder]")
    satl, depth = os.path.abspath(sys.argv[1]), int(sys.argv[2])
    folder = os.path.abspath(sys.argv[3] if len(sys.argv) > 3 else "build/container_shapes")
    os.makedirs(os.path.join(folder, "home", ".satl"), exist_ok=True)
    os.makedirs(os.path.join(folder, "xdg"), mode=0o700, exist_ok=True)
    os.chmod(os.path.join(folder, "xdg"), 0o700)
    every = shapes(depth)
    jobs = []
    for n, shape in enumerate(every):
        for m, one in enumerate(the_programs(shape)):
            jobs.append((satl, folder, "s%05d_%d" % (n, m), one))
    with ThreadPoolExecutor(max_workers=6) as pool:
        results = list(pool.map(run, jobs))
    with ThreadPoolExecutor(max_workers=6) as pool:
        asked = list(pool.map(access_runs, [(satl, folder, n, shape) for n, shape in enumerate(every)
                                            if shape[0] != "leaf"]))
    results += asked
    bad = [r for r in results if not r[0]]
    with open(os.path.join(folder, "failures.txt"), "w") as f:
        for good, path, wanted_code, code, wanted_lines, got, err in bad:
            f.write("=== %s\nexit %s, wanted %s\nwanted %r\ngot    %r\n%s\n" %
                    (path, code, wanted_code, wanted_lines, got, err[-1500:]))
    refusals = sum(1 for job in jobs if job[3][1] != 0)
    print("%d shapes to depth %d: %d programs (%d must run, %d must be refused, %d run what satellite.access "
          "shows), %d wrong%s" %
          (len(every), depth, len(jobs) + len(asked), len(jobs) - refusals, refusals, len(asked), len(bad),
           "" if not bad else " -- " + os.path.join(folder, "failures.txt")))
    sys.exit(1 if bad else 0)


main()
