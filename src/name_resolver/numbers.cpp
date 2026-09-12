// What a number is -- a path taken from the trie or from the `.satc`, a type
// checked against the numbering, and WORD_NUMBERS §1.5's literal option fold.
//
// THE WALK IS NOT WRITTEN TWICE, WHICH IS THE FINDING THIS FILE IS SHORT FOR.
// DESIGN §6.3 says "the trie walk that turns a path into a PathId happens once,
// after parsing, in M7's resolve pass", and this milestone set out to write it
// -- and it already existed. `satellite_cache/paths.cpp` walks a chain against
// the numbering, flattens Member and Call into one list because §6.2 makes them
// peers, collapses aliases first, and slots a call by ARITY rather than by the
// spelling of its argument list. M4.5 wrote it to decide what a `.satc` may
// substitute; that header opens "THIS IS NOT RESOLVE AND MUST NOT BECOME IT",
// and it is right about what it warns against -- a version taking a receiver
// argument would be resolve, and the `.satc` it wrote would name a handler,
// which SATC §7 forbids. Reading it is not that. A second copy would be a
// second place a path's identity is decided, and the two would disagree the day
// a row grew an argument list.
//
// WHAT THIS MILESTONE ADDED TO IT IS ONE THING: where the walk STOPPED. That
// function's own comment says a path the numbering cannot account for is "M7's
// to refuse with M5's did-you-mean over the node the segment failed under", and
// it could not say which node that was. It can now, and the cache ignores the
// two new fields.

#include "name_resolver/resolve_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_words/words.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace satellite::resolve {

namespace {

// Whether the row at `id` names a CALL SHAPE rather than a bare word, which is
// what decides whether the node holding it is a Call or a Member.
//
// arity_of("") IS -1 AND THAT IS THE WHOLE OF THE TEST. satellite_cache/
// paths.hpp spends a paragraph on why "()" and "" are different answers, and
// this is the one place resolve needs the difference: `display` is a word and
// the call after it is the program's, while `input(prompt, target)` IS the
// call and the number covers it.
bool names_a_call(words::PathId id)
{
    return cache::arity_of(words::arguments_of(static_cast<words::NodeId>(id))) >= 0;
}

// The mark whose word ends exactly here, or null. The marks come out of
// unnumber() in the order it wrote them, so they ascend and this is a binary
// search over a vector that is empty for every program read from source.
const cache::Mark *mark_ending_at(const cache::Marks &marks, uint32_t ends)
{
    size_t low = 0;
    size_t high = marks.size();
    while (low < high) {
        const size_t middle = low + (high - low) / 2;
        if (marks[middle].ends < ends)
            low = middle + 1;
        else
            high = middle;
    }
    return low < marks.size() && marks[low].ends == ends ? &marks[low] : nullptr;
}

} // namespace

words::PathId child_named(words::NodeId parent, std::string_view word)
{
    if (word.empty())
        return words::kNoPath;

    // A ROW WITH AN ARGUMENT LIST IS A SHAPE AND NOT A WORD, which is the rule
    // paths.cpp's `word_node` keeps and the defect it records is what happens
    // without it: `satellite.console` with no call matched the first child
    // whose argument list was empty, that was `display`, and the whole of
    // `input(">>>", target)` was written as `1.5.1` -- a different program that
    // still parses. Shapes are found by arity, through cache::shape_path().
    for (words::PathId c = words::first_child(parent); c != words::kNoPath;
         c = words::next_sibling(c)) {
        const words::NodeId child = static_cast<words::NodeId>(c);
        if (words::arguments_of(child).empty() &&
            words::spelling_of(child) == word)
            return c;
    }

    // THE ALIASES, FOR THE REASON words_runtime.hpp's find() GIVES. Leaving
    // them out there let `args` take a USER number while walk() answered the
    // language's for the same spelling under the same parent -- two numbers for
    // one word, and the half that was wrong was the half a parser called. The
    // dotted ones cannot be one segment and are skipped, which is the same
    // filter the suggester and the lexer's spelling table both apply.
    for (size_t i = 0; i < words::kAliasCount; i++) {
        const std::string_view text = words::kAliases[i].text;
        if (words::parent_of(words::kAliases[i].of) == parent &&
            words::arguments_of(text).empty() &&
            words::spelling_of(text) == word)
            return static_cast<words::PathId>(words::kAliases[i].of);
    }
    return words::kNoPath;
}

