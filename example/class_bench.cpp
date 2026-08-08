// The compiled-C++ side of the race in class_bench.satl: one million objects,
// one million prints, one million string transfers.
//
// Written to match the satellite program's SHAPE rather than to be the fastest
// C++ possible — same class, same three loops, same work per iteration — so the
// number that comes out is "what does the same program cost when compiled",
// which is the question being asked.
//
// Two deliberate choices keep the comparison honest rather than flattering:
//
//   * sync_with_stdio(false) and no std::endl, so the print loop is buffered.
//     satellite's display appends to one std::string and writes it once at the
//     end, so an unbuffered or per-line-flushed C++ would be measuring the
//     flush, not the language.
//   * The constructed object is fed to a volatile sink. Without it clang
//     deletes the entire construction loop — the object is unused and its
//     constructor has no side effect — and the measurement becomes zero,
//     which is a true fact about the optimiser and a useless one about the
//     work. satellite's tree walker cannot do that elision, so leaving it in
//     would be comparing a loop against no loop.
//
// Build:  clang++ -std=c++20 -O2 -o class_bench class_bench.cpp

#include <chrono>
#include <iostream>
#include <string>

class holder {
protected:
    std::string the_name = "void";

public:
    holder(std::string input_str) { the_name = input_str; }

    void set(const std::string &input_str) { the_name = input_str; }

    const std::string &name() const { return the_name; }
};

static volatile std::size_t sink = 0;

// Seconds, to match what the satellite side reports. Milliseconds are a unit
// nobody has an intuition for.
static double sec(std::chrono::steady_clock::time_point from,
                  std::chrono::steady_clock::time_point to)
{
    return std::chrono::duration<double>(to - from).count();
}

int main()
{
    std::ios::sync_with_stdio(false);

    const long long rounds = 1000000;

    // 1. Construct a million objects.
    const auto t0 = std::chrono::steady_clock::now();

    for (long long made = 0; made < rounds; made++) {
        holder scratch("some_data");
        sink = sink + scratch.name().size();
    }

    // 2. A million print statements.
    const auto t1 = std::chrono::steady_clock::now();

    for (long long shown = 0; shown < rounds; shown++)
        std::cout << shown << "\n";

    // 3. A million string transfers into one object.
    const auto t2 = std::chrono::steady_clock::now();

    holder target("some_data");
    for (long long sent = 0; sent < rounds; sent++)
        target.set("another_str");

    const auto t3 = std::chrono::steady_clock::now();

    std::cout.precision(9);
    std::cout << std::fixed
              << "c++       construct : " << sec(t0, t1) << " s\n"
              << "c++       display   : " << sec(t1, t2) << " s\n"
              << "c++       transfer  : " << sec(t2, t3) << " s\n"
              << "c++       total     : " << sec(t0, t3) << " s\n";
    return 0;
}
