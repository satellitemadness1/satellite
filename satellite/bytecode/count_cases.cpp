// satellite/bytecode/count_cases.cpp -- proves that every count put_count writes,
// count_at reads back. Not part of the interpreter; check.sh runs it.
//
//     build/count_cases
//
// A [COUNTED] token's count is one code, or long_count_token and a chunk for each
// extra 16 bits. count_at reads ANY code equal to long_count_token (0x090A, 2314)
// as "more follows", so a count whose last code would be 0x090A has to be written
// long (found by the wide-strings review, 2026-09-17: a literal of exactly 2314
// codes swallowed the rest of its file). Most of those counts are far too big for
// a program to carry -- 0x090A0000 codes is 151 million -- so they are checked
// here, straight through the two functions, and check.sh runs the ones a program
// can reach through the interpreter.
//
// Every count below is written into a row with a code after it that is itself
// long_count_token, the worst thing a payload can start with, and must come back
// equal with `at` landing exactly on that code.
//
// BUILT WITH -fsanitize=undefined (the Makefile): a count whose top chunk is 0x090A
// ends in a 0 at shift 64, and count_at's guard against that shift changes no
// answer on this machine -- only the sanitizer sees it go (the count review,
// 2026-09-17).

#include "bytecode_registry.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace satellite004;

int main()
{
    std::vector<unsigned long long int> counts;
    for (unsigned long long int n = 0; n <= 0x2FFFFull; ++n)   // every one-code count, and the first long ones
        counts.push_back(n);
    // Every 64-bit count made of the chunks that matter, in every position.
    const unsigned long long int chunks[] = {0, 1, token::long_count_token - 1, token::long_count_token,
                                             token::long_count_token + 1, 0xFFFF};
    for (const unsigned long long int a : chunks)
        for (const unsigned long long int b : chunks)
            for (const unsigned long long int c : chunks)
                for (const unsigned long long int d : chunks)
                    counts.push_back(a | (b << 16) | (c << 32) | (d << 48));

    unsigned long long int wrong = 0;
    for (const unsigned long long int n : counts) {
        std::vector<std::bitset<16>> row = {std::bitset<16>(token::string_token), std::bitset<16>(0),
                                            std::bitset<16>(token::long_count_token)};
        put_count(row, 1, n);
        std::size_t at = 0;
        const unsigned long long int read = count_at(row, at);
        if (read != n || at != row.size() - 1) {
            if (++wrong <= 10)
                std::printf("  WRONG  count %llu (0x%llX) came back %llu, landing at %zu of %zu codes\n", n, n, read,
                            at, row.size());
        }
    }

    // A COUNT LONGER THAN 64 BITS no writer makes, but a row could still hold one:
    // count_at must land past it, with no shift past 63 on the way. Its value is
    // not checked -- no answer for it is true.
    std::vector<std::bitset<16>> too_long = {std::bitset<16>(token::string_token)};
    for (int chunk = 1; chunk <= 5; ++chunk) {
        too_long.push_back(std::bitset<16>(token::long_count_token));
        too_long.push_back(std::bitset<16>(static_cast<token::Code>(chunk)));
    }
    too_long.push_back(std::bitset<16>(7));
    std::size_t past = 0;
    count_at(too_long, past);
    if (past != too_long.size()) {
        std::printf("  WRONG  a count of five long chunks left `at` on %zu of %zu codes\n", past, too_long.size());
        ++wrong;
    }

    // A COUNT IS NEVER A TOKEN. A literal of 257, 258 and 259 codes -- 258 is 0x0102,
    // error_token -- and a character whose own number is 0x0102 have no character
    // without a code in them; a character outside a string that has no code has one.
    const std::string ascii(258, 'a');
    const struct { std::string line; unsigned long long int wanted; } lines[] = {
        {"s = \"" + ascii.substr(1) + "\"", 0}, {"s = \"" + ascii + "\"", 0},
        {"s = \"" + ascii + "a\"", 0},         {"s = \"\xC4\x82\"", 0},
        {"s = \xE2\x88\x9E", 1},
    };
    for (const auto &line : lines) {
        BytecodeRegistry registry(1);
        tokenise_one_line(line.line, registry[0]);
        const unsigned long long int found = characters_with_no_code(registry);
        if (found != line.wanted) {
            std::printf("  WRONG  %zu bytes of line: %llu characters with no code, wanted %llu\n", line.line.size(),
                        found, line.wanted);
            ++wrong;
        }
    }

    if (wrong != 0) {
        std::printf("%llu wrong, among %zu counts and the rows after them\n", wrong, counts.size());
        return 1;
    }
    std::printf("%zu counts written and read back\n", counts.size());
    return 0;
}
