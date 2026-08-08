// The C++ counterpart of example/py_compare/bench.satl, so that both comparison
// harnesses drive the same satellite program.
//
// Neither side times itself. The driver times the whole process, and a program
// that also printed its own timings could never produce output identical to its
// counterpart's -- which is what compare.cpp verifies before it will report a
// ratio. example/class_bench.satl keeps the self-timing version, for the
// per-phase breakdown in DESIGN.md section 14.

#include <iostream>
#include <string>

class holder {
protected:
    std::string the_name = "void";

public:
    holder(std::string input_str) { the_name = input_str; }
    void set(const std::string &input_str) { the_name = input_str; }
};

static volatile std::size_t sink = 0;

int main()
{
    std::ios::sync_with_stdio(false);
    const long long rounds = 300000;

    for (long long made = 0; made < rounds; made++) {
        holder scratch("some_data");
        sink = sink + 1;
    }

    for (long long shown = 0; shown < rounds; shown++)
        std::cout << shown << "\n";

    holder target("some_data");
    for (long long sent = 0; sent < rounds; sent++)
        target.set("another_str");

    std::cout << "done\n";
    return 0;
}
