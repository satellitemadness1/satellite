#!/usr/bin/env python3
"""Run every worked line in the entries and report which ones do not do what
the entry claims.  A help file whose examples were never executed is the drift
this milestone exists to end, so the examples are measured here rather than
read.

An entry may carry "expect":
    "runs"      (default) the example must compile and exit 0
    "refuses"   the example must exit non-zero -- for paths no milestone built
    "skip"      not runnable on its own (it IS a help call, or a bare path)
"""
import json, os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SATL = os.path.join(ROOT, "satl")

def load_entries():
    entries = {}
    for name in sorted(os.listdir(HERE)):
        if name.startswith("entries_") and name.endswith(".json"):
            with open(os.path.join(HERE, name)) as f:
                for k, v in json.load(f).items():
                    v["_from"] = name
                    entries[k] = v
    return entries

def as_program(example):
    """Wrap a fragment so it is a whole program.  A fragment that declares a
    capsule has to sit OUTSIDE main; loose statements go inside one.  An
    example that already declares `satellite.main` is left to stand on its own,
    or the wrapper would declare a second and the file would refuse."""
    text = example.strip("\n")
    lines = text.split("\n")

    has_include = any(l.startswith("satellite.include") and "(" in l and
                      not l.startswith("satellite.include()") for l in lines)
    has_main = any(l.startswith("satellite.capsule satellite.main") for l in lines)

    head, decls, body = [], [], []
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith("satellite.include"):
            head.append(line)
            i += 1
            continue
        if (line.startswith("satellite.library.")
                and not line.startswith("satellite.library.system.")):
            # A GLOBAL IS A TOP-LEVEL DECLARATION, not a statement.  DESIGN
            # section 6's top_level is include, capsule, spacesuit and global,
            # so a line like `satellite.library.n = 0` belongs beside the
            # capsules and NOT inside the main this wrapper synthesises --
            # putting it in the body makes it an assignment to a name nothing
            # declared.
            #
            # `satellite.library.system.` IS EXCLUDED and is the opposite case:
            # those four are the machine's dials, they are RETUNED rather than
            # declared, and an assignment to one is a statement that belongs
            # inside a body.  Lumping them in here broke three examples.
            decls.append(line)
            i += 1
            continue
        if line.startswith("satellite.capsule "):
            depth, started = 0, False
            while i < len(lines):
                decls.append(lines[i])
                depth += lines[i].count("{") - lines[i].count("}")
                if "{" in lines[i]:
                    started = True
                if started and depth <= 0:
                    break
                i += 1
            decls.append("")
            i += 1
            continue
        body.append(line)
        i += 1

    out = list(head)
    if not has_include:
        out.insert(0, "satellite.include(satellite)")
    out.append("")
    out += decls
    if not has_main:
        out.append("satellite.capsule satellite.main()")
        out.append("{")
        for line in body:
            if line.strip():
                out.append("    " + line)
        out.append("}")
    elif any(l.strip() for l in body):
        raise SystemExit("entry declares main AND has loose statements: %r" % text)
    return "\n".join(out)


def run(text, feed=None):
    with tempfile.NamedTemporaryFile("w", suffix=".satl", delete=False,
                                     dir=tempfile.gettempdir()) as f:
        f.write(text + "\n")
        path = f.name
    env = dict(os.environ, SATL_NO_WINDOW="1")
    try:
        if feed is None:
            p = subprocess.run([SATL, path], capture_output=True, text=True,
                               env=env, timeout=20, stdin=subprocess.DEVNULL)
        else:
            p = subprocess.run([SATL, path], capture_output=True, text=True,
                               env=env, timeout=20, input=feed)
        return p.returncode, (p.stdout + p.stderr)
    finally:
        os.unlink(path)

def main():
    entries = load_entries()
    only = sys.argv[1] if len(sys.argv) > 1 else None
    bad, ok, skipped = [], 0, 0
    for num, e in sorted(entries.items(), key=lambda kv: [int(x) for x in kv[0].split()]):
        if only and not e["_from"].startswith("entries_" + only):
            continue
        example = e.get("example", "").strip()
        expect = e.get("expect", "runs")
        if not example or expect == "skip":
            skipped += 1
            continue
        program = as_program(example)
        code, output = run(program, e.get("stdin"))
        good = (code == 0) if expect == "runs" else (code != 0)
        if good:
            ok += 1
        else:
            bad.append((num, expect, code, output.strip().split("\n")[:4], program))
    for num, expect, code, out, program in bad:
        print("=" * 70)
        print("FAIL %s   expected %s, exit %d" % (num, expect, code))
        for line in out:
            print("   | " + line)
        print("   --- program ---")
        for line in program.split("\n"):
            print("   > " + line)
    print("\nran %d, ok %d, FAILED %d, skipped %d" % (ok + len(bad), ok, len(bad), skipped))
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
