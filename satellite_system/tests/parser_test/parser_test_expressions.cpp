// Expressions: method calls and chaining, indexing and slicing, operators and
// precedence -- everything that has to come back out of unparse spelled the
// way it went in, including the parentheses that are NOT in the tree.
//
// Part of satellite_system/tests/parser_test/, split from a 442-line
// parser_test.cpp.

#include "parser_test.hpp"

using namespace satellite;

void parser_test_method_calls()
{
    // ---- method calls and chaining ---------------------------------------
    round_trip("my_time.some_function()\n");
    round_trip("my_file.some_function()\n");
    round_trip("satellite.time.now().some_function()\n");
    round_trip("my_list.append(1)\n");
    round_trip("my_list.append(other.value(), 2)\n");
    round_trip("a.b().c().d\n");

    // A language call and a user method call parse to the same shape; nothing
    // in the tree records which is which.
    check_expr("satellite.time.now()", "satellite.time.now()");
    check_expr("my_time.some_function()", "my_time.some_function()");
}

void parser_test_indexing_and_slicing()
{
    // ---- indexing and slicing --------------------------------------------
    round_trip("list_name[3]\n");
    round_trip("list_name[some_number]\n");
    round_trip("l[2:5]\n");
    round_trip("l[:5]\n");
    round_trip("l[2:]\n");
    round_trip("l[:]\n");
    round_trip("grid[0][1]\n");
    round_trip("l[0].f()[1:2]\n");
    round_trip("satellite.library.main.x = 5\n");
}

void parser_test_operators_and_precedence()
{
    // ---- operators and precedence ----------------------------------------
    check_expr("a + b * c", "a + b * c");
    check_expr("a * b + c", "a * b + c");
    check_expr("(a + b) * c", "(a + b) * c");
    check_expr("a + b + c", "a + b + c");
    check_expr("a + (b + c)", "a + (b + c)");
    check_expr("a == b + c", "a == b + c");
    check_expr("-x", "-x");
    check_expr("-x + y", "-x + y");
    check_expr("-(x + y)", "-(x + y)");
    check_expr("a <= b", "a <= b");
    check_expr("a >= b", "a >= b");
    check_expr("a != b", "a != b");
    // Redundant parentheses are not in the tree, so they are dropped.
    check_expr("((a))", "a");

    // '<' inside a type is a generic opener; '<' in an expression is
    // less-than. The reservation rule is what keeps these apart.
    check_expr("a < b", "a < b");
    round_trip("satellite.container.list<satellite.variable.number> ns\n");
}
