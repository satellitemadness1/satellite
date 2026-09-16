#pragma once

// The satellite AST unparser -- Milestone 4 Prototype.
//
// Renders an Arena AST back into canonical satellite source syntax.
// Round-trip fixpoint: unparse(parse(unparse(parse(src)))) == unparse(parse(src)).

#include "ast.hpp"

#include <string>

namespace satellite {

std::string unparse(const Program &program, const AstArena &arena);
std::string unparse_node(NodeIndex idx, const AstArena &arena, int level = 0);
std::string unparse(const Type &type);

} // namespace satellite

