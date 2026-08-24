#pragma once

// The shared harness for the ast_test binary.
//
// The AST checks began as one 413-line main(). They are now one file per
// subject — ast_test_types.cpp, ast_test_expressions.cpp and the rest — and
// everything those files hold in common is declared here: the failure counter
// they all report into, the two check helpers that raise it, the small
// builders that keep a hand-built tree readable, and one declaration per
// section so main() can run them in the order they were written.
//
// Each of those lives in exactly one translation unit. The counter in
// particular was `static int failures` while there was a single file; it stops
// being static rather than becoming one counter per file, because a check that
// failed in ast_test_spans.cpp has to be visible to the main() that decides
// the exit status.
//
// The using-directive belongs in a header only because this header is
// test-local: nothing outside satellite_system/tests/ast_test/ includes it, so
// the moved test bodies can go on spelling Type, ExprPtr and make_expr exactly
// the way they did when they all shared one main().

#include "abstract_syntax_tree/ast.hpp"

#include <string>
#include <vector>

using namespace satellite;

// --- the harness -----------------------------------------------------------

extern int failures;

void check(bool ok, const std::string &what);
void check_text(const std::string &got, const std::string &want,
                const std::string &what);

// --- small builders, so the trees below stay readable ----------------------

ExprPtr num(long long v, const std::string &text);
ExprPtr num(int v);
ExprPtr str(const std::string &s);
ExprPtr sat();
ExprPtr name(const std::string &n);
ExprPtr member(ExprPtr target, const std::string &n);
ExprPtr call(ExprPtr target, std::vector<ExprPtr> args = {});
ExprPtr binary(const std::string &op, ExprPtr l, ExprPtr r);
// satellite.time.now  ->  Member(Member(SatelliteLit, "time"), "now")
ExprPtr path(std::vector<std::string> segments);
Type var_type(const std::string &n);

// --- the sections, in the order main() runs them ---------------------------

void ast_test_types();                          // ast_test_types.cpp
void ast_test_declarations();                   // ast_test_types.cpp
void ast_test_receiver_calls();                 // ast_test_expressions.cpp
void ast_test_indexing_and_slicing();           // ast_test_expressions.cpp
void ast_test_operators();                      // ast_test_expressions.cpp
void ast_test_statements();                     // ast_test_statements.cpp
void ast_test_blocks();                         // ast_test_statements.cpp
void ast_test_capsules();                       // ast_test_capsules.cpp
void ast_test_whole_program();                  // ast_test_capsules.cpp
void ast_test_control_flow();                   // ast_test_control_flow.cpp
void ast_test_control_flow_keywords();          // ast_test_control_flow.cpp
void ast_test_spans();                          // ast_test_spans.cpp
void ast_test_source_map();                     // ast_test_spans.cpp
void ast_test_expr_and_value_stay_separate();   // ast_test_expr_and_value.cpp
