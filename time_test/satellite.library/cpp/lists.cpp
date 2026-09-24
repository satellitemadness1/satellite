// cpp/lists.cpp -- the C++ equivalent of programs/lists.satl: 300,000 appends, then the list's sum,
// its largest, and the first of a sorted copy. Not line for line: it first prints the blank line
// satl prints after its dashes; .sum, .max and .sort() are written out below as satl defines them.
// Each item is under 1,000,003 and counter * 7919 under 2,400,000,000; the sum is under
// 300,000,000,000 -- all far inside long long.
//
// BUILD: clang++ -std=c++20 -O2 lists.cpp -o lists      (g++ works the same)

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

long long sum_of(const std::vector<long long> &items)
{
    long long sum = 0;
    for (long long item : items) sum = sum + item;
    return sum;
}

long long max_of(const std::vector<long long> &items)
{
    return *std::max_element(items.begin(), items.end());
}

// satl's .sort() answers a sorted COPY; the list itself is left as it was.
std::vector<long long> sorted_copy(const std::vector<long long> &items)
{
    std::vector<long long> copy = items;
    std::sort(copy.begin(), copy.end());
    return copy;
}

} // namespace

int main()
{
    std::printf("\n");

    std::vector<long long> items;
    long long counter = 0;
    while (counter < 300000) {
        items.push_back(counter * 7919 % 1000003);
        counter = counter + 1;
    }
    std::printf("%lld\n", sum_of(items));
    std::printf("%lld\n", max_of(items));
    std::printf("%lld\n", sorted_copy(items).front());
    return 0;
}
