#pragma once

// The parser test's shared surface: the check family every section calls, the
// counter they report failures through, the one program text two sections
// share, and one declaration per section so main() can run them in order.
//
// Part of satellite_system/tests/parser_test/, split from a 442-line
// parser_test.cpp. Every .cpp in this folder is part of the binary -- the
// Makefile finds them with a wildcard -- so a new section needs a declaration
// here and a call in main(), and no build edit at all.

#include "abstract_syntax_tree/ast.hpp"
#include "syntax_parser/parser.hpp"

#include <string>
#include <vector>

// Defined in parser_test.cpp. It stopped being `static` for one reason: the
// sections are separate translation units now, and the count has to be one
// number for the whole binary or main() would announce PASS while a section in
// another file had already failed.
extern int failures;

// Hello world in canonical form, defined in parser_test.cpp. Two sections read
// it -- the round trip that opens the run and the idempotence loop that closes
// it -- and a second copy of the text would be free to drift out of step with
// the first, which is exactly how a test stops testing what it says it does.
extern const std::string hello_world_source;

// The check family, defined in parser_test.cpp.
void check(bool ok, const std::string &what);
void round_trip(const std::string &src);
void check_expr(const std::string &src, const std::string &want);
void check_fails(const std::string &src, const std::string &what);

// The sections, in the order main() runs them, which is the order they were
// written in. A section that was a braced block inside main() kept its braces
// when it moved, so nothing in any of these bodies was re-indented and the
// code can be diffed against the old parser_test.cpp line for line.
//
// parser_test_round_trip.cpp
void parser_test_hello_world();
void parser_test_duration_literals();
void parser_test_declarations();
// parser_test_expressions.cpp
void parser_test_method_calls();
void parser_test_indexing_and_slicing();
void parser_test_operators_and_precedence();
// parser_test_statements.cpp
void parser_test_control_flow();
void parser_test_capsules();
void parser_test_same_line_rule();
void parser_test_nesting();
void parser_test_top_level_order();
// parser_test_errors.cpp
void parser_test_error_reporting();
void parser_test_error_rendering();
void parser_test_error_recovery();
// parser_test_round_trip.cpp, last because unparse has to be trusted before
// its output can be reparsed.
void parser_test_unparse_idempotence();
