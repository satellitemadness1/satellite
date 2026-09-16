// satellite/satellite_variable_number/number_cases.cpp -- the test harness for
// satellite_number. check_numbers.py feeds it one operation per line on stdin and
// compares every answer line with Python's exact int. Not part of the interpreter.
//
// Numbers are decimal text, read with from_text (so every case also exercises it).
//   add A B | sub A B | mul A B   -> "<answer> <allocations>": a + b, a copy += b and
//                                    a temporary + b (the rvalue overload), which must
//                                    agree; allocations counts operator new during
//                                    all three (0 is required on the fast path)
//   divide A B       -> "<quotient> <remainder>" or "22 division_by_zero"
//   compare A B      -> -1, 0 or 1 (and all six operators must agree with it)
//   negate A         -> -a
//   text :HEX        -> "ok <to_text>" or "3 <bad_offset>"; HEX is the raw bytes
//   digits A | bytes A -> the satellite_number, as text
//   limbs A          -> "<limb_count> <fits_one_limb 1/0>"
//   limbvalues A     -> "<negative 1/0> <limbs in hex, least significant first>"
//   signed V         -> from_signed(V), V a signed long long in decimal
//   make M N         -> satellite_number(M, N == 1), M an unsigned long long in decimal
//   self A           -> a+=a a-=a a*=a  then  a=a+a a=a-a a=a*a, then a=a and
//                       a=std::move(a) through a reference (eight answers)
//   self_divide A    -> divide(a, a, a, b): "<a> <b>" or "22 division_by_zero"
//   divide_same A B  -> divide(a, b, q, q): q (the remainder, by decision)
//   divide_inputs A B -> divide(a, b, a, b) then divide(a2, b2, b2, a2): "q r q r"
//   timed_text A     -> "<to_text> <from_text ns> <to_text ns> <digits ns> <digits>"
// A line the harness cannot read answers "harness_error"; a broken internal
// agreement answers a word saying which (disagree, operators_disagree, touched).

#include "satellite_number.hpp"
#include "../machine/machine_codes.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <utility>
#include <vector>

static std::size_t allocations = 0;

void *operator new(std::size_t size)
{
    allocations++;
    if (void *memory = std::malloc(size ? size : 1))
        return memory;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return operator new(size); }
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept { std::free(memory); }

using namespace satellite004;

