#pragma once

// Source View and Location Mapping -- Milestone 5 Error Reporter.
//
// Converts byte offsets to line/column numbers, and extracts source line slices
// for diagnostic rendering from SourceMap (M4 AST).

#include "ast.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace satellite {

struct SourceLocation {
    uint32_t line = 1;
    uint32_t column = 1;
};

struct SourceLineSlice {
    uint32_t line_number = 1;
    std::string line_text;
    uint32_t col_start = 1; // 1-based start column within this line
    uint32_t col_end = 1;   // 1-based end column
};

// Computes (line, column) for a given span against a SourceMap.
SourceLocation locate_span(const Span &span, const SourceMap &sources);

// Formats location as "path:line:col" or "line:col".
std::string format_span_location(const Span &span, const SourceMap &sources);

// Extracts the line and relevant columns for a single-line or multi-line span.
std::vector<SourceLineSlice> extract_span_lines(
    const Span &span,
    const SourceMap &sources,
    size_t max_lines = 5);

} // namespace satellite
