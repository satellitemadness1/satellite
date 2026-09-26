// AST tests. There is no parser yet, so trees are built by hand and checked
// through unparse — which is exactly the check the parser will need later,
// when unparse(parse(src)) == src becomes the round-trip test.
//
// This file is the harness the rest of the binary shares: the failure counter,
// the two check helpers that raise it, the builders that keep a hand-built
// tree readable, and a main() that runs the sections in the order they were
// written. The checks themselves live one subject to a file, in the
// ast_test_*.cpp beside this one; ast_test.hpp lists them all.

#include "ast_test.hpp"

#include "satellite_value/value.hpp"   // the PASS line prints sizeof(Value) beside the tree's

#include <cstdio>
#include <string>

// One counter for the whole binary. It was static while there was one file;
// now every section file reports into this definition, which is why the
// header declares it extern rather than each file keeping its own.
int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

void check_text(const std::string &got, const std::string &want,
                const std::string &what)
{
    if (got != want) {
        printf("FAIL: %s\n  want: %s\n   got: %s\n", what.c_str(),
               want.c_str(), got.c_str());
        failures++;
    }
}

// --- small builders, so the trees below stay readable ----------------------

ExprPtr num(long long v, const std::string &text)
{
    return make_expr(NumberLit{v, text}, {});
}
ExprPtr num(int v) { return num(v, std::to_string(v)); }

ExprPtr str(const std::string &s)
{
    return make_expr(StringLit{encode(s), s}, {});
}
ExprPtr sat() { return make_expr(SatelliteLit{}, {}); }
ExprPtr name(const std::string &n) { return make_expr(Name{n}, {}); }

ExprPtr member(ExprPtr target, const std::string &n)
{
    return make_expr(Member{std::move(target), n}, {});
}
ExprPtr call(ExprPtr target, std::vector<ExprPtr> args)
{
    return make_expr(Call{std::move(target), std::move(args)}, {});
}
ExprPtr binary(const std::string &op, ExprPtr l, ExprPtr r)
{
    return make_expr(Binary{op, std::move(l), std::move(r)}, {});
}

// satellite.time.now  ->  Member(Member(SatelliteLit, "time"), "now")
ExprPtr path(std::vector<std::string> segments)
{
    ExprPtr node = sat();
    for (const std::string &s : segments)
        node = member(node, s);
    return node;
}

Type var_type(const std::string &n) { return Type{"variable", n, {}, {}}; }

int main()
{
    ast_test_types();
    ast_test_declarations();
    ast_test_receiver_calls();
    ast_test_indexing_and_slicing();
    ast_test_operators();
    ast_test_statements();
    ast_test_blocks();
    ast_test_capsules();
    ast_test_whole_program();
    ast_test_control_flow();
    ast_test_control_flow_keywords();
    ast_test_spans();
    ast_test_source_map();
    ast_test_expr_and_value_stay_separate();

    if (failures) {
        printf("%d ast check(s) failed\n", failures);
        return 1;
    }
    // Span joins the printed budgets because §16 just spent its spare bits on
    // a file id: the claim that this cost nothing is worth showing, not just
    // asserting where nobody reads it.
    printf("PASS: ast (types, generics, chains, slices, capsules, "
           "precedence-aware unparse, spans with a file id; "
           "Value=%zu Expr=%zu Stmt=%zu Span=%zu bytes)\n",
           sizeof(Value), sizeof(Expr), sizeof(Stmt), sizeof(Span));
    return 0;
}
