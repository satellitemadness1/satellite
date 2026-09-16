#pragma once

// The satellite Closure Compiler -- Milestone 7 Prototype.
//
// DESIGN §2.3 & PLAN §8: Walks flattened Arena AST once using M6 ResolveResult
// side-table metadata to produce executable closure trees.

#include "closure.hpp"
#include "ast.hpp"
#include "resolve_types.hpp"
#include "satellite_words/words.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace satellite {

struct CompiledProgram {
    std::vector<StmtPtr> top_level;
    std::unordered_map<std::string, std::shared_ptr<CapsuleClosure>> capsules;
};

class Compiler {
public:
    Compiler(const Program &program, const AstArena &arena,
             const ResolveResult &resolve, words::Words &words);

    CompiledProgram compile();

private:
    ExprPtr compile_expr(NodeIndex node_idx);
    StmtPtr compile_stmt(NodeIndex node_idx);
    std::shared_ptr<CapsuleClosure> compile_capsule(const CapsuleInfo &info);

    bool flatten_dotted_path(NodeIndex node_idx, std::string &out_path);

    const Program &program_;
    const AstArena &arena_;
    const ResolveResult &resolve_;
};

CompiledProgram compile(const Program &program, const AstArena &arena,
                        const ResolveResult &resolve, words::Words &words);

} // namespace satellite

