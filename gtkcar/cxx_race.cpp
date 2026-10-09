
#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>

int main()
{
    std::string satl_command = "./satl-link satl_window.satl";
    std::string py_command = "pypy ./py_window.py";

    auto satl_start = std::chrono::high_resolution_clock::now();

    std::system(satl_command.c_str());

    auto satl_end = std::chrono::high_resolution_clock::now();

    auto py_start = std::chrono::high_resolution_clock::now();

    std::system(py_command.c_str());

    auto py_end = std::chrono::high_resolution_clock::now();

    std::cout << "pypy: " << (satl_end - satl_start).count() << "\n";
    std::cout << "satl: " << (py_end - py_start).count() << "\n";

    return(0);
}