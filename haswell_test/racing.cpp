
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdlib>

int main()
{
    std::vector<std::string> command_list;

    command_list.push_back("./satl ./race_program.satl");
    command_list.push_back("./satl.haswell ./race_program.satl");

    auto start_time_satl = std::chrono::high_resolution_clock::now();

    std::system(command_list[0].c_str());

    auto end_time_satl = std::chrono::high_resolution_clock::now();

    auto ns_satl = end_time_satl - start_time_satl;

    unsigned long long int nanoseconds = ns_satl.count();

    std::cout << "satl: " << nanoseconds << " nanoseconds\n";

    start_time_satl = std::chrono::high_resolution_clock::now();

    std::system(command_list[1].c_str());

    end_time_satl = std::chrono::high_resolution_clock::now();

    ns_satl = end_time_satl - start_time_satl;

    nanoseconds = ns_satl.count();

    std::cout << "hasw: " << nanoseconds << " nanoseconds\n";

    return(0);
}