// satellite/satellite_test/speed_objects_twins.cpp -- the C++ twins of satellite.test.speed()'s objects,
// capsules, containers and strings sections (group speed_objects). Each speed_<name>(passes) does the same
// work as speed_<name>.satl, line for line, in plain idiomatic C++ (std::vector, std::string,
// std::unordered_map for satl's map, a class for a spacesuit), and returns the one line that program
// displays, without its newline. Compiled INTO satl at -O2: no main, no globals with side effects, nothing
// printed. Everything but the speed_* functions sits in an unnamed namespace, so no name here can meet one
// of satl's own.
//
// Positions: satl counts a list's items and a string's characters from 1, C++ from 0, so a satl a[1 + k]
// is a C++ a[k]. .find counts from 0 in both.

#include "speed_objects_twins.hpp"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

namespace satellite004 {

namespace {

// ---------------------------------------------------------------------------------------------------
// HIS SPACESUIT -- test_object from his_race_twin.cpp (official_test/speed_test/race.cpp), line for line,
// with the two reader capsules the .satl files add to it.
// ---------------------------------------------------------------------------------------------------
class test_object {
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

    // ADDED FOR satellite.test.speed(), as in the .satl files: read the object back for the checksum line.
    long long checksum_number()
    {
        return test_number;
    }

    std::string checksum()
    {
        std::string joined;
        for (const std::string &item : str_test_list) {
            joined += item;
        }
        return "number " + std::to_string(test_number) + ", list " + std::to_string(test_list.size())
            + " items summing " + std::to_string(std::accumulate(test_list.begin(), test_list.end(), 0LL)) + ", string "
            + test_str + ", string list " + std::to_string(str_test_list.size()) + " items of "
            + std::to_string(joined.size()) + " characters";
    }
};

// ---------------------------------------------------------------------------------------------------
// what the other sections share: satl's .join, .upper(), .lower() and .replace(a, b) on plain ASCII text
// ---------------------------------------------------------------------------------------------------
std::string join_numbers(const std::vector<long long> &items, const std::string &separator)
{
    std::string joined;
    for (std::size_t k = 0; k < items.size(); k = k + 1) {
        if (k > 0) {
            joined += separator;
        }
        joined += std::to_string(items[k]);
    }
    return joined;
}

std::string join_strings(const std::vector<std::string> &items, const std::string &separator)
{
    std::string joined;
    for (std::size_t k = 0; k < items.size(); k = k + 1) {
        if (k > 0) {
            joined += separator;
        }
        joined += items[k];
    }
    return joined;
}

// a to z and A to Z by hand, not std::toupper: inside satl the C locale may be the user's (GTK sets it
// when a window opens), and a Turkish one would not turn i into I. The sections' text is all ASCII.
std::string upper_case(std::string text)
{
    for (char &c : text) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return text;
}

std::string lower_case(std::string text)
{
    for (char &c : text) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return text;
}

// every from swapped for to, left to right
std::string replace_all(const std::string &text, const std::string &from, const std::string &to)
{
    std::string result;
    std::size_t start = 0;
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, start)) {
        result.append(text, start, at - start);
        result += to;
        start = at + from.size();
    }
    result.append(text, start, std::string::npos);
    return result;
}

// ---------------------------------------------------------------------------------------------------
// speed_capsule_calls.satl's capsules
// ---------------------------------------------------------------------------------------------------
long long add_two(long long a, long long b)
{
    return a + b;
}

long long step(long long total, long long i)
{
    return add_two(total, i % 7);
}

long long weigh(const std::string &word, long long times)
{
    return static_cast<long long>(word.size()) * times;
}

long long seven()
{
    return 7;
}

// ---------------------------------------------------------------------------------------------------
// speed_object_fields.satl's spacesuit
// ---------------------------------------------------------------------------------------------------
class tally {
    // satellite.protected
    std::string name = "";
    long long count = 0;
    long long total = 0;

    void grow(long long n)
    {
        total = total + n;
    }

public:
    explicit tally(const std::string &given)
    {
        name = given;
    }

    void add(long long n)
    {
        count = count + 1;
        grow(n);
    }

    long long total_now()
    {
        return total;
    }

    std::string label()
    {
        return name + " " + std::to_string(count) + " " + std::to_string(total);
    }
};

} // namespace

// =====================================================================================================
// HIS SPEED TEST, ONE LOOP EACH -- his loops from his_race_twin.cpp, counted by passes, and the checksum line
// =====================================================================================================

std::string speed_his_new_object(long long passes)
{
    long long op_counter = 0;
    long long statement1_counter = 0;
    long long number_sum = 0;

    while (statement1_counter < passes) {
        test_object local_test_object;

        local_test_object.call_set_test_number(1000000000000 + 1000000000000);

        number_sum = number_sum + local_test_object.checksum_number();

        statement1_counter = statement1_counter + 1;
    }

    (void)op_counter;
    return "his_new_object: passes " + std::to_string(statement1_counter) + ", number sum "
        + std::to_string(number_sum);
}

