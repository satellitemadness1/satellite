// The exit status of every machine code that matters (PLAN M0.5 entries: 0, 1,
// 255, 256, -1 and 4294967298), through the same exit_status_of() satl's main()
// returns through. No program can stop on 256 yet, so this is where it is proven.
//
//     make build/exit_status_cases && build/exit_status_cases    (check.sh runs it)
//
// Prints one line a case and exits 0 when every status is the one wanted. What
// satl writes on stderr for a code that does not fit goes to stderr here too.

#include "exit_status.hpp"

#include <cstdio>

int main()
{
    struct Case {
        signed long long int code;
        int wanted;
    };
    const Case cases[] = {{0, 0}, {1, 1}, {23, 23}, {254, 254}, {255, 255}, {256, 255}, {257, 255},
                          {-1, 255}, {4294967298LL, 255}, {-9223372036854775807LL - 1, 255},
                          {9223372036854775807LL, 255}};
    int failed = 0;
    for (const Case &c : cases) {
        const int status = satellite004::exit_status_of(c.code);
        const bool right = status == c.wanted;
        failed += right ? 0 : 1;
        std::printf("%s machine code %lld exits %d (wanted %d)\n", right ? "ok  " : "FAIL", c.code, status, c.wanted);
    }
    return failed == 0 ? 0 : 1;
}
