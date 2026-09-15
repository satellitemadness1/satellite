// satellite.random -- the generator, the seam, and the spin.
//
// THE ONLY TRANSLATION UNIT THAT NAMES A PCG ENTITY. The first satellite held to
// the same rule and it is what made the vendored dependency replaceable: the
// contract is one type, one seed adaptor and four operations, so a reimplemented
// generator has one file to satisfy. pcg/README.md enumerates it.

#include "satellite_random/random.hpp"

#include <pcg_extras.hpp>
#include <pcg_random.hpp>

#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <sys/random.h>
#include <unistd.h>

namespace satellite {

// Out of line so the vtable has one home rather than one per translation unit.
Bits32::~Bits32() = default;

namespace {

using clock_type = std::chrono::steady_clock;

// STEADY, not system_clock. A wall clock can be stepped by ntp or by a user
// while a spin is running, and a deadline computed against one that moves
// backwards does not expire. The first satellite carried a draw-count failsafe
// for exactly that; a monotonic clock removes the need for one.
static_assert(clock_type::is_steady, "the spin needs a clock that cannot go backwards");

// DESIGN §11's table. The only thing that separates the three tiers.
constexpr Window kWindows[] = {
    { 50, 300 },      // fast
    { 500, 600 },     // normal
    { 2000, 3000 },   // ultra
};

} // namespace

Window window_for(Tier tier)
{
    return kWindows[static_cast<int>(tier)];
}

// THE SEED COMES FROM THE KERNEL AND CANNOT ABORT -- PLAN M13's done-when
// clause 9, and the first satellite is why it is a clause and not a habit.
// v1 seeded through libstdc++'s std::random_device, whose default token is
// RDRAND: under strace its 65,568 seed bytes never touched the kernel while
// three comments said otherwise, and when RDRAND fails a hundred retries
// libstdc++ THROWS -- with no catch anywhere under src/, the failure mode of
// a language builtin was SIGABRT. v1's own spec measured the replacement:
// getrandom(2) with flags 0, 12.4x faster, an errno instead of an exception,
// and `ldd` unchanged.
//
// getrandom(0) blocks only until the pool is initialised once, and for reads
// this size can return short or be interrupted -- so it loops. ENOSYS (a
// pre-3.17 kernel) falls back to /dev/urandom; if even that cannot be opened
// the loop retries with a pause rather than inventing entropy from the clock,
// because a weak seed taken silently is DESIGN §1.1's "behind their back" and
// a wait on a machine with no kernel RNG at all -- no such machine builds this
// tree -- is at least a true statement about it.
struct KernelSeed {
    // BATCHED, BECAUSE THE TABLE IS 65,568 BYTES AND A SYSCALL IS NOT FREE: a
    // word at a time would be 16,392 round trips into the kernel where whole
    // batches are a handful -- the bulk read is where v1's spec measured its
    // 12.4x. The buffer is local and copied out through the iterators because
    // pcg_extras hands this function whatever iterator generate_to built, and
    // a syscall needs contiguous bytes.
    template <typename It>
    void generate(It begin, It end)
    {
        std::uint32_t buffer[1024];
        while (begin != end) {
            size_t want = 0;
            for (It probe = begin; probe != end && want < 1024; ++probe)
                want++;

            size_t have = 0;
            while (have < want * sizeof(std::uint32_t)) {
                const ssize_t got =
                    getrandom(reinterpret_cast<char *>(buffer) + have,
                              want * sizeof(std::uint32_t) - have, 0);
                if (got > 0) {
                    have += static_cast<size_t>(got);
                    continue;
                }
                if (got < 0 && errno == EINTR)
                    continue;
                if (got < 0 && errno == ENOSYS) {
                    urandom_bytes(reinterpret_cast<char *>(buffer) + have,
                                  want * sizeof(std::uint32_t) - have);
                    have = want * sizeof(std::uint32_t);
                    continue;
                }
                usleep(1000);
            }

            for (size_t i = 0; i < want; i++)
                *begin++ = buffer[i];
        }
    }

private:
    // The pre-3.17-kernel road, and the wait-rather-than-invent rule from the
    // note above applies here whole: a machine where /dev/urandom cannot be
    // opened is a machine this loop is entitled to wait on.
    void urandom_bytes(char *into, size_t count)
    {
        size_t have = 0;
        for (;;) {
            const int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
            if (fd < 0) {
                usleep(1000);
                continue;
            }
            while (have < count) {
                const ssize_t got = read(fd, into + have, count - have);
                if (got > 0)
                    have += static_cast<size_t>(got);
                else if (got < 0 && errno != EINTR)
                    break; // reopen and carry on from where this stopped
            }
            close(fd);
            if (have == count)
                return;
        }
    }
};

// The generator, hidden behind a pointer so no header above this file has to
// include an Apache-2.0 one.
struct Source::State {
    pcg32_k16384 rng;

    // The seed sequence fills the WHOLE 16384-word extension table -- about
    // half a million bits of kernel entropy. That cost is why a Source is
    // constructed once and kept, never made per draw; 040-sources.mk is where
    // it is measured on this machine rather than quoted.
    State() : rng(KernelSeed{}) {}
    explicit State(std::uint64_t seed) : rng(seed) {}
};

Source::Source() : state_(std::make_unique<State>()) {}
Source::Source(std::uint64_t seed) : state_(std::make_unique<State>(seed)) {}
Source::~Source() = default;

unsigned Source::next()
{
    return state_->rng();
}

long long spin(Tier tier, Bits32 &bits, void (*draw_and_discard)(Bits32 &, void *),
               void *context)
{
    const Window window = window_for(tier);

    // The duration is itself random, drawn inside the window, so the length of a
    // spin is not a constant an observer can rely on. DESIGN §11.
    //
    // Inclusive at both ends, and the +1 is what makes maximum_ms reachable.
    // Drawn from the same stream the answer will come from, which costs one word
    // and needs no second generator -- the first satellite used a second one here
    // and its own source says not to.
    const std::uint64_t span =
        static_cast<std::uint64_t>(window.maximum_ms - window.minimum_ms + 1);
    const long long milliseconds =
        window.minimum_ms + static_cast<long long>(bits.next() % span);

    const auto deadline = clock_type::now() + std::chrono::milliseconds(milliseconds);

    // DO/WHILE, and that is the whole of the "at least one" rule. A number whose
    // single draw outlasts the window still costs exactly one; a window already
    // expired on entry still costs one. A plain while() would throw nothing away
    // for a large enough request and the tier would be a lie.
    long long discarded = 0;
    do {
        draw_and_discard(bits, context);
        discarded++;
    } while (clock_type::now() < deadline);

    return discarded;
}

} // namespace satellite
