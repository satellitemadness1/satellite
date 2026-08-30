// Source View implementation -- Milestone 5.

#include "source_view.hpp"

#include <algorithm>

namespace satellite {

namespace {

std::vector<uint32_t> find_line_starts(const std::string &text)
{
    std::vector<uint32_t> line_starts;
    line_starts.push_back(0); // line 1 starts at byte 0
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '\n') {
            line_starts.push_back(static_cast<uint32_t>(i + 1));
        }
    }
    return line_starts;
}

} // namespace

SourceLocation locate_span(const Span &span, const SourceMap &sources)
{
    if (span.file >= sources.size())
        return SourceLocation{span.line > 0 ? span.line : 1, 1};

    const std::string &src = sources.text(span.file);
    if (src.empty())
        return SourceLocation{1, 1};

    auto line_starts = find_line_starts(src);
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), span.start);
    size_t line_idx = std::distance(line_starts.begin(), it);
    if (line_idx > 0) line_idx--;

    uint32_t line_start = line_starts[line_idx];
    uint32_t col = (span.start >= line_start) ? (span.start - line_start + 1) : 1;

    return SourceLocation{static_cast<uint32_t>(line_idx + 1), col};
}

std::string format_span_location(const Span &span, const SourceMap &sources)
{
    const std::string &p = sources.path(span.file);
    SourceLocation loc = locate_span(span, sources);
    std::string out;
    if (!p.empty()) {
        out = p + ":";
    }
    out += std::to_string(loc.line) + ":" + std::to_string(loc.column);
    return out;
}

std::vector<SourceLineSlice> extract_span_lines(
    const Span &span,
    const SourceMap &sources,
    size_t max_lines)
{
    std::vector<SourceLineSlice> result;
    if (span.file >= sources.size()) return result;

    const std::string &src = sources.text(span.file);
    if (src.empty()) return result;

    auto line_starts = find_line_starts(src);
    if (line_starts.empty()) return result;

    SourceLocation start_loc = locate_span(span, sources);
    Span end_span = span;
    end_span.start = (span.end > span.start) ? span.end - 1 : span.start;
    SourceLocation end_loc = locate_span(end_span, sources);

    uint32_t first_line = start_loc.line;
    uint32_t last_line = std::max(start_loc.line, end_loc.line);

    for (uint32_t line = first_line; line <= last_line && result.size() < max_lines; line++) {
        size_t idx = line - 1;
        if (idx >= line_starts.size()) break;

        uint32_t start_byte = line_starts[idx];
        uint32_t end_byte = (idx + 1 < line_starts.size())
            ? (line_starts[idx + 1] > 0 ? line_starts[idx + 1] - 1 : 0)
            : static_cast<uint32_t>(src.size());

        // Strip trailing \r if present
        if (end_byte > start_byte && src[end_byte - 1] == '\r')
            end_byte--;

        std::string line_str = src.substr(start_byte, end_byte - start_byte);

        uint32_t c_start = (line == first_line) ? start_loc.column : 1;
        uint32_t c_end = (line == last_line) ? (end_loc.column + 1) : static_cast<uint32_t>(line_str.size() + 1);

        if (span.start == span.end && line == first_line) {
            c_end = c_start + 1;
        }

        result.push_back(SourceLineSlice{line, std::move(line_str), c_start, c_end});
    }

    return result;
}

} // namespace satellite
