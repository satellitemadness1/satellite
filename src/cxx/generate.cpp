// Turning a satellite.cxx block into a compilable translation unit.
//
// The interesting part is that satellite never has to understand a C++ type.
// `satellite.return(expr)` becomes a call to to_container(expr), and OVERLOAD
// RESOLUTION picks the conversion for whatever type expr happens to have; a
// type with no overload is an ordinary compile error naming that type.
// Arguments work the same way in reverse, through from_container.
#include "cxx_internal.hpp"

#include <cctype>
#include <cstring>
#include <sstream>

namespace satellite {
namespace cxx {

using detail::trim;

namespace {
// Net change in brace depth across a line, ignoring braces inside string and
// char literals and after a line comment.
int brace_delta(const std::string& line) {
    int d = 0;
    for (size_t k = 0; k < line.size(); ++k) {
        char c = line[k];
        if (c == '/' && k + 1 < line.size() && line[k + 1] == '/') break;
        if (c == '"' || c == '\'') {
            char q = c;
            ++k;
            while (k < line.size() && line[k] != q) {
                if (line[k] == '\\') ++k;
                ++k;
            }
            continue;
        }
        if (c == '{') ++d;
        else if (c == '}') --d;
    }
    return d;
}

bool starts_with_word(const std::string& t, const char* w) {
    size_t n = std::strlen(w);
    if (t.compare(0, n, w) != 0) return false;
    return t.size() == n || !(std::isalnum((unsigned char)t[n]) || t[n] == '_');
}

// Declarations that are illegal inside a function body and must go to file
// scope: templates, types, namespaces -- and free functions, since C++ has no
// nested functions.
bool opens_file_scope_construct(const std::string& line) {
    std::string t = trim(line);
    if (t.empty()) return false;

    static const char* kKeywords[] = {
        "template", "struct", "class", "enum", "union", "namespace", "typedef",
    };
    for (const char* k : kKeywords)
        if (starts_with_word(t, k)) return true;

    // A free function definition: something(...) {   -- but not a control
    // statement, which has the same shape.
    static const char* kControl[] = {
        "if", "for", "while", "switch", "do", "else", "return", "try", "catch",
    };
    for (const char* k : kControl)
        if (starts_with_word(t, k)) return false;

    size_t open  = t.find('(');
    size_t brace = t.rfind('{');
    if (open == std::string::npos || brace == std::string::npos) return false;
    if (brace < open) return false;
    if (t.find(';') != std::string::npos) return false;      // a call, not a definition
    if (t.find('=') != std::string::npos) return false;      // an initialisation
    // needs a return type AND a name before the parenthesis: "int helper(...)"
    std::string head = trim(t.substr(0, open));
    return head.find(' ') != std::string::npos || head.find('*') != std::string::npos;
}

// Lift the file-scope directives off the FRONT of a line and return whatever
// code was left behind on it.
//
// This exists because a whole line was previously hoisted whenever it began
// with '#', which quietly destroyed the most natural way to write a one-line
// block:
//
//     satellite.cxx() { #include <iostream> std::cout << "hello"; }
//
// The preprocessor takes <iostream> and discards the rest of the line as
// "extra tokens at end of #include directive" -- a WARNING, not an error.  So
// the block compiled, ran, printed nothing, and reported success.  Splitting
// the line here is what makes the code survive.
//
// Only #include is split.  #define, #pragma and the conditionals genuinely run
// to end of line, and there is no honest way to tell where the directive stops
// and code begins, so those still take the whole line.
std::string lift_directives(std::string t, std::ostringstream& prologue) {
    for (;;) {
        t = trim(t);
        if (t.empty() || t[0] != '#') return t;

        if (!starts_with_word(t.substr(1), "include")) {
            prologue << t << '\n';                    // #define, #pragma, #if...
            return "";
        }

        // #include <header>  or  #include "header"
        size_t open = t.find_first_of("<\"", 8);
        if (open == std::string::npos) { prologue << t << '\n'; return ""; }
        char   close_ch = t[open] == '<' ? '>' : '"';
        size_t close    = t.find(close_ch, open + 1);
        if (close == std::string::npos) { prologue << t << '\n'; return ""; }

        prologue << t.substr(0, close + 1) << '\n';
        t = trim(t.substr(close + 1));
        // `#include <iostream>;` is a stray semicolon the preprocessor would
        // have warned about.  As the first thing in a function body it is a
        // null statement -- legal, and not worth refusing the block over.
        while (!t.empty() && t[0] == ';') t = trim(t.substr(1));
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// satellite.return(EXPR) -- find the expression, matching nested parentheses so
// satellite.return(f(x, g(y))) works.
// ---------------------------------------------------------------------------
std::string return_expression(const std::string& src) {
    static const std::string marker = "satellite.return";
    size_t at = src.find(marker);
    if (at == std::string::npos) return "";

    size_t open = src.find('(', at + marker.size());
    if (open == std::string::npos) return "";

    int depth = 0;
    for (size_t k = open; k < src.size(); ++k) {
        if (src[k] == '(') ++depth;
        else if (src[k] == ')') {
            if (--depth == 0) return trim(src.substr(open + 1, k - open - 1));
        }
    }
    return "";
}

// remove the satellite.return(...) call from the body, leaving the rest
static std::string strip_return(const std::string& src) {
    static const std::string marker = "satellite.return";
    size_t at = src.find(marker);
    if (at == std::string::npos) return src;
    size_t open = src.find('(', at + marker.size());
    if (open == std::string::npos) return src;

    int depth = 0;
    for (size_t k = open; k < src.size(); ++k) {
        if (src[k] == '(') ++depth;
        else if (src[k] == ')' && --depth == 0)
            return src.substr(0, at) + src.substr(k + 1);
    }
    return src;
}

// ---------------------------------------------------------------------------
// Generate the translation unit for a block.
// ---------------------------------------------------------------------------
std::string generate(const std::string& block_source) {
    return generate(block_source, {});
}

std::string generate(const std::string& block_source,
                     const std::vector<CxxArg>& args) {
    std::string ret  = return_expression(block_source);
    std::string body = strip_return(block_source);

    // Split the block into what must live at file scope and what goes inside
    // the function.  #include and #define cannot appear in a function body, and
    // neither can templates, type definitions, or free functions -- C++ has no
    // nested functions.
    std::ostringstream prologue, code;
    std::istringstream in(body);
    std::string line;
    int  depth = 0;
    bool hoisting = false;

    while (std::getline(in, line)) {
        std::string t = trim(line);
        bool to_prologue = hoisting;

        if (!hoisting && depth == 0) {
            // Directives move to file scope; anything sharing the line with
            // them stays code and falls through to be emitted below.
            if (!t.empty() && t[0] == '#') {
                t = lift_directives(t, prologue);
                if (t.empty()) continue;
                line = t;
            }
            // `using namespace std; code...` on one line has the same problem,
            // and fails LOUDLY rather than silently -- the trailing code lands
            // at file scope and does not compile.  Split at the semicolon for
            // the same reason.
            if (t.rfind("using namespace", 0) == 0) {
                size_t semi = t.find(';');
                if (semi == std::string::npos) { prologue << line << '\n'; continue; }
                prologue << t.substr(0, semi + 1) << '\n';
                t = trim(t.substr(semi + 1));
                if (t.empty()) continue;
                line = t;
            }
            if (opens_file_scope_construct(line)) { hoisting = true; to_prologue = true; }
        }

        if (to_prologue) prologue << line << '\n';
        else             code     << "        " << line << '\n';

        depth += brace_delta(line);
        if (depth < 0) depth = 0;
        if (hoisting && depth == 0) hoisting = false;   // construct complete
    }

    std::ostringstream out;
    out << "// generated by satellite.cxx -- do not edit\n"
        // THE CONTRACT STAMP, and the reason it is in the source rather than
        // off to the side: the cache is keyed by a hash of this text, and a
        // module bakes in more than the text it was generated from.  It is
        // compiled against container.hpp, so it carries its own copy of the
        // inline operators and with them the [TCOUNT][TCOUNT] row stride of the
        // dispatch tables.  Grow Type and a cached module still indexes at the
        // old stride, reaching a different cell and calling a different
        // function for the same pair of types -- wrong answers, no diagnostic.
        // Nothing about the block's own text changes when that happens, so
        // hashing the block alone would serve that module up forever.  Stamped
        // here, the hash moves and the block is recompiled.  The ABI check in
        // Bridge::run cannot do this job: it refuses a module, it never
        // rebuilds one.
        << "// contract: abi " << ABI << ", " << TCOUNT << " types, container "
        << sizeof(Container) << " bytes\n"
        << "#include \"satellite/cxx.hpp\"\n"
        << "#include <string>\n"
        << "#include <vector>\n"
        << "#include <exception>\n"
        << prologue.str()
        << "\n"
        // `cout << x` without the std:: -- because a block is a snippet, not a
        // header, and nothing else will ever #include it.  The usual objection
        // to a using-directive is that it leaks into every translation unit
        // that pulls the header in; a block has no such translation unit.
        //
        // The JIT preamble in src/jit.cpp already did this, so without it here
        // the same line behaved differently depending on which compiler ran it.
        // One language, one rule.
        //
        // AFTER the prologue, so it covers the user's own #includes too.  A
        // using-directive is a lookup rule rather than a snapshot, so headers
        // included above it are still reached.
        << "using namespace std;\n"
        << "using namespace satellite;\n"
        << "using namespace satellite::cxx;\n"
        << "\n"
        // The CONSTANT, not its value.  Writing the number here would report
        // the ABI of whatever host generated the text, and the check in run()
        // wants the ABI of the headers the module was actually COMPILED
        // against -- those are the ones whose layout it inlined.  They differ
        // exactly when something is stale, which is the case worth catching.
        << "extern \"C\" int satellite_cxx_abi() { return satellite::cxx::ABI; }\n"
        << "\n"
        << "extern \"C\" satellite::Container satellite_cxx_block(\n"
        << "        const satellite::Container* args, int argc) {\n"
        << "    (void)args; (void)argc;\n";

    // The argument count is checked by the MODULE, not by the caller.  A module
    // is cached on disk and outlives the run that built it, so the only thing
    // that can be trusted to know how many arguments it was compiled for is the
    // module itself.
    if (!args.empty()) {
        out << "    if (argc < " << args.size() << " || !args)\n"
            << "        return satellite::Container::str(\n"
            << "            \"cxx error: block expects " << args.size()
            << " argument(s), got \" + std::to_string(argc));\n";
    }

    // An exception unwinding into the interpreter's frames is undefined
    // behaviour, so nothing is allowed to escape this function.
    out << "    try {\n";

    // Arguments become named, typed locals -- the whole point of the header.
    // Declared INSIDE the try, because a std::string constructor allocates and
    // that is not a reason to take the process down.
    //
    // from_container is an overload set, so the type named on the left picks
    // the conversion on the right.  A type it has no overload for arrives as a
    // Container and is copied through untouched, which is why cxx_type_for can
    // fall back to Container instead of refusing.
    for (size_t k = 0; k < args.size(); ++k) {
        const char* ty = cxx_type_for(args[k].value.type);
        out << "        " << ty << ' ' << args[k].name;
        if (std::strcmp(ty, "satellite::Container") == 0) {
            out << " = args[" << k << "];\n";
        } else {
            out << "{};\n"
                << "        satellite::cxx::from_container(args[" << k
                << "], &" << args[k].name << ");\n";
        }
        // A block is free to ignore an argument it was handed; an unused
        // variable is not a mistake worth failing a compile over.
        out << "        (void)" << args[k].name << ";\n";
    }

    out << code.str();
    if (!ret.empty())
        out << "        return satellite::cxx::to_container(" << ret << ");\n";
    else
        out << "        return satellite::Container::nil();\n";
    out << "    } catch (const std::exception& e) {\n"
        << "        return satellite::Container::str(\n"
        << "            std::string(\"cxx error: \") + e.what());\n"
        << "    } catch (...) {\n"
        << "        return satellite::Container::str(\"cxx error: unknown exception\");\n"
        << "    }\n"
        << "}\n";
    return out.str();
}

}  // namespace cxx
}  // namespace satellite
