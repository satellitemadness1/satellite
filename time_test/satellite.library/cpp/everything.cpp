// time_test/satellite.library/cpp/everything.cpp -- the C++ equivalent of
// programs/everything.satl: one loop of 40,000 turns, and every turn does small and large
// numbers, small and large strings, small and large lists, a small and a large capsule, a
// small and a large spacesuit.
// Not line for line: spacesuits are classes and capsules functions; big and modulus are
// bignum (bignum.hpp), a hand-written big whole number, because C++ has none; every other
// number is long long (NUMBER SIZES below says why none can outgrow it); strings are
// std::u16string because satl's strings are 16-bit; a string argument is passed by const
// reference.
//
// NUMBER SIZES. Only big and modulus pass 64 bits: 3^2048 is 978 digits (51 limbs).
//   - counter < 40,000; counter * 7919 < 3.2e8; counter * 13 < 520,000; twice() < 80,000.
//   - small gains under 28 a turn (the seven remainders it adds: < 7, 5, 3, 3, 5, 2, 3),
//     so it stays under 1,120,000, and tiny.sum under 1,160,011.
//   - items holds values below 1,000,003, so items.sum < 40,000 * 1,000,003 < 4.1e10.
//   - a station: checksum < 1,000,003 before it is times 31 (< 3.2e7); turns, speed, docked
//     and alarms count turns (<= 40,000); heading < 360; fuel, oxygen and power are brought
//     back below 1,001 by settle() every tick; readings are below 1,009.
//   - churn(): a < 1,000, b < 1,000,017, and the rest are sums and remainders of those.
// fuel does go below zero (to -1): C++'s % keeps the dividend's sign, as satl's does
// (satellite_number_divide.cpp), so -1 % 1000 is -1 in both.
//
// THE LARGE STRINGS are 29,696 characters (58 doubled nine times; the .satl's comment says
// 30,208).
//
// BUILD: clang++ -std=c++20 -O2 everything.cpp -o everything   (or g++)

#include "bignum.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace {

// satl's list.sum, list.max, list.sort() (a sorted copy) and list[k] (counted from 1).
long long sum_of(const std::vector<long long> &list)
{
    long long total = 0;
    for (const long long item : list)
        total += item;
    return total;
}

long long max_of(const std::vector<long long> &list) { return *std::max_element(list.begin(), list.end()); }

std::vector<long long> sorted_copy(const std::vector<long long> &list)
{
    std::vector<long long> sorted = list;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

long long item_at(const std::vector<long long> &list, long long position)
{
    return list.at(static_cast<std::size_t>(position - 1));
}

long long size_of(const std::vector<long long> &list) { return static_cast<long long>(list.size()); }

} // namespace

// A SMALL SPACESUIT: one field, one capsule. A new one is made every turn.
class point {
protected:
    long long x = 0;

public:
    explicit point(long long x_input) { x = x_input; }

    long long call_x() { return x; }
};

// A LARGE SPACESUIT: fourteen fields -- numbers, strings, a list -- a constructor, a
// protected capsule and five public ones. One lives the whole run and is worked every
// turn; another is built new every turn.
class station {
protected:
    std::u16string name = u"station";
    std::u16string log = u"";
    std::u16string status = u"idle";
    long long crew = 0;
    long long fuel = 1000;
    long long oxygen = 1000;
    long long power = 500;
    long long heading = 0;
    long long speed = 0;
    long long docked = 0;
    long long alarms = 0;
    long long turns = 0;
    long long checksum = 7;
    std::vector<long long> readings = {};

    void settle()
    {
        fuel = fuel % 1000 + 1;
        oxygen = oxygen % 1000 + 1;
        power = power % 500 + 1;
    }

public:
    // The fields take their defaults first, then the constructor sets three, as in satl.
    explicit station(const std::u16string &name_input)
    {
        name = name_input;
        crew = 3;
        status = u"ready";
    }

    void call_tick(long long n)
    {
        turns = turns + 1;
        fuel = fuel - n % 3;
        oxygen = oxygen - crew % 2;
        power = power + n % 5;
        heading = (heading + n) % 360;
        speed = speed + 1;
        checksum = (checksum * 31 + n) % 1000003;
        settle();
    }

    void call_record(long long reading)
    {
        readings.push_back(reading);
        if (reading % 97 == 0) {
            alarms = alarms + 1;
            status = u"alarm";
        }
    }

