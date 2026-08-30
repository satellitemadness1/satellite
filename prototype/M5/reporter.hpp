#pragma once

// Diagnostic Reporter -- Milestone 5 Error Reporter.
//
// Collector and manager for compiler diagnostics.
// Enforces max error counts, tracks severity counts, and provides non-throwing
// reporting (DESIGN §9.1).

#include "diagnostic.hpp"
#include "renderer.hpp"
#include "source_view.hpp"

#include <cstddef>
#include <iosfwd>
#include <vector>

namespace satellite {

class DiagnosticReporter {
public:
    explicit DiagnosticReporter(size_t max_errors = 50);

    void report(Diagnostic diag);
    void error(ErrorCode code, std::string message, Span span = {});
    void warning(ErrorCode code, std::string message, Span span = {});
    void note(std::string message, Span span = {});
    void help(std::string message, Span span = {});

    bool has_errors() const { return error_count_ > 0; }
    size_t error_count() const { return error_count_; }
    size_t warning_count() const { return warning_count_; }
    size_t total_count() const { return diagnostics_.size(); }
    bool reached_limit() const { return error_count_ >= max_errors_; }

    const std::vector<Diagnostic> &diagnostics() const { return diagnostics_; }
    void clear();

    void emit_all(std::ostream &os, const SourceMap &sources, const DiagnosticRenderer &renderer) const;
    std::string format_all(const SourceMap &sources, const DiagnosticRenderer &renderer) const;

private:
    size_t max_errors_ = 50;
    size_t error_count_ = 0;
    size_t warning_count_ = 0;
    std::vector<Diagnostic> diagnostics_;
};

} // namespace satellite

