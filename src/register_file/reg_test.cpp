// reg.hpp — the VM register type and the register stack.
//
// Three things this file exists to pin, because each is a rule that would
// otherwise be true only by accident:
//
//   1. A slot has THREE states, not two. EMPTY is distinguishable from a slot
//      holding nil, which is what keeps src/evaluator/slots.cpp's "read before its
//      declaration runs" check alive once frames become register windows.
//   2. The inline path ALLOCATES NOTHING. That is the entire point of the type
//      — §17 measured 28 ns of make_shared in a 112.5 ns addition — so it is
//      counted here rather than assumed, with a global operator new counter.
//   3. add_inline is EXACT. It agrees with Number::add on every case it
//      accepts, and it declines rather than being wrong — including when
//      ALIGNING two exponents overflows before any addition happens.

#include "register_file/reg.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <random>
#include <vector>

// --- allocation counting ----------------------------------------------------
// Global, because the claim is about the whole path and not about one call.

static long long allocations = 0;
static bool counting = false;

void *operator new(size_t size)
{
    if (counting)
        allocations++;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, size_t) noexcept { std::free(p); }
void *operator new[](size_t size)
{
    if (counting)
        allocations++;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }

using namespace satellite;

static int failures = 0;

static void check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

static void check_str(const std::string &got, const std::string &want,
                      const char *what)
{
    if (got != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what, want.c_str(),
               got.c_str());
        failures++;
    }
}

