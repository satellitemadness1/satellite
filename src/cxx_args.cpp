// satellite.cxx( ... ) -- the block header.
//
//     satellite.cxx(greeting = "hello, world!", n = 42)
//     {
//         std::cout << greeting << '\n';     // greeting is a std::string
//         satellite.return(n * 2)            // and comes back as a satellite int
//     }
//
// The header is satellite source, so it is lexed with satellite's OWN lexer
// rather than a hand-rolled scanner.  String escapes, number formats and the
// rules for what an identifier may contain are then the language's rules by
// construction -- there is no second dialect to keep in agreement with the
// first, and no way for `"a\tb"` to mean one thing in a header and another
// thing three lines later.
#include "satellite/cxx.hpp"
#include "satellite/lexer.hpp"

#include <cstring>
#include <sstream>

namespace satellite {
namespace cxx {

// ---------------------------------------------------------------------------
const char* cxx_type_for(Type t) {
    switch (t) {
        case Type::Str:  return "std::string";
        case Type::Int:  return "int64_t";
        case Type::Real: return "double";
        case Type::Bool: return "bool";
        // Big, List, Capsule and anything added later have no from_container
        // overload -- and should not get a lossy one invented for them.  The
        // block receives the value itself and decides what to do with it.
        default:         return "satellite::Container";
    }
}

namespace {

// A name that is fine in satellite and fatal in C++.  Without this check
// `satellite.cxx(class = 1)` reaches g++ and comes back as "expected
// unqualified-id before 'class'", pointing into generated source the author
// never wrote and cannot open.
bool is_cxx_keyword(const std::string& w) {
    static const char* kWords[] = {
        "alignas", "alignof", "and", "asm", "auto", "bool", "break", "case",
        "catch", "char", "class", "const", "constexpr", "continue", "decltype",
        "default", "delete", "do", "double", "else", "enum", "explicit",
        "export", "extern", "false", "float", "for", "friend", "goto", "if",
        "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
        "not", "nullptr", "operator", "or", "private", "protected", "public",
        "register", "return", "short", "signed", "sizeof", "static", "struct",
        "switch", "template", "this", "throw", "true", "try", "typedef",
        "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
        "volatile", "while", "xor",
    };
    for (const char* k : kWords)
        if (w == k) return true;
    return false;
}

// Names the generated function already uses.  Shadowing one of these compiles
// -- and then the block's own `args` is not the array it expects.
bool is_reserved_name(const std::string& w) {
    return w == "args" || w == "argc";
}

}  // namespace

// ---------------------------------------------------------------------------
bool parse_args(const std::string& header, std::vector<CxxArg>* out,
                std::string* err) {
    out->clear();
    auto fail = [&](const std::string& m) {
        if (err) *err = m;
        out->clear();
        return false;
    };

    Lexer lx(header);
    std::vector<Token> raw = lx.scan();
    if (!lx.ok()) return fail(lx.errors().front().message);

    // A header may span lines, so the terminators the lexer inserts are noise
    // here.  Dropping them is what makes a multi-line argument list work.
    std::vector<Token> t;
    for (auto& x : raw)
        if (x.kind != Tok::Term && x.kind != Tok::End) t.push_back(std::move(x));

    if (t.empty()) return true;                 // satellite.cxx() { } -- legal

    size_t i = 0;
    int    positional = 0;

    for (;;) {
        // ---- optional `name =` --------------------------------------------
        std::string name;
        bool        named = false;
        if (t[i].kind == Tok::Ident && i + 1 < t.size() &&
            t[i + 1].kind == Tok::Assign) {
            name  = t[i].text;
            named = true;
            i += 2;
            if (i >= t.size())
                return fail("argument `" + name + "` has no value after `=`");
        } else {
            name = "arg" + std::to_string(positional);
        }
        ++positional;

        if (named && is_cxx_keyword(name))
            return fail("`" + name + "` is a C++ keyword and cannot name an "
                        "argument -- the block would not compile");
        if (named && is_reserved_name(name))
            return fail("`" + name + "` is already the name of the block's own "
                        "argument array; pick another");
        for (const auto& prior : *out)
            if (prior.name == name)
                return fail("argument `" + name + "` is given twice");

        // ---- the value ----------------------------------------------------
        // A leading minus belongs to the literal.  The lexer has no negative
        // number token -- `-1` is Minus then Int everywhere in the language --
        // so the sign is folded in here rather than left for a parser that does
        // not exist yet.
        bool neg = false;
        if (t[i].kind == Tok::Minus) {
            neg = true;
            if (++i >= t.size()) return fail("`-` with no number after it");
        }

        CxxArg a;
        a.name = name;
        switch (t[i].kind) {
            case Tok::Str:
                if (neg) return fail("`-` cannot be applied to a string");
                a.value = Container::str(t[i].text);
                break;
            case Tok::Int:
                a.value = Container::integer(neg ? -t[i].ival : t[i].ival);
                break;
            case Tok::Real:
                a.value = Container::real(neg ? -t[i].dval : t[i].dval);
                break;
            case Tok::Ident:
                if (neg) return fail("`-` cannot be applied to `" + t[i].text + "`");
                if (t[i].text == "true" || t[i].text == "false")
                    a.value = Container::boolean(t[i].text == "true");
                else if (t[i].text == "nil")
                    a.value = Container::nil();
                else
                    // The honest error.  There is no VM in version 001, so a
                    // variable cannot be evaluated -- and passing the NAME
                    // through as a string would be a wrong answer that compiles,
                    // which is worse than a refusal that explains itself.
                    return fail("`" + t[i].text + "` is a variable, and version "
                                "001 has no virtual machine to evaluate one -- "
                                "a cxx block argument must be a literal "
                                "(a string, a number, true, false or nil)");
                break;
            default:
                return fail(std::string("expected a value, found `") +
                            tok_name(t[i].kind) + "`");
        }
        out->push_back(std::move(a));
        ++i;

        // ---- comma, or done -----------------------------------------------
        if (i >= t.size()) break;
        if (t[i].kind != Tok::Comma)
            return fail(std::string("expected `,` between arguments, found `") +
                        tok_name(t[i].kind) + "`");
        if (++i >= t.size()) return fail("trailing `,` with no argument after it");
    }

    return true;
}

}  // namespace cxx
}  // namespace satellite
