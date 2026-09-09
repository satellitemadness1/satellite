
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

int main()
{
    std::vector<std::string> command_vector = {"./satl"};
    
    auto start_time = std::chrono::high_resolution_clock::now();

    std::system(command_vector[0].c_str());

    auto end_time = std::chrono::high_resolution_clock::now();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    std::cout << "time: " << elapsed_time << " nanoseconds" << "\n";

    return(0);
}