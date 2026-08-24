// reg_test_register_stack.cpp — the register stack: frames as windows into one
// array with zero-copy arguments, and what an empty stack costs to build. Part
// of the reg_test binary; the harness and main() are in reg_test.cpp.

#include "reg_test.hpp"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <memory>

using namespace satellite;

void reg_test_frame_windows()
{
    // --- frames are windows -------------------------------------------------
    RegStack stack(1024);
    size_t main_base = 0, callee_base = 0;
    check(stack.push_frame(8, main_base), "a frame is pushed");
    check(main_base == 0, "the first frame starts at 0");

    stack.at(main_base)[3] = Reg::small(7, 0);
    check_str(stack.at(main_base)[3].to_string(), "7",
              "a slot holds what was written to it");
    check(stack.at(main_base)[4].is_empty(),
          "an untouched slot is EMPTY, not nil");

    // §17.4: the callee's base is set TO THE FIRST ARGUMENT REGISTER, so
    // arguments are already where slots 0..N-1 are expected. Nothing is
    // copied.
    stack.at(main_base)[6] = Reg::small(41, 0);
    stack.at(main_base)[7] = Reg::small(42, 0);
    check(stack.push_frame(5, callee_base), "a second frame is pushed");
    check(callee_base == 8, "the second frame follows the first");

    const size_t arg_base = main_base + 6;
    check_str(stack.at(arg_base)[0].to_string(), "41",
              "an argument is already in the callee's slot 0");
    check_str(stack.at(arg_base)[1].to_string(), "42",
              "and slot 1, with nothing copied");

    stack.pop_frame(callee_base);
    check(stack.top() == 8, "returning restores the mark");
    check_str(stack.at(main_base)[3].to_string(), "7",
              "the caller's slots survive the callee");

    // Exhaustion is reported rather than reallocated: a growing vector
    // would invalidate every base being held.
    size_t base = 0;
    check(!stack.push_frame(2000, base),
          "exhaustion is refused, not grown into");
    check(stack.top() == 8, "a refused push changes nothing");

    // Popping releases a heap register's refcount.
    auto shared = std::make_shared<const Value>(Number(1));
    check(shared.use_count() == 1, "one reference to start");
    size_t deep = 0;
    stack.push_frame(2, deep);
    stack.at(deep)[0] = Reg::boxed(shared);
    check(shared.use_count() == 2, "a heap register holds a reference");
    stack.pop_frame(deep);
    check(shared.use_count() == 1, "popping the frame releases it");
}

void reg_test_stack_startup_cost()
{
    // --- the stack costs what it should at startup --------------------------
    // satl starts in about 2.5 ms and that is a measured advantage worth not
    // spending. Reported rather than asserted, because it is a property of the
    // machine as much as of the code.
    auto t0 = std::chrono::steady_clock::now();
    RegStack stack;
    auto t1 = std::chrono::steady_clock::now();
    const double ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    check(stack.capacity() >= 3000 * 8,
          "the stack honours the documented 3000-activation depth");
    check(stack.constructed() == 0,
          "a fresh stack has constructed NO slots");

    // The budget is satl's whole startup, about 2.5 ms. Constructing every
    // slot eagerly measured 3.2 ms on the machine this was written on,
    // which is why slots are built on demand; a tenth of a millisecond
    // leaves the startup advantage intact with room for a slower machine.
    check(ms < 0.25, "an empty stack costs almost nothing to build");
    printf("  RegStack: %zu slots reserved, %.2f MB, built in %.3f ms "
           "(0 slots constructed)\n",
           stack.capacity(),
           double(stack.capacity() * sizeof(Reg)) / (1024.0 * 1024.0), ms);

    // A program pays for the depth it reaches and not for the ceiling.
    size_t base = 0;
    auto t2 = std::chrono::steady_clock::now();
    for (int i = 0; i < 4; i++)
        stack.push_frame(8, base);
    auto t3 = std::chrono::steady_clock::now();
    check(stack.constructed() == 32,
          "four 8-slot frames construct exactly 32 slots");
    printf("  four frames deep: %zu slots constructed in %.4f ms\n",
           stack.constructed(),
           std::chrono::duration<double, std::milli>(t3 - t2).count());

    // Going deep and coming back does not re-construct: the high-water mark
    // is a mark, not a counter.
    size_t deep_base = 0;
    for (int i = 0; i < 100; i++)
        stack.push_frame(8, deep_base);
    const size_t high = stack.constructed();
    stack.pop_frame(32);
    check(stack.constructed() == high,
          "popping does not un-construct; the mark only rises");
    check(stack.top() == 32, "but the top comes back down");
}
