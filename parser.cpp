#include "parser.hpp"

#include <utility>

namespace satellite {

namespace {

constexpr size_t MAX_ERRORS = 32;

class Parser {
public:
    // The token vector is copied rather than referenced so that a generic
    // close welded into a '>=' can be split in place; see parse_type.
    explicit Parser(std::vector<Token> tokens) : toks_(std::move(tokens)) {}

    ParseResult run()
    {
        ParseResult result;

        while (!at_end()) {
            const size_t before = pos_;

            if (at_language_path("include")) {
                result.program.items.push_back(parse_include());
            } else if (at_language_path("capsule")) {
                result.program.items.push_back(parse_capsule());
            } else if (at_language_path("spacesuit")) {
                result.program.items.push_back(parse_spacesuit());
            } else {
                StmtPtr s = parse_statement();
                if (s)
                    result.program.items.push_back(s);
            }

            // Error recovery must always consume something, or a malformed
            // token at top level would spin here forever.
            if (pos_ == before) {
                if (errors_.empty())
                    error(peek(), "unexpected " + describe(peek()));
                advance();
            }
            if (panic_)
                synchronize();
            if (errors_.size() >= MAX_ERRORS)
                break;
        }

        result.errors = std::move(errors_);
        return result;
    }

private:
    std::vector<Token> toks_;
    size_t pos_ = 0;
    std::vector<ParseError> errors_;
    bool panic_ = false;

    // --- token helpers -----------------------------------------------------

    const Token &peek(size_t ahead = 0) const
    {
        size_t i = pos_ + ahead;
        return i < toks_.size() ? toks_[i] : toks_.back();
    }

    const Token &previous() const
    {
        return pos_ > 0 ? toks_[pos_ - 1] : toks_.front();
    }

    bool at_end() const
    {
        return peek().kind == TokenKind::End || peek().kind == TokenKind::Error;
    }

    const Token &advance()
    {
        const Token &t = peek();
        if (!at_end())
            pos_++;
        return t;
    }

    bool is_punct(size_t ahead, const char *text) const
    {
        const Token &t = peek(ahead);
        return t.kind == TokenKind::Punct && t.text == text;
    }

    bool is_word(size_t ahead, const char *text) const
    {
        const Token &t = peek(ahead);
        return t.kind == TokenKind::Word && t.text == text;
    }

    bool match_punct(const char *text)
    {
        if (!is_punct(0, text))
            return false;
        advance();
        return true;
    }

    // --- errors ------------------------------------------------------------

    void error(const Token &at, const std::string &message)
    {
        if (panic_ || errors_.size() >= MAX_ERRORS)
            return; // one report per construct; the rest would be noise

        // Pointing at End is useless — it is past the last line, so the caret
        // lands on nothing. Report the last real token instead, which for an
        // unclosed construct is the '(' or '{' that was never closed.
        const Token &where =
            (at.kind == TokenKind::End && pos_ > 0) ? previous() : at;
        errors_.push_back(ParseError{message, span_of(where)});
        panic_ = true;
    }

    // Statements are separated by newlines, so anything left on the line after
    // a statement ends is a mistake. Without this check
    // `satellite.nonsense.thing x = 1` parses silently as two statements —
    // a module path, then an assignment — instead of being reported.
    void expect_statement_end()
    {
        if (at_end() || is_punct(0, "}"))
            return;
        if (peek().line != previous().line)
            return;
        error(peek(), "unexpected " + describe(peek()) +
                          " after the end of a statement");
    }

    bool expect_punct(const char *text, const std::string &what)
    {
        if (match_punct(text))
            return true;
        error(peek(), "expected '" + std::string(text) + "' " + what);
        return false;
    }

    std::string expect_word(const std::string &what)
    {
        if (peek().kind == TokenKind::Word)
            return advance().text;
        error(peek(), "expected " + what);
        return {};
    }

    // Skip forward until something that can plausibly begin a new construct,
    // so a single bad line does not cascade into a page of errors.
    void synchronize()
    {
        panic_ = false;
        const size_t line = previous().line;
        while (!at_end()) {
            if (is_punct(0, "}"))
                return;
            if (peek().line != line && starts_construct())
                return;
            advance();
        }
    }