cache::PathMatch Resolver::path_of(NodeIndex node, bool wants_call)
{
    took_ = false;

    // WHERE THE WORD ENDS, AND NOT WHERE THE CHAIN BEGINS. Both would identify
    // the substitution; only one lines up with a node. A chain's ROOT is the
    // same token for `satellite.time.now()` and for the `.some_function()`
    // around it, so keying on it would hand the outer node the inner one's
    // number. The last segment is where the anchor token is -- ast.hpp's
    // "the token that NAMES the node" -- so the end of the substituted words is
    // the end of exactly one node's anchor, whichever node that turns out to be.
    const NodeIndex anchored = wants_call ? ast_[node].a : node;
    if (anchored != kNoNode && !marks_.empty()) {
        const uint32_t ends = ast_.token_of(anchored).end;
        if (const cache::Mark *mark = mark_ending_at(marks_, ends);
            mark != nullptr && names_a_call(mark->id) == wants_call) {
            const std::string_view args =
                words::arguments_of(static_cast<words::NodeId>(mark->id));
            const bool absorbs =
                cache::is_absorber(args) && wants_call &&
                ast_.list_size(ast_[node].b) == 1 &&
                ast_[ast_.list_at(ast_[node].b, 0)].kind == NodeKind::Satellite;
            out_.from_cache++;
            took_ = true;
            return cache::PathMatch{mark->id, wants_call, absorbs,
                                    cache::arity_of(args)};
        }
    }

    cache::PathMatch found = cache::language_path(ast_, node);
    // COUNTED ONLY WHEN THERE WAS A PATH TO WALK. Every selector call in a
    // program reaches this function and comes back with nothing -- the chain is
    // not rooted at the reserved word, which paths.cpp refuses in one line --
    // and counting those would make the number a count of expressions rather
    // than of walks the `.satc` could have saved.
    if (found.found() || found.under != words::kNoPath)
        out_.walked++;
    return found;
}

// ONE TYPE NODE AND NOT ITS ARGUMENTS, which is the split that lets type_of()
// below keep its own stack. It answers kNoPath for every form there is nothing
// to descend into -- an absent node, a form this milestone does not check, and
// a name the numbering does not have -- so "walk the generic arguments" and
// "this came back with a path" are the same test rather than two.
words::PathId Resolver::type_at(NodeIndex node)
{
    if (node == kNoNode || ast_[node].kind != NodeKind::Type)
        return words::kNoPath;

    const Node &n = ast_[node];

    // NO TYPE SPACE MEANS ONE OF TWO FORMS, AND SINCE M26 ONE OF THEM ANSWERS.
    // parser_types.cpp: `satellite` alone is the singleton runtime type, and a
    // bare IDENT is a spacesuit named by hand -- "is_reserved_word on the node's
    // own token tells them apart in one integer compare".
    //
    // THE SPACESUIT IS THE SECOND PLACE IN THE LANGUAGE WHERE A BARE IDENTIFIER
    // MEANS SOMETHING OTHER THAN A USER'S OWN NAME, which PLAN §8's M26 entry
    // names as the thing to settle -- §7.7's spellings of `arguments` are the
    // first. Its rule for all of them is the one kept here: **the language
    // RECOGNISES a name rather than introducing one.** So this is a lookup in
    // the table pass 2 built, and a bare name that is not a suit is still no
    // type at all rather than a new kind of declaration.
    //
    // PASS 2 HAS ALREADY RUN AND THAT IS WHY THIS CAN ANSWER. DESIGN §7.3 puts
    // spacesuits second and bodies fourth for exactly this reason, one row
    // further along than the capsules it was written for: a field or a local
    // may be declared of a suit written further down the file.
    if (n.a == words::kNoSpelling) {
        const std::string_view bare = ast_.text_of(node);
        if (const Capsule *found = suit_named(bare)) {
            info(node).path = found->path;
            info(node).type = found->path;
            info(node).origin = Origin::Bound;
            return found->path;
        }
        return words::kNoPath;
    }

    const words::NodeId space =
        n.a == words::spelling_id(words::NodeId::CONTAINER) ? words::NodeId::CONTAINER
                                                            : words::NodeId::VARIABLE;
    const std::string_view word = ast_.text_of(node);

    words::PathId id = words::kNoPath;
    if (const cache::Mark *mark =
            marks_.empty() ? nullptr
                           : mark_ending_at(marks_, ast_.token_of(node).end);
        mark != nullptr && !names_a_call(mark->id)) {
        id = mark->id;
        info(node).origin = Origin::Cached;
        out_.from_cache++;
    } else {
        id = child_named(space, word);
        info(node).origin = Origin::Walked;
        out_.walked++;
    }

    if (id == words::kNoPath) {
        problem<errors::Code::RESOLVE_NO_SUCH_TYPE>(node, word,
                                                    words::path_text(space));
        if (const std::string_view near =
                errors::suggest(static_cast<words::PathId>(space), word);
            !near.empty())
            suggest(near);
        return words::kNoPath;
    }

    info(node).path = id;
    info(node).type = id;
    return id;
}

