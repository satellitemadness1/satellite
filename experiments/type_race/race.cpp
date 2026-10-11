#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

// cpython runs Python the way satl runs satellite, one instruction at a time;
// pypy compiles the loop to machine code while it runs.
int main()
{
    std::string cpython_cmd = "/usr/bin/python3 numbers.py";
    std::string pypy_cmd = "pypy numbers.py";
    std::string satl_cmd = "satl numbers.satl";

    auto cpython_start = std::chrono::high_resolution_clock::now();

    std::system(cpython_cmd.c_str());

    auto cpython_end = std::chrono::high_resolution_clock::now();

    auto pypy_start = std::chrono::high_resolution_clock::now();

    std::system(pypy_cmd.c_str());

    auto pypy_end = std::chrono::high_resolution_clock::now();

    auto satl_start = std::chrono::high_resolution_clock::now();

    std::system(satl_cmd.c_str());

    auto satl_end = std::chrono::high_resolution_clock::now();

    std::cout << "cpython: " << std::chrono::duration_cast<std::chrono::nanoseconds>(cpython_end - cpython_start).count() << " ns\n";
    std::cout << "pypy:    " << std::chrono::duration_cast<std::chrono::nanoseconds>(pypy_end - pypy_start).count() << " ns\n";
    std::cout << "satl:    " << std::chrono::duration_cast<std::chrono::nanoseconds>(satl_end - satl_start).count() << " ns\n";

    return 0;
}
