#include <cstdlib>
#include <iostream>
#include <string>
#include <chrono>

int main()
{
    // this races the 003 interpreter against 004

    std::string command_004 = "../../build/satellite-004 /home/madness/code/satl/satl_race_program/satl_race.satl";
    std::string command_py = "python3 /home/madness/code/satl/satl_race_program/race.py";

    // --- Time 004 ---
    auto start_time_004 = std::chrono::high_resolution_clock::now();

    // std::system expects a const char*, so we must use .c_str() on the std::string
    std::system(command_004.c_str());

    auto end_time_004 = std::chrono::high_resolution_clock::now();

    // Calculate duration by subtracting start from end
    auto count_004 = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_004 - start_time_004);

    // --- Time 003 ---
    auto start_time_py = std::chrono::high_resolution_clock::now();

    std::system(command_py.c_str());

    auto end_time_py = std::chrono::high_resolution_clock::now();

    auto count_py = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_py - start_time_py);


    // --- Output Results ---
    // You must use .count() to print the raw integer value of the duration
    std::cout << "004: " << count_004.count() << " ns\n";
    std::cout << "py : " << count_py.count() << " ns\n";

    return 0;
}