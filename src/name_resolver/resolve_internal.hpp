#pragma once

// The Resolver object, shared by the four translation units that make it up.
// See name_resolver/resolve.hpp for what a resolve is and what it promises.
//
// SPLIT BY SUBJECT AND NOT BY SIZE, which is the arrangement parser_internal.hpp
// already describes: the passes and their order here and in resolve.cpp, the
// scope stack and the slots in scopes.cpp, the tree walk in walk.cpp, and
// everything that turns a word into a number in names.cpp. DESIGN §7 splits at
// those seams -- §7.3 is the order, §7.2 and §7.4 are the slots, §7.7 and
// WORD_NUMBERS §1.5 are the numbering -- so the files are the specification's
// own sections rather than an arithmetic over 300 lines.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "name_resolver/resolve.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace satellite::resolve {

class Resolver {
public:
    Resolver(const Ast &ast, words::Words &words, const cache::Marks &marks,
             const cache::Folded &folded, Resolved &out)
        : ast_(ast), words_(words), marks_(marks), folded_(folded), out_(out)
    {
    }

    void run();

private:
    // --- the four passes, DESIGN §7.3 (resolve.cpp) -------------------------

    void collect_capsules();
    void note_spacesuits();
    void link_supers();

    // One suit's members, flattened across its sections -- M26. Keeps its own
    // stack, because a section may hold a section and the nesting is the user's
    // to choose (DESIGN §7.5).
    void gather_members(NodeIndex suit_node, Suit &into, bool is_public);
    void globals();
    void bodies();

    // --- the walk (walk.cpp) ------------------------------------------------

    // WHAT THE WALK'S OWN STACK IS MADE OF. DESIGN §7.5: the language has no
    // depth limit, so no walker may use the C++ stack for a depth the user's
    // program chooses. Four of these six actions are things recursion did for
    // free and an explicit stack has to say out loud -- close a scope after its
    // children, declare a name after its initialiser, and finish member() and
    // call() after the one child each of them reads back.
    enum class Act : uint8_t {
        Expression,      // visit an expression node
        Statement,       // visit a statement node
        CloseScope,      // a Block's or a For's scope, after its children
        Declare,         // a VarDecl's name, after its initialiser
        MemberDone,      // member(), after its receiver
        CallTargetDone,  // call(), after its target
        Topic,           // an unevaluated argument -- resolve it, refuse nothing
    };

    // Twelve bytes, and `type` is read by Declare alone -- a VarDecl's type is
    // known when the statement is met and wanted when the name is bound, which
    // is after the initialiser and after everything inside it.
    struct Work {
        Act act = Act::Expression;
        NodeIndex node = kNoNode;
        words::PathId type = words::kNoPath;
    };

    // The two that seed the stack and drain it. Everything else pushes.
    void statement(NodeIndex node);
    void expression(NodeIndex node);
    void body_of(NodeIndex capsule, Frame &frame);

    void run_work();
    void expression_at(NodeIndex node);
    void statement_at(NodeIndex node);
    void visit_arguments(NodeIndex call);

    void visit_expression(NodeIndex node)
    {
        work_.push_back({Act::Expression, node, words::kNoPath});
    }

    // AN UNEVALUATED ARGUMENT IS STILL RESOLVED, AND THAT IS THE WHOLE
    // SUBTLETY. `satellite.help(x)` `1 19 1` never runs its argument, but the
    // compiler READS what resolve decided about it -- `satellite.console` has
    // to arrive carrying `1 5`, and a name the program declared has to arrive
    // carrying its declared type. So the argument is walked exactly as any
    // other; what is suppressed is one refusal.
    //
    // WHICH ONE, AND WHY IT CANNOT SIMPLY BE LEFT TO FIRE. A bare word here is
    // not a variable being read -- it is a topic being named -- so S0511's
    // "nothing called `random` is in scope here" answers a question nobody
    // asked. The author settled the sentence that belongs here on 2026-09-07
    // and it is S1102: `no variable with the name random -- did you mean
    // satellite.random?`, with a pointer at `satellite.help()`. That sentence
    // is the compiler's to raise, and it can only raise it if resolve leaves
    // the name unresolved instead of stopping the program first.
    void visit_topic(NodeIndex node)
    {
        work_.push_back({Act::Topic, node, words::kNoPath});
    }

    void visit_statement(NodeIndex node)
    {
        work_.push_back({Act::Statement, node, words::kNoPath});
    }

    // --- names, paths and numbers (names.cpp) -------------------------------

    void name(NodeIndex node);

    // Set while an unevaluated argument is being resolved. Read by name() and
    // by nothing else: the deeper shapes an argument can take are paths, and a
    // wrong path still deserves "no such word under satellite".
    bool in_topic_ = false;
    void member(NodeIndex node);
    void member_done(NodeIndex node);
    void call(NodeIndex node);
    void call_target_done(NodeIndex node);
    void statement_form(NodeIndex node, words::NodeId under, const char *word);
    void no_such_word(const cache::PathMatch &stopped);

