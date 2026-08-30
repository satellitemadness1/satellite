// Diagnostic Renderer implementation -- Milestone 5.

#include "renderer.hpp"

#include <iomanip>
#include <sstream>
#include <unistd.h>

namespace satellite {

namespace {

// ANSI escape codes
constexpr const char *kReset     = "\033[0m";
constexpr const char *kBold      = "\033[1m";
constexpr const char *kBoldRed   = "\033[1;31m";
constexpr const char *kBoldYellow= "\033[1;33m";
constexpr const char *kBoldCyan  = "\033[1;36m";
constexpr const char *kBoldGreen = "\033[1;32m";

} // namespace

DiagnosticRenderer::DiagnosticRenderer(RenderOptions options)
    : options_(options)
{
}

bool DiagnosticRenderer::use_color() const
{
    if (options_.color == ColorMode::Always) return true;
    if (options_.color == ColorMode::Never) return false;
    return isatty(fileno(stderr));
}

std::string DiagnosticRenderer::style_header(Severity sev, ErrorCode code) const
{
    if (!use_color()) {
        std::string s;
        if (options_.show_error_codes && code != ErrorCode::None) {
            s += "[" + std::string(code_to_string(code)) + "] ";
        }
        s += severity_name(sev);
        return s;
    }

    std::string s = kBold;
    switch (sev) {
    case Severity::Error:   s += kBoldRed; break;
    case Severity::Warning: s += kBoldYellow; break;
    case Severity::Note:    s += kBoldCyan; break;
    case Severity::Help:    s += kBoldGreen; break;
    }

    if (options_.show_error_codes && code != ErrorCode::None) {
        s += "[" + std::string(code_to_string(code)) + "] ";
    }
    s += severity_name(sev);
    s += kReset;
    return s;
}

std::string DiagnosticRenderer::style_reset() const { return use_color() ? kReset : ""; }
std::string DiagnosticRenderer::style_bold() const { return use_color() ? kBold : ""; }
std::string DiagnosticRenderer::style_gutter() const { return use_color() ? "\033[34m" : ""; }
std::string DiagnosticRenderer::style_caret(Severity sev) const
{
    if (!use_color()) return "";
    switch (sev) {
    case Severity::Error:   return kBoldRed;
    case Severity::Warning: return kBoldYellow;
    case Severity::Note:    return kBoldCyan;
    case Severity::Help:    return kBoldGreen;
    }
    return "";
}

void DiagnosticRenderer::render_snippet(
    std::ostream &os,
    const Span &span,
    Severity sev,
    const SourceMap &sources,
    size_t gutter_width) const
{
    if (!options_.show_source_lines || (span.start == 0 && span.end == 0)) return;

    auto slices = extract_span_lines(span, sources, options_.max_snippet_lines);
    if (slices.empty()) return;

    const std::string gut_color = style_gutter();
    const std::string car_color = style_caret(sev);
    const std::string reset = style_reset();

    for (const auto &slice : slices) {
        // Gutter line
        os << gut_color << std::setw(static_cast<int>(gutter_width)) << slice.line_number
           << " | " << reset << slice.line_text << "\n";

        // Caret line
        os << gut_color << std::string(gutter_width, ' ') << " | " << reset;

        size_t start_col = (slice.col_start > 0) ? (slice.col_start - 1) : 0;
        size_t end_col = (slice.col_end > slice.col_start) ? (slice.col_end - 1) : (start_col + 1);

        // Print spaces before caret
        for (size_t i = 0; i < start_col; i++) {
            if (i < slice.line_text.size() && slice.line_text[i] == '\t')
                os << '\t';
            else
                os << ' ';
        }

        // Print carets
        os << car_color;
        size_t count = (end_col > start_col) ? (end_col - start_col) : 1;
        for (size_t i = 0; i < count; i++) {
            os << '^';
        }
        os << reset << "\n";
    }
}

void DiagnosticRenderer::render(std::ostream &os, const Diagnostic &diag, const SourceMap &sources) const
{
    const std::string loc = format_span_location(diag.primary_span, sources);
    const std::string b = style_bold();
    const std::string r = style_reset();

    // 1. Header: file:line:col: [E0xxx] error: message
    if (!loc.empty()) {
        os << b << loc << ": " << r;
    }
    os << style_header(diag.severity, diag.code) << ": " << b << diag.message << r << "\n";

    // Gutter width calculation
    SourceLocation loc_info = locate_span(diag.primary_span, sources);
    size_t gutter_width = std::max(size_t{2}, std::to_string(loc_info.line).size() + 1);

    // 2. Primary Source Excerpt
    render_snippet(os, diag.primary_span, diag.severity, sources, gutter_width);

    // 3. Notes
    for (const auto &note : diag.notes) {
        std::string note_loc = format_span_location(note.span, sources);
        os << style_gutter() << std::string(gutter_width, ' ') << " = " << r;
        os << (use_color() ? kBoldCyan : "") << "note: " << r << note.message;
        if (!note_loc.empty()) {
            os << " (" << note_loc << ")";
        }
        os << "\n";

        if (note.span.start > 0 || note.span.end > 0) {
            SourceLocation nloc = locate_span(note.span, sources);
            size_t n_gutter = std::max(gutter_width, std::to_string(nloc.line).size() + 1);
            render_snippet(os, note.span, note.severity, sources, n_gutter);
        }
    }

    // 4. Suggestion / Fix-it
    if (options_.show_suggestions && diag.suggestion.has_value()) {
        const auto &sug = *diag.suggestion;
        os << style_gutter() << std::string(gutter_width, ' ') << " = " << r;
        os << (use_color() ? kBoldGreen : "") << "help: " << r << sug.message << "\n";
    }

    // 5. Call stack frames
    if (options_.show_call_stack && !diag.stack.empty()) {
        os << style_gutter() << std::string(gutter_width, ' ') << " = " << r
           << (use_color() ? kBold : "") << "stack backtrace:\n" << r;
        for (size_t i = 0; i < diag.stack.size(); i++) {
            const auto &frame = diag.stack[i];
            std::string frame_loc = format_span_location(frame.call_site, sources);
            os << "    " << std::setw(2) << i << ": in capsule '" << frame.capsule_name << "'";
            if (!frame_loc.empty()) {
                os << " at " << frame_loc;
            }
            os << "\n";
        }
    }
}

std::string DiagnosticRenderer::render(const Diagnostic &diag, const SourceMap &sources) const
{
    std::ostringstream ss;
    render(ss, diag, sources);
    return ss.str();
}

} // namespace satellite
