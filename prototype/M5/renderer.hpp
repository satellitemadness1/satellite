#pragma once

// Diagnostic Renderer -- Milestone 5 Error Reporter.
//
// Renders diagnostics with file/line/col headers, error codes, source excerpts,
// line gutters, multi-character caret spans, secondary notes, and call stack frames.
//
// DESIGN §9: "with rendering in exactly one place. Spans on every node.
// A source excerpt with a caret. Notes carrying their own spans. And 'did you mean'
// over the trie level that failed (§4.6)."

#include "diagnostic.hpp"
#include "source_view.hpp"

#include <iosfwd>
#include <string>

namespace satellite {

enum class ColorMode : uint8_t {
    Auto,   // use colors if stdout/stderr is a TTY
    Always, // always emit ANSI escape codes
    Never,  // never emit ANSI escape codes
};

struct RenderOptions {
    ColorMode color = ColorMode::Auto;
    bool show_error_codes = true;
    bool show_source_lines = true;
    bool show_call_stack = true;
    bool show_suggestions = true;
    size_t max_snippet_lines = 4;
};

class DiagnosticRenderer {
public:
    explicit DiagnosticRenderer(RenderOptions options = {});

    void set_options(RenderOptions options) { options_ = options; }
    const RenderOptions &options() const { return options_; }

    std::string render(const Diagnostic &diag, const SourceMap &sources) const;
    void render(std::ostream &os, const Diagnostic &diag, const SourceMap &sources) const;

private:
    bool use_color() const;
    std::string style_header(Severity sev, ErrorCode code) const;
    std::string style_reset() const;
    std::string style_bold() const;
    std::string style_gutter() const;
    std::string style_caret(Severity sev) const;
    std::string style_note() const;
    std::string style_help() const;

    void render_snippet(std::ostream &os, const Span &span, Severity sev,
                        const SourceMap &sources, size_t gutter_width) const;

    RenderOptions options_;
};

} // namespace satellite

