// Spacesuit tests: access — part of the spacesuit_test binary.
//
// What satellite.protected and satellite.public actually keep out: a method
// that does not exist, a protected method called from outside and from inside,
// a subclass reaching its parent's protected method (which is the whole
// difference between protected and private), and a field, which §12 gives no
// spelling for from outside at all.

#include "spacesuit_test.hpp"

void spacesuit_test_access()
{
    // --- access -------------------------------------------------------------
    check_error(with("my_class_name o\n"
                     "satellite.variable.string s = o.hidden()\n"),
                "has no method hidden", "an unknown method is named");

    check_error("satellite.spacesuit shut()\n"
                "{\n"
                "    satellite.protected\n"
                "    {\n"
                "        satellite.capsule secret()\n"
                "        {\n"
                "            satellite.return(satellite)\n"
                "        }\n"
                "    }\n"
                "}\n"
                "shut s\n"
                "s.secret()\n",
                "satellite.protected",
                "a protected method is not callable from outside");

    check_output("satellite.spacesuit shut()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.capsule secret() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(42)\n"
                 "        }\n"
                 "    }\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule open() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(secret())\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "shut s\n"
                 "satellite.console.display(s.open())\n",
                 "42\n", "a protected method is callable from inside");

    // A subclass reaches its superclass's protected method; that is the
    // difference between protected and private, and the reason both exist.
    check_output("satellite.spacesuit up()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.capsule inner() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(5)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "satellite.spacesuit down(up)\n"
                 "{\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule out() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(inner())\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "down d\n"
                 "satellite.console.display(d.out())\n",
                 "5\n", "a subclass reaches a protected method of its parent");

    // A field is reachable only from inside its spacesuit — §12 keeps bare
    // field access out of the language, so there is no spelling for it at all.
    check_error(with("my_class_name o\n"
                     "satellite.console.display(o.my_str)\n"),
                "no bare field access", "a field has no access path from outside");
}