std::string speed_his_set_number_list(long long passes)
{
    long long op_counter = 0;
    long long statement2_counter = 0;

    test_object local_test_object;

    while (statement2_counter < passes) {
        local_test_object.call_set_test_list_number({547311173, 547311173, 547311173, 547311173, 547311173});

        statement2_counter = statement2_counter + 1;
    }

    (void)op_counter;
    return "his_set_number_list: passes " + std::to_string(statement2_counter) + "; " + local_test_object.checksum();
}

std::string speed_his_append_number(long long passes)
{
    long long op_counter = 0;
    long long statement3_counter = 0;

    test_object local_test_object;

    while (statement3_counter < passes) {
        local_test_object.call_add_test_list_number(547311173);

        statement3_counter = statement3_counter + 1;
    }

    (void)op_counter;
    return "his_append_number: passes " + std::to_string(statement3_counter) + "; " + local_test_object.checksum();
}

std::string speed_his_set_string(long long passes)
{
    long long op_counter = 0;
    long long statement4_counter = 0;

    test_object local_test_object;

    while (statement4_counter < passes) {
        local_test_object.call_set_test_str("547311173");

        statement4_counter = statement4_counter + 1;
    }

    (void)op_counter;
    return "his_set_string: passes " + std::to_string(statement4_counter) + "; " + local_test_object.checksum();
}

std::string speed_his_set_string_list(long long passes)
{
    long long op_counter = 0;
    long long statement5_counter = 0;

    test_object local_test_object;

    while (statement5_counter < passes) {
        local_test_object.call_set_test_str_list(
            {"547311173", "547311173", "547311173", "547311173", "547311173", "547311173"});

        statement5_counter = statement5_counter + 1;
    }

    (void)op_counter;
    return "his_set_string_list: passes " + std::to_string(statement5_counter) + "; " + local_test_object.checksum();
}

std::string speed_his_append_string(long long passes)
{
    long long op_counter = 0;
    long long statement6_counter = 0;

    test_object local_test_object;

    while (statement6_counter < passes) {
        local_test_object.call_add_str_to_list("547311173");

        statement6_counter = statement6_counter + 1;
    }

    (void)op_counter;
    return "his_append_string: passes " + std::to_string(statement6_counter) + "; " + local_test_object.checksum();
}

// =====================================================================================================
// CAPSULES, OBJECTS, CONTAINERS AND STRINGS
// =====================================================================================================

std::string speed_capsule_calls(long long passes)
{
    long long total = 0;

    for (long long i = 0; i < passes; i = i + 1) {
        total = step(total, i);
        total = total + weigh("satellite", i % 3) - seven();
    }

    return "capsule_calls: passes " + std::to_string(passes) + ", total " + std::to_string(total);
}

std::string speed_object_fields(long long passes)
{
    tally red("red");
    tally blue("blue");
    tally &same = red;

    for (long long i = 0; i < passes; i = i + 1) {
        red.add(i % 10);
        blue.add(red.total_now() % 7);
        same.add(1);
    }

    return "object_fields: passes " + std::to_string(passes) + ", " + red.label() + ", " + blue.label();
}

std::string speed_list_ops(long long passes)
{
    std::vector<long long> row = {5, 4, 7, 3, 1, 1, 1, 7, 3};
    long long total = 0;

    for (long long i = 0; i < passes; i = i + 1) {
        row.push_back(i);
        row.insert(row.begin() + i % 10, i % 13);
        row[i % 11] = row[(i + 5) % 11] + i % 3;
        total = total + row[(i + 2) % 11] + static_cast<long long>(row.size());
        long long gone = row[(i * 3) % 11];
        row.erase(std::find(row.begin(), row.end(), gone));
        row.erase(row.begin() + i % 10);
    }

    return "list_ops: passes " + std::to_string(passes) + ", total " + std::to_string(total) + ", list "
        + join_numbers(row, ",");
}

std::string speed_map_ops(long long passes)
{
    std::unordered_map<long long, long long> seen;
    long long total = 0;

    for (long long i = 0; i < passes; i = i + 1) {
        seen[i % 1000] = i;
        if (seen.contains((i * 7) % 1000)) {
            total = total + seen.at((i * 7) % 1000);
        }
        seen[(i * 3) % 1000] = seen.at(i % 1000) + 1;
        if (seen.contains((i * 11) % 1000)) {
            seen.erase((i * 11) % 1000);
        }
        total = total + static_cast<long long>(seen.size());
    }

    long long key_sum = 0;
    long long value_sum = 0;
    for (const auto &[key, value] : seen) {
        key_sum = key_sum + key;
        value_sum = value_sum + value;
    }
    return "map_ops: passes " + std::to_string(passes) + ", total " + std::to_string(total) + ", map "
        + std::to_string(seen.size()) + " keys, keys summing " + std::to_string(key_sum) + ", values summing "
        + std::to_string(value_sum);
}

