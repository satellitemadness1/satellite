
#include <iostream>

void some_function(unsigned long long int counter_input);

int main()
{
    unsigned long long int counter = 0;

    some_function(counter);

    return(0);
}

void some_function(unsigned long long int counter_input)
{
    std::cout << counter_input << "\n";

    some_function(counter_input + 1);
}