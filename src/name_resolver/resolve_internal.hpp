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
             Resolved &out)
        : ast_(ast), words_(words), marks_(marks), out_(out)
    {
    }

    void run();

private:
    // --- the four passes, DESIGN §7.3 (resolve.cpp) -------------------------

    void collect_capsules();
    void note_spacesuits();
    void globals();
    void bodies();

    // --- the walk (walk.cpp) ------------------------------------------------

    void statement(NodeIndex node);
    void expression(NodeIndex node);
    void body_of(NodeIndex capsule, Frame &frame);

    // A DEPTH GUARD RATHER THAN A COUNTER AT EACH SITE, because the two walkers
    // call each other and every early `return` in either of them would have to
    // remember to put the counter back. See resolve.hpp for why this bound is
    // not DESIGN §7.5's.
    struct Depth {
        explicit Depth(int &at) : at_(at) { at_++; }
        ~Depth() { at_--; }
        Depth(const Depth &) = delete;
        Depth &operator=(const Depth &) = delete;

    private:
        int &at_;
    };

    bool too_deep(NodeIndex node);

    // --- names, paths and numbers (names.cpp) -------------------------------

    void name(NodeIndex node);
    void member(NodeIndex node);
    void call(NodeIndex node);
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
    words::PathId type_of(NodeIndex node);

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
    Resolved &out_;

    std::vector<Capsule> capsules_;
    std::vector<Capsule> suits_;
    std::vector<Binding> bindings_;
    std::vector<size_t> scopes_;

    // The frame being filled, or null at the top level. §7.2's whole point is
    // that these two cases are DIFFERENT storage and not one with a flag:
    // `satellite.library` is shared and permanent, and a local is neither.
    Frame *frame_ = nullptr;

    int depth_ = 0;
};

// The child of `parent` spelled `word`, or kNoPath. Aliases count, for the
// reason words_runtime.hpp's find() gives: leaving them out is how one word
// ends up with two numbers.
words::PathId child_named(words::NodeId parent, std::string_view word);

} // namespace satellite::resolve
