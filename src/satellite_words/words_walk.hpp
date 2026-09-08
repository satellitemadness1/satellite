#pragma once

// Reading a path against the trie -- part of words.hpp.
//
// THE WALK HAPPENS ONCE, WHEN A SOURCE IS READ. That is the whole architecture
// in one sentence (DESIGN §4.5): the terminal node this lands on has an interned
// id, that single uint32_t is what the compiled closure holds, and dispatch
// afterwards is handlers[path_id] -- one array index and one indirect call. The
// first satellite reached this point and then joined the path into a heap string
// and ran a chain of string compares on every module call.
//
// The tables it reads are next door in words_spellings.hpp; this file is the act
// of reading them.

#include <cstdint>
#include <string_view>

#include "satellite_words/words_nodes.hpp"
#include "satellite_words/words_numbers.hpp"
#include "satellite_words/words_spellings.hpp"

namespace satellite::words {

// Why a walk stopped where it did.
//
// NOT AN EXCEPTION AND NOT A NULL. DESIGN §9.1 rules out throwing -- measured in
// the first satellite at 8.5 ns returned as an enum against 1537 ns thrown --
// and rules out "record the error and return nullptr" just as firmly, because
// the caller then has an answer that says nothing about what went wrong. M5 is
// the error reporter and it does not exist yet, so what this returns has to be
// enough for M5 to be written against without changing this signature: the id,
// and, when there is no id, WHICH SEGMENT failed and WHICH NODE it failed under.
//
// `under` is the load-bearing half. DESIGN §4.6's "no `consle` under
// `satellite` -- did you mean `console`?" is edit distance over one node's
// children, and that node is this one.
enum class WalkError : uint8_t {
    NONE = 0,
    NOT_ROOTED,      // §1: a language-owned name is a path rooted at `satellite`
    NO_SUCH_WORD,    // nothing under `under` is spelled that way
    NO_SUCH_SHAPE,   // the word is there; that call shape is not
    TRAILING,        // the path matched and then did not end
};

struct Walk {
    PathId id;          // kNoPath unless error == NONE
    WalkError error;
    PathId under;       // the node the failed segment was looked up under
    uint32_t offset;    // where in the input that segment began
};

namespace detail {

// The child of `node` spelled `word` and called with `args`.
//
// TWO PLACES TO LOOK, AND THAT IS THE "TWO DEPTHS" QUESTION ANSWERED IN CODE.
// WORD_NUMBERS §4 asks whether a call shape is a sibling or a child and shows
// both in the authority; words.def's header argues they are one rule. Here is
// what the rule costs: a word with no number of its own keeps its shapes beside
// it (`input()` is 1 5 2, a sibling of `display`), and a word with a number
// keeps them below it (`include()` is 1 1 0, a child of `include`). So a call
// is matched against the siblings first and then against the word's own
// children, and nothing else in the module has to know which kind a word is.
inline PathId match_shape(NodeId node, std::string_view word, std::string_view args)
{
    // AN EMPTY WORD MATCHES NOTHING, and this guard is load-bearing rather than
    // defensive. The 39 bare rows and the 6 argument rows have an EMPTY
    // spelling by design -- words.def says such a row "has no spelling of its
    // own and is never walked to by name" -- so without this, an empty `word`
    // compares equal to every one of them and the first `if (args.empty())`
    // below hands the caller the bare shape.
    //
    // FOUND 2026-08-28 BY REVIEW, and it was reachable from the command line:
    // `satl --words satellite.console.` answered `1 5 0`, and
    // `satellite.include.(satellite)` answered `1 1 1`. A trailing dot and a
    // dot before an argument list are both malformed, and both resolved.
    if (word.empty())
        return kNoPath;

    PathId word_node = kNoPath;

    for (PathId c = first_child(node); c != kNoPath; c = next_sibling(c)) {
        const NodeId child = static_cast<NodeId>(c);
        if (spelling_of(child) != word)
            continue;
        // Written with no arguments at all: the lowest-numbered shape of the
        // word, which is what WORD_NUMBERS §2.2 means by listing
        // satellite.random.normal at 1 7 2 while normal(digits) is 1 7 7.
        // first_child/next_sibling run in ascending order, so this is the first
        // one seen.
        if (args.empty())
            return c;
        if (arguments_of(child) == args)
            return c;
        if (arguments_of(child).empty() && word_node == kNoPath)
            word_node = c;
    }

    // The word has a number of its own, so its shapes are its children.
    if (word_node != kNoPath)
        for (PathId g = first_child(static_cast<NodeId>(word_node)); g != kNoPath;
             g = next_sibling(g))
            if (text_of(static_cast<NodeId>(g)) == args)
                return g;

    return kNoPath;
}

// An alias of a node whose parent is `node`, matched against the whole of what
// is left -- because an alias may carry a dot and span two segments.
//
// THE BOUNDARY CHECK IS NOT OPTIONAL. `argument` is a declared spelling of
// satellite.library.main.arguments and so is `arguments`, and the first is a
// prefix of the second; without requiring the match to end at `.`, `(` or the
// end of the path, walking `arguments` would consume `argument` and then fail
// on a leftover `s`. Longest-match alone does not fix it, because the node's
// OWN text is not in this table.
inline PathId match_alias(NodeId node, std::string_view rest, size_t &length)
{
    PathId found = kNoPath;
    size_t best = 0;
    for (size_t i = 0; i < kAliasCount; i++) {
        if (parent_of(kAliases[i].of) != node)
            continue;
        const std::string_view text = kAliases[i].text;
        // TWO FORMS ANSWER FOR AN ALIAS, EXACTLY AS TWO ANSWER FOR A WORD: the
        // whole of it, and the part before its argument list. Without the
        // second, `satellite.random.normal` resolved to 1 7 2 while
        // `satellite.random.normal.range` -- its own declared spelling, naming
        // one shape and no other -- did not resolve at all, and said "no such
        // word under satellite.random.normal()". A second spelling of a node
        // that behaves differently from the first is not a second spelling.
        // (Found 2026-08-28 by walking SCRATCH.md/WORD_SURFACE.md's survey,
        // which writes all three `.range` rows bare.)
        for (const std::string_view form : {text, spelling_of(text)}) {
            if (form.empty() || form.size() <= best || rest.size() < form.size())
                continue;
            if (rest.compare(0, form.size(), form) != 0)
                continue;
            if (rest.size() > form.size() && rest[form.size()] != '.' &&
                rest[form.size()] != '(')
                continue;
            found = static_cast<PathId>(kAliases[i].of);
            best = form.size();
        }
    }
    length = best;
    return found;
}

} // namespace detail

// What a bare spelling names under `node`, SHAPES INCLUDED -- the lowest-
// numbered shape of the word when the word is only ever written with arguments.
//
// THE QUESTION `satl --words` ALREADY ANSWERS, GIVEN A NAME. Walking
// `satellite.variable.string.find` from the command line lands on `1 6 1 3`
// today, because match_shape above takes the first child with that spelling
// when nothing was written in parentheses. What did not exist until M18 is a
// way for anything else to ask it: resolve's own walk is stricter on purpose --
// `satellite.variable.string.find` written in a program is not an expression --
// and `child_named` in the resolver excludes shapes for a defect it records.
//
// SO THIS IS THE TOPIC LOOKUP AND IT HAS EXACTLY ONE CALLER. A help query names
// a WORD -- "asking about a path brings up that node and the shapes written
// beside it" -- so `satellite.help(satellite.variable.string.find)` has to
// reach a node whose only shape takes an argument, and every listing help
// prints is made of exactly these spellings. Nothing else in the tree may use
// it: a program that writes a shape without its arguments is still wrong, and
// the strictness resolve has is the reason it can say so.
inline PathId word_named(NodeId node, std::string_view word)
{
    return detail::match_shape(node, word, {});
}

// A dotted path to the node it names.
//
// The path is written as WORD_NUMBERS §2.2 writes it, arguments included:
// "satellite.console.display", "satellite.console.input(prompt, target)",
// "satellite.random.fast.range(min, max)".
inline Walk walk(std::string_view path)
{
    const std::string_view root = spelling_of(NodeId::SATELLITE);
    if (path.size() < root.size() || path.compare(0, root.size(), root) != 0 ||
        (path.size() > root.size() && path[root.size()] != '.'))
        return {kNoPath, WalkError::NOT_ROOTED, kNoPath, 0};

    NodeId node = NodeId::SATELLITE;
    size_t at = root.size();

    while (at < path.size()) {
        if (path[at] != '.')
            return {kNoPath, WalkError::TRAILING, static_cast<PathId>(node),
                    static_cast<uint32_t>(at)};
        at++;
        const uint32_t start = static_cast<uint32_t>(at);
        const std::string_view rest = path.substr(at);

        // Aliases first, because one of them spans a dot and segmenting the
        // path would cut `fast.range(min, max)` in half before it could match.
        size_t alias_length = 0;
        if (const PathId aliased = detail::match_alias(node, rest, alias_length)) {
            node = static_cast<NodeId>(aliased);
            at += alias_length;
            continue;
        }

        const size_t word_end = rest.find_first_of(".(");
        const std::string_view word =
            word_end == std::string_view::npos ? rest : rest.substr(0, word_end);

        std::string_view args;
        size_t consumed = word.size();
        if (word_end != std::string_view::npos && rest[word_end] == '(') {
            const size_t close = rest.find(')', word_end);
            if (close == std::string_view::npos)
                return {kNoPath, WalkError::NO_SUCH_SHAPE, static_cast<PathId>(node),
                        start};
            args = rest.substr(word_end, close - word_end + 1);
            consumed = close + 1;
        }

        const PathId next = detail::match_shape(node, word, args);
        if (next == kNoPath) {
            // WHICH OF THE TWO FAILED, because they are different sentences to
            // a user. "no `consle` under `satellite`" is a misspelling; "no
            // `display()` -- display takes one argument" is a call that named a
            // real word the wrong way, and M5 must not print the first for the
            // second.
            const bool word_exists = detail::match_shape(node, word, {}) != kNoPath;
            return {kNoPath,
                    word_exists ? WalkError::NO_SUCH_SHAPE : WalkError::NO_SUCH_WORD,
                    static_cast<PathId>(node), start};
        }
        node = static_cast<NodeId>(next);
        at += consumed;
    }

    return {static_cast<PathId>(node), WalkError::NONE, kNoPath, 0};
}

} // namespace satellite::words
