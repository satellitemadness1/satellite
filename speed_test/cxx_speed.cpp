
#include <iostream>

int main()
{
    signed long long int display_count = 0;

    signed long long int display_target = 5000000;

    while(display_count < display_target)
    {
        std::cout << "cxx: hello, speed world!" << "\n";

        display_count = display_count + 1;
    }
    return 0;
}