    bool starts_construct() const
    {
        return at_type() || at_language_path("statement") ||
               at_language_path("return") || at_language_path("include") ||
               at_language_path("capsule") || at_language_path("spacesuit") ||
               at_language_path("protected") || at_language_path("public") ||
               is_punct(0, "{");
    }

    // --- the dispatch tests ------------------------------------------------

    // `satellite` '.' <segment>
    bool at_language_path(const char *segment) const
    {
        return is_word(0, "satellite") && is_punct(1, ".") &&
               is_word(2, segment);
    }

    // A satellite-rooted type is exactly three tokens of prefix. This is the
    // whole reason '<' is unambiguous: type namespaces are never value
    // expressions, so a '<' reached from parse_type is always a generic opener
    // and a '<' reached from parse_expression is always less-than.
    bool at_type() const
    {
        if (at_language_path("variable") || at_language_path("container"))
            return true;
        // the bare `satellite` singleton type, used in a type position
        if (is_word(0, "satellite") && !is_punct(1, "."))
            return true;

        // A spacesuit type: a bare name followed by the name it types. §5
        // already established that TWO ADJACENT WORDS are a shape no
        // expression can produce, which is what makes this test safe where a
        // dotted path followed by a Word is not — `main.x foo` and
        // `satellite.control.return my_time` both have a Punct between the
        // path and the name, so neither reaches here.
        //
        // Same line, for the same reason the type of an ordinary declaration
        // must be: without it a bare name at the end of one line silently
        // swallows the name that starts the next.
        return peek(0).kind == TokenKind::Word &&
               peek(1).kind == TokenKind::Word &&
               peek(1).line == peek(0).line;
    }

    bool at_statement_keyword(const char *name) const
    {
        return at_language_path("statement") && is_punct(3, ".") &&
               is_word(4, name);
    }

    void take_statement_keyword()
    {
        advance(); // satellite
        advance(); // .
        advance(); // statement
        advance(); // .
        advance(); // if / else / while / for
    }

    Span span_from(size_t first) const
    {
        const Token &a = first < toks_.size() ? toks_[first] : toks_.back();
        return span_join(span_of(a), span_of(previous()));
    }

    // --- types -------------------------------------------------------------

    Type parse_type()
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

    // --- expressions -------------------------------------------------------

    ExprPtr parse_expression(int min_prec = 1)
    {
        ExprPtr left = parse_unary();
        if (!left)
            return nullptr;

        for (;;) {
            if (peek().kind != TokenKind::Punct)
                break;
            const int prec = precedence(peek().text);
            if (prec == 0 || prec < min_prec)
                break;

            const std::string op = advance().text;
            // Every binary operator is left-associative, so the right side
            // binds only tighter operators.
            ExprPtr right = parse_expression(prec + 1);
            if (!right)
                return nullptr;

            Span s = span_join(left->span, right->span);
            left = make_expr(Binary{op, std::move(left), std::move(right)}, s);
        }
        return left;
    }

    ExprPtr parse_unary()
    {
        if (peek().kind == TokenKind::Punct && is_unary_op(peek().text)) {
            const size_t first = pos_;
            const std::string op = advance().text;
            ExprPtr operand = parse_unary();
            if (!operand)
                return nullptr;
            return make_expr(Unary{op, std::move(operand)}, span_from(first));
        }
        return parse_postfix();
    }

