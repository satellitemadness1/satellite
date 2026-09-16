// Diagnostic Reporter implementation -- Milestone 5.

#include "reporter.hpp"

#include <iostream>
#include <sstream>

namespace satellite {

DiagnosticReporter::DiagnosticReporter(size_t max_errors)
    : max_errors_(max_errors)
{
}

void DiagnosticReporter::report(Diagnostic diag)
{
    if (diag.severity == Severity::Error) {
        if (reached_limit()) return;
        error_count_++;
    } else if (diag.severity == Severity::Warning) {
        warning_count_++;
    }
    diagnostics_.push_back(std::move(diag));
}

void DiagnosticReporter::error(ErrorCode code, std::string message, Span span)
{
    report(Diagnostic::error(code, std::move(message), span));
}

void DiagnosticReporter::warning(ErrorCode code, std::string message, Span span)
{
    report(Diagnostic::warning(code, std::move(message), span));
}

void DiagnosticReporter::note(std::string message, Span span)
{
    report(Diagnostic::note(std::move(message), span));
}

void DiagnosticReporter::help(std::string message, Span span)
{
    report(Diagnostic::help(std::move(message), span));
}

void DiagnosticReporter::clear()
{
    diagnostics_.clear();
    error_count_ = 0;
    warning_count_ = 0;
}

void DiagnosticReporter::emit_all(
    std::ostream &os,
    const SourceMap &sources,
    const DiagnosticRenderer &renderer) const
{
    for (const auto &diag : diagnostics_) {
        renderer.render(os, diag, sources);
    }
}

std::string DiagnosticReporter::format_all(
    const SourceMap &sources,
    const DiagnosticRenderer &renderer) const
{
    std::ostringstream ss;
    emit_all(ss, sources, renderer);
    return ss.str();
}

} // namespace satellite