// A type and everything inside it. THE GENERIC ARGUMENTS ARE TYPES AND ARE
// CHECKED, AND THEIR COUNT IS NOT: `satellite.container.map<x>` with one
// argument where two are meant is a TYPE rule, and DESIGN §8 is the value model
// -- M9's. The M6 draft checks the counts here and had to invent the rules to
// do it, including which types may be a map's key; this milestone asks only
// whether every name is a name the language has, which is what a resolve is for.
//
// AND `list<list<list<...>>>` IS A DEPTH THE PROGRAM CHOOSES, so this walk
// keeps its own stack too -- DESIGN §7.5, and NO_LIMITS §3 lists this function
// beside the four bigger ones. Pushed in reverse, so the arguments are checked
// left to right and the diagnostics come out in the order they are written.
words::PathId Resolver::type_of(NodeIndex node)
{
    const words::PathId id = type_at(node);
    if (id == words::kNoPath)
        return id;

    std::vector<NodeIndex> pending;
    const auto descend = [&](NodeIndex type) {
        const ListId arguments = ast_[type].b;
        for (uint32_t i = ast_.list_size(arguments); i-- > 0;)
            pending.push_back(ast_.list_at(arguments, i));
    };

    descend(node);
    while (!pending.empty()) {
        const NodeIndex argument = pending.back();
        pending.pop_back();
        if (type_at(argument) != words::kNoPath)
            descend(argument);
    }
    return id;
}