    void call_dock()
    {
        docked = docked + 1;
        log = name + u" docked";
    }

    long long call_checksum() { return checksum + alarms + docked; }

    long long call_readings() { return size_of(readings); }
};

// A SMALL CAPSULE: one line.
long long twice(long long n) { return n * 2; }

// A LARGE CAPSULE: about twenty-five statements of mixed work -- numbers, strings, a list
// of its own -- every call.
long long churn(long long n, const std::u16string &text)
{
    long long a = n % 1000;
    long long b = a * a + 17;
    long long c = b % 13 + a % 7;
    long long d = (a + b + c) % 101;
    std::u16string label = text + u"/" + u"churn";
    std::u16string again = std::u16string(u"churn") + u"/" + text;
    long long matches = 0;
    if (label == again) {
        matches = matches + 1;
    }
    if (label == text + u"/churn") {
        matches = matches + 2;
    }
    std::vector<long long> local = {a, b, c, d};
    local.push_back(a + d);
    local.push_back(b % 9);
    long long total = sum_of(local);
    long long biggest = max_of(local);
    long long e = total % 1000 + biggest % 100;
    long long f = e * 3 + matches;
    long long g = f % 17 + d;
    long long h = g + c * 2;
    long long i = h % 11;
    long long j = i + a % 5;
    return j + size_of(local);
}

int main()
{
    // THE LARGE THINGS, made once before the loop: a 978-digit number (3 to the 2048th)
    // and two 29,696-character strings, built apart so comparing them compares every
    // character rather than finding one string twice.
    bignum modulus(3);
    long long k = 0;
    while (k < 11) {
        modulus = modulus * modulus;
        k = k + 1;
    }
    bignum big = modulus - bignum(12345);
    std::u16string large = u"the satellite programming language, raced against itself. ";
    std::u16string large_copy = u"the satellite programming language, raced against itself. ";
    k = 0;
    while (k < 9) {
        large = large + large;
        large_copy = large_copy + large_copy;
        k = k + 1;
    }

    std::vector<long long> items;
    std::vector<long long> tiny = {0, 0, 0};
    std::u16string word = u"";
    std::u16string joined = u"";
    long long small = 0;
    long long same = 0;
    long long counter = 0;
    station hq(u"hq");
    while (counter < 40000) {
        // SMALL NUMBERS
        small = small + counter * 3 % 7;
        // LARGE NUMBERS: a 978-digit multiply and remainder
        big = (big * 1000003ull + static_cast<bignum::limb>(counter)) % modulus;
        // SMALL STRINGS
        word = std::u16string(u"satl") + u"-" + u"004";
        if (word == u"satl-004") {
            same = same + 1;
        }
        // LARGE STRINGS: 29,696 characters compared, and copied into a join
        if (large == large_copy) {
            same = same + 1;
        }
        joined = large + word;
        // SMALL CONTAINERS
        tiny = {counter, small, counter % 11};
        small = small + sum_of(tiny) % 5;
        // THE LARGE CONTAINER: grows by one a turn, and an item read back from it
        items.push_back(counter * 7919 % 1000003);
        small = small + item_at(items, counter % size_of(items) + 1) % 3;
        // A SMALL CAPSULE AND A LARGE ONE
        small = small + twice(counter) % 3;
        long long churned = churn(counter, word);
        small = small + churned % 5;
        // A SMALL SPACESUIT, new every turn
        point p(counter);
        small = small + p.call_x() % 2;
        // A LARGE SPACESUIT: the one that lives the whole run, worked, and one built new
        hq.call_tick(counter);
        hq.call_record(counter * 13 % 1009);
        station visitor(u"visitor");
        visitor.call_dock();
        visitor.call_tick(counter);
        small = small + visitor.call_checksum() % 3;
        counter = counter + 1;
    }

    std::printf("\n"); // satl's own empty line after its dashes: the race's answer starts there
    std::printf("%lld\n", small);
    std::printf("%lld\n", same);
    std::printf("%llu\n", big % 1000000007ull);
    std::printf("%lld\n", sum_of(items));
    std::printf("%lld\n", sorted_copy(items).front());
    std::printf("%lld\n", hq.call_checksum());
    std::printf("%lld\n", hq.call_readings());
    return 0;
}
