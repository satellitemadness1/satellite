#pragma once

// The Parser object, shared by the four translation units that make it up.
// See parser/parser.hpp for what a parse is and what it promises.
//
// SPLIT BY SUBJECT AND NOT BY SIZE: the cursor and the dispatch here and in
// parser.cpp, declarations, statements, expressions. DESIGN §6's grammar splits
// at exactly those three seams -- `top_level`, `statement`, `expression` -- so
// the files are the grammar's own sections rather than an arithmetic over 300
// lines.
//
// NO RULE HERE RECURSES ON A DEPTH THE PROGRAM CHOOSES -- M8.5, DESIGN §7.5,
// and it is the largest of the four rewrites `SCRATCH.md/NO_LIMITS.md` §5 asks
// for. Recursive descent is recursion by name, so what changed is where the
// depth is kept: THE GRAMMAR HAS FOUR CYCLES and each one now keeps its own
// stack on the heap.
//
//   expressions   expression -> unary -> postfix -> primary -> '(' expression
//                 and through an argument list and a subscript -- one machine
//                 with an operand stack, an operator stack and a frame per
//                 open bracket (parser_expressions.cpp)
//   statements    statement -> block -> statement, and the three compound
//                 forms whose body is a block -- one machine over `open_`
//                 (parser_statements.cpp, with the heads next door)
//   types         type -> generic_arguments -> type (parser_types.cpp)
//   suit bodies   suit_body -> section -> suit_body (parser_declarations.cpp)
//
// AND THE CYCLES ARE NOT NESTED IN EACH OTHER, which is what makes four
// machines enough. Every edge between them runs one way -- a suit body holds a
// capsule, a capsule holds a block, a statement holds an expression, an
// expression holds no statement -- so the deepest a satellite program can drive
// this parser's C++ stack is one frame per machine, whatever it is nested in.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "parser/parser.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace satellite {

// The eleven segment-1 words that have a parse rule of their own -- DESIGN
// §6.1's table, as an enum.
//
// ELEVEN AND NOT TWELVE OR TEN, and the count is the checklist: §6.1's last two
// rows are ONE rule ("a segment-1 word either has a parse rule of its own or it
// does not"), and PLAN M4 records that eight of these appeared in the backlog
// as work with no milestone until somebody counted them against this table.
// `satellite.console`, `satellite.time` and the other thirteen segment-1 words
// are modules and are parsed as expressions, which is the None row.
enum class Segment1 : uint8_t {
    None = 0,
    Variable,   // a type path -- a following bare word is a declaration
    Container,  // likewise
    Library,    // a value path: shared and global state (DESIGN §7.2)
    Statement,  // if, else, while, for
    Include,
    Capsule,
    Spacesuit,
    Return,
    Returns,
    Protected,
    Public,
    Constructor,  // a spacesuit's third section -- 2026-09-12
};

// Which of the twelve a spelling is, or None.
//
// A SWITCH OVER CONSTEXPR CASE LABELS, and it is chosen over an array of pairs
// for the property FORMAT/CXX.md §7 names: two rows sharing a value is
// `error: duplicate case value`, at compile time, naming both. That is not a
// theoretical hazard in this language -- `spelling_id` answers with the LOWEST
// node spelled that way, and words.def spells `main` twice, `system` twice and
// `capsule` twice. None of those three is in this table today. The switch is
// what will say so if one ever is.
//
// AND IT IS ASKED ONLY AT SEGMENT 1, which is the other half of §6.1 and the
// reason the duplicate spellings above are harmless: `satellite.variable.capsule`
// -- the type of a deferred call, `1 6 16` -- carries the same spelling id as
// `satellite.capsule`, and is never a declaration, because nothing asks this
// question about the word at segment 2.
constexpr Segment1 segment1_of(words::SpellingId id)
{
    switch (id) {
    case words::spelling_id(words::NodeId::VARIABLE):  return Segment1::Variable;
    case words::spelling_id(words::NodeId::CONTAINER): return Segment1::Container;
    case words::spelling_id(words::NodeId::LIBRARY):   return Segment1::Library;
    case words::spelling_id(words::NodeId::STATEMENT): return Segment1::Statement;
    case words::spelling_id(words::NodeId::INCLUDE):   return Segment1::Include;
    case words::spelling_id(words::NodeId::CAPSULE):   return Segment1::Capsule;
    case words::spelling_id(words::NodeId::SPACESUIT): return Segment1::Spacesuit;
    case words::spelling_id(words::NodeId::RETURN):    return Segment1::Return;
    case words::spelling_id(words::NodeId::RETURNS):   return Segment1::Returns;
    case words::spelling_id(words::NodeId::PROTECTED): return Segment1::Protected;
    case words::spelling_id(words::NodeId::PUBLIC):    return Segment1::Public;
    case words::spelling_id(words::NodeId::CONSTRUCTOR): return Segment1::Constructor;
    default: break;
    }
    return Segment1::None;
}

