#include <cstdlib>
#include <iostream>
#include <string>
#include <chrono>

int main()
{
    // this races official_speed_test.satl (satl) against race.cpp (the same work in C++)

    // build race.cpp first -- not timed
    std::system("clang++ -O2 -o race race.cpp");

    // satl's display goes to a file: a terminal would open satl's own console window
    std::string command_satl = "/home/madness/.satl/satl official_speed_test.satl > satl.out";
    std::string command_cpp = "./race > cpp.out";

    // --- Time satl ---
    auto start_time_satl = std::chrono::high_resolution_clock::now();

    std::system(command_satl.c_str());

    auto end_time_satl = std::chrono::high_resolution_clock::now();

    auto count_satl = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_satl - start_time_satl);

    // --- Time C++ ---
    auto start_time_cpp = std::chrono::high_resolution_clock::now();

    std::system(command_cpp.c_str());

    auto end_time_cpp = std::chrono::high_resolution_clock::now();

    auto count_cpp = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_cpp - start_time_cpp);

    // --- Output Results ---
    std::cout << "satl: " << count_satl.count() << " ns\n";
    std::cout << "C++ : " << count_cpp.count() << " ns\n";
    std::cout << "satl runs at " << 100.0 * count_cpp.count() / count_satl.count() << "% of C++ speed\n";

    return 0;
}
