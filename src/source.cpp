#include "satellite/source.hpp"

#include <climits>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#ifdef __linux__
#include <limits.h>
#include <stdlib.h>
#endif

namespace satellite {

// ---------------------------------------------------------------------------
// Source
// ---------------------------------------------------------------------------

// Split KEEPING the newlines.  std::getline would strip them, and then
// all_text() could not reproduce the file byte for byte.
void Source::set_text(const std::string& text) {
    lines.clear();
    size_t start = 0;
    while (start < text.size()) {
        size_t nl = text.find('\n', start);
        if (nl == std::string::npos) {
            lines.push_back(text.substr(start));       // no trailing newline
            break;
        }
        lines.push_back(text.substr(start, nl - start + 1));   // newline kept
        start = nl + 1;
    }
}

std::string_view Source::text_of(size_t n) const {
    if (n >= lines.size()) return {};
    std::string_view v(lines[n]);
    if (!v.empty() && v.back() == '\n') v.remove_suffix(1);
    if (!v.empty() && v.back() == '\r') v.remove_suffix(1);
    return v;
}

std::string Source::all_text() const {
    size_t total = 0;
    for (const auto& l : lines) total += l.size();
    std::string out;
    out.reserve(total);
    for (const auto& l : lines) out += l;
    return out;
}

std::string Source::span_lines(size_t first, size_t last) const {
    std::string out;
    if (first >= lines.size()) return out;
    if (last >= lines.size()) last = lines.size() - 1;
    size_t total = 0;
    for (size_t k = first; k <= last; ++k) total += lines[k].size();
    out.reserve(total);
    for (size_t k = first; k <= last; ++k) out += lines[k];
    return out;
}

uint32_t Source::line_offset(size_t n) const {
    uint32_t off = 0;
    for (size_t k = 0; k < n && k < lines.size(); ++k) off += (uint32_t)lines[k].size();
    return off;
}

uint32_t Source::byte_size() const {
    uint32_t total = 0;
    for (const auto& l : lines) total += (uint32_t)l.size();
    return total;
}

void Source::locate(uint32_t offset, int* line, int* col) const {
    uint32_t running = 0;
    for (size_t k = 0; k < lines.size(); ++k) {
        uint32_t len = (uint32_t)lines[k].size();
        if (offset < running + len) {
            if (line) *line = (int)k + 1;                 // 1-based
            if (col)  *col  = (int)(offset - running) + 1;
            return;
        }
        running += len;
    }
    // past the end: report the last position
    if (line) *line = lines.empty() ? 1 : (int)lines.size();
    if (col)  *col  = lines.empty() ? 1 : (int)lines.back().size() + 1;
}

// ---------------------------------------------------------------------------
// SourceMap
// ---------------------------------------------------------------------------
namespace {

std::string canonical(const std::string& path) {
    char buf[PATH_MAX];
    if (::realpath(path.c_str(), buf)) return std::string(buf);
    return path;                                  // does not exist yet; use as typed
}

bool read_whole_file(const std::string& path, std::string* out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    *out = ss.str();
    return true;
}

}  // namespace

SourceId SourceMap::load(const std::string& path, std::string* err) {
    std::string canon = canonical(path);

    auto it = by_path_.find(canon);
    if (it != by_path_.end()) return it->second;      // already loaded, do not re-read

    std::string text;
    if (!read_whole_file(canon, &text)) {
        if (err) *err = "cannot read " + path;
        return kNoSource;
    }

    sources_.emplace_back();
    Source& s = sources_.back();
    s.id   = (SourceId)sources_.size();              // ids are 1-based; 0 is kNoSource
    s.path = canon;
    s.name = path;
    s.set_text(text);

    by_path_[canon] = s.id;
    return s.id;
}

SourceId SourceMap::add_virtual(std::string name, const std::string& text) {
    sources_.emplace_back();
    Source& s = sources_.back();
    s.id   = (SourceId)sources_.size();
    s.name = std::move(name);
    s.set_text(text);
    return s.id;                                     // not in by_path_: never dedup'd
}

bool SourceMap::has(SourceId id) const {
    return id != kNoSource && id <= sources_.size();
}

const Source& SourceMap::get(SourceId id) const {
    static const Source kEmpty;
    if (!has(id)) return kEmpty;
    return sources_[id - 1];
}

Source& SourceMap::get(SourceId id) {
    static Source kEmpty;
    if (!has(id)) return kEmpty;
    return sources_[id - 1];
}

std::vector<SourceId> SourceMap::ids() const {
    std::vector<SourceId> out;
    out.reserve(sources_.size());
    for (const auto& s : sources_) out.push_back(s.id);
    return out;
}

std::string SourceMap::describe(SourceId id, uint32_t offset) const {
    if (!has(id)) return "<unknown>";
    const Source& s = get(id);
    int line = 0, col = 0;
    s.locate(offset, &line, &col);
    return s.name + ":" + std::to_string(line) + ":" + std::to_string(col);
}

std::string SourceMap::include_chain(SourceId id) const {
    std::vector<std::string> names;
    SourceId cur = id;
    int guard = 0;
    while (has(cur) && guard++ < 256) {              // guard against a cycle
        names.push_back(get(cur).name);
        cur = get(cur).included_from;
    }
    std::string out;
    for (size_t k = names.size(); k-- > 0;) {
        out += names[k];
        if (k) out += " -> ";
    }
    return out;
}

}  // namespace satellite
