#include "bytecode/bytecode_registry.hpp"
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
using namespace satellite004;
using token::Code;

static int failures = 0;
static void check(bool ok, const std::string &what) {
    printf("  %s  %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok) ++failures;
}

// Walks a row the way a reader must: a payload token's count says how many
// codes to SKIP, and skipped codes are never classified.
static std::vector<Code> tokens_of(const std::vector<std::bitset<16>> &row, bool &clean) {
    std::vector<Code> out; clean = true;
    for (size_t i = 0; i < row.size(); ++i) {
        Code c = (Code)row[i].to_ulong();
        if (!token::is_token(c)) { clean = false; continue; }   // a bare character at a token position
        out.push_back(c);
        if (!token::carries_a_count(c) || c == token::long_count_token) continue;
        unsigned long long n = 0; int shift = 0;
        while (++i < row.size() && (Code)row[i].to_ulong() == token::long_count_token) {
            if (++i >= row.size()) break;
            n |= (unsigned long long)row[i].to_ulong() << shift; shift += 16;
        }
        if (i < row.size()) n |= (unsigned long long)row[i].to_ulong() << shift;
        i += n;                                                  // skip the payload
    }
    return out;
}

int main() {
    MachineState state; state.debug_mode = false;
    StartupThreads threads; threads.start(256, state);
    printf("warm threads: %llu\n\n", threads.warm());

    printf("hello_world.satl, line by line:\n");
    const char *src =
        "// satellite-004's first program\n"
        "satellite.include(satellite)\n"
        "satellite.console.display(\"Hello, World!\")\n"
        "satellite.console.display(42)\n";
    BytecodeRegistry reg;
    check(build_bytecode_registry(src, threads, 256, reg, state) == success, "built with no error token");
    check(reg.size() == 5, "5 rows for 4 lines and a trailing newline, got " + std::to_string(reg.size()));
    for (size_t i = 0; i < reg.size() && i < 3; ++i)
        printf("    row %zu: %s\n", i, row_as_bits(reg[i]).substr(0, 120).c_str());

    printf("\nthe count invariant:\n");
    {   // QUAD's own literal: a wide character INSIDE a string.
        std::vector<std::bitset<16>> row; tokenise_one_line("display(\" \xE2\x88\x9E  \")", row);
        bool clean = false; std::vector<Code> ts = tokens_of(row, clean);
        check(clean, "every code at a token position is a token (the wide run was skipped)");
        bool has_string = false, ended = false;
        for (Code c : ts) { if (c == token::string_token) has_string = true;
                            if (has_string && c == token::right_parenthesis_token) ended = true; }
        check(has_string && ended, "the string did not end itself early: ( \" ... \" ) all present");
        check(ts.back() == token::line_end_token, "the row ends with line_end_token");
    }
    {   std::vector<std::bitset<16>> row; tokenise_one_line("x = \"\"", row);
        bool clean = false; tokens_of(row, clean); check(clean, "an empty string literal walks cleanly");
    }
    {   std::vector<std::bitset<16>> row; tokenise_one_line("b1010 xFFAA 0#down", row);
        bool clean = false; std::vector<Code> ts = tokens_of(row, clean);
        check(clean && ts.size() == 4, "b1010 / xFFAA / 0#down are three payload tokens");
        check(ts[0] == token::binary_token, "b1010 is binary_token");
        check(ts[1] == token::hexadecimal_token, "xFFAA is hexadecimal_token");
        check(ts[2] == token::option_token, "0#down is option_token");
    }
    {   std::vector<std::bitset<16>> row; tokenise_one_line("bill x 3.14", row);
        bool clean = false; std::vector<Code> ts = tokens_of(row, clean);
        check(ts[0] == token::name_token && ts[1] == token::name_token, "bill and a lone x are names, not bits");
        check(ts[2] == token::number_token, "3.14 is one number, dot folded in");
    }
    {   std::vector<std::bitset<16>> a, b; tokenise_one_line("a / b", a); tokenise_one_line("dir/ship", b);
        bool c1, c2; std::vector<Code> ta = tokens_of(a, c1), tb = tokens_of(b, c2);
        bool div = false, path = false;
        for (Code c : ta) if (c == token::divide_token) div = true;
        for (Code c : tb) if (c == token::path_separator_token) path = true;
        check(div, "a / b  with blanks both sides is divide_token");
        check(path, "dir/ship  touching is path_separator_token");
    }
    {   std::string big(80000, 'a'); std::vector<std::bitset<16>> row;
        tokenise_one_line("\"" + big + "\"", row);
        bool clean = false; std::vector<Code> ts = tokens_of(row, clean);
        check(clean, "an 80,000-character literal walks cleanly (count past 65535)");
        check(ts.size() == 2 && ts[0] == token::string_token, "and is still exactly one string token");
    }

    printf("\nthe race, 100,000 lines:\n");
    std::string many; for (int i = 0; i < 100000; ++i) many += "    satellite.console.display(\"Hello, World!\")\n";
    BytecodeRegistry big;
    auto t0 = std::chrono::steady_clock::now();
    build_bytecode_registry(many, threads, 256, big, state);
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    printf("    %zu rows, %llu codes, %.1f ms (%.1f MB as bitset<16>)\n",
           big.size(), codes_in(big), ms, codes_in(big) * 8 / 1048576.0);

    printf("\n%s\n", failures == 0 ? "all checks passed" : (std::to_string(failures) + " FAILED").c_str());
    return failures != 0;
}
