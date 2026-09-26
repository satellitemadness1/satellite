// Spacesuit tests: inheritance — part of the spacesuit_test binary.
//
// A subclass reaching its superclass's methods and fields, a superclass-typed
// parameter accepting a subclass instance (and not the other way round), an
// override winning the dispatch, and the two cases that pin down what the
// flattened per-suit method table has to copy: a method with parameters and
// locals, and an inherited constructor.

#include "spacesuit_test.hpp"

void spacesuit_test_inheritance()
{
    // --- inheritance --------------------------------------------------------
    check_output(with("my_class_name o\n"
                      "satellite.console.display(o.greet())\n"),
                 "from the superclass\n", "an inherited method is callable");

    // A subclass instance satisfies a superclass-typed parameter, and the
    // superclass's own instances still do.
    check_output(with("satellite.capsule hello(superclass s) "
                      "satellite.returns(satellite.variable.string)\n"
                      "{\n"
                      "    satellite.return(s.greet())\n"
                      "}\n"
                      "my_class_name derived\n"
                      "superclass base\n"
                      "satellite.console.display(hello(derived))\n"
                      "satellite.console.display(hello(base))\n"),
                 "from the superclass\nfrom the superclass\n",
                 "a subclass matches its superclass's type");

    // ...but not the other way round.
    check_error(with("satellite.capsule need(my_class_name o)\n"
                     "{\n"
                     "    satellite.return(satellite)\n"
                     "}\n"
                     "superclass base\n"
                     "need(base)\n"),
                "cannot pass", "a superclass does not match a subclass's type");

    // Virtual dispatch: the superclass method calls a name the subclass
    // overrode, and the override is what runs. Dispatch is on the OBJECT's
    // suit, which is the whole reason the method table is flattened per suit.
    check_output("satellite.spacesuit base()\n"
                 "{\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule name() "
                 "satellite.returns(satellite.variable.string)\n"
                 "        {\n"
                 "            satellite.return(\"base\")\n"
                 "        }\n"
                 "        satellite.capsule describe() "
                 "satellite.returns(satellite.variable.string)\n"
                 "        {\n"
                 "            satellite.return(\"I am \" + name())\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "satellite.spacesuit derived(base)\n"
                 "{\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule name() "
                 "satellite.returns(satellite.variable.string)\n"
                 "        {\n"
                 "            satellite.return(\"derived\")\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "derived d\n"
                 "satellite.console.display(d.describe())\n",
                 "I am derived\n",
                 "an override is what the inherited method reaches");

    // An inherited field keeps its index in the subclass's layout, which is
    // what makes an inherited method find it at all.
    check_output("satellite.spacesuit parent()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.variable.number n = 7\n"
                 "    }\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule get() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(n)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "satellite.spacesuit child(parent)\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.variable.number m = 9\n"
                 "    }\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule both() "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.return(n + m)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "child c\n"
                 "satellite.console.display(c.get())\n"
                 "satellite.console.display(c.both())\n",
                 "7\n16\n", "a superclass's fields come first in the layout");

    // An inherited method with PARAMETERS and LOCALS. The flattened method
    // table copies a superclass's entry into its subclass, and every layout is
    // built before any body is resolved — so a by-value CapsuleInfo would be
    // copied while still empty and the call would build a frame of no slots.
    // Every earlier inheritance case here happened to use a method with
    // neither, which is exactly why this one is written down.
    check_output("satellite.spacesuit base()\n"
                 "{\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        satellite.capsule scale(satellite.variable.number n) "
                 "satellite.returns(satellite.variable.number)\n"
                 "        {\n"
                 "            satellite.variable.number doubled = n * 2\n"
                 "            satellite.return(doubled + 1)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "satellite.spacesuit derived(base)\n"
                 "{\n"
                 "    satellite.public\n"
                 "    {\n"
                 "    }\n"
                 "}\n"
                 "derived d\n"
                 "satellite.console.display(d.scale(5))\n",
                 "11\n",
                 "an inherited method keeps its parameters and locals");

    // The same hazard through the constructor chain: a subclass with no
    // constructor of its own inherits its superclass's, arguments and all.
    check_output("satellite.spacesuit named()\n"
                 "{\n"
                 "    satellite.protected\n"
                 "    {\n"
                 "        satellite.variable.string label = \"void\"\n"
                 "    }\n"
                 "    satellite.public\n"
                 "    {\n"
                 "        named(satellite.variable.string text)\n"
                 "        {\n"
                 "            label = text\n"
                 "        }\n"
                 "        satellite.capsule name() "
                 "satellite.returns(satellite.variable.string)\n"
                 "        {\n"
                 "            satellite.return(label)\n"
                 "        }\n"
                 "    }\n"
                 "}\n"
                 "satellite.spacesuit tagged(named)\n"
                 "{\n"
                 "    satellite.public\n"
                 "    {\n"
                 "    }\n"
                 "}\n"
                 "tagged t(\"hello\")\n"
                 "satellite.console.display(t.name())\n",
                 "hello\n",
                 "a subclass inherits its superclass's constructor");
}
