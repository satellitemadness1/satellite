#include "satellite/capsule.hpp"

#include <utility>

#include "satellite/library.hpp"

namespace satellite {

// The `satellite.` prefix survives only on main.  Library::normalize_location
// collapses `satellite.helper` and `helper` onto the same location, so for
// every other capsule the qualified spelling names nothing new -- but the entry
// point is REQUIRED to be spelled satellite.main, and its signature should read
// the way the declaration has to be written.
std::string Capsule::signature() const {
    std::string s = is_main ? "satellite." + name : name;
    s += "(";
    for (size_t k = 0; k < params.size(); ++k) {
        if (k) s += ", ";
        if (!params[k].declared_type.empty()) s += params[k].declared_type + " ";
        s += params[k].name;
    }
    return s + ")";
}

// ===========================================================================
// Scanning
// ===========================================================================
namespace {

bool is_ident(const Token& t, const char* w) {
    return t.kind == Tok::Ident && t.text == w;
}

// The declaration keyword.  `capsule` follows a dot and so arrives as an
// ordinary Tok::Ident -- the same shape find_main() matches.
bool at_header(const std::vector<Token>& t, size_t i) {
    return i + 2 < t.size() && t[i].kind == Tok::Satellite &&
           t[i + 1].kind == Tok::Dot && is_ident(t[i + 2], "capsule");
}

// ---------------------------------------------------------------------------
// One forward-only pass.  Every exit from take() returns an index strictly
// greater than the one it was given, which is what makes a malformed
// declaration cost one skipped token rather than an infinite loop.
// ---------------------------------------------------------------------------
struct Scan {
    const std::vector<Token>& t;
    SourceId                  sid;
    CapsuleScan               out;

    Scan(const std::vector<Token>& tokens, SourceId s) : t(tokens), sid(s) {}

    // Every diagnostic points at the header token.  That is deliberate: the
    // lexer records a punctuation token's column AFTER consuming the character,
    // so a caret on a brace or a comma would land one column late, while a word
    // token (`satellite`) carries the exact column it started at.  The detail
    // that would have been carried by position goes in the message instead.
    void error(size_t hdr, const std::string& msg) {
        Diagnostic d;
        d.message  = msg;
        d.line     = t[hdr].line;
        d.col      = t[hdr].col;
        d.is_error = true;
        out.diagnostics.push_back(std::move(d));
    }

    bool declared(const std::string& n) const {
        for (const Container& c : out.capsules)
            if (c.as_capsule()->name == n) return true;
        return false;
    }

    // `satellite.variable.int n` -> Param{"n","int"};  a bare `n` -> Param{"n",""}.
    // Returns false with a diagnostic already recorded; *j is left on the token
    // that could not be read.
    bool params(size_t hdr, const std::string& name, size_t* j,
                std::vector<Param>* into) {
        size_t k = *j;
        if (k < t.size() && t[k].kind == Tok::RParen) { *j = k + 1; return true; }

        for (;;) {
            Param p;
            if (k + 5 < t.size() && t[k].kind == Tok::Satellite &&
                t[k + 1].kind == Tok::Dot && is_ident(t[k + 2], "variable") &&
                t[k + 3].kind == Tok::Dot && t[k + 4].kind == Tok::Ident &&
                t[k + 5].kind == Tok::Ident) {
                p.declared_type = t[k + 4].text;
                p.name          = t[k + 5].text;
                k += 6;
            } else if (k < t.size() && t[k].kind == Tok::Ident) {
                p.name = t[k].text;
                k += 1;
            } else {
                error(hdr, "capsule '" + name + "': expected a parameter name");
                *j = k;
                return false;
            }
            into->push_back(std::move(p));

            if (k < t.size() && t[k].kind == Tok::Comma)  { ++k; continue; }
            if (k < t.size() && t[k].kind == Tok::RParen) { *j = k + 1; return true; }
            error(hdr, "capsule '" + name + "': expected ',' or ')' in the "
                       "parameter list");
            *j = k;
            return false;
        }
    }

