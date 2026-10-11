// polymorph/test.cpp -- time a piece of code in nanoseconds.
//
// Build and run from the satellite folder:
//     g++ -std=c++20 -O2 polymorph/test.cpp -o polymorph/test && ./polymorph/test > polymorph/out.txt && grep nanoseconds polymorph/out.txt

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// forward declaration;
void hello_world();
void call_the_library();

int main()
{
    // our simulated index of numbers

    std::vector<std::map<std::string, std::string>> index = {
        {{"satellite.console.display", "1.1.1"}},
        {{"satellite.print.line", "1.2.3"}},
        {{"satellite.console.write", "1.2.1"}},
        {{"satellite.console.hello_world", "9.98.9"}}
    };

    // so for this simulation, we have to find "9.98.9" within the vector of maps

    auto start = std::chrono::high_resolution_clock::now();

    // first is... 1 million calls to a function that prints "hello, world!"

    // first we find the obscure (because we have to figure in the worst possible scenario, for testing)

    for (signed long long int current_map = 0; current_map < (signed long long int)index.size(); current_map++)
    {
        if(index[current_map].begin()->first == "satellite.console.hello_world")
        {
            // we call the function here because the correct library has been found within the index, so we know what we're calling now

            // the code will be IN memory, so we have to call a function that calls a function, this is going to be "slow" because the C++ we are racing is just... std::cout; but I wonder how slow it's actually going to be? Not sure.
            call_the_library();
        }
    }

    for (signed long long int current_index = 0; current_index < 1000000; current_index++)
    {
        // simulate reading a string first,
        // simulate reading a string we actually write a string, because we have to read a string, then lookup the numbers in a giant index... so we have to deal with a giant index, so that means scanning a std::vector of std::strings for that string, so I will literally do that until the string is found...
        std::string str = "hello, world!";
        hello_world();
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << nanoseconds << " nanoseconds\n";

    // next is 1 million calls to std::cout

    start = std::chrono::high_resolution_clock::now();

    for (signed long long int current_index = 0; current_index < 1000000; current_index++)
    {
        std::cout << "hello, world!\n";
    }

    end = std::chrono::high_resolution_clock::now();

    nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "std::cout::" << nanoseconds << " nanoseconds\n";
}

// the function that resembles a C++ library,
void hello_world()
{
    std::cout << "Hello, world!\n";
}

void call_the_library()
{
    // this simulates calling a library function -- we call our list of functions, passing an argument to the list, and match the argument to the list...
    hello_world();
}
