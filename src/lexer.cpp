#include "satellite/lexer.hpp"

#include <cctype>
#include <cstdlib>

namespace satellite {

const char* tok_name(Tok t) {
    switch (t) {
        case Tok::End:       return "end";
        case Tok::Term:      return "term";
        case Tok::Satellite: return "satellite";
        case Tok::Ident:     return "ident";
        case Tok::Int:       return "int";
        case Tok::Real:      return "real";
        case Tok::Str:       return "str";
        case Tok::CxxBlock:  return "cxx";
        case Tok::Dot:       return ".";
        case Tok::Comma:     return ",";
        case Tok::Colon:     return ":";
        case Tok::LParen:    return "(";
        case Tok::RParen:    return ")";
        case Tok::LBrace:    return "{";
        case Tok::RBrace:    return "}";
        case Tok::LBracket:  return "[";
        case Tok::RBracket:  return "]";
        case Tok::Plus:      return "+";
        case Tok::Minus:     return "-";
        case Tok::Star:      return "*";
        case Tok::Slash:     return "/";
        case Tok::Percent:   return "%";
        case Tok::Assign:    return "=";
        case Tok::Eq:        return "==";
        case Tok::Ne:        return "!=";
        case Tok::Lt:        return "<";
        case Tok::Gt:        return ">";
        case Tok::Le:        return "<=";
        case Tok::Ge:        return ">=";
        case Tok::And:       return "and";
        case Tok::Or:        return "or";
        case Tok::Not:       return "!";
        default:             return "?";
    }
}

// A newline only terminates a statement when the line so far could stand on
// its own.  `x = 1` ends; `x = 1 +` does not.
bool ends_statement(Tok t) {
    switch (t) {
        case Tok::Ident:
        case Tok::Satellite:
        case Tok::Int:
        case Tok::Real:
        case Tok::Str:
        case Tok::CxxBlock:
        case Tok::RParen:
        case Tok::RBracket:
        case Tok::RBrace:
            return true;
        default:
            return false;
    }
}

char32_t op_code_for(Tok t) {
    switch (t) {
        case Tok::Plus:    return charmap::OP_PLUS;
        case Tok::Minus:   return charmap::OP_MINUS;
        case Tok::Star:    return charmap::OP_TIMES;
        case Tok::Slash:   return charmap::OP_DIVIDE;
        case Tok::Percent: return charmap::OP_MODULO;
        case Tok::Assign:  return charmap::OP_ASSIGN;
        case Tok::Eq:      return charmap::OP_EQ;
        case Tok::Ne:      return charmap::OP_NE;
        case Tok::Lt:      return charmap::OP_LT;
        case Tok::Gt:      return charmap::OP_GT;
        case Tok::Le:      return charmap::OP_LE;
        case Tok::Ge:      return charmap::OP_GE;
        default:           return charmap::VOID;
    }
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; } else { ++col_; }
    return c;
}

bool Lexer::match(char c) {
    if (at_end() || src_[pos_] != c) return false;
    advance();
    return true;
}

void Lexer::error(const std::string& msg, int l, int c) {
    errors_.push_back({msg, l, c});
}

void Lexer::scan_number(std::vector<Token>& out) {
    int l = line_, c = col_;
    size_t start = pos_;
    while (std::isdigit((unsigned char)peek()) || peek() == '_') advance();

    bool is_real = false;
    // a dot is only a decimal point if a digit follows -- otherwise it is the
    // member-access dot, as in `5.to_string`
    if (peek() == '.' && std::isdigit((unsigned char)peek(1))) {
        is_real = true;
        advance();
        while (std::isdigit((unsigned char)peek()) || peek() == '_') advance();
    }
    if (peek() == 'e' || peek() == 'E') {
        size_t save = pos_;
        int save_line = line_, save_col = col_;
        advance();
        if (peek() == '+' || peek() == '-') advance();
        if (std::isdigit((unsigned char)peek())) {
            is_real = true;
            while (std::isdigit((unsigned char)peek())) advance();
        } else {
            pos_ = save; line_ = save_line; col_ = save_col;   // not an exponent
        }
    }

    std::string digits;
    for (size_t k = start; k < pos_; ++k)
        if (src_[k] != '_') digits += src_[k];

    Token t;
    t.line = l;
    t.col  = c;
    t.text = digits;
    if (is_real) {
        t.kind = Tok::Real;
        t.dval = std::strtod(digits.c_str(), nullptr);
    } else {
        t.kind = Tok::Int;
        errno = 0;
        t.ival = std::strtoll(digits.c_str(), nullptr, 10);
        if (errno == ERANGE) {
            // too big for int64 -- keep the digits, the compiler promotes it
            // to a satellite Big at constant-folding time
            t.kind = Tok::Int;
            t.ival = 0;
        }
    }
    out.push_back(std::move(t));
}

