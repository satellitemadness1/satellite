// Top-level declarations: include, capsule, spacesuit and its members.
//
// Part of parser/, split from a 1022-line parser.cpp.

#include "parser_internal.hpp"

namespace satellite {

Include Parser::parse_include()
{
    const size_t first = pos_;
    advance(); // satellite
    advance(); // .
    advance(); // include

    Include node;
    if (!expect_punct("(", "after satellite.include"))
        return node;
    node.what = parse_expression();
    if (!node.what)
        return node;
    expect_punct(")", "to close satellite.include");
    node.span = span_from(first);
    return node;
}

Capsule Parser::parse_capsule()
{
    const size_t first = pos_;
    advance(); // satellite
    advance(); // .
    advance(); // capsule

    Capsule capsule;

    // satellite.main is prefixed because the runtime chooses that name;
    // a capsule the user writes is bare, like every other name they pick.
    if (is_word(0, "satellite") && is_punct(1, ".")) {
        advance();
        advance();
        capsule.reserved = true;
    }
    capsule.name = expect_word("a capsule name");
    if (panic_)
        return capsule;

    parse_signature(capsule, true);
    capsule.span = span_from(first);
    return capsule;
}

void Parser::parse_signature(Capsule &capsule, bool allow_returns)
{
    if (!expect_punct("(", "to open the parameter list"))
        return;

    if (!is_punct(0, ")")) {
        do {
            const size_t param_first = pos_;
            Param param;
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
            error(peek(), "a constructor declares no satellite.returns; "
                          "what it produces is the object");
            return;
        }
        advance(); // satellite
        advance(); // .
        advance(); // returns
        if (!expect_punct("(", "after satellite.returns"))
            return;
        capsule.returns = parse_type();
        if (panic_)
            return;
        if (!expect_punct(")", "to close satellite.returns"))
            return;
    }

    capsule.body = parse_block();
}

Spacesuit Parser::parse_spacesuit()
{
    const size_t first = pos_;
    advance(); // satellite
    advance(); // .
    advance(); // spacesuit

    Spacesuit suit;
    suit.name = expect_word("a spacesuit name");
    if (panic_)
        return suit;
    if (suit.name == "satellite") {
        error(previous(), "'satellite' is reserved as the language root and "
                          "cannot name a spacesuit");
        return suit;
    }

    // The parenthesised name is the SUPERCLASS, not a parameter list. A
    // spacesuit is constructed by declaring a variable of its type, so it
    // has nowhere to take arguments and needs no signature; the parens are
    // free to mean the one thing a class declaration does need to say.
    // Empty parens mean no superclass.
    if (!expect_punct("(", "after the spacesuit name"))
        return suit;
    if (!is_punct(0, ")")) {
        const size_t super_first = pos_;
        suit.super = expect_word("a superclass name, or nothing at all");
        if (panic_)
            return suit;
        suit.super_span = span_from(super_first);
    }
    if (!expect_punct(")", "to close the superclass"))
        return suit;

    if (!expect_punct("{", "to open the spacesuit body"))
        return suit;

    while (!at_end() && !is_punct(0, "}")) {
        const size_t before = pos_;
        parse_access_block(suit);
        if (panic_)
            synchronize();
        if (pos_ == before)
            advance();
        if (errors_.size() >= MAX_ERRORS)
            break;
    }
    expect_punct("}", "to close the spacesuit body");

    suit.span = span_from(first);
    return suit;
}

void Parser::parse_access_block(Spacesuit &suit)
{
    Access access = Access::Protected;
    if (at_language_path("public"))
        access = Access::Public;
    else if (!at_language_path("protected")) {
        error(peek(), "expected satellite.protected or satellite.public; "
                      "every spacesuit member belongs to one of them");
        return;
    }
    advance(); // satellite
    advance(); // .
    advance(); // protected / public

    if (!expect_punct("{", "to open the access block"))
        return;

    while (!at_end() && !is_punct(0, "}")) {
        const size_t before = pos_;
        parse_suit_member(suit, access);
        if (panic_)
            synchronize();
        if (pos_ == before)
            advance();
        if (errors_.size() >= MAX_ERRORS)
            break;
    }
    expect_punct("}", "to close the access block");
}

void Parser::parse_suit_member(Spacesuit &suit, Access access)
{
    // A constructor: the spacesuit's own name, then a parameter list. One
    // token of lookahead separates it from everything else a member can be,
    // because a field is `type name` — two adjacent Words — and a method
    // starts with satellite.capsule. A bare Word followed by '(' is neither.
    if (peek().kind == TokenKind::Word && is_punct(1, "(")) {
        const size_t first = pos_;
        if (peek().text != suit.name) {
            error(peek(), "a constructor is named after its spacesuit; "
                          "expected " + suit.name + ", found " +
                          peek().text);
            return;
        }
        Method method;
        method.access = access;
        method.constructor = true;
        method.capsule.name = advance().text;
        parse_signature(method.capsule, false);
        if (panic_)
            return;
        method.capsule.span = span_from(first);
        suit.items.push_back(std::move(method));
        return;
    }

    if (at_language_path("capsule")) {
        // A method is named bare, like every other user-owned thing.
        // satellite.<name> stays reserved for capsules the RUNTIME calls,
        // and the runtime never calls a method.
        if (is_word(3, "satellite")) {
            error(peek(3), "a spacesuit method is named bare; "
                           "satellite.<name> is reserved for the runtime");
            return;
        }
        Method method;
        method.access = access;
        method.capsule = parse_capsule();
        if (panic_)
            return;
        suit.items.push_back(std::move(method));
        return;
    }

    if (!at_type()) {
        error(peek(), "expected a field declaration or a satellite.capsule "
                      "inside a spacesuit");
        return;
    }

    const size_t first = pos_;
    Field field;
    field.access = access;
    field.type = parse_type();
    if (panic_)
        return;

    // Same-line rule as an ordinary declaration: a truncated field would
    // otherwise reach onto the next line and swallow whatever starts it.
    if (peek().kind != TokenKind::Word || peek().line != previous().line) {
        error(peek(), "expected a field name after the type");
        return;
    }
    field.name = advance().text;
    if (field.name == "satellite") {
        error(previous(), "'satellite' is reserved as the language root and "
                          "cannot name a field");
        return;
    }

    if (match_punct("=")) {
        field.init = parse_expression();
        if (!field.init)
            return;
    }
    field.span = span_from(first);
    expect_statement_end();
    suit.items.push_back(std::move(field));
}

} // namespace satellite
