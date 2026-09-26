// Spacesuit tests: constructors — part of the spacesuit_test binary.
//
// When a constructor runs (at the declaration, after every field initialiser),
// which one runs (the superclass's first, then the subclass's — a subclass does
// not override its parent's), who may run it, and the six ways of writing one
// that are not constructors at all and have to be reported as such.

#include "spacesuit_test.hpp"

#include <string>

void spacesuit_test_constructors()
{
    // --- constructors -------------------------------------------------------
    static const char *NAMED =
        "satellite.spacesuit named()\n"
        "{\n"
        "    satellite.protected\n"
        "    {\n"
        "        satellite.variable.string the_name = \"void\"\n"
        "    }\n"
        "    satellite.public\n"
        "    {\n"
        "        named(satellite.variable.string input_str)\n"
        "        {\n"
        "            the_name = input_str\n"
        "        }\n"
        "        satellite.capsule name() "
        "satellite.returns(satellite.variable.string)\n"
        "        {\n"
        "            satellite.return(the_name)\n"
        "        }\n"
        "    }\n"
        "}\n";

    check_output(std::string(NAMED) +
                     "named a(\"some_data\")\n"
                     "satellite.console.display(a.name())\n",
                 "some_data\n", "a constructor runs at the declaration");

    // The declaration form is sugar for the call form, so both spell the same
    // construction.
    check_output(std::string(NAMED) +
                     "named a = named(\"some_data\")\n"
                     "satellite.console.display(a.name())\n",
                 "some_data\n", "named(...) is the same construction");

    check_roundtrip(NAMED, "a constructor round-trips");

    // Field initialisers all run before any constructor, so a constructor sees
    // every field already at its declared value.
    check_output("satellite.spacesuit seen()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.variable.number a = 2\n"
                 "        satellite.variable.number b = 0\n"
                 "    }\n"
                 "\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        seen()\n"
                 "        {\n"
                 "            b = a * 10\n"
                 "        }\n"
                 "\n"
                 "        satellite.capsule get() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(b)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "seen s\n"
                 "satellite.console.display(s.get())\n",
                 "20\n", "a constructor sees every field initialised");

    // Superclass first, then this suit's own — both run, because a subclass
    // does not override its parent's constructor.
    check_output("satellite.spacesuit base()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.container.list<satellite.variable.string> log\n"
                 "    }\n"
                 "\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        base()\n"
                 "        {\n"
                 "            log.append(\"base\")\n"
                 "        }\n"
                 "\n"
                 "        satellite.capsule trace() "
                 "satellite.returns(satellite.container.list<satellite.variable.string>)\n"
                 "        {\n"
                 "            satellite.return(log)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "satellite.spacesuit derived(base)\n"
                 "{\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        derived()\n"
                 "        {\n"
                 "            log.append(\"derived\")\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "derived d\n"
                 "satellite.console.display(d.trace())\n",
                 "[base, derived]\n",
                 "the superclass constructor runs first, then the subclass's");

    // A protected constructor is an abstract base: only the suit itself and its
    // descendants may build one.
    check_error(std::string("satellite.spacesuit shut()\n"
                            "{\n"
                            "    satellite.protected\n"
                            "    {\n"
                            "        shut()\n"
                            "        {\n"
                            "        }\n"
                            "    }\n"
                            "}\n"
                            "shut s\n"),
                "satellite.protected",
                "a protected constructor cannot be used from outside");

    check_error(std::string(NAMED) + "named a\n",
                "has to be declared with them",
                "a declaration must satisfy the constructor");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.public\n"
                "    {\n"
                "        b()\n"
                "        {\n"
                "        }\n"
                "    }\n"
                "}\n",
                "named after its spacesuit",
                "a constructor must carry its spacesuit's name");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.public\n"
                "    {\n"
                "        a()\n"
                "        {\n"
                "        }\n"
                "        a()\n"
                "        {\n"
                "        }\n"
                "    }\n"
                "}\n",
                "already has a constructor",
                "a spacesuit has at most one constructor");

    check_error("satellite.spacesuit a()\n"
                "{\n"
                "    satellite.public\n"
                "    {\n"
                "        a() satellite.returns(satellite.variable.number)\n"
                "        {\n"
                "        }\n"
                "    }\n"
                "}\n",
                "declares no satellite.returns",
                "a constructor declares no return type");

    // Only the most derived constructor is handed the site's arguments.
    check_error("satellite.spacesuit p()\n"
                "{\n"
                "    satellite.public\n"
                "    {\n"
                "        p(satellite.variable.number n)\n"
                "        {\n"
                "        }\n"
                "    }\n"
                "}\n"
                "satellite.spacesuit q(p)\n"
                "{\n"
                "    satellite.public\n"
                "    {\n"
                "        q()\n"
                "        {\n"
                "        }\n"
                "    }\n"
                "}\n",
                "no syntax yet for passing them",
                "a superclass constructor with arguments is reported");

    check_error("satellite.variable.number x(1)\n",
                "only a spacesuit takes constructor arguments",
                "constructor arguments are for spacesuits only");
}
