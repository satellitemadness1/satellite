// satellite.capsule -- a named body of satellite source, and the scan that
// finds every declaration in a token stream.
//
//     satellite.capsule satellite.main(argv)
//     {
//         ... body ...
//     }
//
// THREE THINGS THAT MUST NOT BE CONFLATED
//
//   1. The PARAMETER LIST in the declaration is a list of NAMES.  Text, read
//      out of source, fixed the moment the declaration is scanned.
//   2. The ARGUMENTS at a call site are VALUES -- Containers, bound to those
//      names by position.  They are not source and they never become source.
//   3. The BODY is source, and it is stored as a SPAN: file id, first line,
//      last line.  Never as a copy of the bytes.
//
// WHY A SPAN AND NOT A COPY
//
//   - The bytes already live in the SourceMap.  Copying them into every
//     capsule stores the program twice for no gain.
//   - A diagnostic raised inside a capsule has to name the real file and the
//     real line.  A span already IS that answer; a detached copy would have to
//     carry the mapping back anyway.
//   - A satellite.cxx block inside the body is handed to a C++ compiler and
//     needs the exact bytes.  Source::span_lines() returns them byte for byte,
//     because every line kept its trailing newline.
//
// Code-as-data still works: a Source can be materialized from the span the
// moment someone asks for one.  That is a copy made on demand instead of a
// copy every capsule pays for whether or not anyone ever looks at it.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "satellite/container.hpp"
#include "satellite/interpret.hpp"   // Diagnostic
#include "satellite/lexer.hpp"
#include "satellite/source.hpp"

namespace satellite {

class Library;

// One declared parameter: the NAME, plus whatever type the declaration spelled.
struct Param {
    std::string name;            // "args_sometimes"
    std::string declared_type;   // "" if untyped, else "int"/"string"/... taken
                                 // from a satellite.variable.<type> name
};

// ---------------------------------------------------------------------------
struct Capsule : Obj {
    std::string        name;      // "main", "helper" -- always the bare name
    std::string        location;  // canonical library location

    std::vector<Param> params;    // the parameter NAMES, in declared order

    // The body span.  Lines are 1-BASED, both ends inclusive, because that is
    // what a diagnostic prints.  Source::span_lines() indexes the vector, so
    // materializing the body is span_lines(body_first - 1, body_last - 1).
    SourceId source_id   = kNoSource;
    uint32_t header_line = 0;     // the line `satellite.capsule` is on
    uint32_t body_first  = 0;     // the line holding '{'
    uint32_t body_last   = 0;     // the line holding the matching '}'

    // The same extent in tokens, inclusive: first_token is the Tok::Satellite
    // that opens the header, last_token the Tok::RBrace that closes the body.
    // A parser wants tokens and a diagnostic wants lines; recording both here
    // means neither has to re-derive the other by rescanning.
    size_t   first_token = 0;
    size_t   last_token  = 0;

    bool     is_main = false;

    Capsule() : Obj(Type::Capsule) {}

    size_t      arity() const { return params.size(); }
    std::string signature() const;   // "satellite.main(argv)" / "helper(a, b)"
};

// Not next to as_str() and as_list() because the downcast needs the complete
// type, and container.hpp deliberately only declares one.
inline Capsule* Container::as_capsule() const { return static_cast<Capsule*>(obj); }

// ---------------------------------------------------------------------------
// Scanning
//
// Matches TOKENS, not text.  A `satellite.capsule` written inside a comment or
// a string literal can therefore never be mistaken for a declaration: neither
// one ever reaches the token stream.  find_main() works the same way.
//
// The word `satellite` lexes as Tok::Satellite except directly after a dot,
// where it is an ordinary Tok::Ident.  So a header arrives as
//
//     satellite . capsule satellite . main (
//     ^Satellite  ^Ident  ^Satellite  ^Ident
//
// Errors are collected, never thrown and never fatal: one malformed capsule
// must not hide the twelve that follow it.
// ---------------------------------------------------------------------------
struct CapsuleScan {
    std::vector<Container>  capsules;     // each one Type::Capsule
    std::vector<Diagnostic> diagnostics;
    bool ok = false;                      // no error diagnostics
};

CapsuleScan scan_capsules(const std::vector<Token>& tokens,
                          SourceId sid = kNoSource);

// ---------------------------------------------------------------------------
// Registration
//
// A capsule is defined at the GLOBAL library location under its bare name:
//
//     satellite.library.main            the capsule itself
//     satellite.library.main.counter    a variable inside it
//
// which is why the location is global rather than "satellite" -- it puts the
// capsule and everything declared inside it in one readable hierarchy, and
// `satellite.library.main` then means what a reader expects it to mean.
//
// Returns false and appends a diagnostic when a capsule name collides with an
// existing name that is NOT a capsule.  Redeclaring a capsule over a capsule
// is the Library's ordinary redefine and keeps the slot.
// ---------------------------------------------------------------------------
bool register_capsules(Library& lib, const CapsuleScan& scan,
                       std::vector<Diagnostic>* out);

}  // namespace satellite
