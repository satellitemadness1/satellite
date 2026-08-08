// Type syntax — §4's reservation rule, which is what keeps '<' unambiguous.
//
// Part of parser/, split from a 1022-line parser.cpp.

#include "parser_internal.hpp"

namespace satellite {

Type Parser::parse_type()
{
    const size_t first = pos_;
    Type type;

    if (!is_word(0, "satellite")) {
        // A bare name is a spacesuit type. It is the one type spelled
        // without the satellite root, because §1 makes a user-owned thing
        // bare — and it stays out of §4's ambiguity by taking no generic
        // arguments, so no '<' can follow it here.
        if (peek().kind != TokenKind::Word) {
            error(peek(), "expected a type: either a satellite-rooted type "
                          "path or a spacesuit name");
            return type;
        }
        type.name = advance().text;
        type.span = span_from(first);
        return type;
    }
    advance();

    // A bare `satellite` in a type position is the singleton type.
    if (!is_punct(0, ".")) {
        type.span = span_from(first);
        return type;
    }
    advance(); // .

    type.space = expect_word("'variable' or 'container' after 'satellite.'");
    if (type.space != "variable" && type.space != "container") {
        error(previous(), "'" + type.space +
                              "' is not a type namespace; expected "
                              "'variable' or 'container'");
        return type;
    }
    if (!expect_punct(".", "after '" + type.space + "'"))
        return type;

    type.name = expect_word("a type name");

    if (match_punct("<")) {
        do {
            type.args.push_back(parse_type());
            if (panic_)
                return type;
        } while (match_punct(","));

        // The lexer matches '>=' greedily without knowing it is inside a
        // type, so a generic close can arrive welded to an '='. Splitting
        // it here hands back the '>' this loop wants and leaves the '='
        // for whatever follows. Unreachable in this grammar, since a type
        // is always followed by a name, but the split costs nothing and
        // the alternative is a baffling error years from now.
        if (is_punct(0, ">="))
            split_punct(toks_, pos_);
        expect_punct(">", "to close the generic arguments");
    }

    type.span = span_from(first);
    return type;
}

} // namespace satellite
