// Parser top-level declarations (include, capsule, spacesuit, global) -- Milestone 4 Prototype.

#include "parser_internal.hpp"

#include <utility>

namespace satellite {

namespace {
constexpr size_t kMaxErrors = 50;
}

NodeIndex Parser::parse_include()
{
    const size_t first = pos_;
    advance(); // satellite
    advance(); // .
    advance(); // include

    if (!expect_punct("(", "after satellite.include"))
        return kNullNode;
    NodeIndex what = parse_expression();
    expect_punct(")", "to close satellite.include");
    expect_statement_end();
    return arena_.make_include(what, span_from(first));
}

NodeIndex Parser::parse_capsule()
{
    const size_t first = pos_;
    advance(); // satellite
    advance(); // .
    advance(); // capsule

    CapsuleDecl cap;
    if (is_word(0, "satellite") && is_punct(1, ".")) {
        advance(); // satellite
        advance(); // .
        cap.reserved = true;
    }
    cap.name = expect_word("a capsule name");
    if (panic_)
        return kNullNode;

    if (cap.reserved && cap.name == "main") {
        cap.path_id = static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN);
    } else {
        cap.path_id = words_.intern(words::NodeId::CAPSULE, cap.name);
    }

    parse_signature(cap, true);
    return arena_.make_capsule(cap.reserved, cap.name, cap.params, cap.returns, cap.body, cap.path_id, span_from(first));
}

void Parser::parse_signature(CapsuleDecl &capsule, bool allow_returns)
{
    if (!expect_punct("(", "to open the parameter list"))
        return;

    if (!is_punct(0, ")")) {
        do {
            const size_t param_first = pos_;
            ParamDecl param;
            param.type = parse_type();
            if (panic_)
                return;
            param.name = expect_word("a parameter name after its type");
            if (panic_)
                return;
            param.span = span_from(param_first);
            capsule.params.push_back(std::move(param));
        } while (match_punct(","));
    }
    if (!expect_punct(")", "to close the parameter list"))
        return;

    if (at_language_path("returns")) {
        if (!allow_returns) {
            error(peek(), "a constructor declares no satellite.returns");
            return;
        }
        advance(); advance(); advance(); // satellite.returns
        if (!expect_punct("(", "after satellite.returns"))
            return;
        capsule.returns = parse_type();
        if (!expect_punct(")", "to close satellite.returns"))
            return;
    }

    capsule.body = parse_block();
}

NodeIndex Parser::parse_spacesuit()
{
    const size_t first = pos_;
    advance(); // satellite
    advance(); // .
    advance(); // spacesuit

    SpacesuitDecl suit;
    suit.name = expect_word("a spacesuit name");
    if (panic_)
        return kNullNode;

    suit.path_id = words_.intern(words::NodeId::SPACESUIT, suit.name);

    if (match_punct("(")) {
        if (!is_punct(0, ")")) {
            const size_t super_first = pos_;
            suit.super = expect_word("a superclass name, or nothing");
            suit.super_span = span_from(super_first);
        }
        expect_punct(")", "to close superclass");
    }

    // Optional colon before block (e.g. satellite.spacesuit name():)
    match_punct(":");

    // Skip any newlines before {
    while (!at_end() && peek().kind == TokenKind::Newline)
        advance();

    if (!expect_punct("{", "to open the spacesuit body"))
        return kNullNode;

    while (!at_end() && !is_punct(0, "}")) {
        while (!at_end() && (peek().kind == TokenKind::Newline || is_punct(0, ";")))
            advance();
        if (at_end() || is_punct(0, "}"))
            break;

        const size_t before = pos_;
        parse_access_block(suit);
        if (panic_)
            synchronize();
        if (pos_ == before)
            advance();
        if (errors_.size() >= kMaxErrors)
            break;
    }
    expect_punct("}", "to close the spacesuit body");
    return arena_.make_spacesuit(suit.name, suit.super, suit.super_span, suit.items, suit.path_id, span_from(first));
}

void Parser::parse_access_block(SpacesuitDecl &suit)
{
    Access access = Access::Protected;
    if (at_language_path("public"))
        access = Access::Public;
    else if (!at_language_path("protected")) {
        error(peek(), "expected satellite.protected or satellite.public");
        return;
    }
    advance(); advance(); advance(); // satellite.protected/public

    while (!at_end() && peek().kind == TokenKind::Newline)
        advance();

    if (!expect_punct("{", "to open access block"))
        return;

    while (!at_end() && !is_punct(0, "}")) {
        while (!at_end() && (peek().kind == TokenKind::Newline || is_punct(0, ";")))
            advance();
        if (at_end() || is_punct(0, "}"))
            break;

        const size_t before = pos_;
        parse_suit_member(suit, access);
        if (panic_)
            synchronize();
        if (pos_ == before)
            advance();
        if (errors_.size() >= kMaxErrors)
            break;
    }
    expect_punct("}", "to close access block");
}

void Parser::parse_suit_member(SpacesuitDecl &suit, Access access)
{
    // Constructor: suit name followed by '('
    if (peek().kind == TokenKind::Word && is_punct(1, "(") && peek().text == suit.name) {
        const size_t first = pos_;
        MethodDecl method;
        method.access = access;
        method.constructor = true;
        method.capsule.name = advance().text;
        parse_signature(method.capsule, false);
        method.capsule.span = span_from(first);
        method.span = span_from(first);
        suit.items.push_back(std::move(method));
        return;
    }

    if (at_language_path("capsule")) {
        advance(); advance(); advance(); // satellite.capsule
        MethodDecl method;
        method.access = access;
        method.capsule.name = expect_word("method name");
        method.capsule.path_id = words_.intern(words::NodeId::CAPSULE, method.capsule.name);
        parse_signature(method.capsule, true);
        suit.items.push_back(std::move(method));
        return;
    }

    if (at_type()) {
        const size_t first = pos_;
        FieldDecl field;
        field.access = access;
        field.type = parse_type();
        field.name = expect_word("field name");
        if (match_punct("=")) {
            field.init = parse_expression();
        }
        field.span = span_from(first);
        expect_statement_end();
        suit.items.push_back(std::move(field));
        return;
    }

    error(peek(), "expected a field declaration or a method in spacesuit");
}

NodeIndex Parser::parse_global_decl()
{
    const size_t first = pos_;
    advance(); // satellite
    advance(); // .
    advance(); // library
    advance(); // .

    std::string name = expect_word("a library variable name");
    words::PathId path_id = words_.intern(words::NodeId::LIBRARY, name);
    NodeIndex init = kNullNode;
    if (match_punct("=")) {
        init = parse_expression();
    }
    expect_statement_end();
    return arena_.make_global_decl(std::move(name), init, path_id, span_from(first));
}

} // namespace satellite

