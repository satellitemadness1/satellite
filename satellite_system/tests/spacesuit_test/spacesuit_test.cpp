// Spacesuit tests: classes — declaration, construction, fields, methods,
// inheritance and access.
//
// Everything here drives the real pipeline through interp.hpp, so a break
// anywhere between the lexer and satellite.library shows up as a failure here,
// and none of it links gtk4 or vte. The round-trip cases go through parse and
// unparse directly, because canonical source is a property of the tree rather
// than of anything the evaluator does.
//
// This file is the driver: the check family every case reports through, the
// two pieces of source they share, and a main() that calls the sections in
// order. The cases themselves are in spacesuit_test_<topic>.cpp beside it, one
// file per topic, all declared in spacesuit_test.hpp.

#include "spacesuit_test.hpp"

#include "interpreter/interp.hpp"
#include "satellite_library/library.hpp"
#include "syntax_parser/parser.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_value/value.hpp"

#include <cstdio>
#include <string>

using namespace satellite;

// Counted here and read by main(); declared extern in spacesuit_test.hpp so
// that every section file adds to this one counter.
int failures = 0;

// Each case runs in its own satellite.library namespace, because the Library is
// a process-wide singleton and variables written by one case would otherwise be
// visible to the next.
static int ns_counter = 0;

static std::string fresh_ns()
{
    return "s" + std::to_string(++ns_counter);
}

void check_output(const std::string &source, const std::string &want,
                  const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.output != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what.c_str(),
               want.c_str(), result.output.c_str());
        failures++;
    }
}

void check_error(const std::string &source, const std::string &fragment,
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
void check_roundtrip(const std::string &source, const std::string &what)
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
// parameter, used by most of the cases in the section files.
const char *PAIR =
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

std::string with(const std::string &tail)
{
    return std::string(PAIR) + tail;
}

// The sections run in the order they were written. A later case takes the
// earlier ones for granted — the inheritance cases assume a method can be
// called at all, the error cases assume a spacesuit that is spelled correctly
// works — so this order is what makes reading the failures top to bottom narrow
// a break down to its first cause instead of its last symptom.
int main()
{
    spacesuit_test_feature();
    spacesuit_test_canonical_source();
    spacesuit_test_reference_semantics();
    spacesuit_test_inheritance();
    spacesuit_test_access();
    spacesuit_test_scope();
    spacesuit_test_constructors();
    spacesuit_test_time();
    spacesuit_test_static_errors();
    spacesuit_test_nil();

    if (failures == 0)
        printf("spacesuit_test: PASS\n");
    else
        printf("spacesuit_test: FAIL (%d)\n", failures);
    return failures ? 1 : 0;
}
