// Parser tests. The main check is the round trip: for canonically formatted
// source, unparse(parse(src)) must give back exactly src. That single property
// exercises the whole front end at once, because a parse that drops, reorders
// or misgroups anything shows up as a text difference.
//
// This file holds what every section needs: the check family, the shared
// program text, the failure count, and a main() that is nothing but the
// section list in order. The sections themselves live in the
// parser_test_*.cpp files next to it.

#include "parser_test.hpp"

#include <cstdio>
#include <string>

using namespace satellite;

int failures = 0;

// Hello world, canonical. Kept here rather than inside a section because two
// sections use it; see the note in parser_test.hpp.
const std::string hello_world_source =
    "satellite.include(satellite)\n"
    "\n"
    "satellite.capsule satellite.main("
    "satellite.container.list<satellite.variable.string> argz)\n"
    "{\n"
    "    satellite.console.display(\"hello, world!\")\n"
    "    satellite.return(satellite)\n"
    "}\n";

// The check family. None of these is `static` any more, because the callers
// moved out of this file; they are the one place a failure gets counted.
void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

// The core property. `src` must already be in canonical form.
void round_trip(const std::string &src)
{
    ParseResult r = parse(src);
    if (!r.ok()) {
        printf("FAIL: did not parse: %s\n", src.c_str());
        for (const ParseError &e : r.errors)
            printf("%s\n", format_error(e, SourceMap(src)).c_str());
        failures++;
        return;
    }
    const std::string got = unparse(r.program);
    if (got != src) {
        printf("FAIL: round trip\n  want: %s\n   got: %s\n", src.c_str(),
               got.c_str());
        failures++;
    }
}

// Parse a lone expression by wrapping it in a statement position.
static ExprPtr expr_of(const std::string &src)
{
    ParseResult r = parse(src);
    if (!r.ok() || r.program.items.size() != 1)
        return nullptr;
    const StmtPtr *s = std::get_if<StmtPtr>(&r.program.items[0]);
    if (!s)
        return nullptr;
    const ExprStmt *e = std::get_if<ExprStmt>(s->get());
    return e ? e->expr : nullptr;
}

void check_expr(const std::string &src, const std::string &want)
{
    ExprPtr e = expr_of(src);
    if (!e) {
        printf("FAIL: did not parse as an expression: %s\n", src.c_str());
        failures++;
        return;
    }
    if (unparse(*e) != want) {
        printf("FAIL: %s\n  want: %s\n   got: %s\n", src.c_str(), want.c_str(),
               unparse(*e).c_str());
        failures++;
    }
}

void check_fails(const std::string &src, const std::string &what)
{
    ParseResult r = parse(src);
    if (r.ok()) {
        printf("FAIL: expected a parse error (%s): %s\n", what.c_str(),
               src.c_str());
        failures++;
    }
}

int main()
{
    // The same order the sections ran in when they were blocks in one main(),
    // and the order is worth keeping: the run reads from hello world out to
    // the error paths, and the idempotence section belongs last because it
    // trusts unparse, which everything above it is busy proving.
    parser_test_hello_world();
    parser_test_duration_literals();
    parser_test_declarations();
    parser_test_method_calls();
    parser_test_indexing_and_slicing();
    parser_test_operators_and_precedence();
    parser_test_control_flow();
    parser_test_capsules();
    parser_test_same_line_rule();
    parser_test_nesting();
    parser_test_top_level_order();
    parser_test_error_reporting();
    parser_test_error_rendering();
    parser_test_error_recovery();
    parser_test_unparse_idempotence();

    if (failures) {
        printf("%d parser check(s) failed\n", failures);
        return 1;
    }
    printf("PASS: parser (round trip, segment-1 dispatch, generics vs "
           "less-than, same-line postfix, error recovery, errors rendered "
           "against their own spaceship)\n");
    return 0;
}
