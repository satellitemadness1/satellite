#include "bytecode/word_codes.hpp"
#include <cstdio>
#include <string>
using namespace satellite004;
static int bad = 0;
static void check(bool ok, const std::string &what) {
    printf("  %s  %s\n", ok ? "ok  " : "FAIL", what.c_str()); if (!ok) ++bad;
}
int main() {
    printf("int int int int -> one 16-bit code:\n");
    check(word::code_of(1) == 4097, "code_of(1) = satellite = 4097");
    check(word::code_of(1, 5, 1) == 4163, "code_of(1,5,1) = satellite.console.display = 4163");
    check(word::code_of(1, 1, 1) == 4100, "code_of(1,1,1) = satellite.include(satellite)");
    check(word::code_of(1, 14, 1, 1, 5, 1) == 4357, "six numbers deep works");
    printf("\nthe two spellings are the same word:\n");
    for (token::Code c : {4097, 4163, 4357}) {
        unsigned int depth = 0; const int *n = word::numbers_of(c, depth);
        std::string path; for (unsigned i = 0; i < depth; ++i) path += (i ? " " : "") + std::to_string(n[i]);
        check(word::code_of(n, depth) == c, std::to_string(c) + " -> " + path + " -> back to " + std::to_string(c) + "  (" + word::spelling_of(c) + ")");
    }
    printf("\nevery word in the table round-trips:\n");
    size_t round = 0;
    for (token::Code c = word::kFirst; c < word::kFirst + word::kWordsInTable; ++c) {
        unsigned int depth = 0; const int *n = word::numbers_of(c, depth);
        if (n && depth && word::code_of(n, depth) == c) ++round;
    }
    check(round == word::kWordsInTable, std::to_string(round) + " of " + std::to_string(word::kWordsInTable) + " round-trip");
    printf("\nno code, and no ceiling:\n");
    check(word::code_of(9, 9, 9) == 0, "a path no word has answers 0, so the caller uses word_number_token");
    check(word::code_of(1, 5, 1, 1, 1, 1, 1, 1) == 0, "deeper than kMaxDepth answers 0, never a wrong code");
    check(word::code_of(1, 300) == 0, "a number past 255 answers 0");
    check(!word::is_word_code(4095) && word::is_word_code(4096) && word::is_word_code(8191) && !word::is_word_code(8192),
          "the range is 4096..8191 and nothing else");
    {   unsigned int depth = 1;
        check(word::numbers_of(word::kBase, depth) == nullptr && depth == 0,
              "4096 itself is reserved: no word has it"); }
    printf("\n%s\n", bad == 0 ? "all checks passed" : (std::to_string(bad) + " FAILED").c_str());
    return bad != 0;
}