class Parser {
public:
    Parser(Ast &ast, words::Words &words,
           words::PathId library = static_cast<words::PathId>(words::NodeId::LIBRARY))
        : ast_(ast), words_(words), library_(library)
    {
    }

    void run();

    std::vector<errors::Diagnostic> take_errors() { return std::move(errors_); }

    // Whether the run stopped short of the end of the file -- parse() turns
    // this into PARSE_TOO_MANY_ERRORS rather than letting the output simply
    // end.
    bool gave_up() const { return stop(); }

private:
    // --- the cursor ---------------------------------------------------------

    const std::vector<Token> &toks() const { return ast_.tokens(); }
    const Token &peek(size_t ahead = 0) const;
    uint32_t here() const { return static_cast<uint32_t>(pos_); }
    bool at_end() const { return peek().kind == TokenKind::End; }
    const Token &advance();

    bool at_punct(std::string_view text, size_t ahead = 0) const;
    bool at_word(size_t ahead = 0) const;
    bool take_punct(std::string_view text);

    // `opener` IS THE TOKEN THIS CLOSER WOULD CLOSE, or kNoOpener.
    //
    // ONE PARAMETER AND TEN CALL SITES, which is why the note is here rather
    // than composed at each of them. DESIGN §9 asks for "notes carrying their
    // own spans" and the commonest one in any parser is "the bracket you did
    // not close is over there" -- so the site that already knows where the `(`
    // was hands the index over and this function decides whether a note is
    // wanted. Written the other way round, ten sites would each have to ask
    // whether the error was actually recorded, because a parser already in
    // panic records nothing and a note attached to nothing lands on the
    // PREVIOUS error.
    bool expect_punct(std::string_view text, std::string_view what,
                      uint32_t opener = kNoOpener);
    uint32_t expect_word(std::string_view what);

    // A `satellite . WORD` opening, and which of §6.1's eleven it is.
    Segment1 opening() const;

    void skip_newlines();
    void end_of_statement();

    // Newlines are tokens (DESIGN §5.6), and that is what enforces §6.2's
    // same-line rule without a comparison. See parser.cpp.
    void open_bracket();
    void close_bracket();

    // --- errors -------------------------------------------------------------

    // Token 0 is the first token of a file, so it cannot also mean "no token" --
    // but the only rule that could report at token 0 is the one for an empty
    // file, which has no bracket to be unclosed. kNoOpener is therefore 0 with
    // that argument written down, rather than a second sentinel.
    static constexpr uint32_t kNoOpener = 0;

    // One thing wrong, at one token, in the words errors.def has for it.
    //
    // THE CODE IS A TEMPLATE PARAMETER AND THAT IS THE POINT. errors::make
    // static_asserts that the arguments match the sentence's holes, so a site
    // that hands two strings to a three-hole sentence is a compile error naming
    // the code -- the check the first satellite's 199 sites could not have had,
    // because there the sentence WAS the argument.
    template <errors::Code C, typename... Args>
    void error(uint32_t token, Args &&...arguments)
    {
        fresh_ = !panic_;
        if (panic_)
            return;
        panic_ = true;
        errors_.push_back(
            errors::make<C>(span_of(token), std::forward<Args>(arguments)...));
    }

    // Attach a note, or a suggestion, to the error just reported.
    //
    // `fresh_` IS WHY THESE ARE FUNCTIONS AND NOT `errors_.back()`. error()
    // above records NOTHING while the parser is already lost -- one error per
    // synchronisation, which parser.cpp argues for -- so a caller that reached
    // for the last diagnostic unconditionally would hang its note on whichever
    // error came before, pointing a second caret at a line with nothing to do
    // with it. Both of these are no-ops unless the error immediately before
    // them was actually recorded.
    void attach(errors::Note remark);

    // DESIGN §4.6, at the four places in this parser where a word was looked up
    // against the trie and was not there.
    void suggest(uint32_t token, words::PathId under);