namespace {

bool read_number(const std::string &text, satellite_number &out)
{
    std::size_t offset = 0;
    return satellite_number::from_text(text, out, offset) == success;
}

std::string hex_to_bytes(const std::string &hex)
{
    std::string bytes;
    for (std::size_t index = 0; index + 1 < hex.size(); index += 2)
        bytes.push_back((char)std::strtoul(hex.substr(index, 2).c_str(), nullptr, 16));
    return bytes;
}

long long nanoseconds_since(std::chrono::steady_clock::time_point start)
{
    return (long long)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
}

std::string arithmetic(const std::string &operation, const satellite_number &a, const satellite_number &b)
{
    satellite_number compound(a);
    const std::size_t before = allocations;
    satellite_number result, from_temporary;
    if (operation == "add") {
        result = a + b;
        compound += b;
        from_temporary = satellite_number(a) + b;
    } else if (operation == "sub") {
        result = a - b;
        compound -= b;
        from_temporary = satellite_number(a) - b;
    } else {
        result = a * b;
        compound *= b;
        from_temporary = satellite_number(a) * b;
    }
    const std::size_t used = allocations - before;
    if (result != compound || result.to_text() != compound.to_text() || result.to_text() != from_temporary.to_text())
        return "disagree " + result.to_text() + " " + compound.to_text() + " " + from_temporary.to_text();
    return result.to_text() + " " + std::to_string(used);
}

std::string divided(const satellite_number &a, const satellite_number &b)
{
    satellite_number quotient, remainder;
    if (satellite_number::divide(a, b, quotient, remainder) != success)
        return std::to_string(division_by_zero) + " division_by_zero";
    return quotient.to_text() + " " + remainder.to_text();
}

std::string answer(const std::vector<std::string> &word)
{
    const std::string &operation = word[0];
    satellite_number a, b;
    if (operation == "text" && word.size() == 2 && word[1][0] == ':') {
        satellite_number out(12345);
        std::size_t offset = 999;
        const signed long long int code = satellite_number::from_text(hex_to_bytes(word[1].substr(1)), out, offset);
        if (code == success)
            return "ok " + out.to_text() + (out.negative() ? " negative" : "");
        return out == satellite_number(12345) ? std::to_string(code) + " " + std::to_string(offset) : "touched";
    }
    if (operation == "signed" && word.size() == 2)
        return satellite_number::from_signed(std::strtoll(word[1].c_str(), nullptr, 10)).to_text();
    if (operation == "make" && word.size() == 3) {
        satellite_number made(std::strtoull(word[1].c_str(), nullptr, 10), word[2] == "1");
        return made.to_text() + (made.negative() ? " negative" : "");
    }
    if (word.size() < 2 || !read_number(word[1], a) || (word.size() > 2 && !read_number(word[2], b)))
        return "harness_error";

    if (word.size() == 3) {
        if (operation == "add" || operation == "sub" || operation == "mul")
            return arithmetic(operation, a, b);
        if (operation == "divide")
            return divided(a, b);
        if (operation == "compare") {
            const int order = satellite_number::compare(a, b);
            if ((a == b) != (order == 0) || (a != b) != (order != 0) || (a < b) != (order < 0) ||
                (a <= b) != (order <= 0) || (a > b) != (order > 0) || (a >= b) != (order >= 0))
                return "operators_disagree";
            return std::to_string(order);
        }
        if (operation == "divide_same") {
            satellite_number same(777);
            if (satellite_number::divide(a, b, same, same) != success)
                return std::to_string(division_by_zero) + " division_by_zero";
            return same.to_text();
        }
        if (operation == "divide_inputs") {
            satellite_number a2(a), b2(b);
            if (satellite_number::divide(a, b, a, b) != success || satellite_number::divide(a2, b2, b2, a2) != success)
                return std::to_string(division_by_zero) + " division_by_zero";
            return a.to_text() + " " + b.to_text() + " " + b2.to_text() + " " + a2.to_text();
        }
        return "harness_error";
    }
    if (operation == "negate")
        return (-a).to_text();
    if (operation == "digits")
        return a.digits().to_text();
    if (operation == "bytes")
        return a.bytes().to_text();
    if (operation == "limbs")
        return std::to_string(a.limb_count()) + " " + (a.fits_one_limb() ? "1" : "0");
    if (operation == "limbvalues") {
        char buffer[24];
        std::string out = a.negative() ? "1 " : "0 ";
        for (std::size_t index = 0; index < a.limb_count(); index++) {
            std::snprintf(buffer, sizeof buffer, "%s%llx", index ? "," : "", a.limb(index));
            out += buffer;
        }
        return a.limb(a.limb_count()) == 0 && a.limb(a.limb_count() + 5) == 0 ? out : "limb_past_the_top_not_0";
    }
    if (operation == "self") {
        satellite_number x(a), y(a), z(a), p(a), q(a), r(a), copied(a), moved(a);
        x += x;
        y -= y;
        z *= z;
        p = p + p;
        q = q - q;
        r = r * r;
        satellite_number &copied_alias = copied, &moved_alias = moved;
        copied = copied_alias;
        moved = std::move(moved_alias);
        return x.to_text() + " " + y.to_text() + " " + z.to_text() + " " + p.to_text() + " " + q.to_text() + " " +
               r.to_text() + " " + copied.to_text() + " " + moved.to_text();
    }
    if (operation == "self_divide") {
        if (satellite_number::divide(a, a, a, b) != success)
            return std::to_string(division_by_zero) + " division_by_zero";
        return a.to_text() + " " + b.to_text();
    }
    if (operation == "timed_text") {
        auto start = std::chrono::steady_clock::now();
        satellite_number again;
        read_number(word[1], again);
        const long long from_ns = nanoseconds_since(start);
        start = std::chrono::steady_clock::now();
        const std::string text = again.to_text();
        const long long to_ns = nanoseconds_since(start);
        start = std::chrono::steady_clock::now();
        const satellite_number count = again.digits();
        const long long digits_ns = nanoseconds_since(start);
        return text + " " + std::to_string(from_ns) + " " + std::to_string(to_ns) + " " +
               std::to_string(digits_ns) + " " + count.to_text();
    }
    return "harness_error";
}

} // namespace

int main()
{
    std::ios::sync_with_stdio(false);
    std::string line, out;
    std::vector<std::string> word;
    while (std::getline(std::cin, line)) {
        word.clear();
        std::size_t start = 0;
        while (start <= line.size()) {
            std::size_t end = line.find(' ', start);
            if (end == std::string::npos)
                end = line.size();
            word.push_back(line.substr(start, end - start));
            start = end + 1;
        }
        out += word.empty() || word[0].empty() ? "harness_error" : answer(word);
        out += '\n';
        if (out.size() > (1u << 20)) {
            std::cout << out;
            out.clear();
        }
    }
    std::cout << out;
    return 0;
}
