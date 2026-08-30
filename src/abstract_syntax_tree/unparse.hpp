#pragma once

// The tree, printed back as a program -- what `satl --unparse` answers.
//
// THIS IS HOW WE KNOW THE PARSER IS RIGHT BEFORE ANYTHING CAN RUN, which is the
// clause PLAN M4 states the milestone in: `satl --unparse file.satl`
// round-trips. Nothing executes until M10, so a tree is otherwise a thing that
// can only be inspected by whoever wrote the code that built it -- and the
// first satellite's answer to that was a `--dump-ast` printing node names,
// which can look perfect over a tree that has dropped an operand.
//
// WHAT ROUND-TRIP MEANS HERE, EXACTLY, because the obvious reading is wrong and
// would make the test a lie. It does NOT mean the output equals the input: the
// lexer discards comments (DESIGN §5.6), blank lines are not tokens, and a
// program may write `((x))`. It means the output is a FIXPOINT --
//
//     unparse(parse(unparse(parse(s)))) == unparse(parse(s))
//
// -- which is the strongest statement available and a real one: it fails if the
// parser drops a node, if the printer forgets a bracket the precedence needs,
// or if either of them is not the other's inverse on some form. The first pass
// normalises whitespace and brackets; everything after it must be identical,
// character for character.
//
// SEPARATE FROM ast.cpp BECAUSE IT IS THE ONLY PART THAT PRINTS, which is the
// same seam satellite_words/dump.cpp and lexical_analyzer/dump.cpp are cut on.
// It is not called dump.cpp because it is not a dump: those two print what a
// module DECIDED, in a format for a person to read, and this prints a satellite
// program that satl can read back.

#include "abstract_syntax_tree/ast.hpp"

#include <string>

namespace satellite {

// The whole program.
std::string unparse(const Ast &ast);

// One node, with no trailing newline -- for a test that wants to name what it
// is checking, and for M5, which will want to quote an expression in a message.
std::string unparse(const Ast &ast, NodeIndex node);

} // namespace satellite