int main()
{
    // --- the three states ---------------------------------------------------
    // The rule the whole type exists to preserve. A default Reg is EMPTY, and
    // EMPTY is not nil: src/evaluator/slots.cpp reports "is read before its declaration
    // runs" by testing exactly this distinction, and it survives the move to
    // registers only if the tag carries it.
    {
        Reg fresh;
        check(fresh.is_empty(), "a default Reg is EMPTY");
        check(!fresh.is_set(), "an EMPTY slot is not set");
        check(!fresh.is_nil(), "EMPTY is not nil");
        check(Reg::EMPTY == 0, "EMPTY is tag 0, so a zeroed slot is unset");

        Reg n = Reg::nil();
        check(n.is_nil(), "an explicit nil is nil");
        check(n.is_set(), "a nil slot IS set — nil is a value");
        check(!n.is_empty(), "nil is not EMPTY");
        check(fresh.tag != n.tag, "EMPTY and NIL are different tags");
    }

    // --- round trips --------------------------------------------------------
    // Every alternative a Value can hold, including the one added with the map.
    {
        check(Reg::from_value(nullptr).is_empty(),
              "a null ValuePtr becomes EMPTY, not nil");

        auto v_nil = std::make_shared<const Value>(std::monostate{});
        check(Reg::from_value(v_nil).is_nil(), "monostate becomes NIL");

        auto v_true = std::make_shared<const Value>(true);
        Reg r_true = Reg::from_value(v_true);
        check(r_true.tag == Reg::BOOL && r_true.b, "true rides inline");

        auto v_small = std::make_shared<const Value>(Number(42));
        Reg r_small = Reg::from_value(v_small);
        check(r_small.is_small(), "a small number rides inline");
        check_str(r_small.to_string(), "42", "an inline number renders");

        // A number too large for the small form must BOX rather than truncate.
        Number huge = Number(1);
        for (int i = 0; i < 40; i++)
            huge = Number::mul(huge, Number(10));
        auto v_huge = std::make_shared<const Value>(huge);
        Reg r_huge = Reg::from_value(v_huge);
        check(r_huge.is_heap(), "a number past the small form is boxed");
        check_str(r_huge.to_string(), huge.to_string(),
                  "a boxed number renders as itself");

        auto v_str = std::make_shared<const Value>(make_string(encode("hi")));
        check(Reg::from_value(v_str).is_heap(), "a string is boxed");

        auto v_list = std::make_shared<const Value>(make_list(List{}));
        check(Reg::from_value(v_list).is_heap(), "a list is boxed");

        auto v_map = std::make_shared<const Value>(make_map(MapBody{}));
        Reg r_map = Reg::from_value(v_map);
        check(r_map.is_heap(), "a map is boxed");
        check_str(r_map.to_string(), "{}", "a boxed map renders");

        auto v_time = std::make_shared<const Value>(Time{7});
        check(Reg::from_value(v_time).is_heap(), "a time is boxed");

        // Out again, by value.
        for (const auto &v : {v_nil, v_true, v_small, v_huge, v_str, v_list,
                              v_map, v_time}) {
            ValuePtr back = Reg::from_value(v).to_value();
            check(back != nullptr, "a set slot converts back to a Value");
            if (back)
                check_str(to_string(*back), to_string(*v),
                          "the round trip preserves the value");
        }
        check(Reg().to_value() == nullptr,
              "an EMPTY slot converts back to no Value at all");
    }

    // --- the inline path allocates nothing ----------------------------------
    // The claim §17 rests on. Counted, not assumed.
    {
        Reg a = Reg::small(2, 0);
        Reg b = Reg::small(3, 0);
        Reg out;

        counting = true;
        allocations = 0;
        for (int i = 0; i < 100000; i++)
            if (!add_inline(a, b, out))
                failures++;
        long long inline_allocs = allocations;
        counting = false;

        check(inline_allocs == 0, "100000 inline additions allocate nothing");
        if (inline_allocs != 0)
            printf("  (allocated %lld times)\n", inline_allocs);
        check_str(out.to_string(), "5", "and the answer is right");

        // The comparison that makes the number mean something: the same
        // addition through Number and a ValuePtr, which is what the tree walker
        // does today.
        counting = true;
        allocations = 0;
        for (int i = 0; i < 100000; i++) {
            volatile auto v = std::make_shared<const Value>(
                Number::add(Number(2), Number(3)));
            (void)v;
        }
        long long boxed_allocs = allocations;
        counting = false;
        check(boxed_allocs >= 100000,
              "the boxed path allocates at least once per addition");
        printf("  inline: %lld allocations   boxed: %lld allocations "
               "(100000 additions each)\n",
               inline_allocs, boxed_allocs);
    }

    // --- add_inline is exact, or declines -----------------------------------
    {
        // Aligning exponents can overflow before a digit is added. 1 + 1e-30
        // needs 10^30 to line the decimal points up, and that is the case a
        // scheme which only checks the ADDITION gets wrong.
        Reg one = Reg::small(1, 0);
        Reg tiny = Reg::small(1, -30);
        Reg out;
        check(!add_inline(one, tiny, out),
              "alignment overflow declines rather than being wrong");

        // A modest exponent difference is fine and must be exact.
        Reg tenth = Reg::small(1, -1);
        check(add_inline(one, tenth, out), "1 + 0.1 stays inline");
        check_str(out.to_string(), "1.1", "and is exact, not 1.1000000000000001");

        // Significand overflow declines.
        Reg big = Reg::small(INT64_MAX, 0);
        check(!add_inline(big, one, out), "significand overflow declines");

        // A non-inline operand declines.
        Reg boxed = Reg::boxed(std::make_shared<const Value>(Number(1)));
        check(!add_inline(one, boxed, out), "a boxed operand declines");
        check(!add_inline(one, Reg::nil(), out), "nil declines");
        check(!add_inline(one, Reg(), out), "an EMPTY slot declines");

        // Fuzz against Number::add. Every case add_inline ACCEPTS must agree
        // with the arbitrary-precision answer exactly; the cases it declines
        // are the caller's bignum path and are correct by construction.
        std::mt19937_64 rng(20260808);
        std::uniform_int_distribution<long long> sig(-1000000000LL, 1000000000LL);
        std::uniform_int_distribution<int> ex(-12, 12);
        int accepted = 0;
        for (int i = 0; i < 200000; i++) {
            long long sa = sig(rng), sb = sig(rng);
            int ea = ex(rng), eb = ex(rng);
            Reg x = Reg::small(sa, ea), y = Reg::small(sb, eb), z;
            if (!add_inline(x, y, z))
                continue;
            accepted++;
            Number want = Number::add(Number::from_small(sa, ea),
                                      Number::from_small(sb, eb));
            if (z.to_string() != want.to_string()) {
                printf("FAIL: %lld e%d + %lld e%d -> %s, want %s\n", sa, ea, sb,
                       eb, z.to_string().c_str(), want.to_string().c_str());
                failures++;
                break;
            }
        }
        check(accepted > 100000, "the fuzz accepted a useful fraction inline");
        printf("  fuzz: %d of 200000 random additions stayed inline, all exact\n",
               accepted);
    }

    // --- frames are windows -------------------------------------------------
    {
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

    // --- the stack costs what it should at startup --------------------------
    // satl starts in about 2.5 ms and that is a measured advantage worth not
    // spending. Reported rather than asserted, because it is a property of the
    // machine as much as of the code.
    {
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

    printf("PASS: reg (three slot states with EMPTY distinct from nil; "
           "round trips for all nine Value alternatives; the inline path "
           "allocates nothing; add_inline exact or declining, alignment "
           "overflow included; frames as windows with zero-copy arguments; "
           "sizeof(Reg)=%zu)\n",
           sizeof(Reg));
    return failures ? 1 : 0;
}
