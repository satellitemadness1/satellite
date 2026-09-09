#pragma once

// A dotted chain in the tree, matched against the numbering -- PLAN M4.5.
//
// SATC.md §5.1's FIRST FOUR STEPS ARE THIS FILE and its fifth -- "walk the trie
// and emit" -- is write.cpp doing something with what this returns. The four
// are: collapse aliases, classify every dotted chain, absorb language-owned
// arguments, slot by arity. §5.1 states them in that order and says why, and
// the order is kept below rather than reasoned about again at each call.
//
// THIS IS NOT RESOLVE AND MUST NOT BECOME IT, which SATC.md §3.2 spends a
// section on because the whole file format depends on it. A PATH is rooted at
// `satellite` and resolves with no context, so substituting its number is sound
// anywhere; a SELECTOR is a bare word after a receiver -- `sort` in
// `my_list.sort()` -- and reaching its number needs the receiver's TYPE, which
// nothing has decided yet at M4.5. So a chain that does not start at the
// reserved word is refused here in one line and stays sugar in the file. A
// version of this function that grew a receiver argument would be resolve, and
// the `.satc` it wrote would name a handler -- SATC.md §7's one prohibition.
//
// WHY NOT words::walk(). The trie's own walk takes a path as TEXT and matches a
// call shape by its argument list's SPELLING -- `input(prompt, target)` against
// the row spelled that way. A program does not write the row's spelling, it
// writes `input("name: ", x)`, so what the writer has is an ARITY and not a
// text. §5.1 step 4 says so in as many words: "slot by arity ... counting, not
// resolving". Everything else here is walk()'s rules re-stated over the tree
// instead of over a string, and where the two must agree -- the alias table,
// siblings before children -- the comments below name the function next door
// that says why.

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace satellite::cache {

// What a postfix chain turned out to be.
//
// `id` IS kNoPath FOR EVERYTHING THE LANGUAGE DOES NOT OWN, which is the answer
// for a user's capsule call, for a selector, and for a language path written in
// a shape the numbering does not have. All three are written into the `.satc`
// as they were written in the source -- the first two because SATC.md §3 says
// a name stays a name, and the third because a program the numbering cannot
// account for is M7's to refuse and this milestone's to preserve intact.
struct PathMatch {
    words::PathId id = words::kNoPath;

    // Whether the number covers the Call node it was asked about, rather than
    // only the Member chain underneath it. `satellite.console.input()` is one
    // number, 1 5 2, and the parentheses are part of what that number says;
    // `satellite.console.display("x")` is 1 5 1 with a call still to print.
    bool takes_call = false;

    // Whether the row's argument list is the reserved word itself -- §5.1 step
    // 3, "absorb language-owned arguments: include(satellite) -> 1.1.1". The
    // argument is not written, because the NUMBER is what names it.
    bool absorbs_argument = false;

    // The row's argument list, or -1 when the row has none. A writer prints the
    // call's arguments when this is above 0, and prints nothing when it is 0 --
    // `1.5.2` already says `input()`. At -1 the call is the program's own and
    // is printed exactly as written, empty parentheses included.
    int shape_arity = -1;

    // WHERE THE WALK STOPPED, WHICH IS THE HALF THIS FILE USED TO THROW AWAY.
    // The comment below in language_path() has said since M4.5 that a path the
    // numbering cannot account for is "M7's to refuse with M5's did-you-mean
    // over the node the segment failed under" -- and it could not say which
    // node that was, so M7 would have had to walk the chain a second time to
    // find out. `under` is the trie node the failing segment was looked for
    // under and `at` is the Member node that named it; both stay empty when the
    // chain was never a language path at all, which is every selector and every
    // user name and is the majority answer.
    //
    // THE CACHE IGNORES BOTH, and that is the point of adding them here rather
    // than writing a second walk next door. One function decides what a path
    // is; two would disagree the day a row grows an argument list.
    words::PathId under = words::kNoPath;
    NodeIndex at = kNoNode;

    bool found() const { return id != words::kNoPath; }
};

// The language-owned path the chain ENDING AT `node` names, if it names one.
//
// ASKED FROM THE OUTSIDE IN, which is the direction a printer already walks and
// is what makes `satellite.time.now().some_function()` fall out with no rule of
// its own: the outer chain does not match, so the printer prints `.some_function`
// around whatever the inner one came back as, and the inner one is a number.
// DESIGN §6.2 makes Member and Call peers for the same reason.
PathMatch language_path(const Ast &ast, NodeIndex node);

// How many arguments a row's argument list names, or -1 when it has none.
//
// "()" AND "" ARE DIFFERENT ANSWERS, which words_nodes.hpp says at length and
// this function is where the difference is converted into a number a comparison
// can use. "" is a word written with no call at all -- `satellite.console.display`,
// whose number is the whole of it -- and comes back as -1 so that it can never
// compare equal to a call's argument count, not even a call with no arguments.
//
// IN THE HEADER BECAUSE BOTH DIRECTIONS NEED IT. paths.cpp asks it on the way
// out, to slot a call by arity (SATC.md §5.1 step 4); read.cpp asks it on the
// way back, to decide whether a number already says its own parentheses. A
// second copy would be a second place the difference between "()" and "" lives,
// and getting that difference wrong is the defect paths.cpp records.
constexpr int arity_of(std::string_view args)
{
    if (args.size() < 2)
        return -1;
    const std::string_view inner = args.substr(1, args.size() - 2);
    if (inner.empty())
        return 0;
    int count = 1;
    for (const char c : inner)
        if (c == ',')
            count++;
    return count;
}

// Whether a row's argument list is the reserved word itself.
//
// SATC.md §5.1 STEP 3 IS ONE ROW SHAPE AND NOT A CLASS OF THEM. The numbering
// has exactly two: `satellite.include(satellite)` 1 1 1 and
// `satellite.return(satellite)` 1 15 1, and words.def introduces the pair
// together -- "both mean the reserved word and not a user value". Absorbing the
// argument is legal precisely because the number already names it.
constexpr bool is_absorber(std::string_view args)
{
    return args == "(satellite)";
}

// The character that says the token after it is a word and not a value.
//
// WITHOUT IT THE FORMAT IS AMBIGUOUS, AND THAT WAS FOUND BY WRITING IT.
// SATC.md §1.1 wrote a path as bare digits joined by dots -- `1.5.1` for
// satellite.console.display -- and `satellite.console` is `1 5`, which closes
// up to `1.5`, which is also the float one-and-a-half. Both are legal tokens in
// the same file and example/super_advanced.satl already puts a float and a path
// within three lines of each other. It happens to be decidable today, because
// all 24 two-segment paths are namespaces and a namespace is never a value --
// but that is a property of the numbering that nothing enforces, and WORD_NUMBERS
// §3 lets a numbering grow. One character settles it permanently instead.
//
// `#` BECAUSE NO SATELLITE PROGRAM CAN CONTAIN ONE. The lexer gives it back as
// Punct(#) and the parser has no rule that accepts it, so a `#` in a file is
// proof the file is a `.satc` and not a source -- which is the property a
// marker is worth having at all.
//
// Decided by the author on 2026-08-30, against the alternative of leaving the
// file as §1.1 wrote it and having the reader disambiguate by grammar position.
inline constexpr char kPathMark = '#';

// THE OPTION TOKEN -- `0#down`, M19.6, and the author's spelling. SATC.md §3
// gains a row here rather than losing one: `0#down` is not a literal, it is a
// third kind of token beside `#1.4.2.5` and `"down"`, so "literals stay
// literal" is untouched and the file gains a way to say what resolve decided.
//
// `0#` BECAUSE `#` ALREADY MEANS "a number follows" AND AN OPTION IS NOT ONE.
// The author's sentence is the whole derivation -- "`#` stands for a number in
// a `.satc` file, so `0#` stands for an option" -- and the `0` is readable as
// the numbering's own `0`, which WORD_NUMBERS §1.3 already spends on "nothing
// in that position".
//
// NO QUOTES, AND THE AUTHOR NAMED THE REASON ON 2026-09-08: the prefix has
// already said this is an option rather than a string, and `0#"down"` would be
// the file saying the same thing twice in two notations that could disagree. It
// would also put a quote inside a token, which is the one thing that would make
// §3's line unreadable from the other side.
//
// WHAT MAKES IT UNAMBIGUOUS IS A LETTER AFTER IT, and that is worth stating
// because the reader scans bytes rather than tokens. `0` is an ordinary number
// literal in a `.satc` and `#` begins a path, so `0#` could in principle be a
// literal `0` butted against a path -- except that a path is `#` then DIGITS
// and an option is `#` then a LETTER, and the writer never emits a number
// against a path with no operator between them. unnumber.cpp checks both ends:
// a letter after, and no identifier character before.
inline constexpr char kOptionMark[] = "0#";

// Whether `c` may appear in an option word -- the identifier alphabet, which is
// DESIGN §5.1's and includes the underscore because `read_append` is one.
inline constexpr bool is_option_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

// The number as a `.satc` writes it -- `#` then WORD_NUMBERS §2.2's "1 5 1"
// with the spaces closed up to dots, so that a path is ONE token in the file.
//
// SATC.md §1.1 IS THE AUTHORITY OVER THE SPELLING and it is a format decision
// rather than a cosmetic one: "segments are joined with `.`, so a path is one
// token". A reader that had to know how many segments to collect would need the
// numbering in order to lex the file.
std::string number_text(words::PathId id);

// The shape a language word takes when it is written with `argc` arguments.
//
// FOR THE TWO STATEMENT FORMS THE PARSER DOES NOT BUILD AS A CHAIN, and there
// are exactly two: `satellite.return(x)` is a Return node and
// `satellite.include(satellite)` is an Include node, so neither ever reaches
// language_path() above and both still have to find their row. That they are
// also the numbering's only two absorbers (§5.1 step 3) is not a coincidence --
// words.def introduces the pair together, because both mean the reserved word
// rather than a user value, and the reserved word is what a statement form is
// for.
//
// `argc` IS -1 FOR A WORD WRITTEN WITH NO CALL AT ALL, which is the same
// convention PathMatch::shape_arity uses and for the same reason.
PathMatch shape_path(words::NodeId under, std::string_view word, int argc,
                     bool argument_is_satellite);

} // namespace satellite::cache
