// Spacesuit tests: errors caught before anything runs, and nil — part of the
// spacesuit_test binary.
//
// Everything a spacesuit declaration can get wrong that the resolver is
// supposed to catch with no program having run yet: unknown names, duplicate
// and colliding ones, inheritance cycles, type errors in a field initialiser or
// assignment, the reserved word at each new binding site, and the §5 collision
// that keeps a dotted path followed by a Word from being a declaration. The nil
// case at the end is the one error of this shape that can only be found while
// the program is running, which is why it is written down separately.

#include "spacesuit_test.hpp"

void spacesuit_test_static_errors()
{
    // --- errors caught before anything runs ---------------------------------
    check_error("no_such_class x\n", "no such spacesuit: no_such_class",
                "an unknown spacesuit type is a resolve error");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.public\n"
                "    {\n"
                "        satellite.capsule m()\n"
                "        {\n"
                "            typo = 1\n"
                "        }\n"
                "    }\n"
                "}\n",
                "unknown variable in spacesuit a: typo",
                "an unknown name in a method is a resolve error");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        satellite.variable.number n = 1\n"
                "        satellite.variable.number n = 2\n"
                "    }\n"
                "}\n",
                "already declared", "a field declared twice is an error");

    check_error("satellite.spacesuit p()\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        satellite.variable.number n = 1\n"
                "    }\n"
                "}\n"
                "satellite.spacesuit q(p)\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        satellite.variable.number n = 2\n"
                "    }\n"
                "}\n",
                "already declared in p",
                "a field that shadows a superclass field is an error");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.public\n"
                "    {\n"
                "        satellite.capsule m()\n"
                "        {\n"
                "        }\n"
                "        satellite.capsule m()\n"
                "        {\n"
                "        }\n"
                "    }\n"
                "}\n",
                "already defined", "a method defined twice is an error");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        satellite.variable.number n = 1\n"
                "    }\n"
                "    satellite.public\n"
                "    {\n"
                "        satellite.capsule n()\n"
                "        {\n"
                "        }\n"
                "    }\n"
                "}\n",
                "cannot share a name", "a field and a method cannot collide");

    check_error("satellite.spacesuit a(a)\n"
                "{\n"
                "}\n",
                "inherits from itself", "direct self-inheritance is an error");

    check_error("satellite.spacesuit a(b)\n"
                "{\n"
                "}\n"
                "satellite.spacesuit b(a)\n"
                "{\n"
                "}\n",
                "inherits from itself", "an inheritance cycle is an error");

    check_error("satellite.spacesuit a(nowhere)\n"
                "{\n"
                "}\n",
                "no such spacesuit to inherit from: nowhere",
                "an unknown superclass is an error");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "}\n"
                "satellite.spacesuit a()\n"
                "{\n"
                "}\n",
                "already defined", "a spacesuit defined twice is an error");

    check_error("satellite.capsule dup()\n"
                "{\n"
                "}\n"
                "satellite.spacesuit dup()\n"
                "{\n"
                "}\n",
                "cannot share a name",
                "a spacesuit and a capsule cannot share a name");

    check_error(with("my_class_name o\n"
                     "o.my_func(1)\n"),
                "cannot pass", "a method checks its parameter types");

    check_error(with("my_class_name o\n"
                     "o.my_func()\n"),
                "argument", "a method checks its arity");

    // A spacesuit declares no constructor, so construction takes no arguments.
    check_error(with("my_class_name o = my_class_name(1)\n"),
                "takes 0 arguments, got 1",
                "construction takes no arguments");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        satellite.variable.number n = \"text\"\n"
                "    }\n"
                "}\n"
                "a x\n",
                "cannot initialise", "a field initialiser is type-checked");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        satellite.variable.number n = 1\n"
                "    }\n"
                "    satellite.public\n"
                "    {\n"
                "        satellite.capsule m()\n"
                "        {\n"
                "            n = \"text\"\n"
                "        }\n"
                "    }\n"
                "}\n"
                "a x\n"
                "x.m()\n",
                "cannot assign", "a field assignment is type-checked");

    // A field initialiser cannot read the field it is initialising, the same
    // rule a local declaration already follows.
    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        satellite.variable.number n = n\n"
                "    }\n"
                "}\n",
                "unknown variable in spacesuit a: n",
                "a field initialiser cannot read itself");

    // A member outside an access block has no canonical spelling, so it is not
    // grammar at all.
    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.variable.number n = 1\n"
                "}\n",
                "satellite.protected or satellite.public",
                "every member belongs to an access block");

    // A spacesuit is not a value, and neither is a method.
    check_error(with("satellite.console.display(my_class_name)\n"),
                "cannot be used as a value", "a spacesuit is not a value");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.public\n"
                "    {\n"
                "        satellite.capsule m()\n"
                "        {\n"
                "            satellite.console.display(m)\n"
                "        }\n"
                "    }\n"
                "}\n"
                "a x\n"
                "x.m()\n",
                "cannot be used as a value", "a method is not a value");

    // The one reserved word still is one, at every new binding site.
    check_error("satellite.spacesuit satellite()\n"
                "{\n"
                "}\n",
                "reserved", "a spacesuit cannot be called satellite");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        satellite.variable.number satellite = 1\n"
                "    }\n"
                "}\n",
                "reserved", "a field cannot be called satellite");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.public\n"
                "    {\n"
                "        satellite.capsule satellite.main()\n"
                "        {\n"
                "        }\n"
                "    }\n"
                "}\n",
                "named bare", "a method cannot take a reserved name");

    // §5's collision must stay fixed: a dotted path followed by a Word is not a
    // declaration, however much it looks like one.
    check_error("satellite.nonsense.thing x = 1\n", "after the end of a statement",
                "a dotted path followed by a Word is still not a declaration");
}

void spacesuit_test_nil()
{
    // --- nil is not an object -----------------------------------------------
    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        a other\n"
                "    }\n"
                "    satellite.public\n"
                "    {\n"
                "        satellite.capsule reach()\n"
                "        {\n"
                "            other.reach()\n"
                "        }\n"
                "    }\n"
                "}\n"
                "a x\n"
                "x.reach()\n",
                "nil has no methods", "a nil field has no methods");
}
