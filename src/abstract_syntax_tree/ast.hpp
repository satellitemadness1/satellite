#pragma once

#include "lexical_analyzer/lexer.hpp"
#include "satellite_string/satellite_string.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// The satellite syntax tree.
//
// Expr is deliberately a std::variant of node structs held by
// shared_ptr<const>, mirroring the idiom value.hpp uses for runtime values —
// but it is a SEPARATE variant, and that separation is on purpose.
//
// The original plan was to fold expression kinds into Value itself, so that
// one node type held both code and data. It reads well and it is why lists in
// value.hpp already hold pointers to other nodes. The reason not to do it is
// measurable rather than stylistic: adding the expression kinds grows every
// runtime Value from 40 to 96 bytes, so a list of a million numbers goes from
// 38 MB to 91 MB — and every runtime type check would need arms meaning "this
// can't happen", because there is no satellite.variable.<something> for a call
// node.
//
// None of the idea is lost. Code becomes data again with ONE alternative added
// to Value later — a Quoted{ProgramPtr, const Expr *}, spelled
// satellite.variable.expression — rather than fifteen. Data becomes code with
// one alternative added here. The bridge stays a bridge instead of collapsing
// the two sides into each other.
//
// Nothing here resolves anything. `satellite.time.now()` parses to a plain
// Member/Member/Call chain, exactly like `my_time.some_function()`, and the
// evaluator works out what each segment means at run time. Teaching the parser
// the shape of the standard library would mean a new grammar rule per
// namespace.

// The tree is split across three headers purely for file size; this one is
// the door, and including it gives everything it always gave. Order is
// bottom-up: spans first, then expressions and types, then statements and
// everything built on them.
#include "abstract_syntax_tree/ast_span.hpp"
#include "abstract_syntax_tree/ast_expr.hpp"
#include "abstract_syntax_tree/ast_stmt.hpp"