std::string speed_string_build(long long passes)
{
    std::string piece = "";
    std::string line = ">";
    long long built = 0;
    long long lines = 0;

    for (long long i = 0; i < passes; i = i + 1) {
        piece = "satellite " + std::to_string(i);
        piece = piece + "/" + std::to_string(i % 97);
        piece.append(";");
        if (line.size() > 120) {
            lines = lines + 1;
            line.clear();
        }
        line.append(piece);
        line = line + " " + std::to_string(i % 10);
        built = built + static_cast<long long>(piece.size());
    }

    return "string_build: passes " + std::to_string(passes) + ", built " + std::to_string(built) + " characters, lines "
        + std::to_string(lines) + ", last line " + std::to_string(line.size()) + " characters from " + line.substr(0, 1)
        + " to " + line.substr(line.size() - 1, 1);
}

std::string speed_string_methods(long long passes)
{
    std::string base = "satellite 547311173 is satellite spelled in digits";
    std::string base_upper = upper_case(base);
    std::string base_turned(base.rbegin(), base.rend());
    std::string piece = "";
    std::string shouted = "";
    std::string swapped = "";
    std::string turned = "";
    long long total = 0;

    for (long long i = 0; i < passes; i = i + 1) {
        piece = base.substr(i % 40, 9);
        total = total + static_cast<long long>(base.find(piece));
        shouted = upper_case(piece);
        total = total + static_cast<long long>(base_upper.find(shouted));
        total = total + static_cast<long long>(base.find(lower_case(shouted)));
        swapped = replace_all(base, piece.substr(0, 1), "##");
        total = total + static_cast<long long>(swapped.size());
        turned = std::string(piece.rbegin(), piece.rend());
        total = total + static_cast<long long>(base_turned.find(turned));
        if (base[i % static_cast<long long>(base.size())] == 'e') {
            total = total + 1;
        }
    }

    return "string_methods: passes " + std::to_string(passes) + ", total " + std::to_string(total) + ", last piece ["
        + piece + "]";
}

std::string speed_string_list(long long passes)
{
    std::vector<std::string> names = {"satellite"};
    std::string word = "";
    std::string joined = "";
    long long total = 0;

    for (long long i = 0; i < passes; i = i + 1) {
        names.push_back("sat" + std::to_string(i % 100));
        word = names[(i * 7) % static_cast<long long>(names.size())];
        total = total + static_cast<long long>(word.size());
        if (std::find(names.begin(), names.end(), "sat" + std::to_string(i % 50)) != names.end()) {
            total = total + 1;
        }
        names[i % static_cast<long long>(names.size())] = upper_case(word);
        joined = join_strings(names, ",");
        total = total + static_cast<long long>(joined.size());
        if (names.size() > 32) {
            names.erase(names.begin());
        }
    }

    return "string_list: passes " + std::to_string(passes) + ", total " + std::to_string(total) + ", list "
        + std::to_string(names.size()) + " items: " + join_strings(names, ",");
}

std::string speed_nested_containers(long long passes)
{
    std::vector<std::vector<long long>> grid = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    std::unordered_map<std::string, std::vector<long long>> teams = {{"red", {}}, {"green", {}}, {"blue", {}}};
    std::vector<std::unordered_map<std::string, long long>> rows
        = {{{"seen", 0}, {"sum", 0}}, {{"seen", 0}, {"sum", 0}}};
    std::vector<std::string> colours = {"red", "green", "blue"};
    std::string key = "";
    long long total = 0;

    for (long long i = 0; i < passes; i = i + 1) {
        grid[i % 3][i % 4] = grid[i % 3][i % 4] + i % 10;
        key = colours[i % 3];
        teams[key].push_back(i % 100);
        if (teams[key].size() > 8) {
            teams[key].erase(teams[key].begin());
        }
        total = total + teams[key][teams[key].size() - 1] + grid[(i + 1) % 3][(i + 2) % 4];
        rows[i % 2]["seen"] = rows[i % 2]["seen"] + 1;
        rows[i % 2]["sum"] = rows[i % 2]["sum"] + static_cast<long long>(teams[key].size());
    }

    return "nested_containers: passes " + std::to_string(passes) + ", total " + std::to_string(total) + ", grid "
        + join_numbers(grid[0], ",") + "/" + join_numbers(grid[1], ",") + "/" + join_numbers(grid[2], ",") + ", red "
        + join_numbers(teams["red"], ",") + ", green " + join_numbers(teams["green"], ",") + ", blue "
        + join_numbers(teams["blue"], ",") + ", rows " + std::to_string(rows[0]["seen"]) + " "
        + std::to_string(rows[0]["sum"]) + " " + std::to_string(rows[1]["seen"]) + " " + std::to_string(rows[1]["sum"]);
}

} // namespace satellite004
