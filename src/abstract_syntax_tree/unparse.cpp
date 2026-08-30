// Printing a tree back as a program. See abstract_syntax_tree/unparse.hpp for
// what round-trip means and why it is the milestone's done-when.
//
// EVERY LITERAL IS PRINTED FROM ITS TOKEN'S `text`, WHICH IS THE SPELLING THE
// FILE USED. lexer.hpp keeps both halves of a string literal for exactly this
// reason and says so: `str` is the value the program means and `text` is what
// the file says, expansion is not reversible -- "a\nb" and a body with a real
// newline in it expand to the same SatString -- so a printer that read `str`
// would rewrite the source it was handed. The same rule is why `x0009` comes
// back as `x0009` and not as `x9`: DESIGN §8.5 makes the width part of the
// value, and the token is where the width is.
//
// BRACKETS ARE PUT BACK FROM PRECEDENCE AND NOT FROM MEMORY, because no node
// records that a parenthesis was written (parser_expressions.cpp says why). A
// child needs brackets when it binds LOOSER than its parent, and the right-hand
// child needs them when it binds EQUALLY TOO -- `a - (b - c)` must not come
// back as `a - b - c`, which is the one case that silently changes an answer.

#include "abstract_syntax_tree/unparse.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace satellite {

namespace {

// Above the highest binary level in ast.cpp's table, so that "does this child
// bind looser than its parent" is true of every operator when the parent is a
// unary.
constexpr int kBindsTighterThanAny = 99;

class Printer {
public:
    explicit Printer(const Ast &ast) : ast_(ast) {}

    std::string take() { return std::move(out_); }

    void program(NodeIndex node)
    {
        const ListId items = ast_[node].a;
        for (uint32_t i = 0; i < ast_.list_size(items); i++) {
            if (i > 0)
                out_ += "\n";
            declaration(ast_.list_at(items, i));
        }
    }

    void declaration(NodeIndex node)
    {
        const Node &n = ast_[node];
        switch (n.kind) {
        case NodeKind::Include:
            line("satellite.include(" + expression(n.a) + ")");
            return;
        case NodeKind::Global:
            line("satellite.library." + text(node) +
                 (n.b != kNoNode ? " = " + expression(n.b) : ""));
            return;
        case NodeKind::Capsule:
            capsule(node);
            return;
        case NodeKind::Spacesuit:
            spacesuit(node);
            return;
        default:
            statement(node);
            return;
        }
    }

    void capsule(NodeIndex node)
    {
        const Node &n = ast_[node];
        // THE RESERVED NAME IS THE ONE THE LANGUAGE HAS A NUMBER FOR, and
        // is_language_word on the path id is how that is asked -- one compare
        // against the frozen half's boundary, which words_nodes.hpp calls the
        // predicate anything about to write a PathId down has to ask first.
        std::string head = "satellite.capsule ";
        if (words::is_language_word(n.a))
            head += "satellite.";
        head += text(node) + "(";
        for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
            if (i > 0)
                head += ", ";
            const NodeIndex param = ast_.list_at(n.b, i);
            head += type(ast_[param].a) + " " + text(param);
        }
        head += ")";
        if (n.c != kNoNode)
            head += " satellite.returns(" + type(n.c) + ")";
        line(head);
        block(n.d);
    }

    void spacesuit(NodeIndex node)
    {
        const Node &n = ast_[node];
        std::string head = "satellite.spacesuit " + text(node);
        if (n.c != kNoNode)
            head += "(" + text(n.c) + ")";
        line(head);
        members(n.b);
    }

    // A spacesuit's body, or a section's -- one function, because the parser
    // reads them with one loop and for the reason it records: a section holds
    // what a suit block holds.
    void members(ListId items)
    {
        line("{");
        indent_++;
        for (uint32_t i = 0; i < ast_.list_size(items); i++) {
            if (i > 0)
                out_ += "\n";
            const NodeIndex item = ast_.list_at(items, i);
            if (ast_[item].kind == NodeKind::Section) {
                line("satellite." + text(item));
                members(ast_[item].a);
            } else {
                declaration(item);
            }
        }
        indent_--;
        line("}");
    }

    void block(NodeIndex node)
    {
        line("{");
        indent_++;
        const ListId statements = ast_[node].a;
        for (uint32_t i = 0; i < ast_.list_size(statements); i++)
            declaration(ast_.list_at(statements, i));
        indent_--;
        line("}");
    }

    void statement(NodeIndex node)
    {
        const Node &n = ast_[node];
        switch (n.kind) {
        case NodeKind::VarDecl:
            line(type(n.a) + " " + text(node) +
                 (n.b != kNoNode ? " = " + expression(n.b) : ""));
            return;
        case NodeKind::Assign:
            line(expression(n.a) + " = " + expression(n.b));
            return;
        case NodeKind::ExprStmt:
            line(expression(n.a));
            return;
        case NodeKind::Return:
            line("satellite.return(" +
                 (n.a != kNoNode ? expression(n.a) : std::string()) + ")");
            return;
        case NodeKind::Block:
            block(node);
            return;
        case NodeKind::If:
            line("satellite.statement.if (" + expression(n.a) + ")");
            block(n.b);
            if (n.c != kNoNode) {
                line("satellite.statement.else");
                // AN ELSE-IF IS AN If IN THE else SLOT and is printed as one --
                // on its own line, which is legal because the parser crosses
                // the newline between `else` and what follows it. Printing
                // `else if` on one line would be the C shape and would make
                // the tree's own nesting invisible.
                statement(n.c);
            }
            return;
        case NodeKind::While:
            line("satellite.statement.while (" + expression(n.a) + ")");
            block(n.b);
            return;
        case NodeKind::For:
            // The init and the step are statements written inline, which is the
            // one place in the grammar where a statement has no line of its own.
            line("satellite.statement.for (" + one(n.a) + "; " +
                 (n.b != kNoNode ? expression(n.b) : std::string()) + "; " +
                 one(n.c) + ")");
            block(n.d);
            return;
        default:
            line(expression(node));
            return;
        }
    }

