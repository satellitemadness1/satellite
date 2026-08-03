#include "satellite/string.hpp"

#include <algorithm>
#include <cstring>

namespace satellite {

// ===========================================================================
// satellite_string -- 32 bits per character, holding satellite charmap codes
// ===========================================================================
void SatString::reserve(uint32_t want) {
    if (want <= cap) return;
    uint32_t ncap = cap * 2;
    if (ncap < want) ncap = want;
    char32_t* nb = new char32_t[ncap];
    std::memcpy(nb, data(), (size_t)len * sizeof(char32_t));
    delete[] heap;
    heap = nb;
    cap  = ncap;
}

void SatString::append(const char32_t* p, uint32_t n) {
    reserve(len + n);
    std::memcpy(data() + len, p, (size_t)n * sizeof(char32_t));
    len += n;
}

// Anything the charmap does not cover becomes code 0 (void).  That is the
// design: the map defines what a satellite character IS, and the 32-bit slot
// leaves room to extend it later without touching stored strings.
void SatString::append_text(const std::string& ascii) {
    reserve(len + (uint32_t)ascii.size());
    char32_t* d = data();
    for (char c : ascii) d[len++] = charmap::from_ascii(c);
}

std::string SatString::text() const {
    std::string out;
    out.reserve(len);
    const char32_t* d = data();
    for (uint32_t k = 0; k < len; ++k) {
        if (charmap::is_operator(d[k])) { out += charmap::op_text(d[k]); continue; }
        char c = charmap::to_ascii(d[k]);
        out += c ? c : '?';                 // void renders visibly
    }
    return out;
}

int64_t SatString::find(const SatString& needle, uint32_t from) const {
    if (needle.len == 0 || needle.len > len) return -1;
    const char32_t* h = data();
    const char32_t* n = needle.data();
    for (uint32_t k = from; k + needle.len <= len; ++k)
        if (std::memcmp(h + k, n, (size_t)needle.len * sizeof(char32_t)) == 0)
            return (int64_t)k;
    return -1;
}

void SatString::erase(uint32_t at, uint32_t n) {
    if (at >= len) return;
    if (at + n > len) n = len - at;
    std::memmove(data() + at, data() + at + n, (size_t)(len - at - n) * sizeof(char32_t));
    len -= n;
}

int64_t SatString::find_last(const SatString& needle) const {
    if (needle.len == 0 || needle.len > len) return -1;
    for (uint32_t k = len - needle.len + 1; k-- > 0;)
        if (std::memcmp(data() + k, needle.data(), (size_t)needle.len * sizeof(char32_t)) == 0)
            return (int64_t)k;
    return -1;
}

uint32_t SatString::count(const SatString& needle) const {
    if (needle.len == 0) return 0;
    uint32_t n = 0;
    for (uint32_t k = 0; k + needle.len <= len;) {
        if (std::memcmp(data() + k, needle.data(), (size_t)needle.len * sizeof(char32_t)) == 0) {
            ++n;
            k += needle.len;                       // non-overlapping
        } else {
            ++k;
        }
    }
    return n;
}

bool SatString::starts_with(const SatString& n) const {
    return n.len <= len &&
           std::memcmp(data(), n.data(), (size_t)n.len * sizeof(char32_t)) == 0;
}

bool SatString::ends_with(const SatString& n) const {
    return n.len <= len &&
           std::memcmp(data() + (len - n.len), n.data(),
                       (size_t)n.len * sizeof(char32_t)) == 0;
}

int SatString::compare(const SatString& o) const {
    uint32_t n = len < o.len ? len : o.len;
    for (uint32_t k = 0; k < n; ++k) {
        uint32_t a = charmap::collation_key(data()[k]);
        uint32_t b = charmap::collation_key(o.data()[k]);
        if (a != b) return a < b ? -1 : 1;
    }
    if (len == o.len) return 0;
    return len < o.len ? -1 : 1;
}

void SatString::insert(uint32_t at, const char32_t* p, uint32_t n) {
    if (at > len) at = len;
    reserve(len + n);
    char32_t* d = data();
    std::memmove(d + at + n, d + at, (size_t)(len - at) * sizeof(char32_t));
    std::memcpy(d + at, p, (size_t)n * sizeof(char32_t));
    len += n;
}

void SatString::replace_all(const SatString& from, const SatString& to) {
    if (from.len == 0) return;
    for (uint32_t k = 0; k + from.len <= len;) {
        if (std::memcmp(data() + k, from.data(), (size_t)from.len * sizeof(char32_t)) == 0) {
            erase(k, from.len);
            insert(k, to.data(), to.len);
            k += to.len;                           // never rescan what we wrote
        } else {
            ++k;
        }
    }
}

void SatString::remove_all(const SatString& needle) {
    if (needle.len == 0) return;
    for (uint32_t k = 0; k + needle.len <= len;) {
        if (std::memcmp(data() + k, needle.data(), (size_t)needle.len * sizeof(char32_t)) == 0)
            erase(k, needle.len);
        else
            ++k;
    }
}

void SatString::upper() { char32_t* d = data(); for (uint32_t k=0;k<len;++k) d[k]=charmap::to_upper(d[k]); }
void SatString::lower() { char32_t* d = data(); for (uint32_t k=0;k<len;++k) d[k]=charmap::to_lower(d[k]); }

void SatString::trim() {
    uint32_t b = 0, e = len;
    while (b < e && charmap::is_space(data()[b]))     ++b;
    while (e > b && charmap::is_space(data()[e - 1])) --e;
    if (b) erase(0, b);
    len = e - b;
}

void SatString::reverse() {
    char32_t* d = data();
    for (uint32_t k = 0; k < len / 2; ++k) std::swap(d[k], d[len - 1 - k]);
}

void SatString::repeat(uint32_t times) {
    if (times == 0) { len = 0; return; }
    uint32_t base = len;
    reserve(base * times);
    char32_t* d = data();
    for (uint32_t k = 1; k < times; ++k)
        std::memcpy(d + base * k, d, (size_t)base * sizeof(char32_t));
    len = base * times;
}

void SatString::assign_slice(const SatString& o, uint32_t at, uint32_t n) {
    len = 0;
    if (at >= o.len) return;
    if (at + n > o.len) n = o.len - at;
    append(o.data() + at, n);
}

}  // namespace satellite
