// satellite/satellite_random/random_source.cpp -- the generator, seeded from the kernel, one
// a thread. random_source.hpp is the seam and says what it is.
//
// THE ONLY TRANSLATION UNIT OF satl THAT NAMES A PCG TYPE (random_cases.cpp, the harness,
// is the other). 003 held the same rule and it is what keeps the Apache-2.0 dependency
// replaceable: one file to satisfy.

#include "random_source.hpp"

#include "pcg_512.hpp"

#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <sys/random.h>
#include <unistd.h>

namespace satellite004 {

LimbSource::~LimbSource() = default;

namespace {

// THE PRE-3.17-KERNEL ROAD, and the wait-rather-than-invent rule applies here whole: a
// machine where /dev/urandom cannot be opened is one this loop is entitled to wait on.
void urandom_bytes(char *into, std::size_t count)
{
    std::size_t have = 0;
    for (;;) {
        const int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            usleep(1000);
            continue;
        }
        while (have < count) {
            const ssize_t got = read(fd, into + have, count - have);
            if (got > 0)
                have += static_cast<std::size_t>(got);
            else if (got < 0 && errno != EINTR)
                break;   // reopen and carry on from where this stopped
        }
        close(fd);
        if (have == count)
            return;
    }
}

} // namespace

// THE SEED COMES FROM THE KERNEL AND CANNOT ABORT -- 003's M13 clause 9, kept, and the
// first satellite is why it is a clause: it seeded through std::random_device, whose
// default token is RDRAND, and when RDRAND fails a hundred retries libstdc++ THROWS -- the
// failure mode of a language builtin was SIGABRT. getrandom(2) with flags 0 blocks only
// until the pool is initialised once, answers an errno instead of an exception, and can
// return short or be interrupted for a read this size -- so it loops. Anything it refuses
// that is not EINTR or ENOSYS is waited out with a pause rather than papered over with
// the clock: a weak seed taken silently is DESIGN's "behind their back".
void kernel_random_bytes(void *into, std::size_t count)
{
    char *at = static_cast<char *>(into);
    std::size_t have = 0;
    while (have < count) {
        const ssize_t got = getrandom(at + have, count - have, 0);
        if (got > 0) {
            have += static_cast<std::size_t>(got);
            continue;
        }
        if (got < 0 && errno == EINTR)
            continue;
        if (got < 0 && errno == ENOSYS) {
            urandom_bytes(at + have, count - have);
            return;
        }
        usleep(1000);
    }
}

namespace {

// WHAT pcg SEEDS FROM: anything with generate(begin, end) over 32-bit words (a SeedSeq, in
// the standard's sense, less the members no engine calls). pcg_extras::generate_to always
// hands this a uint32_t RANGE -- the table itself, or a buffer it then spreads over wider
// words -- so the whole table is one read from the kernel. Only that overload exists on
// purpose: should an upstream release ever hand over another iterator, the build stops
// here and says so, rather than a slower path running unnoticed.
struct KernelSeed {
    void generate(std::uint32_t *begin, std::uint32_t *end)
    {
        kernel_random_bytes(begin, static_cast<std::size_t>(end - begin) * sizeof(std::uint32_t));
    }
};

// pcg512_k16384, 512 bits a call, handed out as eight limbs, least significant first,
// before the next call -- the generator's own stream, a limb at a time.
class Source final : public LimbSource {
public:
    Source() : rng_(KernelSeed{}) {}
    explicit Source(unsigned long long int seed) : rng_(uint1024(seed)) {}
    unsigned long long int next_limb() override
    {
        if (left_ == 0) {
            held_ = rng_();
            left_ = uint512::limb_count;
        }
        return held_.limb[uint512::limb_count - left_--];
    }

private:
    pcg512_k16384 rng_;
    uint512 held_;
    unsigned left_ = 0;
};

} // namespace

// ONE A THREAD, MADE ON FIRST USE, and a unique_ptr so that a thread which never draws
// never holds the table: a thread_local pcg512_k16384 would put 1 MiB into every thread satl
// starts, 1024 of them at start-up alone. The object is a megabyte, so it lives on the heap.
LimbSource &random_source()
{
    thread_local std::unique_ptr<LimbSource> source;
    if (source == nullptr)
        source = std::make_unique<Source>();
    return *source;
}

std::unique_ptr<LimbSource> seeded_random_source(unsigned long long int seed)
{
    return std::make_unique<Source>(seed);
}

RandomGeneratorFacts random_generator_facts()
{
    return {"pcg512_k16384", 512, pcg512_k16384::period_pow2(), sizeof(pcg512_k16384)};
}

} // namespace satellite004
