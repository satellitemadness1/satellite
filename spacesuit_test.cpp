// Spacesuit tests: classes — declaration, construction, fields, methods,
// inheritance and access.
//
// Everything here drives the real pipeline through interp.hpp, so a break
// anywhere between the lexer and satellite.library shows up as a failure here,
// and none of it links gtk4 or vte. The round-trip cases go through parse and
// unparse directly, because canonical source is a property of the tree rather
// than of anything the evaluator does.

#include "interp.hpp"
#include "library.hpp"
#include "parser.hpp"
#include "satellite_string.hpp"
#include "value.hpp"

#include <cstdio>
#include <string>

using namespace satellite;

static int failures = 0;

// Each case runs in its own satellite.library namespace, because the Library is
// a process-wide singleton and variables written by one case would otherwise be
// visible to the next.
static int ns_counter = 0;

static std::string fresh_ns()
{
    return "s" + std::to_string(++ns_counter);
}

static void check_output(const std::string &source, const std::string &want,
                         const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.output != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what.c_str(),
               want.c_str(), result.output.c_str());
        failures++;
    }
}

static void check_error(const std::string &source, const std::string &fragment,
                        const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.ok || result.output.find(fragment) == std::string::npos) {
        printf("FAIL: %s\n  want error containing: %s\n  got: %s\n",
               what.c_str(), fragment.c_str(), result.output.c_str());
        failures++;
    }
}

// unparse(parse(src)) == src, which is the parser's contract for every form in
// the grammar.
static void check_roundtrip(const std::string &source, const std::string &what)
{
    ParseResult parsed = parse(source);
    if (!parsed.ok()) {
        printf("FAIL: %s\n  did not parse: %s\n", what.c_str(),
               parsed.errors.front().message.c_str());
        failures++;
        return;
    }
    const std::string back = unparse(parsed.program);
    if (back != source) {
        printf("FAIL: %s\n  want:\n%s\n  got:\n%s\n", what.c_str(),
               source.c_str(), back.c_str());
        failures++;
    }
}

// The design's own example, with a superclass to inherit from and a typed
// parameter, used by most of the cases below.
static const char *PAIR =
    "satellite.spacesuit superclass()\n"
    "{\n"
    "    satellite.public\n"
    "    {\n"
    "        satellite.capsule greet() satellite.returns(satellite.variable.string)\n"
    "        {\n"
    "            satellite.return(\"from the superclass\")\n"
    "        }\n"
    "    }\n"
    "}\n"
    "\n"
    "satellite.spacesuit my_class_name(superclass)\n"
    "{\n"
    "    satellite.protected\n"
    "    {\n"
    "        satellite.variable.string my_str = \"some_str\"\n"
    "    }\n"
    "    satellite.public\n"
    "    {\n"
    "        satellite.capsule my_func(satellite.variable.string s)\n"
    "        {\n"
    "            my_str = s\n"
    "        }\n"
    "        satellite.capsule read() satellite.returns(satellite.variable.string)\n"
    "        {\n"
    "            satellite.return(my_str)\n"
    "        }\n"
    "    }\n"
    "}\n";

static std::string with(const std::string &tail)
{
    return std::string(PAIR) + tail;
}

int main()
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

    // --- satellite.variable.time --------------------------------------------
    // An instant is read off the clock, never written down, and the difference
    // of two is a plain number of nanoseconds (§8.2).
    check_output("satellite.variable.time t = satellite.time.now()\n"
                 "satellite.console.display(t.minus(t))\n",
                 "0\n", "an instant minus itself is zero nanoseconds");

    check_output("satellite.variable.time epoch\n"
                 "satellite.console.display(epoch)\n",
                 "1970-01-01T00:00:00.000000000Z\n",
                 "an uninitialised time is the epoch");

    check_output("satellite.variable.time epoch\n"
                 "satellite.console.display(epoch.nanoseconds())\n",
                 "0\n", "nanoseconds() is the count since the epoch");

    check_output("satellite.variable.time a = satellite.time.now()\n"
                 "satellite.variable.time b = satellite.time.now()\n"
                 "satellite.console.display(b.minus(a) >= 0)\n",
                 "true\n", "the clock does not run backwards within a program");

    check_error("satellite.variable.time t = 1\n", "cannot initialise",
                "a time is not a number");

    check_error("satellite.variable.time t = satellite.time.now()\n"
                "satellite.console.display(t.minus(1))\n",
                "wants a satellite.variable.time",
                "minus takes another instant");

    check_error("satellite.time.now(1)\n", "takes 0 arguments",
                "satellite.time.now takes no arguments");

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

    if (failures == 0)
        printf("spacesuit_test: PASS\n");
    else
        printf("spacesuit_test: FAIL (%d)\n", failures);
    return failures ? 1 : 0;
}
