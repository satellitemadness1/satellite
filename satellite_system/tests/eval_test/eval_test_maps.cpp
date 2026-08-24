// satellite.container.map (§8.6): insertion order, key canonicalisation,
// equality by content, and the container type checks the resolver makes before
// any of it runs. Part of the eval_test binary; the harness these call is
// declared in eval_test.hpp.

#include "eval_test.hpp"

#include <string>

void eval_test_maps()
{
    // --- satellite.container.map (§8.6) -------------------------------------
    // A declaration is an empty map, not nil, so it can be .set() into
    // immediately — the same reason a list starts empty.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m\n"
                 "m.length()\n",
                 "{}\n0\n", "a declared map is empty, not nil");

    // INSERTION ORDER, and it is not cosmetic: .keys() is how a map is walked,
    // and check_output is exact string equality, so a map that rendered in hash
    // order would make this file intermittently red.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m.set(\"bolt\", 40)\n"
                 "m.set(\"washer\", 100)\n"
                 "m.set(\"nut\", 7)\n"
                 "m\n"
                 "m.keys()\n"
                 "m.values()\n"
                 "m.length()\n",
                 // The MAP still echoes as one value -- only a list echoes as a
                 // listing -- so the map keeps its braces and .keys()/.values()
                 // are lists and do not.
                 "{bolt: 40, washer: 100, nut: 7}\n"
                 "bolt\nwasher\nnut\n"
                 "40\n100\n7\n"
                 "3\n",
                 "map keeps insertion order");

    // Updating an existing key KEEPS ITS POSITION. A symbol table that
    // reordered itself whenever a binding was refined would make .keys()
    // useless for reporting.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m.set(\"a\", 1)\n"
                 "m.set(\"b\", 2)\n"
                 "m.set(\"a\", 9)\n"
                 "m\n"
                 "m.length()\n",
                 "{a: 9, b: 2}\n2\n", "set on an existing key keeps position");

    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m.set(\"a\", 1)\n"
                 "m.set(\"b\", 2)\n"
                 "m.get(\"b\")\n"
                 "m[\"a\"]\n"
                 "m.has(\"a\")\n"
                 "m.has(\"zz\")\n"
                 "m.remove(\"a\")\n"
                 "m\n",
                 "2\n1\ntrue\nfalse\n{b: 2}\n", "get, subscript, has, remove");

    // A NUMBER key canonicalises through to_string(), which is exact: 1 and 1.0
    // are one number (Number::operator== is compare()==0) and must therefore be
    // one key. The second set updates rather than inserts.
    check_output("satellite.container.map<satellite.variable.number, "
                 "satellite.variable.string> m\n"
                 "m.set(1, \"one\")\n"
                 "m.set(2, \"two\")\n"
                 "m.set(1.0, \"ONE\")\n"
                 "m\n"
                 "m.length()\n"
                 "m[1]\n",
                 "{1: ONE, 2: two}\n2\nONE\n", "1 and 1.0 are one key");

    // The type tag in the canonical form is what stops these colliding.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> s\n"
                 "s.set(\"12\", 1)\n"
                 "s.has(\"12\")\n",
                 "true\n", "a string key and a number key do not collide");

    // EQUALITY IS BY CONTENT, and this is the arm that matters most. A MapRef
    // is a handle to something with value semantics, exactly like the string
    // handle whose missing arm shipped, passed all eleven test binaries, and
    // was caught only by the bootstrap lexer. Two maps built independently,
    // never sharing a pointer.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> a\n"
                 "satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> b\n"
                 "a.set(\"x\", 1)\n"
                 "a.set(\"y\", 2)\n"
                 "b.set(\"x\", 1)\n"
                 "b.set(\"y\", 2)\n"
                 "a == b\n",
                 "true\n", "two independently built maps compare equal");

    // Order-INSENSITIVE. Order is how a map prints and is walked; it is not
    // part of what a map IS. Two symbol tables that disagree only about which
    // name was seen first hold the same symbols.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> a\n"
                 "satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> b\n"
                 "a.set(\"x\", 1)\n"
                 "a.set(\"y\", 2)\n"
                 "b.set(\"y\", 2)\n"
                 "b.set(\"x\", 1)\n"
                 "a == b\n"
                 "a\n"
                 "b\n",
                 "true\n{x: 1, y: 2}\n{y: 2, x: 1}\n",
                 "map equality ignores order, printing does not");

    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> a\n"
                 "satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> b\n"
                 "a.set(\"x\", 1)\n"
                 "b.set(\"x\", 2)\n"
                 "a == b\n",
                 "false\n", "maps differing in a value are not equal");

    // Walking a map with the only loop the language has (§5).
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m.set(\"a\", 10)\n"
                 "m.set(\"b\", 20)\n"
                 "satellite.variable.number total = 0\n"
                 "satellite.container.list<satellite.variable.string> k = m.keys()\n"
                 "satellite.statement.for (satellite.variable.number i = 0; "
                 "i < k.length(); i = i + 1) { total = total + m[k[i]] }\n"
                 "total\n",
                 "30\n", "a map is walked through .keys()");

    // Nesting, which is what makes it usable as a symbol table.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.container.list<satellite.variable.number>> m\n"
                 "satellite.container.list<satellite.variable.number> l\n"
                 "l.append(1)\n"
                 "l.append(2)\n"
                 "m.set(\"nums\", l)\n"
                 "m\n"
                 "m[\"nums\"][1]\n",
                 "{nums: [1, 2]}\n2\n", "a map of lists nests");

    // A missing key is an ERROR, not nil — §7 already settled the sibling case
    // for an out-of-range index. nil cannot mean absent because nil is a
    // legitimate stored value.
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.get(\"nope\")\n",
                "no such key in the map", "get on a missing key is an error");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm[\"nope\"]\n",
                "no such key in the map", "subscript on a missing key errors");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.remove(\"nope\")\n",
                "no such key in the map", "remove of a missing key errors");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.set(satellite.bool.true, 1)\n",
                "as a key", "a bool cannot be a key");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.set(\"a\", \"not a number\")\n",
                "cannot store", "the value type is checked at insertion");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm[0:1]\n",
                "cannot be sliced", "a map cannot be sliced");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm[\"a\"] = 1\n",
                "cannot assign to this expression",
                "m[k] = v is not in the language, exactly as l[i] = v is not");

    // The resolver rejects a malformed container type before anything runs.
    // This is what makes args[1] safe to read at every runtime site.
    check_error("satellite.container.map<satellite.variable.string> m\n",
                "takes two type arguments", "a map needs two type arguments");
    check_error("satellite.container.list<satellite.variable.string, "
                "satellite.variable.number> l\n",
                "takes one type argument", "a list takes exactly one");
    check_error("satellite.container.zzz<satellite.variable.string> z\n",
                "no such container type", "an unknown container is rejected");
    check_error("satellite.container.map<satellite.variable.bool, "
                "satellite.variable.number> m\n",
                "a map key must be", "a bool key is rejected at resolve time");

    // A selector belongs to ONE container, and which container the receiver is
    // is a question only the receiver can answer.
    //
    // Deriving it from the method name instead was a heap-buffer-overflow read,
    // found by AddressSanitizer and not by this suite: `.set` took the map path
    // whatever the receiver was, so a list's one-element generic argument
    // vector was indexed at [1]. At -O2 there was no crash — just a diagnostic
    // about storing a value in a list, produced by matching against a Type
    // whose std::strings were built from unallocated heap bytes.
    check_error("satellite.container.list<satellite.variable.number> l\n"
                "l.append(1)\n"
                "l.set(1, 2)\n",
                "satellite.container.list has no method set",
                "set on a list is refused, and does not read past its args");
    check_error("satellite.container.list<satellite.variable.string> l\n"
                "l.set(\"a\", \"b\")\n",
                "satellite.container.list has no method set",
                "set on a list of strings is refused too");
    check_error("satellite.container.list<satellite.variable.number> l\n"
                "l.remove(1)\n",
                "satellite.container.list has no method remove",
                "remove on a list is refused, with the list's own wording");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.append(1)\n",
                "satellite.container.map has no method append",
                "append on a map is refused");
    // The bare, un-parameterised forms must survive the same routing.
    check_error("satellite.container.list l\nl.set(1, 2)\n",
                "has no method set", "set on a bare list is refused");

    // Only a container is generic. Pre-existing laxness — the parser validates
    // a type's SPACE and not its name, so this resolved clean before the
    // container arity check existed.
    check_error("satellite.variable.number<satellite.variable.string> x\n",
                "is not generic", "a variable type takes no type arguments");
    check_error("satellite.variable.string<satellite.variable.number, "
                "satellite.variable.bool> s\n",
                "is not generic", "and not two of them either");

    check_output("satellite.container.list<satellite.variable.number> l\n"
                 "l.append(2)\n"
                 "l.contains(2)\n"
                 "l.first()\n"
                 "l.last()\n",
                 "true\n2\n2\n", "contains, first, last");
}
