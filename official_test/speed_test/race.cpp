// race.cpp -- the same work as official_speed_test.satl, line for line, in C++.
// timer.cpp builds it (clang++ -O2 -o race race.cpp) and times it against satl.

#include <iostream>
#include <string>
#include <vector>

class test_object
{
    // satellite.protected
    long long test_number = 0;
    std::vector<long long> test_list = {0, 0, 0, 0};
    std::string test_str = "VOID";
    std::vector<std::string> str_test_list = {"str1", "str2"};

    std::vector<test_object> my_list_of_stuff;

    void set_test_number(long long test_input)
    {
        test_number = test_input;
    }

    void set_test_list_number(const std::vector<long long> &test_input)
    {
        test_list = test_input;
    }

    void add_test_list_number(long long test_input)
    {
        test_list.push_back(test_input);
    }

    void set_test_str(const std::string &str_input_test)
    {
        test_str = str_input_test;
    }

    void set_test_str_list(const std::vector<std::string> &input_str_list)
    {
        str_test_list = input_str_list;
    }

    void add_str_to_list(const std::string &str_input)
    {
        str_test_list.push_back(str_input);
    }

public:
    void call_set_test_number(long long test_input)
    {
        set_test_number(test_input);
    }

    void call_set_test_list_number(const std::vector<long long> &test_input)
    {
        set_test_list_number(test_input);
    }

    void call_set_test_str(const std::string &test_input)
    {
        set_test_str(test_input);
    }

    void call_add_test_list_number(long long test_input)
    {
        add_test_list_number(test_input);
    }

    void call_set_test_str_list(const std::vector<std::string> &test_input)
    {
        set_test_str_list(test_input);
    }

    void call_add_str_to_list(const std::string &test_input)
    {
        add_str_to_list(test_input);
    }
};

int main()
{
    std::cout << "SATELLITE SPEED TEST\n\nVERSION: 001.01\n\n" << "\n";

    long long op_counter = 0;
    long long statement1_counter = 0;
    long long statement2_counter = 0;
    long long statement3_counter = 0;
    long long statement4_counter = 0;
    long long statement5_counter = 0;
    long long statement6_counter = 0;

    while (statement1_counter < 1000000)
    {
        test_object local_test_object;

        local_test_object.call_set_test_number(1000000000000 + 1000000000000);

        statement1_counter = statement1_counter + 1;
    }

    test_object local_test_object;

    while (statement2_counter < 1000000)
    {
        local_test_object.call_set_test_list_number({547311173, 547311173, 547311173, 547311173, 547311173});

        statement2_counter = statement2_counter + 1;
    }

    while (statement3_counter < 1000000)
    {
        local_test_object.call_add_test_list_number(547311173);

        statement3_counter = statement3_counter + 1;
    }

    while (statement4_counter < 1000000)
    {
        local_test_object.call_set_test_str("547311173");

        statement4_counter = statement4_counter + 1;
    }

    while (statement5_counter < 1000000)
    {
        local_test_object.call_set_test_str_list({"547311173", "547311173", "547311173", "547311173", "547311173", "547311173"});

        statement5_counter = statement5_counter + 1;
    }

    while (statement6_counter < 1000000)
    {
        local_test_object.call_add_str_to_list("547311173");

        statement6_counter = statement6_counter + 1;
    }

    (void)op_counter;
    return 0;
}
