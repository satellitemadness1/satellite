#pragma once

// Diagnostic Data Model -- Milestone 5 Error Reporter.
//
// Structured diagnostic representation:
// { ErrorCode, Span, vector<Note>, vector<FrameRef>, optional<Suggestion> }
//
// DESIGN §9: "The shape here: { ErrorCode, Span, vector<Note>, vector<FrameRef> }
// with rendering in exactly one place. Spans on every node. A source excerpt
// with a caret. Notes carrying their own spans. And 'did you mean' over the
// trie level that failed (§4.6)."

#include "ast.hpp"
#include "diagnostic_code.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace satellite {

// Severity level of a diagnostic or note.
enum class Severity : uint8_t {
    Error,
    Warning,
    Note,
    Help,
};

const char *severity_name(Severity sev);

// Secondary note attached to a diagnostic with its own span.
struct Note {
    std::string message;
    Span span;
    Severity severity = Severity::Note;
};

// Execution / resolve frame reference for call stack traces.
struct FrameRef {
    std::string capsule_name;
    Span call_site;
    words::PathId path_id = words::kNoPath;
};

// Fix-it or "did you mean" suggestion.
struct Suggestion {
    std::string message;         // e.g. "did you mean 'console'?"
    Span span;                   // span to be replaced
    std::string replacement;     // suggested text replacement
};

// Full diagnostic record.
struct Diagnostic {
    ErrorCode code = ErrorCode::None;
    Severity severity = Severity::Error;
    std::string message;
    Span primary_span;
    std::vector<Note> notes;
    std::vector<FrameRef> stack;
    std::optional<Suggestion> suggestion;

    // Fluent builder helpers
    Diagnostic &add_note(std::string msg, Span sp = {}, Severity sev = Severity::Note);
    Diagnostic &add_frame(std::string cap, Span site, words::PathId pid = words::kNoPath);
    Diagnostic &with_suggestion(std::string msg, Span sp = {}, std::string repl = "");

    // Factory methods
    static Diagnostic error(ErrorCode code, std::string message, Span span = {});
    static Diagnostic warning(ErrorCode code, std::string message, Span span = {});
    static Diagnostic note(std::string message, Span span = {});
    static Diagnostic help(std::string message, Span span = {});
};

} // namespace satellite
