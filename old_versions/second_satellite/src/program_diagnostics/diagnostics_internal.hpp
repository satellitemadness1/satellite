#pragma once

// What the checks share. See program_diagnostics/diagnose.hpp for the module.
//
// ONE STRUCT AND ONE HELPER, WHICH IS AS MUCH AS A CATALOGUE SHOULD SHARE. Each
// check is a free function over `Subject` that appends what it found, so adding
// one is adding a file and a line in diagnose.cpp -- and a check that needed a
// member of another check would be two checks that should have been one.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "evaluator/evaluate.hpp"
#include "name_resolver/resolve.hpp"

#include <vector>

namespace satellite::diagnostics {

// The program, in the three shapes the passes left it in.
struct Subject {
    const Ast &ast;
    const resolve::Resolved &resolved;
    const eval::Program &program;

    // A CARET FROM A NODE, which every check needs and none should spell twice.
    // The same three fields evaluator/compile.cpp's span_of() reads, for the
    // same reason: a diagnostic without a span is a sentence a person cannot
    // act on, and `Field::at` and `Suit::node` are what the resolver kept so
    // that this was possible.
    errors::Span span_of(NodeIndex node) const
    {
        const Token &at = ast.token_of(node);
        return errors::Span{at.start, at.end, at.line};
    }
};

// --- the catalogue, one line per file ---

// A ring of spacesuits that holds itself alive. program_diagnostics/suit_cycles.cpp
void find_suit_cycles(const Subject &subject, std::vector<errors::Diagnostic> &into);

} // namespace satellite::diagnostics
