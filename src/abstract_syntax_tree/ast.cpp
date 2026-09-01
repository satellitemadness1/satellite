// The arena's three writers. See abstract_syntax_tree/ast.hpp for what a Node
// is and why it is 24 bytes.
//
// SMALL ON PURPOSE. Everything about this module that is worth knowing is a
// property of the LAYOUT, and a layout is stated in a header where every
// consumer inherits it; what is left over is three functions that push onto a
// vector. The first satellite's equivalent was a set of constructors, one per
// node type, each taking the fields of that type by name -- which reads better
// at a call site and is why its tree could never be a POD.

#include "abstract_syntax_tree/ast.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace satellite {

NodeIndex Ast::add(NodeKind kind, uint32_t token, uint32_t a, uint32_t b, uint32_t c,
                   uint32_t d)
{
    nodes_.push_back(Node{kind, token, a, b, c, d});
    return static_cast<NodeIndex>(nodes_.size() - 1);
}

ListId Ast::add_list(const std::vector<NodeIndex> &items)
{
    // THE EMPTY LIST IS ALWAYS HANDLE 0 and is never appended, so a program
    // full of `f()` calls costs one word for all of them -- and, more usefully,
    // so that "no children" has exactly one representation. Two encodings of
    // empty is how a comparison of two trees starts answering "different" for
    // trees that are not.
    if (items.empty())
        return kNoList;

    const ListId at = static_cast<ListId>(lists_.size());
    lists_.push_back(static_cast<NodeIndex>(items.size()));
    lists_.insert(lists_.end(), items.begin(), items.end());
    return at;
}

std::string_view Ast::text_of(NodeIndex index) const
{
    return token_of(index).text;
}

namespace {

struct Operator {
    const char *text;
    int precedence;
    BinaryOp op;
};

// THE ONE PLACE IN THE CODE THE OPERATOR SET LIVES -- DESIGN §6.6 is the
// specification it transcribes:
//
//   * / %      4      the four operations, and DESIGN §8.6 specifies them
//   + -        3
//   < > <= >=  2      the greedy two-character operators DESIGN §5.5 permits
//   == !=      1
//
// A chain of `if (op == ...)` arms is the obvious way to write the function
// below and is what FORMAT/CXX.md §1 rules out: a table can be read as a table,
// counted, and compared against the operators DESIGN §5.5 says the lexer will
// produce. `<<` and `>>` are absent because §5.5 refuses them permanently, and
// there is no bitwise or logical row because DESIGN §13 holds that open --
// the lexer hands a `&` over as a Punct, this table gives it 0, and it ends an
// expression and is reported rather than guessed at.
constexpr Operator kBinaryOperators[] = {
    {"==", 1, BinaryOp::Equal},        {"!=", 1, BinaryOp::NotEqual},
    {"<",  2, BinaryOp::Less},         {">",  2, BinaryOp::Greater},
    {"<=", 2, BinaryOp::LessEqual},    {">=", 2, BinaryOp::GreaterEqual},
    {"+",  3, BinaryOp::Add},          {"-",  3, BinaryOp::Subtract},
    {"*",  4, BinaryOp::Multiply},     {"/",  4, BinaryOp::Divide},
    {"%",  4, BinaryOp::Modulo},
};

// ONE ROW PER ENUMERATOR, WHICH IS WHAT MAKES THE ENUM A COLUMN OF THIS TABLE
// RATHER THAN A SECOND LIST OF THE SAME OPERATORS. ast.hpp's note is the
// argument; this is the half a build can check. An operator added to DESIGN
// §6.6 and to the enum but not to the rows fails here, at compile time, rather
// than at the first program that writes it.
static_assert(sizeof(kBinaryOperators) / sizeof(kBinaryOperators[0]) ==
                  static_cast<size_t>(BinaryOp::NotAnOperator),
              "ast.cpp: DESIGN §6.6 has one row per BinaryOp. NotAnOperator is "
              "the count because it is last and is not one of them");

constexpr struct {
    std::string_view text;
    UnaryOp op;
} kUnaryOperators[] = {
    {"-", UnaryOp::Negate},
    {"!", UnaryOp::Not},
};

static_assert(sizeof(kUnaryOperators) / sizeof(kUnaryOperators[0]) ==
                  static_cast<size_t>(UnaryOp::NotAnOperator),
              "ast.cpp: DESIGN §6.6 has two unary operators and UnaryOp has two "
              "before NotAnOperator");

} // namespace

int precedence_of(std::string_view op)
{
    for (const Operator &row : kBinaryOperators)
        if (op == row.text)
            return row.precedence;
    return 0;
}

BinaryOp binary_op_of(std::string_view op)
{
    for (const Operator &row : kBinaryOperators)
        if (op == row.text)
            return row.op;
    return BinaryOp::NotAnOperator;
}

UnaryOp unary_op_of(std::string_view op)
{
    for (const auto &row : kUnaryOperators)
        if (op == row.text)
            return row.op;
    return UnaryOp::NotAnOperator;
}

std::string_view text_of(BinaryOp op)
{
    for (const Operator &row : kBinaryOperators)
        if (row.op == op)
            return row.text;
    return "?";
}

std::string_view text_of(UnaryOp op)
{
    for (const auto &row : kUnaryOperators)
        if (row.op == op)
            return row.text;
    return "?";
}

const char *kind_name(NodeKind kind)
{
    // A SWITCH AND NOT A TABLE INDEXED BY THE ENUM, which is FORMAT/CXX.md §7's
    // reason for a switch stated for a smaller case: -Wall's
    // -Wswitch names a kind added to the enum and forgotten here, at compile
    // time, in this file. An array would have printed "?" and been right about
    // nothing.
    switch (kind) {
    case NodeKind::None:      return "None";
    case NodeKind::Program:   return "Program";
    case NodeKind::Number:    return "Number";
    case NodeKind::String:    return "String";
    case NodeKind::Bits:      return "Bits";
    case NodeKind::Satellite: return "Satellite";
    case NodeKind::Name:      return "Name";
    case NodeKind::Member:    return "Member";
    case NodeKind::Call:      return "Call";
    case NodeKind::Index:     return "Index";
    case NodeKind::Slice:     return "Slice";
    case NodeKind::Unary:     return "Unary";
    case NodeKind::Binary:    return "Binary";
    case NodeKind::Type:      return "Type";
    case NodeKind::VarDecl:   return "VarDecl";
    case NodeKind::Assign:    return "Assign";
    case NodeKind::ExprStmt:  return "ExprStmt";
    case NodeKind::Return:    return "Return";
    case NodeKind::Block:     return "Block";
    case NodeKind::If:        return "If";
    case NodeKind::While:     return "While";
    case NodeKind::For:       return "For";
    case NodeKind::Include:   return "Include";
    case NodeKind::Capsule:   return "Capsule";
    case NodeKind::Spacesuit: return "Spacesuit";
    case NodeKind::Section:   return "Section";
    case NodeKind::Global:    return "Global";
    }
    return "?";
}

} // namespace satellite