void Lexer::scan_string(std::vector<Token>& out) {
    int l = line_, c = col_;
    advance();                       // opening quote
    std::string body;
    bool closed = false;
    while (!at_end()) {
        char ch = peek();
        if (ch == '"') { advance(); closed = true; break; }
        if (ch == '\n') break;       // unterminated: do not swallow the line
        if (ch == '\\') {
            advance();
            char e = at_end() ? '\0' : advance();
            switch (e) {
                case 'n':  body += '\n'; break;
                case 't':  body += '\t'; break;
                case 'r':  body += '\r'; break;
                case '0':  body += '\0'; break;
                case '\\': body += '\\'; break;
                case '"':  body += '"';  break;
                default:
                    error(std::string("unknown escape \\") + e, line_, col_);
                    body += e;
            }
            continue;
        }
        body += advance();
    }
    if (!closed) error("unterminated string", l, c);

    Token t;
    t.kind = Tok::Str;
    t.text = std::move(body);
    t.line = l;
    t.col  = c;
    out.push_back(std::move(t));
}

// Captures everything between the braces WITHOUT lexing it -- the contents are
// C++, not satellite.  Brace counting has to skip strings, char literals, raw
// strings and comments, or a '}' inside any of them would end the block early.
void Lexer::scan_cxx_block(std::vector<Token>& out) {
    int l = line_, c = col_;
    advance();                                   // the opening {
    size_t start = pos_;
    int depth = 1;
    bool closed = false;

    while (!at_end()) {
        char ch = peek();

        if (ch == '/' && peek(1) == '/') {                       // line comment
            while (!at_end() && peek() != '\n') advance();
            continue;
        }
        if (ch == '/' && peek(1) == '*') {                       // block comment
            advance(); advance();
            while (!at_end() && !(peek() == '*' && peek(1) == '/')) advance();
            if (!at_end()) { advance(); advance(); }
            continue;
        }
        if (ch == 'R' && peek(1) == '"' &&                       // raw string
            (pos_ == 0 || !(std::isalnum((unsigned char)src_[pos_ - 1]) ||
                            src_[pos_ - 1] == '_'))) {
            advance(); advance();
            std::string delim;
            while (!at_end() && peek() != '(') { delim += peek(); advance(); }
            if (!at_end()) advance();
            std::string term = ")" + delim + "\"";
            while (!at_end()) {
                if (src_.compare(pos_, term.size(), term) == 0) {
                    for (size_t i = 0; i < term.size(); ++i) advance();
                    break;
                }
                advance();
            }
            continue;
        }
        if (ch == '"' || ch == '\'') {                           // string / char
            char q = ch;
            advance();
            while (!at_end() && peek() != q) {
                if (peek() == '\\') advance();
                if (!at_end()) advance();
            }
            if (!at_end()) advance();
            continue;
        }

        if (ch == '{') { ++depth; advance(); continue; }
        if (ch == '}') {
            if (--depth == 0) { closed = true; break; }
            advance();
            continue;
        }
        advance();
    }

    std::string body = src_.substr(start, pos_ - start);
    if (closed) advance();                                       // the closing }
    else error("unterminated satellite.cxx block", l, c);

    Token t;
    t.kind = Tok::CxxBlock;
    t.text = std::move(body);
    t.line = l;
    t.col  = c;
    out.push_back(std::move(t));
}

void Lexer::scan_word(std::vector<Token>& out, bool after_dot) {
    int l = line_, c = col_;
    size_t start = pos_;
    while (std::isalnum((unsigned char)peek()) || peek() == '_') advance();
    std::string w = src_.substr(start, pos_ - start);

    Token t;
    t.line = l;
    t.col  = c;
    t.text = w;

    // THE rule: `satellite` is the language's only keyword, but after a dot it
    // is just a name -- that is what lets a user reach a variable they called
    // `satellite` via satellite.library.some_capsule.satellite
    if (w == "satellite" && !after_dot)  t.kind = Tok::Satellite;
    else if (w == "and")                 t.kind = Tok::And;
    else if (w == "or")                  t.kind = Tok::Or;
    else                                 t.kind = Tok::Ident;

    out.push_back(std::move(t));
}

