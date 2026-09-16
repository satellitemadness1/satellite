#pragma once

// The previous prompt entries and persistent history store.
// Milestone 11 Prototype in prototype/M11.

#include <cstddef>
#include <string>
#include <vector>

namespace satellite {

class History {
public:
    explicit History(std::string path = {});

    void add(const std::string &line);

    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

    const std::string &at(size_t index) const;

    const std::vector<std::string> &entries() const { return entries_; }
    const std::string &path() const { return path_; }

    bool load();
    bool save() const;

    static std::string default_path();
    static constexpr size_t MAX_ENTRIES = 1000;

private:
    std::vector<std::string> entries_;
    std::string path_;
};

} // namespace satellite

