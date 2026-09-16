// One thread per line, against batches of lines -- through the REAL StartupThreads.
#include "../../../../../home/madness/code/cxx/satellite/satellite/threads/startup_threads.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace satellite004;

// A real-enough tokeniser: words, numbers, strings, comments, two-char ops, punctuation.
static void tokenise_line(const std::string &s, std::vector<uint16_t> &out) {
    size_t i = 0, n = s.size();
    while (i < n) {
        char c = s[i];
        if (c == ' ' || c == '\t') { ++i; continue; }
        if (c == '/' && i + 1 < n && s[i+1] == '/') { out.push_back(2051); break; }
        if (c == '"') { out.push_back(2307); ++i; size_t st = i;
            while (i < n && s[i] != '"') ++i;
            out.push_back(uint16_t(i - st));
            for (size_t k = st; k < i; ++k) out.push_back(uint16_t((unsigned char)s[k]));
            if (i < n) ++i; continue; }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            size_t st = i; while (i < n && (isalnum((unsigned char)s[i]) || s[i]=='_' || s[i]=='.')) ++i;
            out.push_back(2305); out.push_back(uint16_t(i - st));
            for (size_t k = st; k < i; ++k) out.push_back(uint16_t((unsigned char)s[k])); continue; }
        if (c >= '0' && c <= '9') { size_t st = i;
            while (i < n && (isdigit((unsigned char)s[i]) || s[i]=='.')) ++i;
            out.push_back(2306); out.push_back(uint16_t(i - st));
            for (size_t k = st; k < i; ++k) out.push_back(uint16_t((unsigned char)s[k])); continue; }
        if (i + 1 < n) { std::string two = s.substr(i, 2);
            if (two=="=="||two=="!="||two=="<="||two==">=") { out.push_back(1280); i += 2; continue; } }
        out.push_back(uint16_t(512 + (unsigned char)c % 64)); ++i;
    }
    out.push_back(256); // line_end_token
}

template <class F> static double ms(F f) {
    auto a = std::chrono::steady_clock::now(); f();
    auto b = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(b - a).count();
}

int main() {
    const size_t kLines = 100000;
    std::vector<std::string> src; src.reserve(kLines);
    const char *forms[] = {
        "    satellite.console.display(\"Hello, World!\")",
        "    satellite.console.display(42)",
        "    my_total = my_total + counter_value * 3",
        "    satellite.statement.if (counter_value <= 100)",
        "    // this one is a comment and runs to the end",
        "    satellite.container.list<satellite.variable.string> arguments",
    };
    for (size_t i = 0; i < kLines; ++i) src.push_back(forms[i % 6]);

    MachineState state; state.debug_mode = false;
    StartupThreads threads;
    threads.start(256, state);
    printf("warm threads: %llu, hardware_concurrency: %u\n\n",
           threads.warm(), std::thread::hardware_concurrency());

    std::vector<std::vector<uint16_t>> out(kLines);

    double one = ms([&]{ for (size_t l = 0; l < kLines; ++l) { out[l].clear(); tokenise_line(src[l], out[l]); } });
    size_t tokens = 0; for (auto &v : out) tokens += v.size();

    std::atomic<size_t> done{0};
    double per_line = ms([&]{
        for (size_t l = 0; l < kLines; ++l)
            threads.submit([&, l]{ out[l].clear(); tokenise_line(src[l], out[l]); done.fetch_add(1, std::memory_order_relaxed); });
        while (done.load(std::memory_order_relaxed) < kLines) std::this_thread::yield();
    });

    for (size_t chunks : {24u, 256u}) {
        std::atomic<size_t> cdone{0};
        const size_t per = (kLines + chunks - 1) / chunks;
        double batched = ms([&]{
            for (size_t c = 0; c < chunks; ++c)
                threads.submit([&, c]{
                    for (size_t l = c * per; l < std::min(kLines, (c + 1) * per); ++l) { out[l].clear(); tokenise_line(src[l], out[l]); }
                    cdone.fetch_add(1, std::memory_order_relaxed); });
            while (cdone.load(std::memory_order_relaxed) < chunks) std::this_thread::yield();
        });
        printf("%-42s %9.1f ms   x%.2f\n",
               ("batches of lines (" + std::to_string(chunks) + " jobs)").c_str(), batched, one / batched);
    }
    printf("%-42s %9.1f ms   x%.2f\n", "one thread, no pool", one, 1.0);
    printf("%-42s %9.1f ms   x%.2f\n", "ONE JOB PER LINE (100,000 jobs)", per_line, one / per_line);
    printf("\n%zu lines -> %zu tokens, %.0f ns a line on one thread\n", kLines, tokens, one * 1e6 / kLines);
    return 0;
}