bool Resolver::fold_option(NodeIndex call_node, NodeIndex target,
                           words::PathId under)
{
    const Node &n = ast_[call_node];
    if (ast_.list_size(n.b) == 0)
        return false;
    const NodeIndex first = ast_.list_at(n.b, 0);
    if (ast_[first].kind != NodeKind::String)
        return false;

    const std::string_view word = ast_.text_of(target);
    const std::string_view option = ast_.text_of(first);
    const words::NodeId parent = static_cast<words::NodeId>(under);

    // THE `.satc` ALREADY DECIDED THIS -- M19.6, and it is the whole of what
    // the option token buys. `0#down` in the file says that the string sitting
    // at position 0 is an OPTION and not an argument, which is the question the
    // rest of this function exists to answer: `takes_options()` against
    // words.def's sixth list, the sibling scan that collects `down, up` for a
    // message, and the bare-word retry below are all skipping the same fact.
    //
    // ONE WALK STILL HAPPENS AND SATC.md §5.1 SAYS WHY IT MUST. The file names
    // the OPTION and not the ROW -- the author's call -- so `sort_down`'s
    // number is not in it, and `1 4 2 5` still has to be looked up under the
    // receiver's type. What is gone is the DECIDING; what remains is a lookup,
    // which is the same shape a Mark leaves behind for a path.
    if (!folded_.empty() &&
        std::binary_search(folded_.begin(), folded_.end(),
                           ast_.token_of(target).end)) {
        const std::string named = std::string(word) + "_" + std::string(option);
        const int taken = static_cast<int>(ast_.list_size(n.b)) - 1;
        out_.from_cache++;

        cache::PathMatch said = cache::shape_path(parent, named, taken, false);

        // AND THE BARE WORD SECOND, WHICH IS M16'S RULE AND NOT A SECOND GUESS.
        // WORD_NUMBERS §1.5: "a fold may land on the bare word it was spelled
        // from", because `sort_up()` does not exist and §2.2 says `sort()`
        // ALREADY IS sorting up. example/containers.satl writes all three
        // shapes on three lines -- `sort()`, `sort(0#down)`, `sort(0#up)` --
        // and without this the third would take the token's word for it, miss
        // `sort_up`, and fall through to decide the whole thing again. The file
        // has already said this is an option; what is left is which row, and
        // that is two lookups rather than a decision.
        if (!said.found())
            said = cache::shape_path(parent, word, taken, false);

        if (said.found()) {
            info(target).path = said.id;
            info(target).origin = Origin::Cached;
            info(target).folded_option = true;
            return true;
        }

        // THE FILE SAID A FOLD AND THE NUMBERING NO LONGER HAS IT, which is a
        // stale `.satc` and not a broken one. Falling through re-decides from
        // the source's own words and gets the right answer or the right error,
        // which is §4's "the walk is the fallback" applied to a decision rather
        // than to a number. Not a diagnostic: the numbering's digest already
        // invalidates every file on the machine when a row moves, so reaching
        // here at all means something subtler, and a program that runs
        // correctly is not the place to talk about it.
    }

    // IS THIS A WORD THAT TAKES AN OPTION AT ALL? ASKED OF A TABLE, AND THE
    // TABLE IS words.def's SIXTH LIST -- corrected at M19, 2026-09-08, from a
    // guess that was wrong about two words out of the three it fired on.
    //
    // THE GUESS WAS "a word takes options when the rows beside it are spelled
    // `<word>_<something>`", and it shipped in M16. It is right about `sort`,
    // which has `sort_down` and `sort_up`. It is wrong about `remove`, whose
    // neighbours `remove_first`, `remove_last` and `remove_at` are three
    // separate verbs -- so `parts.remove("bolt")`, taking a string out of a
    // list of strings, was refused with S0524 "`bolt` is not an option remove
    // has", and M18's help pass then recorded that as a fact about the language
    // rather than as the bug it was. And it is wrong about `write` `1 6 2 11`,
    // which M19 minted beside `write_line` `1 6 2 4` and which refused
    // `f.write("one")` on its first run. Two words in two milestones is a rule
    // that is wrong, not two unlucky names.
    //
    // `contains("x")` was always safe and still is, by a different route: it
    // has no sibling spelled `contains_anything`, and now it is simply not in
    // the list.
    if (!words::takes_options(under, word))
        return false;

    // THE OPTIONS THEMSELVES ARE STILL READ OFF THE SIBLINGS, and for a word
    // the list declares that reading is correct -- `sort_down` and `sort_up`
    // ARE where `down` and `up` come from. A second list naming them would be a
    // second place for the words and the rows to disagree, which is the drift
    // words.def exists to remove.
    const std::string prefix = std::string(word) + "_";
    std::string options;
    for (words::PathId c = words::first_child(parent); c != words::kNoPath;
         c = words::next_sibling(c)) {
        const std::string_view spelling =
            words::spelling_of(static_cast<words::NodeId>(c));
        if (spelling.size() <= prefix.size() ||
            spelling.compare(0, prefix.size(), prefix) != 0)
            continue;
        const std::string_view named = spelling.substr(prefix.size());
        if (options.find(std::string(named)) == std::string::npos) {
            if (!options.empty())
                options += ", ";
            options += named;
        }
    }

    const std::string folded = prefix + std::string(option);
    const int argc = static_cast<int>(ast_.list_size(n.b)) - 1;

    out_.walked++;
    const cache::PathMatch shape = cache::shape_path(parent, folded, argc, false);
    if (shape.found()) {
        info(target).path = shape.id;
        info(target).origin = Origin::Walked;
        info(target).folded_option = true;
        return true;
    }

    // THE OPTION EXISTS AND THE SHAPE DOES NOT, WHICH IS A DIFFERENT SENTENCE
    // AND A DIFFERENT FIX -- the argument S0809 makes one block down in
    // errors.def. `my_list.sort("up")` lands here: `sort_up(key)` is `1 4 2 7`
    // and there is no `sort_up()`, because WORD_NUMBERS §2.2 assigns
    // `sort()` `1 4 2 3` as "ascending, no key" and that IS sorting up. Naming
    // the row that exists is the honest answer; folding to `sort()` would be
    // this milestone deciding that a fold may land on a word other than the one
    // it was spelled from, which is a numbering decision and the author's.
    bool named_at_all = false;
    std::string shapes;
    for (words::PathId c = words::first_child(parent); c != words::kNoPath;
         c = words::next_sibling(c)) {
        const words::NodeId child = static_cast<words::NodeId>(c);
        if (words::spelling_of(child) != folded)
            continue;
        named_at_all = true;
        if (!shapes.empty())
            shapes += " and ";
        shapes += words::text_of(child);
    }

    // THE AUTHOR'S DECISION OF 2026-09-05, AT M16: A FOLD MAY LAND ON THE
    // BARE WORD IT WAS SPELLED FROM, which closes MILESTONES/M7.md §6 item 1.
    // `my_list.sort("up")` retries `sort` at the written count, finds
    // `sort()` `1 4 2 3`, and is that row -- no alias was minted, and this is
    // where the rule lives. WORD_NUMBERS §1.5 carries it.
    //
    // AFTER `named_at_all` AND NOT BEFORE IT, WHICH IS THE WHOLE OF WHAT
    // KEEPS IT NARROW -- and getting that order wrong was caught by
    // resolve_test rather than by reading. `takes_options` says only that
    // SOME sibling is spelled `<word>_<something>`; it does not say that
    // THIS option is one of them. Run ahead of the loop above, the fallback
    // swallowed `sort("sideways")` into `sort()` -- an option the numbering
    // has never heard of, silently becoming a sort. So the condition is that
    // the folded word NAMES A ROW and only its SHAPE is missing: `sort_up`
    // is a row (`sort_up(key)`), `sort_sideways` is not, and the second is
    // still S0524.
    //
    // The risk the rule takes on is written down in WORD_NUMBERS §1.5: this
    // is only correct while a `<word>_<option>` row whose shape is missing
    // MEANS the bare word, which is true of `sort_up` because §2.2 says
    // `sort()` already is it. A future option that CHANGES the meaning must
    // get its own shape row rather than lean on this.
    if (named_at_all) {
        const cache::PathMatch bare =
            cache::shape_path(parent, word, argc, false);
        if (bare.found()) {
            info(target).path = bare.id;
            info(target).origin = Origin::Walked;
            info(target).folded_option = true;
            return true;
        }
    }

    if (named_at_all) {
        const std::string written =
            argc == 0 ? std::string("nothing after it")
                      : (argc == 1 ? std::string("1 argument after it")
                                   : std::to_string(argc) + " arguments after it");
        problem<errors::Code::RESOLVE_NO_SUCH_SHAPE>(first, folded, written,
                                                     shapes);
    }
    else
        problem<errors::Code::RESOLVE_NO_SUCH_OPTION>(first, option, word, options);
    return true;
}

} // namespace satellite::resolve
