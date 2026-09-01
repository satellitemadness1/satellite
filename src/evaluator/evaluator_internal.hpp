#pragma once

// What the evaluator's files share -- the op functions and the compiler. Not
// a door onto this module; evaluator/evaluate.hpp is. See parser_internal.hpp,
// resolve_internal.hpp and write_internal.hpp, which are the same file three
// modules over and for the same reason.
//
// THE OP FUNCTIONS ARE DECLARED HERE BECAUSE TWO FILES NEED THEM AND FOR
// OPPOSITE REASONS. operations*.cpp defines them; compile*.cpp takes their
// ADDRESSES, which is what closure compilation IS -- the compiler's whole
// output is a vector of function pointers with their operands already decided.
// A header naming both halves is what stops the compiler emitting an op the
// machine has no arm for.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "evaluator/closure.hpp"
#include "evaluator/machine.hpp"
#include "name_resolver/resolve.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace satellite::eval {

// --- the machine's arms -----------------------------------------------------
//
// One per row of closure.hpp's table, in that order. Each is written against
// machine.hpp's contract: an EXPRESSION op leaves exactly one value on the
// value stack and a STATEMENT op leaves none.

void op_no_op(Machine &m, const Op &op, uint32_t step);

void op_constant(Machine &m, const Op &op, uint32_t step);
void op_local(Machine &m, const Op &op, uint32_t step);
void op_global(Machine &m, const Op &op, uint32_t step);
void op_unary(Machine &m, const Op &op, uint32_t step);
void op_binary(Machine &m, const Op &op, uint32_t step);
void op_call(Machine &m, const Op &op, uint32_t step);
void op_enter(Machine &m, const Op &op, uint32_t step);
void op_dispatch(Machine &m, const Op &op, uint32_t step);
void op_refuse(Machine &m, const Op &op, uint32_t step);

void op_block(Machine &m, const Op &op, uint32_t step);
void op_expression(Machine &m, const Op &op, uint32_t step);
void op_store(Machine &m, const Op &op, uint32_t step);
void op_store_global(Machine &m, const Op &op, uint32_t step);
void op_return(Machine &m, const Op &op, uint32_t step);
void op_if(Machine &m, const Op &op, uint32_t step);
void op_while(Machine &m, const Op &op, uint32_t step);
void op_for(Machine &m, const Op &op, uint32_t step);

// What an op function is called, for `satl --compile`. A switch over addresses,
// so an op added without a name is a name that reads "?" rather than a build
// that fails -- which is why tests/eval_test checks every arm has one.
const char *op_name(OpFn fn);

// --- the compiler -----------------------------------------------------------

// Arena AST in, closure tree out -- PLAN §2.3, and it is "the same pass as
// resolve, measured in microseconds".
//
// IT KEEPS ITS OWN STACK, AND IT IS THE FIFTH WALK IN THIS TREE TO DO SO. M8.5
// rewrote four -- the parser, the resolver and both printers -- because DESIGN
// §7.5 says the language has no depth limit and a walker that recurses on the
// C++ stack dies at a depth `ulimit -s` chose. This walk did not exist then and
// there is no exception in §7.5 for a pass that runs once: a program that parses
// at 100,000 deep and then segfaults being COMPILED would have moved M8.5's
// crash rather than removed it. So this is born the shape the other four were
// rewritten into, and MILESTONES/M8.5.md §3.3 is where the idiom comes from.
//
// THE SHAPE IS THE MACHINE'S, WHICH IS WORTH NOTICING RATHER THAN ARRANGING.
// A compiler and an evaluator are the same walk over the same tree: one leaves
// op indices behind and the other leaves values. So `tasks_` is `work_`,
// `results_` is `value_`, and again()/visit()/finish() are again()/push()/done()
// -- the four lines below are machine.hpp's four with the nouns changed. Nothing
// was factored out of the two, because they share no type; the resemblance is
// the point and a shared base class would hide it.
class Compiler {
public:
    Compiler(const Ast &ast, const resolve::Resolved &resolved, words::Words &words);

    Compiled compile();

    std::vector<errors::Diagnostic> take_problems() { return std::move(problems_); }

private:
    // Compile one subtree and answer its op. ONE C++ FRAME PER CALL, whatever
    // the subtree's depth, which is what the task machine buys -- and the only
    // callers are the passes below, one per capsule and one per global.
    OpIndex compile_tree(NodeIndex root);

    // One visit to one node. Switches to the two halves below, which are
    // compile_expressions.cpp and compile_statements.cpp.
    void step(NodeIndex node, uint32_t step_number);
    bool step_expression(NodeIndex node, uint32_t step_number);
    bool step_statement(NodeIndex node, uint32_t step_number);

    // --- what a case may do, and it is machine.hpp's four ---
    void again(uint32_t step_number) { tasks_.back().step = step_number; }
    void visit(NodeIndex node) { tasks_.push_back({node, 0}); }
    void finish(OpIndex op) { tasks_.pop_back(); results_.push_back(op); }

    OpIndex take()
    {
        const OpIndex out = results_.back();
        results_.pop_back();
        return out;
    }

    // The last `count` results, in the order they were written. See the note in
    // compile_expressions.cpp about why one function owns the reversal.
    std::vector<OpIndex> take_many(uint32_t count);

    // Children visited so that the FIRST one compiles first. A stack hands
    // back what went on last, so this pushes them backwards -- the one place
    // that inversion lives, which is MILESTONES/M8.5.md §3.3's rule.
    void visit_reversed(ListId list);

    // The `Call` case's second half. It is a function of its own because it
    // chooses between three shapes -- a capsule this program declared, a
    // language path through handlers[path_id], and a refusal -- and that choice
    // is DESIGN §6.4's dispatch question rather than a step of the walk.
    OpIndex call(NodeIndex node);

    void capsule(NodeIndex node);
    void global(NodeIndex node);

    // An op that refuses at RUN time rather than at compile time, naming the
    // milestone that will build it. errors.def's S0720 note is the argument:
    // between now and M28 every milestone ships a language whose grammar is
    // wider than its evaluator, and saying so with a code, a caret and a
    // milestone number is DESIGN §1.1 applied to the gap itself.
    OpIndex not_built(NodeIndex node, const std::string &what, const char *milestone);

    OpIndex emit(OpFn fn, NodeIndex node, uint32_t a = 0, uint32_t b = 0,
                 uint32_t c = 0, uint32_t d = 0);

    const resolve::Info &info(NodeIndex node) const { return resolved_.at(node); }

    errors::Span span_of(NodeIndex node) const;

    // One node part-way through being compiled. `step` is the return address,
    // exactly as it is in eval::Work.
    struct Task {
        NodeIndex node = kNoNode;
        uint32_t step = 0;
    };

    const Ast &ast_;
    const resolve::Resolved &resolved_;
    words::Words &words_;

    Compiled out_;
    std::vector<errors::Diagnostic> problems_;

    std::vector<Task> tasks_;
    std::vector<OpIndex> results_;

    // Which compiled capsule a capsule's PathId is. Built in a pass of its own
    // before any body is compiled, so that a call to a capsule declared further
    // down the file resolves -- which is DESIGN §7.3's reason for resolve
    // running in four passes, and this walk has the same forward reference.
    std::unordered_map<words::PathId, uint32_t> capsules_;

    // Which global slot a `satellite.library.NAME` PathId is.
    std::unordered_map<words::PathId, uint32_t> globals_;
};

} // namespace satellite::eval
