#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

static std::string show(double v)
{
    if (v == 0)
        return "0";
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof buffer, v,
                                      std::chars_format::fixed);
    return std::string(buffer, result.ptr);
}

static std::string show(bool v)
{
    return v ? "true" : "false";
}

static std::string show(const std::string &v)
{
    return v;
}

static std::string show(const std::vector<double> &v)
{
    std::string out = "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i)
            out += ", ";
        out += show(v[i]);
    }
    return out + "]";
}

static std::string show(const std::vector<std::vector<double>> &v)
{
    std::string out = "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i)
            out += ", ";
        out += show(v[i]);
    }
    return out + "]";
}

template <class T>
static void display(const T &v)
{
    std::cout << show(v) << "\n";
}

template <class T>
static T slice(const T &v, long long lo, long long hi)
{
    const long long n = static_cast<long long>(v.size());
    if (lo < 0)
        lo += n;
    if (hi < 0)
        hi += n;
    lo = std::max(0LL, std::min(lo, n));
    hi = std::max(0LL, std::min(hi, n));
    if (hi < lo)
        hi = lo;
    return T(v.begin() + lo, v.begin() + hi);
}

template <class T>
static auto at(const T &v, long long i)
{
    const long long n = static_cast<long long>(v.size());
    if (i < 0)
        i += n;
    return v[static_cast<size_t>(i)];
}

static bool contains(const std::vector<double> &v, double x)
{
    return std::find(v.begin(), v.end(), x) != v.end();
}

static double total = 0;

class item {
protected:
    std::string label = "unnamed";
    double count = 0;

public:
    item(std::string name, double amount)
    {
        label = name;
        count = amount;
    }

    virtual ~item() = default;

    std::string name() const
    {
        return label;
    }

    double quantity() const
    {
        return count;
    }

    void restock(double amount)
    {
        count = count + amount;
    }

    virtual std::string describe() const
    {
        return label + " x" + show(count);
    }
};

class crate : public item {
protected:
    double per_box = 12;

public:
    using item::item;

    double boxes() const
    {
        return std::floor(count / per_box);
    }

    std::string describe() const override
    {
        return label + " x" + show(count) + " in " + show(boxes()) + " boxes";
    }
};

static double fact(double n)
{
    if (n <= 1)
        return 1;

    return n * fact(n - 1);
}

static bool is_odd(double n);

static bool is_even(double n)
{
    if (n == 0)
        return true;

    return is_odd(n - 1);
}

static bool is_odd(double n)
{
    if (n == 0)
        return false;

    return is_even(n - 1);
}

static double tally(double n)
{
    total = total + n;

    return total;
}

static std::string report(const std::shared_ptr<item> &thing)
{
    return thing->describe();
}

int main(int argc, char **)
{
    const double a = 17;
    const double b = 5;

    display(a + b);
    display(a - b);
    display(a * b);
    display(a / b);
    display(std::fmod(a, b));
    display(-a);
    display(a + b);
    display(a - b);
    display(a * b);
    display(a / b);
    display(std::fmod(a, b));
    display(std::floor(a / b));
    display(std::ceil(a / b));
    display(std::round(a / b));
    display(std::fabs(-a));
    display(fact(18));

    display(a > b);
    display(a >= b);
    display(a < b);
    display(a <= b);
    display(a == 17);
    display(a != b);

    const bool yes = true;
    const bool no = false;

    display(!yes);
    display(yes && no);
    display(yes || no);
    display(!no);

    const std::string greeting = "hello, satellite";

    display(greeting);
    display(static_cast<double>(greeting.size()));
    display(greeting.find("sat") != std::string::npos);
    display(greeting.rfind("hello", 0) == 0);
    display(greeting.size() >= 4 && greeting.compare(greeting.size() - 4, 4, "lite") == 0);
    display(greeting + "!");
    display(greeting + "!");
    display(std::string(1, at(greeting, 0)));
    display(std::string(1, at(greeting, -1)));
    display(slice(greeting, 7, static_cast<long long>(greeting.size())));
    display(slice(greeting, 0, 5));
    display(slice(greeting, 7, 10));

    std::vector<double> primes;

    primes.push_back(2);
    primes.push_back(3);
    primes.push_back(5);
    primes.push_back(7);
    primes.push_back(11);

    display(primes);
    display(static_cast<double>(primes.size()));
    display(primes.front());
    display(primes.back());
    display(contains(primes, 7));
    display(contains(primes, 4));
    display(at(primes, 2));
    display(at(primes, -1));
    display(slice(primes, 1, 4));
    display(slice(primes, 0, 2));
    display(slice(primes, 3, static_cast<long long>(primes.size())));
    display(slice(primes, 2, 100));

    std::vector<std::vector<double>> grid;

    grid.push_back(slice(primes, 0, 2));
    grid.push_back(slice(primes, 3, static_cast<long long>(primes.size())));

    display(grid);

    double i = 0;

    while (i < 3) {
        display(i);

        i = i + 1;
    }

    for (double j = 0; j < 3; j = j + 1) {
        display(j * j);
    }

    if (a > b) {
        display(std::string("a wins"));
    } else if (a == b) {
        display(std::string("tie"));
    } else {
        display(std::string("b wins"));
    }

    display(is_even(10));
    display(is_odd(10));
    display(tally(4));
    display(tally(6));

    std::shared_ptr<item> bolt = std::make_shared<item>("bolt", 40);

    display(bolt->name());
    display(bolt->quantity());
    display(bolt->describe());

    bolt->restock(8);

    display(bolt->describe());

    std::shared_ptr<item> alias = bolt;

    alias->restock(2);

    display(bolt->describe());
    display(bolt == alias);

    std::shared_ptr<item> washer = std::make_shared<crate>("washer", 100);

    display(std::static_pointer_cast<crate>(washer)->boxes());
    display(washer->describe());
    display(report(bolt));
    display(report(washer));

    const auto started = std::chrono::system_clock::now();
    const auto ended = std::chrono::system_clock::now();

    display(std::chrono::duration_cast<std::chrono::nanoseconds>(ended - started).count() >= 0);
    display(argc >= 1);

    return 0;
}
