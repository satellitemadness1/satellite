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

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
void op_to_float(Machine &m, const Op &op, uint32_t step);
void op_call(Machine &m, const Op &op, uint32_t step);
void op_enter(Machine &m, const Op &op, uint32_t step);
void op_package(Machine &m, const Op &op, uint32_t step);
void op_construct(Machine &m, const Op &op, uint32_t step);
void op_field(Machine &m, const Op &op, uint32_t step);
void op_field_store(Machine &m, const Op &op, uint32_t step);
void op_dispatch(Machine &m, const Op &op, uint32_t step);
void op_method(Machine &m, const Op &op, uint32_t step);
void op_method_global(Machine &m, const Op &op, uint32_t step);
void op_method_field(Machine &m, const Op &op, uint32_t step);
void op_place(Machine &m, const Op &op, uint32_t step);
void op_place_global(Machine &m, const Op &op, uint32_t step);
void op_refuse(Machine &m, const Op &op, uint32_t step);
void op_no_question(Machine &m, const Op &op, uint32_t step);
void op_misuse(Machine &m, const Op &op, uint32_t step);
void op_options(Machine &m, const Op &op, uint32_t step);

void op_include(Machine &m, const Op &op, uint32_t step);

void op_block(Machine &m, const Op &op, uint32_t step);
void op_expression(Machine &m, const Op &op, uint32_t step);
void op_index(Machine &m, const Op &op, uint32_t step);
void op_slice(Machine &m, const Op &op, uint32_t step);
void op_store(Machine &m, const Op &op, uint32_t step);
void op_index_store(Machine &m, const Op &op, uint32_t step);
void op_index_store_global(Machine &m, const Op &op, uint32_t step);
void op_store_global(Machine &m, const Op &op, uint32_t step);
void op_retune(Machine &m, const Op &op, uint32_t step);
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
// WHAT EVERY FILE'S COMPILER SHARES -- M25, 2026-09-13. A run of several files
// is ONE program: one op arena, one capsule table, one `satellite.library`, so
// a call from `host.satl` into `ship.satl` is an ordinary op_call and a thread
// started in either walks the same arena. What is per file is the tree and
// what resolve said about it, and that is all a Compiler below holds of its own.
struct Linking {
    Compiled out;
    std::vector<errors::Diagnostic> problems;
    std::vector<errors::Diagnostic> deferred_refusals;
    std::vector<uint32_t> op_began;
    std::unordered_map<words::PathId, uint32_t> capsules;
    std::unordered_map<words::PathId, uint32_t> globals;
    std::unordered_map<words::PathId, uint32_t> suits;

    // Every file's resolve, by file id -- a spacesuit is looked up in all of
    // them, because `ship.box b` in one file constructs a layout another made.
    std::vector<const resolve::Resolved *> files;

    // Which file an Include node loads, per including file: (node, file id).
    std::vector<std::vector<std::pair<NodeIndex, uint32_t>>> includes;

    // EVERY LAYOUT'S FIELD INITIALISERS, COMPILED ONCE BY THE FILE THAT
    // DECLARED THE SUIT -- M25, by layout index. `ship.crate box` in another
    // file constructs a crate whose initialisers are nodes of `ship.satl`'s
    // tree, and only ship's compiler reads that tree. Found by running
    // example/spaceships/launch.satl, whose first `cargo.crate` compiled
    // cargo's node numbers against launch.satl and refused with a caret on the
    // wrong line of the wrong file.
    std::unordered_map<uint32_t, OpListId> fields;

    // File 0's top-level spaceship includes that ran on an earlier prompt line.
    uint32_t already_included = 0;
};

class Compiler {
public:
    Compiler(const Ast &ast, const resolve::Resolved &resolved, words::Words &words);

    // ONE FILE OF A RUN -- M25. `linking.files[file]` must be this file's.
    Compiler(Linking &linking, uint32_t file, const Ast &ast,
             const resolve::Resolved &resolved, words::Words &words);

    Compiled compile();

    // THE THREE HALVES OF compile(), for a run of several files. register_file()
    // is passes 1, 1b and 2 -- every capsule, layout and global slot -- and runs
    // for every file before compile_file() runs for any, which is DESIGN §7.3's
    // forward reference across files. finish_run() is the last step of
    // compile() and runs once.
    void register_file();
    void compile_fields();
    void compile_file();
    static Compiled finish_run(Linking &linking, const std::vector<uint32_t> &setup_order);

    std::vector<errors::Diagnostic> take_problems() { return std::move(problems_); }

