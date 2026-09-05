// A dotted chain matched against the numbering. See satellite_cache/paths.hpp
// for why this is not resolve and must not become it.
//
// THE CHAIN IS FLATTENED BEFORE IT IS WALKED, and that is the one structural
// decision in this file. DESIGN §6.2 makes Member and Call peers, so the tree
// holds `satellite.random.fast(a, b)` as a Call wrapping three Members and the
// arity lives one node ABOVE the word it slots -- while §5.1 step 4 needs the
// word and its arity together. Reading the chain into a flat vector first puts
// them side by side; the alternative is a matcher that looks up the tree at
// every step, which is the same walk written twice and is where an off-by-one
// between `input()` and `input(prompt)` would live.

#include "satellite_cache/paths.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace satellite::cache {

namespace {

// One link of a flattened postfix chain, innermost first.
struct Link {
    bool is_call = false;
    std::string_view word;            // a Member's name
    uint32_t argc = 0;                // a Call's argument count
    bool argument_is_satellite = false;
    NodeIndex node = kNoNode;         // the node this link came off
};

// Whether a dotted spelling's segments are the next words of the chain, and how
// many links it covers.
//
// AN ALIAS MAY SPAN TWO MEMBERS and three of the nine do -- words.def's
// `fast.range(min, max)` is "a FOUR segment spelling of a node that is three
// numbers", which is exactly why it could not be written as a node. In the tree
// that is two Member nodes, so matching it against one word can never succeed
// and `satellite.random.fast.range(a, b)` would be written into a `.satc` as
// text. Written as a loop over segments rather than a special case for two,
// because a three-segment alias would otherwise fail silently.
uint32_t member_run(const std::vector<Link> &links, size_t i,
                    std::string_view spelling)
{
    uint32_t used = 0;
    size_t at = 0;
    for (;;) {
        const size_t dot = spelling.find('.', at);
        const std::string_view segment =
            spelling.substr(at, dot == std::string_view::npos ? dot : dot - at);
        if (i + used >= links.size() || links[i + used].is_call ||
            links[i + used].word != segment)
            return 0;
        used++;
        if (dot == std::string_view::npos)
            return used;
        at = dot + 1;
    }
}

// One step of the walk outward from `satellite`.
struct Step {
    words::PathId id = words::kNoPath;
    uint32_t links_used = 0;
    bool takes_call = false;
    bool absorbs_argument = false;
    int shape_arity = -1;
};

// The shape of `word` under `at` that a call of `argc` arguments slots into.
//
// SIBLINGS BEFORE CHILDREN, which is words_walk.hpp's match_shape() rule and is
// repeated here rather than shared because that function matches an argument
// list by its SPELLING and this one by its COUNT. Its comment is the argument
// for the order and it is the two-depths question WORD_NUMBERS §4 asks: a word
// with no number of its own keeps its shapes beside it (`input()` is a sibling
// of `display`), and a word with a number keeps them below it (`include()` is a
// child of `include`).
words::PathId shape_of(words::NodeId at, std::string_view word, int argc,
                       bool argument_is_satellite, words::PathId &word_node)
{
    words::PathId absorber = words::kNoPath;
    words::PathId plain = words::kNoPath;

    const auto consider = [&](words::PathId candidate) {
        const std::string_view args =
            words::arguments_of(static_cast<words::NodeId>(candidate));
        // A ROW WITH NO ARGUMENT LIST IS A WORD AND NOT A SHAPE, and leaving
        // this out was a defect that produced a WRONG PROGRAM rather than a
        // failure. arity_of("") is -1 and `argc` is -1 for a word written with
        // no call, so `satellite.console` -- no call, looking for word_node --
        // reached the loop below over console's children and matched the first
        // one whose argument list was empty. That is `display`. The whole of
        // `satellite.console.input(">>>", target)` was then written as
        // `1.5.1(">>>", target)`: satellite.console.display, with the right
        // arguments, reading back as a different program that still parses.
        // Found 2026-08-30 by looking at what --satc printed for
        // example/advanced.satl, which is the reason PLAN asks every milestone
        // to build a consumer.
        if (args.empty())
            return;
        if (arity_of(args) != argc)
            return;
        if (is_absorber(args)) {
            if (absorber == words::kNoPath)
                absorber = candidate;
        } else if (plain == words::kNoPath) {
            plain = candidate;
        }
    };

    for (words::PathId c = words::first_child(at); c != words::kNoPath;
         c = words::next_sibling(c)) {
        const words::NodeId child = static_cast<words::NodeId>(c);
        if (words::spelling_of(child) != word)
            continue;
        if (words::arguments_of(child).empty())
            word_node = c;
        else
            consider(c);
    }

    // The word has a number of its own, so its shapes are its children.
    //
    // AND ONLY WHEN THERE IS A CALL TO SLOT. A word written with no call has no
    // arity, so there is nothing for a shape to match and looking is how the
    // defect above got in; the guard says that once, here, rather than relying
    // on a comparison against -1 never coming out true.
    if (argc >= 0 && absorber == words::kNoPath && plain == words::kNoPath &&
        word_node != words::kNoPath)
        for (words::PathId g = words::first_child(static_cast<words::NodeId>(word_node));
             g != words::kNoPath; g = words::next_sibling(g))
            consider(g);

    // THE RESERVED WORD DECIDES BETWEEN TWO ROWS OF EQUAL ARITY, and getting it
    // backwards is silent: `satellite.include(satellite)` and
    // `satellite.include(my_ship)` are both one argument, and the numbering has
    // a row for each. Taking the absorber for the second would write 1.1.1 and
    // DROP the spaceship's name, which reads back as a different program.
    if (argument_is_satellite && absorber != words::kNoPath)
        return absorber;
    return plain != words::kNoPath ? plain : absorber;
}

// The alias of a child of `at` that the chain begins with, longest first.
//
// ALIASES ARE COLLAPSED BEFORE ANYTHING ELSE IS TRIED -- SATC.md §5.1 step 1,
// "first, because an alias has no number of its own to write" -- and that is
// also the order words_walk.hpp takes them in, for the reason it gives:
// segmenting the path first would cut `fast.range(min, max)` in half before it
// could match.
Step alias_step(const std::vector<Link> &links, size_t i, words::NodeId at)
{
    Step best;
    uint32_t best_members = 0;
    for (size_t a = 0; a < words::kAliasCount; a++) {
        const words::Alias &alias = words::kAliases[a];
        if (words::parent_of(alias.of) != at)
            continue;
        const std::string_view text = alias.text;
        const uint32_t used = member_run(links, i, words::spelling_of(text));
        if (used == 0 || used <= best_members)
            continue;

        const std::string_view args = words::arguments_of(text);
        const int arity = arity_of(args);
        if (arity < 0) {
            best = {static_cast<words::PathId>(alias.of), used, false, false, -1};
            best_members = used;
            continue;
        }

        // A SHAPE ALIAS NEEDS ITS CALL, AND THE CALL IS THE LINK AFTER ITS LAST
        // WORD -- not the link after its first. Written as "after the first" the
        // three `.range` aliases never matched at all:
        // `satellite.random.fast.range(1, 10)` looked past `fast` for a call,
        // found the Member `range`, and came away with no arity to compare, so
        // the whole chain was written out as `1.7.fast.range(1, 10)` -- half a
        // number and half a name, which is not a form the format has. Found
        // 2026-08-30 by running the writer over a fixture that used one.
        const size_t call = i + used;
        if (call < links.size() && links[call].is_call &&
            static_cast<int>(links[call].argc) == arity) {
            best = {static_cast<words::PathId>(alias.of), used + 1, true,
                    is_absorber(args) && links[call].argument_is_satellite, arity};
            best_members = used;
        }
    }
    return best;
}

// Read a chain inward from `node`, innermost link first. False when the chain
// is not rooted at the reserved word, which is every selector and every user
// name and is the majority answer.
bool flatten(const Ast &ast, NodeIndex node, std::vector<Link> &links)
{
    for (NodeIndex at = node;;) {
        const Node &n = ast[at];
        if (n.kind == NodeKind::Satellite)
            break;
        if (n.kind == NodeKind::Member) {
            links.push_back({false, ast.text_of(at), 0, false, at});
        } else if (n.kind == NodeKind::Call) {
            const uint32_t argc = ast.list_size(n.b);
            links.push_back({true, {}, argc,
                             argc == 1 && ast[ast.list_at(n.b, 0)].kind ==
                                              NodeKind::Satellite,
                             at});
        } else {
            // An Index, a Slice, a Name or a literal. None of them can appear
            // inside a path -- the numbering has no subscripted rows -- so the
            // chain is sugar and the caller writes it as it was written.
            return false;
        }
        at = n.a;
    }
    for (size_t i = 0, j = links.size(); i < j; i++, j--)
        std::swap(links[i], links[j - 1]);
    return !links.empty();
}

} // namespace