    // The language path a chain names, taking it from the `.satc` when the file
    // already numbered it. Reports nothing; the callers decide. `took_` is how
    // it came back, and is read immediately or not at all.
    cache::PathMatch path_of(NodeIndex node, bool wants_call);
    bool took_ = false;

    // A type, checked against the numbering and recorded on the node. Returns
    // the node the type path ends at, or kNoPath for `satellite` itself and for
    // a spacesuit named bare -- both of which this milestone cannot check.
    // type_of() is the type and everything inside it and keeps its own stack;
    // type_at() is one node of it and descends into nothing.
    words::PathId type_of(NodeIndex node);
    words::PathId type_at(NodeIndex node);

    // WORD_NUMBERS §1.5's fold, over the numbering rather than over a table.
    bool fold_option(NodeIndex call_node, NodeIndex target, words::PathId under);

    // DESIGN §7.7, and only for `satellite.main`'s parameter -- see names.cpp
    // for why a capsule of the user's own does not get one.
    void main_parameter(NodeIndex decl, std::string_view spelling, Slot slot);
    void arguments_member(NodeIndex node, std::string_view word);

    // --- scopes and slots (scopes.cpp) --------------------------------------

    struct Binding {
        std::string_view name;
        Slot slot = kSlotGlobal;
        words::PathId type = words::kNoPath;
        NodeIndex at = kNoNode;

        // DESIGN §7.7's object, carried on the binding rather than looked up
        // again at every read: `arguments.machine.cores` asks three times.
        bool arguments = false;
    };

    // A capsule this file declares, found by pass 1 and called by pass 4.
    struct Capsule {
        words::PathId path = words::kNoPath;
        std::string_view name;
        NodeIndex node = kNoNode;
    };

    Slot declare(NodeIndex decl, uint32_t token, words::PathId type);
    const Binding *lookup(std::string_view spelling) const;
    const Capsule *capsule_named(std::string_view spelling) const;
    const Capsule *suit_named(std::string_view spelling) const;
    words::PathId conversion_named(std::string_view spelling) const;

    void open_scope() { scopes_.push_back(bindings_.size()); }
    void close_scope();

    // DESIGN §4.6 over the names that are in scope, which is the one candidate
    // list errors::suggest() cannot search -- it walks the frozen trie, and
    // these names were met four milestones after it was written.
    std::string_view nearest_in_scope(std::string_view spelling) const;

    // --- diagnostics (scopes.cpp) -------------------------------------------

    errors::Span span_of(NodeIndex node) const;

    template <errors::Code C, typename... Args>
    void problem(NodeIndex at, Args &&...arguments)
    {
        out_.problems.push_back(
            errors::make<C>(span_of(at), std::forward<Args>(arguments)...));
    }

    void attach(errors::Note remark) { out_.problems.back().notes.push_back(remark); }
    void suggest(std::string_view word) { out_.problems.back().suggestion = std::string(word); }

    Info &info(NodeIndex node);

    const Ast &ast_;
    words::Words &words_;
    const cache::Marks &marks_;

    // WHICH SELECTORS THE `.satc` SAID WERE FOLDED -- M19.6. Empty for a
    // program walked from source, which is the ordinary case and costs one
    // comparison; cache.hpp's `Folded` says why it is offsets and not numbers.
    const cache::Folded &folded_;
    Resolved &out_;

    std::vector<Capsule> capsules_;
    std::vector<Capsule> suits_;
    std::vector<Binding> bindings_;
    std::vector<size_t> scopes_;

    // THE DEPTH OF THE WALK LIVES HERE AND NOWHERE ELSE. It is a vector on the
    // heap, so what bounds a nested expression is memory -- and running out of
    // memory is a thing this language already has words and an exit status for
    // (PLAN §4.5.2's watchdog), where running off the C++ stack is signal 11.
    std::vector<Work> work_;

    // The frame being filled, or null at the top level. §7.2's whole point is
    // that these two cases are DIFFERENT storage and not one with a flag:
    // `satellite.library` is shared and permanent, and a local is neither.
    Frame *frame_ = nullptr;

    // THE SPACESUIT WHOSE METHOD IS BEING WALKED, or null -- M26. Set for the
    // length of one method body by pass 4 and read by name(): a bare word
    // inside a method is a local first, then a FIELD of this suit, then a
    // capsule, then a suit. That order is DESIGN §7.1's rule ("a local shadows
    // a capsule") extended by one row, and the row goes where it does because a
    // field is storage and a capsule is not -- so a parameter named `n` in a
    // suit that also has a field `n` means the parameter, which is what every
    // language with both does and what a reader expects.
    const Suit *inside_ = nullptr;

};

// The child of `parent` spelled `word`, or kNoPath. Aliases count, for the
// reason words_runtime.hpp's find() gives: leaving them out is how one word
// ends up with two numbers.
words::PathId child_named(words::NodeId parent, std::string_view word);

} // namespace satellite::resolve