    // WHAT THE COMPILER KNOWS WILL BE REFUSED IF IT IS EVER REACHED -- M26.5,
    // and it is a SECOND list on purpose. `problems_` stops a program: a
    // compile that filled it produced nothing runnable. These do not, because
    // the ops are emitted and the program runs right up until control arrives
    // at one -- which is op_refuse's whole argument in operations.cpp, that a
    // `satellite.include` in a branch nobody takes is a program that runs.
    //
    // SO THE KNOWLEDGE IS PUBLISHED RATHER THAN ACTED ON. `satl --check` prints
    // these and a run does not, which means the answer arrives before the seven
    // minutes rather than after them, and no program that runs today stops.
    // `deferred_refusals_` AND NOT `deferred_`, WHICH IS TAKEN. M23's
    // packaging already owns that name for a set of nodes whose arguments
    // are NOT evaluated at the call (SAT_DEFER). Two different meanings of
    // "deferred" in one class is how a reader loses ten minutes.
    std::vector<errors::Diagnostic> take_deferred_refusals() { return std::move(deferred_refusals_); }

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
    // chooses between four shapes -- a method on a value, a capsule this
    // program declared, a language path through handlers[path_id], and a
    // refusal -- and that choice is DESIGN §6.4's dispatch question rather
    // than a step of the walk.
    OpIndex call(NodeIndex node);

    // The receiver a call is a METHOD on, or kNoNode when it is not one.
    // DESIGN §6.4: `s.upper()` is `satellite.variable.string.upper(s)` in the
    // table, so the receiver is an argument the user did not write -- the Call
    // case visits it, and call() prepends it. A call is a method call when its
    // target's selector folded to a language path THROUGH a receiver that is
    // not itself a language word: resolve's names.cpp did the folding, and the
    // one receiver it folds through is a declared name, which is what makes
    // every method receiver a slot or a global at this milestone.
    NodeIndex method_receiver(NodeIndex call_node) const;

    // S0723 -- a selector a declared type does not have. True when it applies
    // and `*out` is the op; false when the receiver is not a declared name, in
    // which case the caller has its own sentence. BOTH the call road and the
    // bare member read reach it; compile_expressions.cpp carries why that took
    // until M20 to be true.
    bool no_question(NodeIndex at, NodeIndex member, OpIndex *out);

    // Which written argument of this call is an UNEVALUATED topic, or
    // words::kNoTopicParameter. words.def's fifth list is the declaration and
    // `satellite.help(x)` `1 19 1` is its one row; PLAN M18 is the argument for
    // the list existing at all.
    //
    // ASKED TWICE ON PURPOSE, and the two askers want opposite things. The Call
    // case asks so it can decline to COMPILE that argument -- which is the whole
    // point: `satellite.help(satellite.console)` read as an expression is an
    // op_dispatch that refuses at run time, so the argument would die before
    // help was entered. call() asks so it knows how many results are actually
    // on the value stack, because an argument that was never visited left none.
    uint32_t topic_parameter(NodeIndex call_node) const;

    // Which written argument of this call is a deferred call, or
    // words::kNoDeferParameter. words.def's seventh list, read on the CALL node
    // for topic_parameter's reason exactly -- resolve puts the number on the
    // whole shape, parentheses included, and the target member carries none.
    uint32_t defer_parameter(NodeIndex call_node) const;

    // Not a capsule this program declared. A real index is a position in
    // Compiled::capsules(), so the sentinel is one past anything a program of
    // four billion capsules could reach.
    static constexpr uint32_t kNotACapsule = 0xFFFFFFFFu;

    // WHICH COMPILED CAPSULE THIS CALL WOULD HAVE ENTERED, or kNotACapsule --
    // M23. It is call()'s two capsule arms asked as a question instead of taken
    // as a branch, which is the only reason it exists: op_package needs the
    // same index op_call would have got, decided by the same two lookups, and a
    // third copy of them would be a third place they can drift.
    uint32_t capsule_index(const resolve::Info &about,
                           words::PathId declared) const;

    // Which spacesuit this path is a method of, or null -- M26.
    const resolve::Suit *suit_of_method(words::PathId path) const;

    // A spacesuit declared in any file of the run -- M25.
    const resolve::Suit *suit_anywhere(words::PathId path) const;

    // `satellite.include(ship(args))` -- M25. The file it loads, or kNotLoaded
    // when the loader did not load one, which is a program built on its own.
    static constexpr uint32_t kNotLoaded = 0xFFFFFFFFu;
    uint32_t file_included(NodeIndex include) const;
    OpIndex include(NodeIndex node);
    std::vector<NodeIndex> include_arguments(NodeIndex node) const;