    ExprPtr parse_postfix()
    {
        const size_t first = pos_;
        ExprPtr node = parse_primary();
        if (!node)
            return nullptr;

        for (;;) {
            if (match_punct(".")) {
                std::string name = expect_word("a member name after '.'");
                if (panic_)
                    return nullptr;
                node = make_expr(Member{std::move(node), std::move(name)},
                                 span_from(first));
                continue;
            }

            // A call or a subscript must open on the same line as the thing it
            // applies to. Without this, a line beginning '(' or '[' is silently
            // absorbed by the line above — the defect that forced JavaScript's
            // automatic-semicolon-insertion rules. Statements here are
            // newline-separated, so this is what keeps them separated.
            const bool same_line = peek().line == previous().line;

            if (same_line && match_punct("(")) {
                std::vector<ExprPtr> args;
                if (!is_punct(0, ")")) {
                    do {
                        ExprPtr arg = parse_expression();
                        if (!arg)
                            return nullptr;
                        args.push_back(std::move(arg));
                    } while (match_punct(","));
                }
                if (!expect_punct(")", "to close the argument list"))
                    return nullptr;
                node = make_expr(Call{std::move(node), std::move(args)},
                                 span_from(first));
                continue;
            }

            if (same_line && match_punct("[")) {
                node = parse_subscript(std::move(node), first);
                if (!node)
                    return nullptr;
                continue;
            }

            return node;
        }
    }

    // Already past the '['. Distinguishes l[i] from l[a:b], where either bound
    // of a slice may be absent.
    ExprPtr parse_subscript(ExprPtr target, size_t first)
    {
        ExprPtr lo;
        if (!is_punct(0, ":")) {
            lo = parse_expression();
            if (!lo)
                return nullptr;
        }

        if (match_punct(":")) {
            ExprPtr hi;
            if (!is_punct(0, "]")) {
                hi = parse_expression();
                if (!hi)
                    return nullptr;
            }
            if (!expect_punct("]", "to close the slice"))
                return nullptr;
            return make_expr(
                Slice{std::move(target), std::move(lo), std::move(hi)},
                span_from(first));
        }

        if (!lo) {
            error(peek(), "expected an index or a slice inside '[ ]'");
            return nullptr;
        }
        if (!expect_punct("]", "to close the index"))
            return nullptr;
        return make_expr(Index{std::move(target), std::move(lo)},
                         span_from(first));
    }

    ExprPtr parse_primary()
    {
        const Token &t = peek();

        if (t.kind == TokenKind::Number) {
            advance();
            return make_expr(NumberLit{t.number, t.text}, span_of(t));
        }
        if (t.kind == TokenKind::String) {
            advance();
            return make_expr(StringLit{t.str, t.text}, span_of(t));
        }
        if (t.kind == TokenKind::Word) {
            advance();
            // `satellite` is the runtime singleton as a value, which is what
            // makes satellite.time.now() ordinary member access rather than a
            // special path form.
            if (t.text == "satellite")
                return make_expr(SatelliteLit{}, span_of(t));
            return make_expr(Name{t.text}, span_of(t));
        }
        if (is_punct(0, "(")) {
            advance();
            ExprPtr inner = parse_expression();
            if (!inner)
                return nullptr;
            if (!expect_punct(")", "to close the group"))
                return nullptr;
            return inner;
        }

        error(t, "expected an expression, found " + describe(t));
        return nullptr;
    }

    // --- statements --------------------------------------------------------

    StmtPtr parse_block()
    {
        const size_t first = pos_;
        if (!expect_punct("{", "to open a block"))
            return nullptr;

        Block block;
        while (!at_end() && !is_punct(0, "}")) {
            const size_t before = pos_;
            StmtPtr s = parse_statement();
            if (s)
                block.statements.push_back(std::move(s));
            if (panic_)
                synchronize();
            if (pos_ == before)
                advance();
            if (errors_.size() >= MAX_ERRORS)
                break;
        }
        if (!expect_punct("}", "to close the block"))
            return nullptr;
        return make_stmt(std::move(block), span_from(first));
    }

    StmtPtr parse_statement()
    {
        if (is_punct(0, "{"))
            return parse_block();
        if (at_statement_keyword("if"))
            return parse_if();
        if (at_statement_keyword("while"))
            return parse_while();
        if (at_statement_keyword("for"))
            return parse_for();

        StmtPtr s = at_language_path("return") ? parse_return()
                                               : parse_simple_statement();
        if (s)
            expect_statement_end();
        return s;
    }

