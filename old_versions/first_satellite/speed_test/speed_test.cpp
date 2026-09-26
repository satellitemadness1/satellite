
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>

int main(int argc, char *argv[])
{
    auto satl_start = std::chrono::high_resolution_clock::now();

    std::system("../satl ./speed_test.satl");

    auto satl_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> satl_elapsed = satl_end - satl_start;

    auto py_start = std::chrono::high_resolution_clock::now();

    std::system("python3 ./speed_test.py");

    auto py_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> py_elapsed = py_end - py_start;

    auto cxx_start = std::chrono::high_resolution_clock::now();

    std::system("./cxx_speed");

    auto cxx_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> cxx_elapsed = cxx_end - cxx_start;
    
    auto asm_start = std::chrono::high_resolution_clock::now();
    
    std::system("./nasm_speed");
    
    auto asm_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> asm_elapsed = asm_end - asm_start;

    std::cout << "cxx elapsed time:  " << cxx_elapsed.count() << " seconds" << "\n";
    std::cout << "py elapsed time:   " << py_elapsed.count() << " seconds" << "\n";
    std::cout << "satl elapsed time: " << satl_elapsed.count() << " seconds" << "\n";
    std::cout << "asm elapsed time: " << asm_elapsed.count() << " seconds" << "\n";

    return(0);
}
