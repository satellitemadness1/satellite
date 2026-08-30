#pragma once

// The satellite Closure Compiler -- Milestone 9 Prototype.
//
// Translates parsed AST and resolved symbols into the executable closure tree.
// Compiles control flow statements (If, While, For), short-circuiting logic,
// comparisons, string methods, and module constants.

#include "ast.hpp"
#include "closure.hpp"
#include "resolver.hpp"
#include "satellite_words/words.hpp"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace satellite {

struct CompiledProgram {
    std::unordered_map<std::string, std::shared_ptr<CapsuleClosure>> capsules;
    std::vector<StmtPtr> top_level;
};

class Compiler {
public:
    Compiler(const Program &program, const AstArena &arena,
             const ResolveResult &resolve, words::Words &words);

    CompiledProgram compile();

    ExprPtr compile_expr(NodeIndex node_idx);
    StmtPtr compile_stmt(NodeIndex node_idx);
    std::shared_ptr<CapsuleClosure> compile_capsule(const CapsuleInfo &info);

private:
    const Program &program_;
    const AstArena &arena_;
    const ResolveResult &resolve_;
    words::Words &words_;

    bool flatten_dotted_path(NodeIndex node_idx, std::string &out_path);
};

CompiledProgram compile(const Program &program, const AstArena &arena,
                        const ResolveResult &resolve, words::Words &words);

} // namespace satellite
