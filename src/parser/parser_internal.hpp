#pragma once

// The Parser object, shared by the four translation units that make it up.
// See parser/parser.hpp for what a parse is and what it promises.
//
// SPLIT BY SUBJECT AND NOT BY SIZE: the cursor and the dispatch here and in
// parser.cpp, declarations, statements, expressions. DESIGN §6's grammar splits
// at exactly those three seams -- `top_level`, `statement`, `expression` -- so
// the files are the grammar's own sections rather than an arithmetic over 300
// lines.

#include "abstract_syntax_tree/ast.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "parser/parser.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
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
};

// Which of the eleven a spelling is, or None.
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
    default: break;
    }
    return Segment1::None;
}

class Parser {
public:
    Parser(Ast &ast, words::Words &words) : ast_(ast), words_(words) {}

    void run();

    std::vector<ParseError> take_errors() { return std::move(errors_); }

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
    bool expect_punct(std::string_view text, std::string_view what);
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

    void error(uint32_t token, std::string reason);
    void synchronise();
    bool stop() const;

    // --- top level and declarations (parser_declarations.cpp) ---------------

    NodeIndex top_level();
    NodeIndex include_decl();
    NodeIndex capsule_decl(words::PathId owner);
    NodeIndex spacesuit_decl();
    NodeIndex global_decl();
    NodeIndex section(words::PathId owner);
    ListId suit_body(words::PathId owner);
    NodeIndex suit_member(words::PathId owner);

    // The one call in this milestone that gives a name a number. `owner` is a
    // PathId and not a NodeId because a spacesuit's members are owned by the
    // spacesuit, whose id is the user's -- see the function for what M2's
    // tables can and cannot do with that.
    words::PathId define_name(words::PathId owner, uint32_t token, const char *what);

    // --- types (parser_types.cpp) -------------------------------------------

    NodeIndex type();
    ListId generic_arguments();
    ListId param_list();
    NodeIndex returns_clause();

    // --- statements (parser_statements.cpp) ---------------------------------

    NodeIndex statement();
    NodeIndex block();
    NodeIndex var_decl(NodeIndex declared_type);
    NodeIndex return_stmt();
    NodeIndex assign_or_expression();
    bool at_declaration() const;

    // --- if, while, for (parser_control_flow.cpp) ---------------------------

    NodeIndex if_stmt();
    NodeIndex while_stmt();
    NodeIndex for_stmt();
    uint32_t take_statement_keyword();
    NodeIndex condition(const char *after);
    bool at_else() const;

    // --- expressions (parser_expressions.cpp) -------------------------------

    NodeIndex expression(int min_precedence = 1);
    NodeIndex unary();
    NodeIndex postfix();
    NodeIndex primary();
    ListId argument_list();
    NodeIndex subscript(NodeIndex target, uint32_t opener);

    Ast &ast_;
    words::Words &words_;
    std::vector<ParseError> errors_;
    size_t pos_ = 0;
    uint32_t brackets_ = 0;
    bool panic_ = false;
};

} // namespace satellite
