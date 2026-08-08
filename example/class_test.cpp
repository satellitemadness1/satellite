
#include <iostream>
#include <string>

class my_class()
{
    protected:

    std::string the_name = "void";

    my_class(std::string input_str)
    {
        the_name = input_str;
    }

    public:
};

int main(int argc, char *argv[])
{
    signed long long int my_counter = 0;

    while (my_counter < 99999)
    {
        create_and_display(my_counter);

        my_counter = my_counter + 1;
    }
}

void create_and_display(signed long long int input_number)
{
    my_class my_object("some_data");

    std::cout << input_number << "\n";
}