PathMatch language_path(const Ast &ast, NodeIndex node)
{
    std::vector<Link> links;
    if (!flatten(ast, node, links))
        return {};

    words::NodeId at = words::NodeId::SATELLITE;
    Step last;
    for (size_t i = 0; i < links.size();) {
        if (links[i].is_call)
            return {};

        const bool have_call = i + 1 < links.size() && links[i + 1].is_call;
        const int argc = have_call ? static_cast<int>(links[i + 1].argc) : -1;
        const bool sat = have_call && links[i + 1].argument_is_satellite;

        Step step = alias_step(links, i, at);
        if (step.id == words::kNoPath) {
            words::PathId word_node = words::kNoPath;
            const words::PathId shape =
                shape_of(at, links[i].word, argc, sat, word_node);
            if (shape != words::kNoPath) {
                const std::string_view args =
                    words::arguments_of(static_cast<words::NodeId>(shape));
                step = {shape, 2, true, is_absorber(args) && sat, arity_of(args)};
            } else if (word_node != words::kNoPath) {
                step = {word_node, 1, false, false, -1};
            } else if (words::PathId ignored = words::kNoPath,
                       zero = i + 1 == links.size() && argc == -1
                                  ? shape_of(at, links[i].word, 0, false,
                                             ignored)
                                  : words::kNoPath;
                       zero != words::kNoPath) {
                // A BARE SPELLING FOLDS TO ITS ZERO-ARGUMENT SHAPE -- the
                // author, 2026-09-04, taken for the random tiers and stated
                // as a rule about spelling: `satellite.random.fast` and
                // `fast()` are the same number (`1 7 1`), because WORD_NUMBERS
                // §2.2's parentheses NAME the zero-argument call shape and a
                // word with no bare node has no other number the spelling
                // could mean. Only the FINAL segment folds -- an interior one
                // still has to be a word something can hang under, and the
                // alias step above has already taken `fast.range(...)` whole
                // -- so `satellite.random.fast.foo` still stops under
                // `random` and is reported over the level that failed.
                //
                // What the fold hands the evaluator is a zero-argument
                // dispatch, exactly what compile_expressions' module-constant
                // arm makes of any language path read without being called;
                // for the tiers that row is S0901's refusal by design, so
                // both spellings refuse through one text. M14 inherits this
                // rule for `satellite.console.input` -- a bare `input` will
                // mean `input()` `1 5 2` -- and its entry is where to look if
                // that is ever to read differently.
                step = {zero, 1, true, false, 0};
            } else {
                // THE PATH DOES NOT RESOLVE, AND THAT IS NOT THIS MILESTONE'S
                // ERROR TO REPORT. A misspelled word under `satellite` is a
                // program M7 refuses with M5's "did you mean" over the node the
                // segment failed under; what a cache owes it is the program
                // UNCHANGED, so the writer prints the chain as text and the
                // reader hands M7 the same tree the source would have.
                //
                // WHERE IT STOPPED IS CARRIED OUT AS OF M7 and this arm is the
                // only one that fills it. Every caller here still reads
                // `found()` and nothing else; PathMatch says why the two fields
                // are on this struct rather than in a walk of resolve's own.
                PathMatch stopped;
                stopped.under = static_cast<words::PathId>(at);
                stopped.at = links[i].node;
                return stopped;
            }
        }

        at = static_cast<words::NodeId>(step.id);
        i += step.links_used;
        last = step;
    }

    return {last.id, last.takes_call, last.absorbs_argument, last.shape_arity};
}

