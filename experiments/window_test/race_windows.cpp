#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

int main()
{
    std::string py_cmd = "pypy window_and_extras.py";
    std::string satl_cmd = "satl window_and_extras.satl";

    auto py_start = std::chrono::high_resolution_clock::now();

    std::system(py_cmd.c_str());

    auto py_end = std::chrono::high_resolution_clock::now();

    auto satl_start = std::chrono::high_resolution_clock::now();

    std::system(satl_cmd.c_str());

    auto satl_end = std::chrono::high_resolution_clock::now();

    std::cout << "pypy: " << py_end - py_start << "\n";
    std::cout << "satl: " << satl_end - satl_start << "\n";

    return 0;
}
