// The display buffer on its own: order, piece boundaries, wide characters, threads, overrun.
#include "satellite/display/display_buffer.hpp"
#include "satellite/machine/machine_codes.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using namespace satellite004;

static satellite_string of(const std::string &utf8)
{
    satellite_string s; std::size_t bad = 0;
    if (satellite_string::from_utf8(utf8, s, bad) != success) { std::fprintf(stderr, "bad test text\n"); std::exit(99); }
    return s;
}

int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);
    display_takes_std_cout();
    const std::string mode = argv[1];
    if (mode == "mix") {
        FILE *want = std::fopen(argv[2], "wb");
        const std::size_t piece = kDisplayPieceBytes / 2;   // units in a piece
        std::vector<std::string> texts = {
            "hello",
            std::string(piece - 1, 'a') + "\xF0\x9F\x98\x80" + "b",          // an emoji across a piece's edge
            std::string(piece * 2 + 7, 'c'),                                   // longer than two pieces
            std::string(3 * piece, 'd') + "\xF0\x9F\x98\x80\xF0\x99\xB1\x80" + std::string(piece, 'e'), // U+1F600, U+19C40
            "\xE4\xB8\xAD\xE6\x96\x87 (chinese) caf\xC3\xA9",
            "",
        };
        for (int round = 0; round < 3000; ++round) {
            const std::string &t = texts[round % texts.size()];
            if (round % 7 == 3) {                       // std::cout's bytes, between the strings
                std::cout << round << " from std::cout\n";
                std::fprintf(want, "%d from std::cout\n", round);
            } else if (round % 97 == 5) {               // not UTF-8
                const char bad[] = "\xFF\xFE raw\n";
                std::cout.write(bad, sizeof bad - 1);
                std::fwrite(bad, 1, sizeof bad - 1, want);
            } else if (round % 11 == 0 || t.size() < 100) {
                if (display_string(of(t), true) != success) return 2;
                std::fwrite(t.data(), 1, t.size(), want); std::fputc('\n', want);
            } else if (round < 60) {                    // the big ones, but not 3000 of them
                if (display_string(of(t), round % 2 == 0) != success) return 2;
                std::fwrite(t.data(), 1, t.size(), want); if (round % 2 == 0) std::fputc('\n', want);
            }
        }
        std::fclose(want);
        std::cout.flush();
        return std::cout ? 0 : 3;
    }
    if (mode == "threads") {   // four threads, 100,000 lines each: every line whole, each thread's in order
        std::vector<std::thread> all;
        for (int t = 0; t < 4; ++t)
            all.emplace_back([t] {
                for (int i = 0; i < 100000; ++i) {
                    const std::string line = "thread " + std::to_string(t) + " line " + std::to_string(i) + " " + std::string(t * 10 + i % 50, 'x');
                    if (i % 3 == 0) { std::string with = line + "\n"; display_bytes(with.data(), with.size()); }
                    else display_string(of(line), true);
                }
            });
        for (auto &th : all) th.join();
        std::cout.flush();
        return 0;
    }
    if (mode == "overrun") {   // nobody reads: 1 MiB strings until the buffer refuses
        const satellite_string mb = of(std::string(1 << 20, 'z'));
        for (long long i = 0;; ++i) {
            satellite_string copy = mb;
            const signed long long int code = display_string(std::move(copy), true);
            if (code != success) {
                std::fprintf(stderr, "refused after %lld strings of 1 MiB characters: code %lld\n", i, code);
                std::fprintf(stderr, "again: %lld, drain: %lld\n",
                             display_string(of("x"), true), display_drain());
                return static_cast<int>(code);
            }
        }
    }
    return 1;
}