std::vector<Token> Lexer::scan() {
    std::vector<Token> out;
    Tok prev = Tok::End;
    // Inside ( ) or [ ] a newline never ends a statement -- an argument list
    // or an index expression may span as many lines as it likes.  Braces are
    // deliberately NOT counted: statements inside a capsule body do terminate.
    int group_depth = 0;

    auto push = [&](Tok k) {
        Token t;
        t.kind = k;
        t.line = line_;
        t.col  = col_;
        t.text = tok_name(k);
        out.push_back(std::move(t));
        prev = k;
    };

    while (!at_end()) {
        char c = peek();

        // --- newline: terminator only if the line could end here ------------
        if (c == '\n') {
            advance();
            if (group_depth == 0 && ends_statement(prev)) {
                Token t;
                t.kind = Tok::Term;
                t.line = line_ - 1;
                t.col  = col_;
                t.text = "\\n";
                out.push_back(std::move(t));
                prev = Tok::Term;
            }
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r') { advance(); continue; }

        // --- comments -------------------------------------------------------
        if (c == '/' && peek(1) == '/') {
            while (!at_end() && peek() != '\n') advance();
            continue;                       // newline handled on the next pass
        }
        if (c == '/' && peek(1) == '*') {
            int l = line_, cc = col_;
            advance(); advance();
            bool closed = false;
            while (!at_end()) {
                if (peek() == '*' && peek(1) == '/') { advance(); advance(); closed = true; break; }
                advance();
            }
            if (!closed) error("unterminated block comment", l, cc);
            continue;
        }

        if (std::isdigit((unsigned char)c)) {
            scan_number(out);
            prev = out.back().kind;
            continue;
        }
        if (c == '"') {
            scan_string(out);
            prev = out.back().kind;
            continue;
        }
        if (std::isalpha((unsigned char)c) || c == '_') {
            scan_word(out, prev == Tok::Dot);
            prev = out.back().kind;

            // `satellite.cxx {` -- everything to the matching brace is C++
            size_t n = out.size();
            if (n >= 3 && out[n - 1].kind == Tok::Ident && out[n - 1].text == "cxx" &&
                out[n - 2].kind == Tok::Dot && out[n - 3].kind == Tok::Satellite) {
                size_t save = pos_;
                int save_line = line_, save_col = col_;
                while (!at_end() && (peek() == ' ' || peek() == '\t' ||
                                     peek() == '\r' || peek() == '\n')) advance();
                if (peek() == '{') {
                    out.resize(n - 3);            // the block token replaces the three
                    scan_cxx_block(out);
                    prev = Tok::CxxBlock;
                    continue;
                }
                pos_ = save; line_ = save_line; col_ = save_col;
            }
            continue;
        }

        advance();   // consume the punctuation character
        switch (c) {
            case '.': push(Tok::Dot);      break;
            case ',': push(Tok::Comma);    break;
            case ':': push(Tok::Colon);    break;
            case '(': ++group_depth; push(Tok::LParen); break;
            case ')': if (group_depth) --group_depth; push(Tok::RParen); break;
            case '{': push(Tok::LBrace);   break;
            case '}': push(Tok::RBrace);   break;
            case '[': ++group_depth; push(Tok::LBracket); break;
            case ']': if (group_depth) --group_depth; push(Tok::RBracket); break;
            case '+': push(Tok::Plus);     break;
            case '-': push(Tok::Minus);    break;
            case '*': push(Tok::Star);     break;
            case '/': push(Tok::Slash);    break;
            case '%': push(Tok::Percent);  break;
            case '=': push(match('=') ? Tok::Eq : Tok::Assign); break;
            case '!': push(match('=') ? Tok::Ne : Tok::Not);    break;
            case '<': push(match('=') ? Tok::Le : Tok::Lt);     break;
            case '>': push(match('=') ? Tok::Ge : Tok::Gt);     break;
            default:
                error(std::string("unexpected character '") + c + "'", line_, col_ - 1);
                push(Tok::Unknown);
                break;
        }
    }

    // a file that ends without a trailing newline still ends its last statement
    if (ends_statement(prev)) {
        Token t;
        t.kind = Tok::Term;
        t.line = line_;
        t.col  = col_;
        t.text = "\\n";
        out.push_back(std::move(t));
    }

    Token e;
    e.kind = Tok::End;
    e.line = line_;
    e.col  = col_;
    e.text = "end";
    out.push_back(std::move(e));
    return out;
}

}  // namespace satellite
