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
#include <vector>

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
            else if (got == 0 || errno != EINTR)
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

// A SEEDED GENERATOR IS BUILT FROM ITS SEED IN A DEFINED ORDER -- the table, then the
// state, then the stream, each limb the next word of a splitmix64 run from the seed --
// and handed to pcg's constructor that takes all three (extended(data, seed, stream)).
// NOT pcg's one-number constructor: that one fills the table through selfinit(), whose
// first line is `baseclass::operator()() - baseclass::operator()()`, two calls as the
// operands of one `-`, which C++ leaves unsequenced; clang and g++ evaluate them in
// opposite orders for a class type, and the review of 0b6fe8f measured two different
// sequences for one seed from the two compilers. The kernel path never meets this: it
// seeds through generate_to, which is a loop. Splitmix64 is Steele, Lea and Flood's
// (2014) mixer, the one Java's SplittableRandom and every seed expander since use.
struct SeededParts {
    std::vector<uint512> table;
    uint1024 state, stream;

    explicit SeededParts(const satellite_number &seed) : table(1u << 14)
    {
        // THE SEED IS ANY WHOLE NUMBER. Its lowest limb is the splitmix state; every limb
        // above it is folded in, one step each; a negative seed is the complement of a
        // positive one's state. A seed that fits one limb and is not negative is that limb,
        // exactly, which is what the harness's pinned first limb of 12345 holds to.
        unsigned long long int next = seed.limb(0);
        const auto mixed = [&next] {
            next += 0x9E3779B97F4A7C15ull;
            unsigned long long int z = next;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            return z ^ (z >> 31);
        };
        for (std::size_t i = 1; i < seed.limb_count(); ++i)
            next = mixed() ^ seed.limb(i);
        if (seed.negative())
            next = ~next;
        for (uint512 &cell : table)
            for (unsigned long long int &limb : cell.limb)
                limb = mixed();
        for (unsigned long long int &limb : state.limb)
            limb = mixed();
        for (unsigned long long int &limb : stream.limb)
            limb = mixed();
    }
};

// pcg512_k16384, 512 bits a call, handed out as eight limbs, least significant first,
// before the next call -- the generator's own stream, a limb at a time.
class Source final : public LimbSource {
public:
    Source() : rng_(KernelSeed{}) {}
    explicit Source(const satellite_number &seed) : Source(SeededParts(seed)) {}
    unsigned long long int next_limb() override
    {
        if (left_ == 0) {
            held_ = rng_();
            left_ = uint512::limb_count;
        }
        return held_.limb[uint512::limb_count - left_--];
    }

private:
    explicit Source(const SeededParts &parts) : rng_(parts.table.data(), parts.state, parts.stream) {}

    pcg512_k16384 rng_;
    uint512 held_;
    unsigned left_ = 0;
};

} // namespace

// ONE A THREAD, MADE ON FIRST USE, and a unique_ptr so that a thread which never draws
// never holds the table: a thread_local pcg512_k16384 would put 1 MiB into every thread satl
// starts, 1024 of them at start-up alone. The object is a megabyte, so it lives on the heap.
namespace {

std::unique_ptr<LimbSource> &this_threads_source()
{
    thread_local std::unique_ptr<LimbSource> source;
    return source;
}

} // namespace

LimbSource &random_source()
{
    std::unique_ptr<LimbSource> &source = this_threads_source();
    if (source == nullptr)
        source = std::make_unique<Source>();
    return *source;
}

// THE MOST RECENT SEED WINS, whatever this thread had: a kernel-seeded generator, or an
// earlier seed's. The old one is freed; the new one costs one pass of splitmix over 1 MiB.
void reseed_random_source(const satellite_number &seed)
{
    this_threads_source() = std::make_unique<Source>(seed);
}

std::unique_ptr<LimbSource> seeded_random_source(const satellite_number &seed)
{
    return std::make_unique<Source>(seed);
}

RandomGeneratorFacts random_generator_facts()
{
    return {"pcg512_k16384", 512, kJointPeriodPow2, sizeof(pcg512_k16384)};
}

} // namespace satellite004
