// The interpreter front door -- version 001.
//
// Everything here is honest about how far the implementation actually goes.
// There is no parser and no virtual machine, so this file lexes, checks the two
// structural requirements that can be checked from a token stream alone, runs
// any satellite.cxx blocks (that path IS complete), and then stops at
// Stage::Validate saying so.
#include "satellite/interpret.hpp"

#include <sys/stat.h>

#include <fstream>
#include <sstream>

#include "satellite/cxx.hpp"

namespace satellite {

// ---------------------------------------------------------------------------
const char* stage_name(Stage s) {
    switch (s) {
        case Stage::Lex:      return "lex";
        case Stage::Validate: return "validate";
        case Stage::Parse:    return "parse";
        case Stage::Compile:  return "compile";
        case Stage::Execute:  return "execute";
    }
    return "?";
}

// ---------------------------------------------------------------------------
std::string Diagnostic::format(const std::string& file) const {
    std::ostringstream ss;
    ss << file << ':' << line << ':' << col << ": "
       << (is_error ? "error: " : "warning: ") << message;
    return ss.str();
}

bool InterpretResult::has_errors() const {
    for (const auto& d : diagnostics)
        if (d.is_error) return true;
    return false;
}

int InterpretResult::error_count() const {
    int n = 0;
    for (const auto& d : diagnostics)
        if (d.is_error) ++n;
    return n;
}

// ---------------------------------------------------------------------------
// Path resolution
// ---------------------------------------------------------------------------
static bool is_readable_file(const std::string& path) {
    if (path.empty()) return false;
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) return false;
    // A directory named `hello` must not shadow `hello.satl`.
    return S_ISREG(st.st_mode) != 0;
}

std::string resolve_satl_path(const std::string& typed) {
    if (typed.empty()) return "";
    if (is_readable_file(typed)) return typed;
    std::string with_ext = typed + ".satl";
    if (is_readable_file(with_ext)) return with_ext;
    return "";
}

// ---------------------------------------------------------------------------
// Structural checks
//
// These match on the token stream directly.  Note that in
// `satellite.include(satellite)` the two occurrences of the word lex
// DIFFERENTLY: the first is Tok::Satellite, and so is the second -- it follows
// a '(' rather than a '.', and the keyword rule only demotes the word to an
// identifier when a dot precedes it.  `include` and `main`, which DO follow a
// dot, arrive as Tok::Ident.  Verified against the real lexer.
// ---------------------------------------------------------------------------
static bool is_ident(const Token& t, const char* name) {
    return t.kind == Tok::Ident && t.text == name;
}

bool declares_satellite_include(const std::vector<Token>& tokens) {
    if (tokens.size() < 6) return false;
    for (size_t i = 0; i + 5 < tokens.size(); ++i) {
        if (tokens[i].kind     == Tok::Satellite &&
            tokens[i + 1].kind == Tok::Dot       &&
            is_ident(tokens[i + 2], "include")   &&
            tokens[i + 3].kind == Tok::LParen    &&
            tokens[i + 4].kind == Tok::Satellite &&
            tokens[i + 5].kind == Tok::RParen)
            return true;
    }
    return false;
}

