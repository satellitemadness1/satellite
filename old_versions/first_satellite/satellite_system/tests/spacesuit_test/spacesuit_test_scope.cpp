// Spacesuit tests: scope inside a suit — part of the spacesuit_test binary.
//
// A method body is a scope like any other: a local shadows a field, a name
// declared further down is still reachable, and locals are frame slots so a
// method recurses. The rest of these cases are about a field being STORAGE
// rather than a value — it survives between calls, .append() writes back
// through it, and one of suit type starts nil because default-constructing it
// would make a self-referential suit an infinitely deep object.

#include "spacesuit_test.hpp"

void spacesuit_test_scope()
{
    // --- scope inside a suit ------------------------------------------------
    // A local shadows a field, exactly as an inner block shadows an outer one.
    check_output("satellite.spacesuit shadow()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.variable.number n = 1\n"
                 "    }\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule both() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.variable.number n = 100\n"
                 "            satellite.return(n)\n"
                 "        }\n"
                 "        satellite.capsule field_only() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(n)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "shadow s\n"
                 "satellite.console.display(s.both())\n"
                 "satellite.console.display(s.field_only())\n",
                 "100\n1\n", "a local shadows a field");

    // A method may call one declared further down, the same way a capsule may.
    check_output("satellite.spacesuit order()\n"
                 "{\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule first() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(second())\n"
                 "        }\n"
                 "        satellite.capsule second() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(3)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "order o\n"
                 "satellite.console.display(o.first())\n",
                 "3\n", "a method may call one declared below it");

    // A method's locals are frame slots like any capsule's, so recursion works.
    check_output("satellite.spacesuit math()\n"
                 "{\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule fact(satellite.variable.number n) "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.statement.if(n <= 1)\n"
                 "            {\n"
                 "                satellite.return(1)\n"
                 "            }\n"
                 "            satellite.return(n * fact(n - 1))\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "math m\n"
                 "satellite.console.display(m.fact(10))\n",
                 "3628800\n", "a method recurses on its own frames");

    // A field is mutated through the receiver, and a second call sees the first
    // call's write — the field is storage, not a per-activation local.
    check_output("satellite.spacesuit counter()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.variable.number n = 0\n"
                 "    }\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule bump()\n"
                 "        {\n"
                 "            n = n + 1\n"
                 "        }\n"
                 "        satellite.capsule value() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(n)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "counter c\n"
                 "c.bump()\n"
                 "c.bump()\n"
                 "c.bump()\n"
                 "satellite.console.display(c.value())\n",
                 "3\n", "a field persists across calls");

    // A list field mutates in place through .append(), which needs the field to
    // be reachable as STORAGE rather than as a value.
    check_output("satellite.spacesuit bag()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.container.list<satellite.variable.number> items\n"
                 "    }\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule add(satellite.variable.number n)\n"
                 "        {\n"
                 "            items.append(n)\n"
                 "        }\n"
                 "        satellite.capsule all() "
                 "satellite.returns(satellite.container.list<satellite.variable.number>)\n"
                 "        {\n"
                 "            satellite.return(items)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "bag b\n"
                 "b.add(1)\n"
                 "b.add(2)\n"
                 "satellite.console.display(b.all())\n",
                 "[1, 2]\n", "append writes back through a field");

    // A field of spacesuit type starts nil — default-constructing it would make
    // a self-referential suit an infinitely deep object — and a method fills it.
    check_output("satellite.spacesuit node()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        node next\n"
                 "    }\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule linked() "
                 "satellite.returns(satellite.variable.bool)\n"
                 "        {\n"
                 "            satellite.return(next != satellite)\n"
                 "        }\n"
                 "        satellite.capsule link()\n"
                 "        {\n"
                 "            next = node()\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "node n\n"
                 "satellite.console.display(n.linked())\n"
                 "n.link()\n"
                 "satellite.console.display(n.linked())\n",
                 "false\ntrue\n", "a field of suit type starts nil and is fillable");
}
