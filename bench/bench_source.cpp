// One contiguous string per file, vs a vector<string> of lines.
// Both are reasonable; measure which costs what on a realistic include set.
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <malloc.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using Clock = std::chrono::steady_clock;

static size_t heap_used() {
    // mallinfo2 gives bytes actually obtained from the OS + in-use chunks
    auto mi = mallinfo2();
    return mi.uordblks;   // total allocated space in use
}

// ---------------------------------------------------------------------------
// A) one contiguous string + a line index (the proposed design)
// ---------------------------------------------------------------------------
struct SourceContig {
    std::string           text;
    std::vector<uint32_t> line_starts;

    void build_index() {
        line_starts.clear();
        line_starts.push_back(0);
        for (uint32_t k = 0; k < text.size(); ++k)
            if (text[k] == '\n') line_starts.push_back(k + 1);
    }
    // O(1): direct index, no allocation, no copy
    std::string_view line(size_t n) const {
        uint32_t b = line_starts[n];
        uint32_t e = (n + 1 < line_starts.size()) ? line_starts[n + 1] - 1
                                                  : (uint32_t)text.size();
        return std::string_view(text.data() + b, e - b);
    }
    size_t line_count() const { return line_starts.size(); }
};

// ---------------------------------------------------------------------------
// B) vector<string>, one string per line
// ---------------------------------------------------------------------------
struct SourceLines {
    std::vector<std::string> lines;

    void from_text(const std::string& text) {
        lines.clear();
        std::istringstream in(text);
        std::string l;
        while (std::getline(in, l)) lines.push_back(l);
    }
    const std::string& line(size_t n) const { return lines[n]; }
    size_t line_count() const { return lines.size(); }
};

// ---------------------------------------------------------------------------
static std::string make_file(int lines_per_file, int seed) {
    std::string s;
    s.reserve(lines_per_file * 46);
    s += "satellite.include(satellite)\n";
    for (int k = 0; k < lines_per_file; ++k) {
        switch ((k + seed) % 6) {
            case 0: s += "satellite.variable.int counter_" + std::to_string(k) + " = " + std::to_string(k * 7) + "\n"; break;
            case 1: s += "    satellite.console.display(\"line " + std::to_string(k) + " of output\")\n"; break;
            case 2: s += "// a comment explaining what capsule " + std::to_string(k) + " does\n"; break;
            case 3: s += "satellite.capsule helper_" + std::to_string(k) + "(argument_value)\n{\n"; break;
            case 4: s += "    satellite.library.main.total = satellite.library.main.total + " + std::to_string(k) + "\n"; break;
            default: s += "}\n"; break;
        }
    }
    return s;
}

int main() {
    const int NFILES = 200;
    const int LINES  = 500;

    // build the corpus once, in memory, so disk is not what we measure
    std::vector<std::string> corpus;
    corpus.reserve(NFILES);
    size_t total_bytes = 0, total_lines = 0;
    for (int k = 0; k < NFILES; ++k) {
        corpus.push_back(make_file(LINES, k));
        total_bytes += corpus.back().size();
    }
    for (const auto& f : corpus)
        for (char c : f) if (c == '\n') ++total_lines;

    printf("corpus: %d files, %zu lines, %.2f MB\n\n",
           NFILES, total_lines, total_bytes / 1048576.0);

    // ---- A: contiguous ----------------------------------------------------
    size_t base = heap_used();
    auto t0 = Clock::now();
    std::deque<SourceContig> A;
    for (const auto& f : corpus) {
        A.emplace_back();
        A.back().text = f;
        A.back().build_index();
    }
    double a_ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    size_t a_mem = heap_used() - base;

    // ---- B: vector<string> of lines ---------------------------------------
    size_t base2 = heap_used();
    auto t1 = Clock::now();
    std::deque<SourceLines> B;
    for (const auto& f : corpus) {
        B.emplace_back();
        B.back().from_text(f);
    }
    double b_ms = std::chrono::duration<double, std::milli>(Clock::now() - t1).count();
    size_t b_mem = heap_used() - base2;

    printf("%-34s %10s %12s\n", "", "load ms", "heap MB");
    printf("%-34s %10.1f %12.2f\n", "A) one string + line index", a_ms, a_mem / 1048576.0);
    printf("%-34s %10.1f %12.2f\n", "B) vector<string> per line", b_ms, b_mem / 1048576.0);
    printf("%-34s %9.2fx %11.2fx\n", "   B vs A", b_ms / a_ms, (double)b_mem / a_mem);

    // ---- line access ------------------------------------------------------
    volatile size_t sink = 0;
    auto t2 = Clock::now();
    for (int rep = 0; rep < 20; ++rep)
        for (const auto& s : A)
            for (size_t k = 0; k < s.line_count(); ++k) sink += s.line(k).size();
    double a_acc = std::chrono::duration<double, std::milli>(Clock::now() - t2).count();

    auto t3 = Clock::now();
    for (int rep = 0; rep < 20; ++rep)
        for (const auto& s : B)
            for (size_t k = 0; k < s.line_count(); ++k) sink += s.line(k).size();
    double b_acc = std::chrono::duration<double, std::milli>(Clock::now() - t3).count();

    printf("\n%-34s %10s\n", "walk every line 20x", "ms");
    printf("%-34s %10.1f\n", "A) one string + line index", a_acc);
    printf("%-34s %10.1f\n", "B) vector<string> per line", b_acc);

    // ---- allocation count -------------------------------------------------
    printf("\nallocations for the corpus:\n");
    printf("  A: %d   (one string + one index vector per file)\n", NFILES * 2);
    printf("  B: %zu   (one string per LINE, plus the vector)\n", total_lines + NFILES);

    // ---- the thing that actually decides it -------------------------------
    printf("\n--- a token that spans lines ---\n");
    std::string blocky =
        "satellite.cxx\n{\n    #include <string>\n    std::string s = \"hi\"\n"
        "    satellite.return(s)\n}\n";
    SourceContig ca; ca.text = blocky; ca.build_index();
    SourceLines  cb; cb.from_text(blocky);

    // contiguous: the block is one substring, already there
    size_t ob = ca.text.find('{'), oe = ca.text.rfind('}');
    printf("A) cxx block is a substring: %zu bytes, zero copies\n", oe - ob + 1);

    // line-based: must stitch lines back together, and the newlines were eaten
    std::string rebuilt;
    for (size_t k = 1; k + 1 < cb.line_count(); ++k) { rebuilt += cb.line(k); rebuilt += '\n'; }
    printf("B) cxx block must be rebuilt from %zu lines -> %zu bytes, 1 copy + N appends\n",
           cb.line_count() - 2, rebuilt.size());
    printf("   (getline ATE the newlines; they have to be put back, and any\n");
    printf("    trailing whitespace or \\r is now unrecoverable)\n");

    printf("\nsink %zu\n", (size_t)sink);
    return 0;
}
