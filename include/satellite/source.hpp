// satellite_source -- a loaded source file.
//
// Storage is a vector<string>, one per line.  Adding, removing and editing
// lines is what a vector is good at, and that is what source manipulation
// mostly is:
//
//     src.lines.insert(src.lines.begin() + 2, "new line\n");
//     src.lines.erase(src.lines.begin() + 7);
//     for (const std::string& line : src.lines) { ... }
//
// ONE RULE MAKES THIS CORRECT: every line KEEPS its trailing newline.
//
// std::getline strips newlines, and if you store lines stripped you can no
// longer reconstruct the file byte for byte -- trailing whitespace and \r are
// gone, and a satellite.cxx block rebuilt from those lines comes back a byte
// short.  That block is handed straight to a C++ compiler, so it has to arrive
// exactly as written.  Keeping the newline in the string makes concatenation
// exact and costs nothing.
//
// The last line has no newline if the file did not end with one.  That is how
// "does this file end in a newline" is remembered.
#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace satellite {

using SourceId = uint32_t;
constexpr SourceId kNoSource = 0;      // 0 means "not from a file"

class Source {
public:
    SourceId    id   = kNoSource;
    SourceId    included_from    = kNoSource;   // who pulled this in
    int         included_at_line = 0;           // and from which line
    std::string path;                  // canonical absolute path, "" if virtual
    std::string name;                  // what diagnostics print

    // The storage.  Public on purpose -- it is a vector, use it like one.
    // Each entry keeps its trailing '\n' except possibly the last.
    std::vector<std::string> lines;

    Source() = default;
    explicit Source(const std::string& text) { set_text(text); }

    // -----------------------------------------------------------------------
    // Convenience over `lines`
    // -----------------------------------------------------------------------
    size_t size()  const { return lines.size(); }
    bool   empty() const { return lines.empty(); }

    // The line WITHOUT its newline -- for display, diagnostics, comparison.
    std::string_view text_of(size_t n) const;

    // Split a whole file into lines, keeping every newline.
    void set_text(const std::string& text);

    // -----------------------------------------------------------------------
    // Exact bytes -- what satellite.cxx blocks and block comments need
    // -----------------------------------------------------------------------

    // Byte-for-byte reconstruction.  Exact because the newlines were kept.
    std::string all_text() const;

    // Lines [first, last] inclusive, newlines intact.
    std::string span_lines(size_t first, size_t last) const;

    // Byte offset of the start of a line, and the file's total byte size.
    uint32_t line_offset(size_t n) const;
    uint32_t byte_size() const;

    // byte offset -> 1-based line and column
    void locate(uint32_t offset, int* line, int* col) const;
};

// ---------------------------------------------------------------------------
// SourceMap -- owns every loaded file.
//
// std::deque, not std::vector: a deque never moves its elements as it grows,
// so a `const Source&` held while more files load stays valid.  This part is
// not about ergonomics -- a vector<Source> reallocating would invalidate
// references that other code is still holding, and that shows up as corrupted
// data rather than a crash.
// ---------------------------------------------------------------------------
class SourceMap {
public:
    // Canonicalises the path.  Already loaded -> returns the existing id and
    // does not re-read.  That is the deduplication.
    SourceId load(const std::string& path, std::string* err = nullptr);

    // For tests, the GUI's text buffer, and the REPL.
    SourceId add_virtual(std::string name, const std::string& text);

    const Source& get(SourceId id) const;
    Source&       get(SourceId id);
    bool          has(SourceId id) const;
    size_t        size() const { return sources_.size(); }

    std::vector<SourceId> ids() const;                 // load order

    std::string describe(SourceId id, uint32_t offset) const;  // "a.satl:12:5"
    std::string include_chain(SourceId id) const;              // "a -> b -> c"

private:
    std::deque<Source> sources_;                          // stable addresses
    std::unordered_map<std::string, SourceId> by_path_;   // dedup + cycle detect
};

}  // namespace satellite
