#pragma once

#include "lexical_analyzer/lexer.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace satellite {

// Half-open source range, in bytes. Byte offsets rather than decoded columns
// because decode() is neither injective nor stable — \cwd changes width after
// a chdir — so a caret computed from decoded text drifts.
//
// `file` indexes the SourceMap below, and exists because a program is about to
// stop being one file (§16). Without it a runtime error inside an included
// spaceship prints its line number against the WRONG spaceship's text, and a
// language whose errors lie about where they are is unusable.
//
// The id is free. `line` gives up 32 bits it never had a use for — a source
// with four billion lines is not a source — and `file` spends them, so the
// struct is the same 24 bytes it was and the Expr=96 / Stmt=200 budgets below
// are untouched.
struct Span {
    size_t start = 0;
    size_t end = 0;
    uint32_t line = 1;
    uint32_t file = 0;
};

// The struct sizes below are budgets DESIGN.md quotes as settled facts, and
// until now they were enforced by nothing: ast_test printed them in a banner
// line, and its one actual check was relative — sizeof(Value) < sizeof(Expr) —
// which passes just as happily at Value=400, Expr=4000. A printed number that
// nobody compares is not a budget.
//
// Span is the one to watch. §16 gave it a file id so an error inside an
// included spaceship can name that spaceship, and the plan was that two size_t
// plus two uint32 is still 24 bytes. That has now happened, and this assert is
// what makes the claim testable instead of hopeful.
//
// Guarded on 64-bit because both Debian binary packages are Architecture: any,
// and size_t is 4 bytes on i386 and armhf, where every figure here halves. The
// budgets are about the 64-bit layout; a hard assert would just break those
// builds without telling anyone anything true.
static_assert(sizeof(void *) != 8 || sizeof(Span) == 24,
              "Span must stay 24 bytes on 64-bit — Expr=96 and Stmt=200 rest on it");

// One source text per loaded spaceship, indexed by Span::file.
//
// This is C++ bookkeeping internal to the interpreter and is NOT a language
// feature. In particular it is emphatically not satellite.container.map (§8.6):
// that distinction is not about which one exists — the map landed with M9 — it
// is that a SourceMap is a vector the loader owns and would not become
// satellite's map even now that satellite has one. Ids are dense and handed out
// in load order, so a vector index is the whole lookup.
//
// A path may be empty. That is not a defect to paper over: the REPL evaluates
// a line that came from no file at all, and an error in it has to say "line 3"
// rather than name a file that does not exist.
class SourceMap {
public:
    SourceMap() = default;

    // The one-source case — the REPL, and every test that renders an error.
    explicit SourceMap(std::string text, std::string path = {});

    // Returns the id to put in every Span parsed from this text.
    uint32_t add(std::string text, std::string path = {});

    // Both answer an empty string for an id that was never added, so a
    // malformed Span degrades to a caret with no source line rather than
    // reading off the end of a vector.
    const std::string &text(uint32_t file) const;
    const std::string &path(uint32_t file) const;

    size_t size() const { return files_.size(); }

private:
    struct Entry {
        std::string text;
        std::string path;
    };
    std::vector<Entry> files_;
};

// "line 3" when the span's source has no path, "lexer.satl:3" when it has one.
// Shared by all three format_error implementations, which otherwise render
// nothing alike — the parser's caret block and the resolver's one-liner were
// written years apart and there is no reason to unify them, but WHERE an error
// happened must read the same everywhere or a multi-file program teaches the
// reader two vocabularies for one idea.
std::string span_location(const Span &span, const SourceMap &sources);

Span span_of(const Token &token, uint32_t file = 0);
Span span_join(Span first, Span last);

} // namespace satellite