    size_t take(size_t hdr) {
        size_t j = hdr + 3;                    // past `satellite . capsule`

        // --- the name: `helper` or `satellite.main` -------------------------
        std::string name;
        bool qualified = false;
        if (j + 2 < t.size() && t[j].kind == Tok::Satellite &&
            t[j + 1].kind == Tok::Dot && t[j + 2].kind == Tok::Ident) {
            name = t[j + 2].text;
            qualified = true;
            j += 3;
        } else if (j < t.size() && t[j].kind == Tok::Ident) {
            name = t[j].text;
            j += 1;
        } else {
            error(hdr, "satellite.capsule must be followed by a capsule name");
            return j;
        }

        // --- the parameter list ---------------------------------------------
        if (j >= t.size() || t[j].kind != Tok::LParen) {
            error(hdr, "capsule '" + name + "' has no parameter list -- "
                       "write '()' for a capsule that takes none");
            return j;
        }
        ++j;
        std::vector<Param> ps;
        if (!params(hdr, name, &j, &ps)) return j;

        // --- the body -------------------------------------------------------
        // A newline between ')' and '{' is ordinary style and the lexer emits a
        // Term for it.  Nothing else may sit there.
        while (j < t.size() && t[j].kind == Tok::Term) ++j;
        if (j >= t.size() || t[j].kind != Tok::LBrace) {
            error(hdr, "capsule '" + name + "' has no body -- expected '{'");
            return j;
        }
        size_t lbrace = j;

        // Brace counting is exact for a stream that LEXED CLEANLY, because the
        // two constructs that could hide a brace never present one to this
        // loop: Tok::CxxBlock is a single token whose braces scan_cxx_block
        // already consumed -- Lexer::skip_block_gap crosses comments as well as
        // whitespace looking for the '{', so a comment before it cannot spill
        // the C++ back into this stream -- and a string literal is a single
        // Tok::Str whose text is the decoded body.
        //
        // The qualifier is load-bearing.  `'}'` is not satellite syntax, so on
        // a stream that failed to lex it arrives as Unknown RBrace Unknown and
        // that RBrace closes the body early.  interpret_source stops at
        // Stage::Lex before reaching here, so the pipeline never sees it; a
        // caller that scans tokens from a failed lex will.
        int    depth = 0;
        size_t k     = lbrace;
        for (; k < t.size(); ++k) {
            if (t[k].kind == Tok::LBrace) ++depth;
            else if (t[k].kind == Tok::RBrace && --depth == 0) break;
        }
        if (k >= t.size()) {
            error(hdr, "capsule '" + name + "' has an unterminated body -- no '}' "
                       "closes the '{' on line " + std::to_string(t[lbrace].line));
            return t.size();                   // nothing past this is scannable
        }

        if (declared(name)) {
            error(hdr, "capsule '" + name + "' is already declared");
            return k + 1;
        }

        auto* c = new Capsule();
        c->name = name;
        // The capsule's variables live at satellite.library.<name>.<var>, so
        // its canonical location is the bare name -- which is exactly what
        // Library::normalize_location maps both spellings onto.
        c->location    = name;
        c->params      = std::move(ps);
        c->source_id   = sid;
        c->header_line = (uint32_t)t[hdr].line;
        c->body_first  = (uint32_t)t[lbrace].line;
        c->body_last   = (uint32_t)t[k].line;
        c->first_token = hdr;
        c->last_token  = k;
        // Only the qualified spelling is the entry point, matching find_main().
        // A capsule declared as a bare `main` is a capsule that happens to be
        // called main, and the program still has no entry point.
        c->is_main = qualified && name == "main";

        out.capsules.push_back(Container::capsule(c));
        return k + 1;
    }
};

}  // namespace

CapsuleScan scan_capsules(const std::vector<Token>& tokens, SourceId sid) {
    Scan s(tokens, sid);

    for (size_t i = 0; i + 2 < tokens.size();) {
        if (at_header(tokens, i)) i = s.take(i);
        else                      ++i;
    }

    s.out.ok = true;
    for (const Diagnostic& d : s.out.diagnostics)
        if (d.is_error) { s.out.ok = false; break; }
    return std::move(s.out);
}

// ===========================================================================
// Registration
// ===========================================================================
bool register_capsules(Library& lib, const CapsuleScan& scan,
                       std::vector<Diagnostic>* out) {
    bool ok = true;

    for (const Container& v : scan.capsules) {
        if (v.type != Type::Capsule) continue;
        const Capsule* c = v.as_capsule();

        // Redefining a capsule over a capsule is the Library's ordinary
        // redefine and keeps the slot.  Landing on a variable or a spacesuit is
        // a genuine clash: one name cannot mean both.
        Slot s = lib.slot_of(kGlobal, c->name);
        if (s != kNoSlot && lib.kind_of(s) != Kind::Capsule) {
            if (out) {
                Diagnostic d;
                d.message = "capsule '" + c->name + "' collides with " +
                            Library::display_path(kGlobal, c->name) +
                            ", which is already a " + kind_name(lib.kind_of(s));
                d.line     = (int)c->header_line;
                d.col      = 0;
                d.is_error = true;
                out->push_back(std::move(d));
            }
            ok = false;
            continue;
        }

        lib.define(kGlobal, c->name, v, Kind::Capsule);
    }
    return ok;
}

}  // namespace satellite