bool declares_main(const std::vector<Token>& tokens) {
    if (tokens.size() < 6) return false;
    for (size_t i = 0; i + 5 < tokens.size(); ++i) {
        if (tokens[i].kind     == Tok::Satellite &&
            tokens[i + 1].kind == Tok::Dot       &&
            is_ident(tokens[i + 2], "capsule")   &&
            tokens[i + 3].kind == Tok::Satellite &&
            tokens[i + 4].kind == Tok::Dot       &&
            is_ident(tokens[i + 5], "main"))
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Interpretation
// ---------------------------------------------------------------------------
InterpretResult interpret_source(const std::string& source,
                                 const std::string& name,
                                 InterpretOptions opts) {
    InterpretResult r;
    r.file    = name;
    r.reached = Stage::Lex;
    r.ok      = false;

    // ---- lex ---------------------------------------------------------------
    Lexer lx(source);
    std::vector<Token> tokens = lx.scan();
    if (!lx.ok()) {
        for (const auto& e : lx.errors())
            r.diagnostics.push_back({e.message, e.line, e.col, true});
        std::ostringstream ss;
        ss << "failed -- " << r.error_count() << " lex error"
           << (r.error_count() == 1 ? "" : "s") << ", stopped at lex";
        r.summary = ss.str();
        return r;
    }

    size_t cxx_blocks = 0;
    for (const auto& t : tokens)
        if (t.kind == Tok::CxxBlock) ++cxx_blocks;

    // ---- validate ----------------------------------------------------------
    bool structural_failure = false;

    if (opts.require_main_include && !declares_satellite_include(tokens)) {
        r.diagnostics.push_back(
            {"every main file must declare satellite.include(satellite)",
             1, 1, true});
        structural_failure = true;
    }
    if (opts.require_main && !declares_main(tokens)) {
        r.diagnostics.push_back(
            {"every main file must declare a satellite.capsule satellite.main "
             "capsule",
             1, 1, true});
        structural_failure = true;
    }

    if (structural_failure) {
        r.reached = Stage::Validate;
        std::ostringstream ss;
        ss << "failed -- " << r.error_count() << " error"
           << (r.error_count() == 1 ? "" : "s") << ", stopped at validate";
        r.summary = ss.str();
        return r;
    }

    r.reached = Stage::Validate;

    // ---- the one thing that really executes: satellite.cxx blocks ----------
    size_t ran = 0;
    if (opts.run_cxx_blocks && cxx_blocks > 0) {
        cxx::Bridge bridge;
        size_t n = 0;
        for (const auto& t : tokens) {
            if (t.kind != Tok::CxxBlock) continue;
            ++n;
            std::string err;
            Container v = bridge.run(t.text, &err);
            if (!err.empty()) {
                std::ostringstream ms;
                ms << "cxx block " << n << " failed: " << err;
                r.diagnostics.push_back({ms.str(), t.line, t.col, true});
                continue;
            }
            std::ostringstream ls;
            ls << "cxx block " << n << " (line " << t.line << ") => "
               << v.to_string() << " [" << type_name(v.type) << "]";
            r.output.push_back(ls.str());
            ++ran;
        }
    }

    // ---- stop, and say so --------------------------------------------------
    // Version 001 has no parser and no VM.  A file that validates cleanly has
    // NOT been executed, and the result must never imply that it was.
    r.diagnostics.push_back(
        {"execution stopped at validate: version 001 has no parser or virtual "
         "machine, so satellite statements were checked but not executed",
         1, 1, false});

    r.ok = !r.has_errors();

    std::ostringstream ss;
    ss << (r.ok ? "ok" : "failed") << " -- " << tokens.size() << " tokens, "
       << cxx_blocks << " cxx block" << (cxx_blocks == 1 ? "" : "s");
    if (opts.run_cxx_blocks && cxx_blocks > 0)
        ss << " (" << ran << " ran)";
    ss << ", stopped at validate (no parser yet)";
    r.summary = ss.str();
    return r;
}

// ---------------------------------------------------------------------------
InterpretResult interpret_file(const std::string& path, InterpretOptions opts) {
    std::string resolved = resolve_satl_path(path);
    if (resolved.empty()) {
        InterpretResult r;
        r.file    = path;
        r.reached = Stage::Lex;
        r.ok      = false;
        r.diagnostics.push_back(
            {"no such file: " + path + " (tried " + path + " and " + path +
                 ".satl)",
             0, 0, true});
        r.summary = "failed -- cannot open " + path;
        return r;
    }

    std::ifstream f(resolved, std::ios::binary);
    if (!f) {
        InterpretResult r;
        r.file    = resolved;
        r.reached = Stage::Lex;
        r.ok      = false;
        r.diagnostics.push_back({"cannot read " + resolved, 0, 0, true});
        r.summary = "failed -- cannot read " + resolved;
        return r;
    }
    std::ostringstream buf;
    buf << f.rdbuf();

    return interpret_source(buf.str(), resolved, opts);
}

}  // namespace satellite