    // `satellite.help(x)`'s one argument, compiled. The path or the declared
    // type is folded to a constant HERE, at compile time, and the misuses are
    // refused here too -- so nothing about the ask is decided while the program
    // is running and the handler is handed a path it can trust.
    OpIndex topic(NodeIndex node, words::PathId path, NodeIndex written);

    void capsule(NodeIndex node);
    void global(NodeIndex node);

    // An op that refuses at RUN time rather than at compile time, naming the
    // milestone that will build it. errors.def's S0720 note is the argument:
    // between now and M28 every milestone ships a language whose grammar is
    // wider than its evaluator, and saying so with a code, a caret and a
    // milestone number is DESIGN §1.1 applied to the gap itself.
    OpIndex not_built(NodeIndex node, const std::string &what, const char *milestone);

    // The float declaration's conversion, shared by VarDecl and both Assign
    // arms -- compile_statements.cpp says why it is an op and not a check.
    OpIndex into_declared(const resolve::Info &about, NodeIndex node,
                          OpIndex value);

    // One spacesuit, constructed -- M26. See compile_statements.cpp.
    OpIndex construct(NodeIndex at, uint32_t layout);
    std::vector<OpIndex> constructor_chain(NodeIndex at, const resolve::Info &about,
                                           OpIndex stored,
                                           const std::vector<OpIndex> &arguments);

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

    // THE SHARED HALF, OWNED HERE FOR A PROGRAM OF ONE FILE -- M25. Declared
    // first so the references below bind to a living object.
    std::unique_ptr<Linking> own_;
    Linking &linking_;
    uint32_t file_ = 0;

    const Ast &ast_;
    const resolve::Resolved &resolved_;
    words::Words &words_;

    Compiled &out_;
    std::vector<errors::Diagnostic> &problems_;
    std::vector<errors::Diagnostic> &deferred_refusals_;

    std::vector<Task> tasks_;

    // WHERE EACH NODE'S OPS BEGAN, and each op's node's beginning -- what
    // Compiled::statement_writes() is computed from at the end of compile().
    std::unordered_map<NodeIndex, uint32_t> began_;
    std::vector<uint32_t> &op_began_;
    std::vector<OpIndex> results_;

    // Which compiled capsule a capsule's PathId is. Built in a pass of its own
    // before any body is compiled, so that a call to a capsule declared further
    // down the file resolves -- which is DESIGN §7.3's reason for resolve
    // running in four passes, and this walk has the same forward reference.
    std::unordered_map<words::PathId, uint32_t> &capsules_;

    // Which global slot a `satellite.library.NAME` PathId is.
    std::unordered_map<words::PathId, uint32_t> &globals_;

    // Which layout a spacesuit's PathId is -- M26, `capsules_`' shape one
    // declaration kind over, and filled in the same pass and for the same
    // reason: a field initialiser may construct a suit declared further down.
    std::unordered_map<words::PathId, uint32_t> &suits_;

    // THIS FILE'S TOP LEVEL, AS register_file() AND compile_file() LEAVE IT --
    // M25. For file 0 it is the program's top block; for a spaceship it is the
    // globals' initialisers (`setup`) and the includes written at its top.
    std::vector<OpIndex> setup_;
    std::vector<OpIndex> top_includes_;
    uint32_t included_so_far_ = 0;

    // The spacesuit whose method is being compiled, or null. resolve's member
    // of the same name is the other half; a field index means nothing without
    // the suit it indexes.
    const resolve::Suit *inside_ = nullptr;

    // The root expression of the statement being compiled, so call() can
    // tell statement position from every other -- the whole of how M14's
    // place-writing call is confined to "a statement of its own".
    // compile_statements' ExprStmt arm is the writer and the only one.
    NodeIndex statement_root_ = kNoNode;

    // THE CALLS THAT ARE PACKAGED RATHER THAN PERFORMED -- M23, words.def's
    // seventh list. A parent marks its deferred argument on the way DOWN, at
    // step 0 of the Call case; call() reads it on the way back UP and emits
    // op_package where it would have emitted op_call.
    //
    // A SET AND NOT A SINGLE NodeIndex, and the reason is a shape nobody should
    // have to reason about twice. The walk is a task stack, so a marked child
    // is compiled between its parent's step 0 and step 1 -- one field would
    // work today. It would stop working the moment a deferred call appeared
    // inside another one's arguments, and it would stop working SILENTLY, by
    // compiling a real call as a package or the other way round. A set cannot:
    // each node is marked at most once, by the one parent that defers it, and
    // membership is a fact rather than a state.
    //
    // AND IT IS MARKED AND NEVER UNMARKED, which is what makes the previous
    // sentence true. Nothing here depends on compile order.
    std::unordered_set<NodeIndex> deferred_;
};

} // namespace satellite::eval