std::string number_text(words::PathId id)
{
    // WORD_NUMBERS §2.2's SPELLING WITH ITS SPACES CLOSED UP, rather than a
    // second walk of the parent chain. One function builds the number and it is
    // the one tests/words_test checks against the authority for all 222 rows;
    // a second would be a second place the numbering lives.
    std::string out = words::number_text(static_cast<words::NodeId>(id));
    for (char &c : out)
        if (c == ' ')
            c = '.';
    return kPathMark + out;
}

PathMatch shape_path(words::NodeId under, std::string_view word, int argc,
                     bool argument_is_satellite)
{
    words::PathId word_node = words::kNoPath;
    const words::PathId shape =
        shape_of(under, word, argc, argument_is_satellite, word_node);
    if (shape != words::kNoPath) {
        const std::string_view args =
            words::arguments_of(static_cast<words::NodeId>(shape));
        return {shape, true, is_absorber(args) && argument_is_satellite,
                arity_of(args)};
    }
    // NO SHAPE AND THE BARE WORD IS NOT AN ANSWER HERE, which is the one place
    // this differs from the chain walk above. There a word with no matching
    // shape is still a path -- `satellite.console.display` is 1 5 1 and the
    // call is the program's own. A statement form has no such reading: the
    // parser only builds a Return or an Include when it has ALREADY seen the
    // word, so a shape that does not match is a program the numbering cannot
    // account for, and handing back the bare word would write `1 15` where the
    // source said `satellite.return(a, b)`.
    return {};
}

} // namespace satellite::cache