    // A declaration, an assignment, or a bare expression. Shared with the for
    // header, which is why it does not consume any terminator.
    StmtPtr parse_simple_statement()
    {
        const size_t first = pos_;

        if (at_type()) {
            Type type = parse_type();
            if (panic_)
                return nullptr;

            // The name must sit on the same line as its type. Without this a
            // truncated declaration reaches onto the next line and swallows
            // whatever starts it, so one bad line takes the following capsule
            // down with it.
            if (peek().kind != TokenKind::Word ||
                peek().line != previous().line) {
                error(peek(), "expected a variable name after the type");
                return nullptr;
            }
            std::string name = advance().text;

            if (name == "satellite") {
                error(previous(),
                      "'satellite' is reserved as the language root and "
                      "cannot name a variable");
                return nullptr;
            }
            ExprPtr init;

            // `my_suit x("data")` — constructor arguments at the declaration.
            //
            // SUGAR, desugared here rather than carried in the tree: it becomes
            // `my_suit x = my_suit("data")`, which is the same statement with
            // the construction written out. Doing it in the parser means the
            // resolver's arity check, the type check and the evaluator all see
            // one form instead of two, and the only thing lost is that the
            // sugar is not what unparse emits — which is what "canonical" means.
            //
            // Same line, like every other postfix '(': a '(' starting the next
            // line belongs to that line.
            if (is_punct(0, "(") && peek().line == previous().line) {
                if (!type.is_spacesuit()) {
                    error(peek(), "only a spacesuit takes constructor "
                                  "arguments; " + unparse(type) +
                                  " is initialised with '='");
                    return nullptr;
                }
                const size_t call_first = pos_;
                ExprPtr callee = make_expr(Name{type.name}, type.span);
                advance(); // (

                std::vector<ExprPtr> args;
                if (!is_punct(0, ")")) {
                    do {
                        ExprPtr arg = parse_expression();
                        if (!arg)
                            return nullptr;
                        args.push_back(std::move(arg));
                    } while (match_punct(","));
                }
                if (!expect_punct(")", "to close the constructor arguments"))
                    return nullptr;

                init = make_expr(Call{std::move(callee), std::move(args)},
                                 span_from(call_first));
            } else if (match_punct("=")) {
                init = parse_expression();
                if (!init)
                    return nullptr;
            }
            return make_stmt(
                VarDecl{std::move(type), std::move(name), std::move(init)},
                span_from(first));
        }

        ExprPtr target = parse_expression();
        if (!target)
            return nullptr;

        if (match_punct("=")) {
            ExprPtr value = parse_expression();
            if (!value)
                return nullptr;
            return make_stmt(Assign{std::move(target), std::move(value)},
                             span_from(first));
        }
        return make_stmt(ExprStmt{std::move(target)}, span_from(first));
    }

    StmtPtr parse_if()
    {
        const size_t first = pos_;
        take_statement_keyword();

        if (!expect_punct("(", "after satellite.statement.if"))
            return nullptr;
        ExprPtr condition = parse_expression();
        if (!condition)
            return nullptr;
        if (!expect_punct(")", "after the condition"))
            return nullptr;

        StmtPtr then_branch = parse_block();
        if (!then_branch)
            return nullptr;

        StmtPtr else_branch;
        if (at_statement_keyword("else")) {
            take_statement_keyword();
            // else-if chains: the else branch may be another if.
            else_branch = at_statement_keyword("if") ? parse_if() : parse_block();
            if (!else_branch)
                return nullptr;
        }

        return make_stmt(If{std::move(condition), std::move(then_branch),
                            std::move(else_branch)},
                         span_from(first));
    }

    StmtPtr parse_while()
    {
        const size_t first = pos_;
        take_statement_keyword();

        if (!expect_punct("(", "after satellite.statement.while"))
            return nullptr;
        ExprPtr condition = parse_expression();
        if (!condition)
            return nullptr;
        if (!expect_punct(")", "after the condition"))
            return nullptr;

        StmtPtr body = parse_block();
        if (!body)
            return nullptr;
        return make_stmt(While{std::move(condition), std::move(body)},
                         span_from(first));
    }

