// Spacesuit tests: the feature itself, its canonical source, and reference
// semantics — part of the spacesuit_test binary.
//
// These are the cases that would have to pass before any of the others mean
// anything: a class can be declared, an object of it constructed, a method
// called; the tree the parser built for that source unparses back to the source
// it came from; and a second name for an object is the SAME object rather than
// a copy of it.

#include "spacesuit_test.hpp"

void spacesuit_test_feature()
{
    // --- the feature itself -------------------------------------------------
    // Declare a class, declare an object of it, call a method that writes a
    // field, and read the field back out. This is the whole request.
    check_output(with("my_class_name my_object_of_class\n"
                      "satellite.console.display(my_object_of_class.read())\n"
                      "my_object_of_class.my_func(\"another_str\")\n"
                      "satellite.console.display(my_object_of_class.read())\n"),
                 "some_str\nanother_str\n", "declare, construct, call, mutate");

    // A declaration with no initialiser constructs: there is no other
    // construction site in the syntax, so this has to be one.
    check_output(with("my_class_name o\n"
                      "satellite.console.display(o)\n"),
                 "<my_class_name>\n", "an instance prints as its suit name");

    // The same, spelled as an expression, which is what lets a field be filled.
    check_output(with("my_class_name o = my_class_name()\n"
                      "satellite.console.display(o.read())\n"),
                 "some_str\n", "my_class_name() builds an instance");
}

void spacesuit_test_canonical_source()
{
    // --- canonical source ---------------------------------------------------
    check_roundtrip(PAIR, "the design's own example round-trips");

    check_roundtrip("satellite.spacesuit empty()\n"
                    "{\n"
                    "}\n",
                    "a spacesuit with no members round-trips");

    // Consecutive members of one access share one block, which is what makes
    // the canonical form stable.
    check_roundtrip("satellite.spacesuit two()\n"
                    "{\n"
                    "    satellite.public\n"
                    "    {\n"
                    "        satellite.variable.number a = 1\n"
                    "        satellite.variable.number b = 2\n"
                    "    }\n"
                    "}\n",
                    "consecutive members share one access block");

    check_roundtrip("satellite.spacesuit holder()\n"
                    "{\n"
                    "    satellite.protected\n"
                    "    {\n"
                    "        satellite.container.list<satellite.variable.number> l\n"
                    "    }\n"
                    "}\n"
                    "\n"
                    "satellite.capsule take(holder h) satellite.returns(holder)\n"
                    "{\n"
                    "    satellite.return(h)\n"
                    "}\n",
                    "a spacesuit type in a signature round-trips");
}

void spacesuit_test_reference_semantics()
{
    // --- reference semantics ------------------------------------------------
    // Two names, one object. This is the half of the design that value
    // semantics could not have given: a method mutating a field has to be
    // visible through every name that reached the object.
    check_output(with("my_class_name a\n"
                      "my_class_name b = a\n"
                      "a.my_func(\"shared\")\n"
                      "satellite.console.display(b.read())\n"),
                 "shared\n", "two names share one object");

    check_output(with("my_class_name a\n"
                      "my_class_name b = a\n"
                      "my_class_name c\n"
                      "satellite.console.display(a == b)\n"
                      "satellite.console.display(a == c)\n"),
                 "true\nfalse\n", "equality on an instance is identity");

    // Passing an object to a capsule passes the object, not a copy of it.
    check_output(with("satellite.capsule touch(my_class_name o)\n"
                      "{\n"
                      "    o.my_func(\"touched\")\n"
                      "}\n"
                      "my_class_name x\n"
                      "touch(x)\n"
                      "satellite.console.display(x.read())\n"),
                 "touched\n", "an argument is the object, not a copy");
}