    errors::Span span_of(uint32_t token) const;

    void synchronise();
    bool stop() const;

    // --- top level and declarations (parser_declarations.cpp) ---------------

    NodeIndex top_level();
    NodeIndex include_decl();
    NodeIndex capsule_decl(words::PathId owner);
    NodeIndex spacesuit_decl();
    NodeIndex global_decl();
    ListId suit_body(words::PathId owner);
    NodeIndex suit_member(words::PathId owner);
    NodeIndex constructor_decl(words::PathId owner);

    // The one call in this milestone that gives a name a number. `owner` is a
    // PathId and not a NodeId because a spacesuit's members are owned by the
    // spacesuit, whose id is the user's -- see the function for what M2's
    // tables can and cannot do with that.
    words::PathId define_name(words::PathId owner, uint32_t token, const char *what);

    // --- types (parser_types.cpp) -------------------------------------------

    // A type and every type inside it -- the `<...>` nesting is kept on a
    // vector in the function rather than on the C++ stack, so
    // `list<list<list<...>>>` has no depth of its own. generic_arguments() is
    // gone with the recursion: it was one half of the cycle.
    NodeIndex type();
    ListId param_list();
    NodeIndex returns_clause();

    // --- statements (parser_statements.cpp) ---------------------------------

    // WHAT A HALF-BUILT STATEMENT LOOKS LIKE, and it is the whole of what the
    // C++ stack used to hold for one. A Block is collecting its statements; the
    // three compound forms have their head and are waiting for the block that
    // is above them on this stack. `before` is the block loop's guard against a
    // rule that consumes nothing, kept per frame because each block has its own.
    struct Open {
        enum class Kind : uint8_t { Block, If, While, For };
        Kind kind = Kind::Block;
        uint32_t at = 0;              // the '{', or the statement's keyword
        size_t before = 0;            // Block: where the current statement began
        NodeIndex a = kNoNode;        // If/While: the test.  For: the init
        NodeIndex b = kNoNode;        // If: the then block.  For: the test
        NodeIndex c = kNoNode;        // For: the step
        bool otherwise = false;       // If: the then block is done, this is the else
        std::vector<NodeIndex> items; // Block
    };

    NodeIndex block();
    bool open_block();
    NodeIndex var_decl(NodeIndex declared_type);
    NodeIndex return_stmt();
    NodeIndex assign_or_expression();
    bool at_declaration() const;

    // --- if, while, for (parser_control_flow.cpp) ---------------------------

    // A HEAD AND NOT A STATEMENT, which is what the machine next door needs:
    // each reads its keyword and its parenthesised part, pushes the frame that
    // remembers them, and leaves the body to the block the machine opens after
    // it. `false` means nothing was pushed and the statement is kNoNode.
    bool if_head();
    bool while_head();
    bool for_head();
    uint32_t take_statement_keyword();
    NodeIndex condition(const char *after);
    bool at_else() const;

    // --- expressions (parser_expressions.cpp) -------------------------------

    // ONE FUNCTION WHERE THERE WERE SIX. unary(), postfix(), primary(),
    // argument_list() and subscript() were the cycle, and a machine cannot be
    // half of one -- the operand it is part way through building has to be
    // reachable from wherever the next token is read.
    NodeIndex expression();

    Ast &ast_;
    words::Words &words_;

    // WHERE THIS FILE'S OWN NAMES ARE NUMBERED -- parser.hpp's parse() says why.
    words::PathId library_;
    std::vector<errors::Diagnostic> errors_;

    // Where each name this program declared was declared, so a second
    // declaration can point at the first.
    //
    // HERE AND NOT IN words::Words, which is the choice worth recording. A
    // PathId's number is the numbering's business and words_runtime.hpp is
    // careful to hold nothing else -- it does not know what a token is and a
    // `.satc` reader that includes it should not have to. Where a name was
    // WRITTEN is a fact about one file, which is the parser's, and it dies with
    // the parse the way a token index has to.
    std::vector<std::pair<words::PathId, uint32_t>> declared_at_;

    // The statement machine's frames. A MEMBER AND NOT A LOCAL, because the
    // three heads that push one live next door -- and block() takes the size it
    // found as its floor, so a capsule declared inside a spacesuit parses its
    // body without seeing the frames of whatever is around it.
    std::vector<Open> open_;

    size_t pos_ = 0;
    uint32_t brackets_ = 0;
    bool panic_ = false;
    bool fresh_ = false;
};

} // namespace satellite
