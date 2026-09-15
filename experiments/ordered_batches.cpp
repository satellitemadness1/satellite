// A batch is a std::vector<Call>: library functions in the order the program wrote them.
// Each call writes into the buffer of the batch it belongs to, never straight to the screen,
// so batches can run on different threads and still print in order.
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
using hrc = std::chrono::high_resolution_clock;

struct Output { std::string text; };                          // one per batch

// Three "libraries", in the shape of satellite-numbers scenarios.
[[gnu::noinline]] long display_text(Output &out, unsigned long long, const char *text) { out.text += text; return 0; }
[[gnu::noinline]] long display_number(Output &out, unsigned long long i, const char *) { out.text += std::to_string(i); return 0; }
[[gnu::noinline]] long display_line_end(Output &out, unsigned long long, const char *) { out.text += '\n'; return 0; }

struct Call { long (*function)(Output &, unsigned long long, const char *); const char *text; };

int main()
{
    // The loop body, compiled once: display("item "), display(i), display(line end) -- in that order.
    const std::vector<Call> body = {{display_text, "item "}, {display_number, nullptr}, {display_line_end, nullptr}};
    const unsigned long long iterations = 1000000;

    auto run_batch = [&body](Output &out, unsigned long long from, unsigned long long to) {
        for (unsigned long long i = from; i < to; i++)            // iterations in order...
            for (const Call &call : body)                          // ...and the calls inside each one in order
                if (call.function(out, i, call.text) != 0) return;
    };

    // One thread, the whole loop.
    auto t = hrc::now();
    Output one;
    run_batch(one, 0, iterations);
    double one_s = std::chrono::duration<double>(hrc::now() - t).count();

    // 24 batches on 24 threads, each into its own buffer, joined back in batch order.
    t = hrc::now();
    const unsigned batches = 24;
    std::vector<Output> outputs(batches);
    std::vector<std::thread> threads;
    for (unsigned k = 0; k < batches; k++)
        threads.emplace_back([&, k] {
            unsigned long long from = iterations / batches * k, to = k + 1 == batches ? iterations : iterations / batches * (k + 1);
            run_batch(outputs[k], from, to);
        });
    for (std::thread &thread : threads) thread.join();
    std::string joined;
    for (const Output &o : outputs) joined += o.text;               // batch 0 first, batch 23 last
    double many_s = std::chrono::duration<double>(hrc::now() - t).count();

    std::printf("one thread:   %.3f s   %zu bytes\n", one_s, one.text.size());
    std::printf("24 batches:   %.3f s   %zu bytes   (x%.1f)\n", many_s, joined.size(), one_s / many_s);
    std::printf("identical output, every byte in the same order: %s\n", joined == one.text ? "yes" : "NO");
    std::printf("first lines: %.28s ...  last line: %s", joined.c_str(), joined.c_str() + joined.rfind('\n', joined.size() - 2) + 1);
}