    StmtPtr parse_for()
    {
        const size_t first = pos_;
        take_statement_keyword();

        if (!expect_punct("(", "after satellite.statement.for"))
            return nullptr;

        // Each of the three parts may be empty, so for(;;) is the infinite
        // loop. init and step are ordinary statements, which is why the loop
        // needs no forms the language did not already have.
        StmtPtr init;
        if (!is_punct(0, ";")) {
            init = parse_simple_statement();
            if (!init)
                return nullptr;
        }
        if (!expect_punct(";", "after the for-loop initialiser"))
            return nullptr;

        ExprPtr condition;
        if (!is_punct(0, ";")) {
            condition = parse_expression();
            if (!condition)
                return nullptr;
        }
        if (!expect_punct(";", "after the for-loop condition"))
            return nullptr;

        StmtPtr step;
        if (!is_punct(0, ")")) {
            step = parse_simple_statement();
            if (!step)
                return nullptr;
        }
        if (!expect_punct(")", "to close the for-loop header"))
            return nullptr;

        StmtPtr body = parse_block();
        if (!body)
            return nullptr;

        return make_stmt(For{std::move(init), std::move(condition),
                             std::move(step), std::move(body)},
                         span_from(first));
    }

    StmtPtr parse_return()
    {
        const size_t first = pos_;
        advance(); // satellite
        advance(); // .
        advance(); // return

        if (!expect_punct("(", "after satellite.return"))
            return nullptr;

        ExprPtr value;
        if (!is_punct(0, ")")) {
            value = parse_expression();
            if (!value)
                return nullptr;
        }
        if (!expect_punct(")", "to close satellite.return"))
            return nullptr;

        return make_stmt(Return{std::move(value)}, span_from(first));
    }

    // --- top level ---------------------------------------------------------

    Include parse_include()
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

    Capsule parse_capsule()
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

    // The parameter list, the optional satellite.returns and the body — shared
    // by a capsule, a method and a constructor, which differ only in how they
    // are named and in whether a return type means anything.
    void parse_signature(Capsule &capsule, bool allow_returns)
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

    // --- spacesuits --------------------------------------------------------

    Spacesuit parse_spacesuit()
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

    void parse_access_block(Spacesuit &suit)
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

    void parse_suit_member(Spacesuit &suit, Access access)
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
};

} // namespace

ParseResult parse(const std::vector<Token> &tokens)
{
    // A lexer error is a parse error too; there is nothing useful to parse
    // past it.
    for (const Token &t : tokens) {
        if (t.kind == TokenKind::Error) {
            ParseResult result;
            result.errors.push_back(ParseError{t.text, span_of(t)});
            return result;
        }
    }
    return Parser(tokens).run();
}

ParseResult parse(const std::string &source)
{
    return parse(lex(source));
}

std::string format_error(const ParseError &error, const std::string &source)
{
    // Token offsets are byte offsets, so the line can be sliced straight out
    // of the source. Computing this from decoded text would drift: decode()
    // expands \cwd to whatever the working directory happens to be.
    size_t begin = 0;
    if (error.span.start > 0 && error.span.start <= source.size()) {
        size_t nl = source.rfind('\n', error.span.start - 1);
        begin = (nl == std::string::npos) ? 0 : nl + 1;
    }
    size_t end = source.find('\n', begin);
    if (end == std::string::npos)
        end = source.size();

    const std::string line = source.substr(begin, end - begin);
    const size_t column = error.span.start >= begin ? error.span.start - begin : 0;

    std::string number = std::to_string(error.span.line);
    std::string gutter(number.size(), ' ');

    std::string out = "satellite: line " + number + ": " + error.message + "\n";
    out += " " + number + " | " + line + "\n";
    out += " " + gutter + " | " + std::string(column, ' ') + "^";

    // Underline the whole token when it spans more than one character.
    if (error.span.end > error.span.start + 1)
        out += std::string(error.span.end - error.span.start - 1, '~');

    return out;
}

} // namespace satellite
