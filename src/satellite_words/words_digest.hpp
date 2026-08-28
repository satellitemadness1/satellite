#pragma once

// The digest over the numbering -- part of words.hpp.
//
// SATC.md §2 requires a `.satc` to name the numbering it was written against,
// because a cached program is meaningless except against the numbering that
// produced it and DESIGN §4.3's whole warning is that a changed numbering
// SILENTLY CHANGES WHAT A PROGRAM MEANS. A digest that has moved stops every
// stale `.satc` on the machine being read on the same instant.
//
// SATC.md §6 asks what the digest is over and leaves it open. It is over THE
// NUMBERING, not over the file's bytes, and that is a narrower answer than §2's
// wording ("the digest is over words.def") on purpose:
//
//   - Every input to the numbering is in this one file, so the objection §6
//     actually raises -- "if the trie is built from more than that file, two
//     numberings can share one digest" -- does not arise. If that ever stops
//     being true, this is the function that has to grow, and it is one place.
//   - Hashing the bytes would move the digest when a COMMENT moved. words.def
//     is more comment than data and its comments are the argument for the
//     encoding, so they will be edited; invalidating every cache on the machine
//     to record that a sentence was rewritten is a cost with nothing bought.
//   - It needs no build step and no shell. A `sha256sum` in the Makefile would
//     put the digest somewhere `make` computes and a hand-compiled translation
//     unit does not, which is the one difference version.hpp's defaults exist to
//     paper over -- and a `.satc` written by a binary whose digest defaulted to
//     "unrecorded" is worse than no cache.
//
// WHAT IT COVERS IS EVERY FIELD A CACHE COULD BE WRONG ABOUT: each node's
// parent, its number, its kind and its text, in file order, and then every
// alias. Appending a word moves it, moving a row moves it, renaming a spelling
// moves it, adding an alias moves it. SATC.md §2 accepts that an append
// invalidates caches it need not have -- "a `.satc` that is merely out-of-date
// costs one walk to rebuild, and a `.satc` that is wrong costs a program that
// does the wrong thing."

#include <cstdint>
#include <string>
#include <string_view>

#include "satellite_words/words_nodes.hpp"
#include "satellite_words/words_numbers.hpp"

namespace satellite::words {

namespace detail {

// FNV-1a, 64-bit. Not a cryptographic choice and not one that has to be: this
// answers "is this the same numbering", against a file in the same source tree,
// and nothing is defending against somebody who wants two numberings to collide.
// It is here rather than from a library because the whole of it is four lines
// and it has to run at compile time.
constexpr uint64_t kFnvOffset = 1469598103934665603ull;
constexpr uint64_t kFnvPrime = 1099511628211ull;

constexpr uint64_t fold(uint64_t hash, std::string_view text)
{
    for (const char c : text) {
        hash ^= static_cast<uint8_t>(c);
        hash *= kFnvPrime;
    }
    return hash;
}

// A number folded as its bytes, and a separator after every field.
//
// THE SEPARATOR IS NOT DECORATION. Without it the fields run together and two
// different tables hash the same: a node spelled "ab" beside one spelled "c"
// folds identically to "a" beside "bc", and that is exactly the edit -- moving
// a character from one row to the next -- that this is supposed to catch.
constexpr uint64_t fold(uint64_t hash, uint64_t value)
{
    for (int i = 0; i < 8; i++) {
        hash ^= static_cast<uint8_t>(value >> (i * 8));
        hash *= kFnvPrime;
    }
    return hash;
}

constexpr uint64_t compute_digest()
{
    uint64_t hash = kFnvOffset;
    for (PathId i = 1; i <= kNodeCount; i++) {
        hash = fold(hash, static_cast<uint64_t>(kNodes[i].parent));
        hash = fold(hash, static_cast<uint64_t>(number_of(static_cast<NodeId>(i))));
        hash = fold(hash, static_cast<uint64_t>(kNodes[i].kind));
        hash = fold(hash, std::string_view(kNodes[i].text));
        hash = fold(hash, uint64_t{0});
    }
    for (size_t i = 0; i < kAliasCount; i++) {
        hash = fold(hash, static_cast<uint64_t>(kAliases[i].of));
        hash = fold(hash, std::string_view(kAliases[i].text));
        hash = fold(hash, uint64_t{0});
    }
    return hash;
}

} // namespace detail

// The numbering's identity, as one 64-bit number.
inline constexpr uint64_t kDigest = detail::compute_digest();

// The sixteen hex digits a `.satc` header carries (SATC.md §2's `words` line).
//
// LOWER CASE AND FIXED WIDTH, because it is compared as text by a reader that
// has not parsed it yet, and a digest that is sometimes fifteen characters is a
// header line whose fields move.
inline std::string digest_text()
{
    static const char kHex[] = "0123456789abcdef";
    std::string out(16, '0');
    for (int i = 0; i < 16; i++)
        out[15 - i] = kHex[(kDigest >> (i * 4)) & 0xf];
    return out;
}

} // namespace satellite::words
