// Diagnostic Data Model implementation -- Milestone 5.

#include "diagnostic.hpp"

namespace satellite {

const char *severity_name(Severity sev)
{
    switch (sev) {
    case Severity::Error: return "error";
    case Severity::Warning: return "warning";
    case Severity::Note: return "note";
    case Severity::Help: return "help";
    }
    return "unknown";
}

Diagnostic &Diagnostic::add_note(std::string msg, Span sp, Severity sev)
{
    notes.push_back(Note{std::move(msg), sp, sev});
    return *this;
}

Diagnostic &Diagnostic::add_frame(std::string cap, Span site, words::PathId pid)
{
    stack.push_back(FrameRef{std::move(cap), site, pid});
    return *this;
}

Diagnostic &Diagnostic::with_suggestion(std::string msg, Span sp, std::string repl)
{
    suggestion = Suggestion{std::move(msg), sp, std::move(repl)};
    return *this;
}

Diagnostic Diagnostic::error(ErrorCode code, std::string message, Span span)
{
    Diagnostic d;
    d.code = code;
    d.severity = Severity::Error;
    d.message = std::move(message);
    d.primary_span = span;
    return d;
}

Diagnostic Diagnostic::warning(ErrorCode code, std::string message, Span span)
{
    Diagnostic d;
    d.code = code;
    d.severity = Severity::Warning;
    d.message = std::move(message);
    d.primary_span = span;
    return d;
}

Diagnostic Diagnostic::note(std::string message, Span span)
{
    Diagnostic d;
    d.code = ErrorCode::None;
    d.severity = Severity::Note;
    d.message = std::move(message);
    d.primary_span = span;
    return d;
}

Diagnostic Diagnostic::help(std::string message, Span span)
{
    Diagnostic d;
    d.code = ErrorCode::None;
    d.severity = Severity::Help;
    d.message = std::move(message);
    d.primary_span = span;
    return d;
}

} // namespace satellite