    std::string expression(NodeIndex node) const
    {
        const Node &n = ast_[node];
        switch (n.kind) {
        case NodeKind::Number:
        case NodeKind::Bits:
        case NodeKind::Name:
            return text(node);
        case NodeKind::String:
            // The body as written, quotes back around it. The escapes in it
            // were never expanded on this side of the tree.
            return "\"" + text(node) + "\"";
        case NodeKind::Satellite:
            return "satellite";
        case NodeKind::Member:
            return expression(n.a) + "." + text(node);
        case NodeKind::Call: {
            std::string out = expression(n.a) + "(";
            for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
                if (i > 0)
                    out += ", ";
                out += expression(ast_.list_at(n.b, i));
            }
            return out + ")";
        }
        case NodeKind::Index:
            return expression(n.a) + "[" + expression(n.b) + "]";
        case NodeKind::Slice:
            return expression(n.a) + "[" +
                   (n.b != kNoNode ? expression(n.b) : std::string()) + ":" +
                   (n.c != kNoNode ? expression(n.c) : std::string()) + "]";
        case NodeKind::Unary:
            // ABOVE EVERY BINARY LEVEL, so a binary operand is always
            // bracketed: `-(a + b)` is not `-a + b`.
            return text(node) + bracketed(n.a, kBindsTighterThanAny, false);
        case NodeKind::Binary: {
            const int level = precedence_of(text(node));
            return bracketed(n.a, level, false) + " " + text(node) +
                   " " + bracketed(n.b, level, true);
        }
        case NodeKind::Type:
            return type(node);
        default:
            // NOT REACHABLE FROM A PARSE, and named rather than silently
            // printed as something plausible: every kind above is one DESIGN
            // §6's `expression` can produce, and a statement in an expression
            // slot is a tree that was built by hand and is wrong.
            return "<" + std::string(kind_name(n.kind)) + " is not an expression>";
        }
    }

    std::string type(NodeIndex node) const
    {
        if (node == kNoNode)
            return std::string();
        const Node &n = ast_[node];
        // No type space: either the singleton `satellite` or a spacesuit named
        // bare, and the token says which without a third field.
        if (n.a == words::kNoSpelling)
            return text(node);

        std::string out = "satellite." +
                          std::string(words::spelling_of(
                              static_cast<words::NodeId>(n.a))) +
                          "." + text(node);
        if (n.b == kNoList)
            return out;
        out += "<";
        for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
            if (i > 0)
                out += ", ";
            out += type(ast_.list_at(n.b, i));
        }
        return out + ">";
    }

    // One node inline: an expression, or one of the three statements that can
    // be written inside a `for` head. A block-shaped statement has no inline
    // form and comes back as its kind.
    std::string one(NodeIndex node) const
    {
        if (node == kNoNode)
            return std::string();
        const Node &n = ast_[node];
        switch (n.kind) {
        case NodeKind::VarDecl:
            return type(n.a) + " " + text(node) +
                   (n.b != kNoNode ? " = " + expression(n.b) : "");
        case NodeKind::Assign:
            return expression(n.a) + " = " + expression(n.b);
        case NodeKind::ExprStmt:
            return expression(n.a);
        default:
            return expression(node);
        }
    }

private:
    // A child, with brackets when it binds looser than its parent -- or as
    // tightly, on the right, which is what makes `a - (b - c)` survive.
    std::string bracketed(NodeIndex node, int level, bool on_the_right) const
    {
        const Node &n = ast_[node];
        const int child = n.kind == NodeKind::Binary ? precedence_of(text(node))
                                                     : 0;
        const bool needs = n.kind == NodeKind::Binary &&
                           (child < level || (on_the_right && child == level));
        return needs ? "(" + expression(node) + ")" : expression(node);
    }

    // A STRING AND NOT A string_view, because every use of it here is a
    // concatenation and a view does not concatenate with a literal. The cost is
    // one copy per token printed, in the one function in the tree whose job is
    // to build a string.
    std::string text(NodeIndex node) const { return std::string(ast_.text_of(node)); }

    void line(const std::string &text)
    {
        out_.append(static_cast<size_t>(indent_) * 4, ' ');
        out_ += text;
        out_ += "\n";
    }

    const Ast &ast_;
    std::string out_;
    int indent_ = 0;
};

} // namespace

std::string unparse(const Ast &ast)
{
    if (ast.root() == kNoNode)
        return std::string();
    Printer printer(ast);
    printer.program(ast.root());
    return printer.take();
}

std::string unparse(const Ast &ast, NodeIndex node)
{
    Printer printer(ast);
    return printer.one(node);
}

} // namespace satellite
