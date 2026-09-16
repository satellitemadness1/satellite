// Can the 16-bit registry carry a C++ FILE, byte for byte?
//
// POLYMORPH/M6.md:51-53 said counting brackets cannot find the end of a C++
// block, because C++ has { and } inside strings, comments and '{' literals, so
// it "needs a closing marker nothing in C++ can contain". A COUNT needs no such
// marker: nothing inside the payload can end it, because nothing inside it is
// read as a token at all. This proves that on real C++ source.

#include "bytecode/bytecode_registry.hpp"
#include "satellite_variable_string/character_table.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace satellite004;

// Write any text as a counted payload, exactly as a string literal travels.
static void carry(const std::string &text, std::vector<std::bitset<16>> &row)
{
    row.push_back(std::bitset<16>(token::string_token));
    const std::size_t count_at = row.size();
    row.push_back(std::bitset<16>(0));

    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        std::uint32_t v = c;
        std::size_t extra = 0;
        if (c >= 0xF0) { extra = 3; v = c & 0x07u; }
        else if (c >= 0xE0) { extra = 2; v = c & 0x0Fu; }
        else if (c >= 0xC0) { extra = 1; v = c & 0x1Fu; }
        if (extra != 0 && i + extra < text.size()) {
            for (std::size_t k = 1; k <= extra; ++k)
                v = (v << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3Fu);
            i += extra + 1;
        } else {
            v = c;
            ++i;
        }
        if (v < 128) {
            row.push_back(std::bitset<16>(character_table::code_of_ascii[v]));
        } else if (v <= 0xFFFF) {
            row.push_back(std::bitset<16>(token::wide_run_token));
            row.push_back(std::bitset<16>(1));
            row.push_back(std::bitset<16>(static_cast<std::uint16_t>(v)));
        } else {
            row.push_back(std::bitset<16>(token::wide_run_32_token));
            row.push_back(std::bitset<16>(1));
            row.push_back(std::bitset<16>(static_cast<std::uint16_t>(v >> 16)));
            row.push_back(std::bitset<16>(static_cast<std::uint16_t>(v & 0xFFFFu)));
        }
    }

    const std::size_t n = row.size() - count_at - 1;
    if (n <= 0xFFFFu) {
        row[count_at] = std::bitset<16>(static_cast<std::uint16_t>(n));
        return;
    }
    // The same long-count spelling put_payload writes, so no file has a ceiling.
    std::vector<std::bitset<16>> chunks;
    unsigned long long int m = n;
    while (m > 0xFFFFull) {
        chunks.push_back(std::bitset<16>(token::long_count_token));
        chunks.push_back(std::bitset<16>(static_cast<std::uint16_t>(m & 0xFFFFull)));
        m >>= 16;
    }
    chunks.push_back(std::bitset<16>(static_cast<std::uint16_t>(m)));
    row.erase(row.begin() + static_cast<long>(count_at));
    row.insert(row.begin() + static_cast<long>(count_at), chunks.begin(), chunks.end());
}

int main(int argc, char **argv)
{
    int bad = 0;
    for (int a = 1; a < argc; ++a) {
        std::ifstream in(argv[a]);
        std::ostringstream held;
        held << in.rdbuf();
        const std::string original = held.str();
        if (original.empty()) { printf("  skip  %s (empty or unreadable)\n", argv[a]); continue; }

        std::vector<std::bitset<16>> row;
        carry(original, row);

        // A TOKEN sits immediately after the payload. If the count were not
        // honoured the reader would stop early or run straight into it -- which
        // is exactly the failure a closing marker was supposed to prevent.
        row.push_back(std::bitset<16>(token::right_brace_token));

        std::size_t at = 0;
        const std::string back = text_at(row, at);
        const bool same = back == original;
        const bool landed = at < row.size() &&
                            static_cast<token::Code>(row[at].to_ulong()) == token::right_brace_token;

        printf("  %s  %-54s %6zu bytes -> %6zu codes  %s\n",
               (same && landed) ? "ok  " : "FAIL", argv[a], original.size(), row.size() - 2,
               same ? (landed ? "byte-identical, reader landed exactly past it"
                              : "identical, but the reader OVERRAN")
                    : "CAME BACK DIFFERENT");
        if (!same || !landed) ++bad;
    }
    printf("\n%s\n", bad != 0 ? "FAILED" : "every C++ file came back byte for byte");
    return bad != 0;
}
