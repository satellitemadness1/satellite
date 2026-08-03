// satellite_string -- 32 bits per character, holding satellite charmap codes.
#pragma once

#include <cstdint>
#include <string>

#include "satellite/charmap.hpp"
#include "satellite/types.hpp"

namespace satellite {

// satellite_string.  One character is one char32_t holding a satellite charmap
// code, so indexing is exact and O(1): my_str.remove(0) removes the first
// CHARACTER, never part of one.
//
// Short strings live in the inline buffer and never touch the allocator; most
// strings in most programs are short, and allocation -- not copying -- is what
// actually costs.
struct SatString : Obj {
    static constexpr uint32_t INLINE_CAP = 16;   // characters, not bytes

    uint32_t  len  = 0;              // in characters
    uint32_t  cap  = INLINE_CAP;
    char32_t* heap = nullptr;        // null while the string is inline
    char32_t  buf[INLINE_CAP];

    SatString() : Obj(Type::Str) { buf[0] = 0; }
    ~SatString() { delete[] heap; }

    char32_t*       data()       { return heap ? heap : buf; }
    const char32_t* data() const { return heap ? heap : buf; }
    bool inline_stored() const { return heap == nullptr; }

    // rendered back to ASCII for display; void characters show as '?'
    std::string text() const;
    char32_t at(uint32_t i) const { return i < len ? data()[i] : charmap::VOID; }

    void reserve(uint32_t want);
    void append(const char32_t* p, uint32_t n);
    void append_text(const std::string& ascii);   // maps through the charmap

    // --- read ---------------------------------------------------------------
    bool     empty() const { return len == 0; }
    int64_t  find(const SatString& needle, uint32_t from = 0) const;  // -1 if absent
    int64_t  find_last(const SatString& needle) const;
    uint32_t count(const SatString& needle) const;                    // non-overlapping
    bool     contains(const SatString& n) const { return find(n) >= 0; }
    bool     starts_with(const SatString& n) const;
    bool     ends_with(const SatString& n) const;
    int      compare(const SatString& o) const;   // collation order, -1/0/1

    // --- mutate in place ------------------------------------------------------
    void erase(uint32_t at, uint32_t n);
    void insert(uint32_t at, const char32_t* p, uint32_t n);
    void set(uint32_t i, char32_t c) { if (i < len) data()[i] = c; }
    void clear() { len = 0; }
    void replace_all(const SatString& from, const SatString& to);
    void remove_all(const SatString& needle);
    void upper();
    void lower();
    void trim();
    void reverse();
    void repeat(uint32_t times);
    void assign(const SatString& o) { len = 0; append(o.data(), o.len); }
    void assign_slice(const SatString& o, uint32_t at, uint32_t n);
};

}  // namespace satellite
