#pragma once
// satellite/satellite_random/random_source.hpp -- satellite.random's generator, behind one
// seam that hands out 64 bits at a time.
//
// THE AUTHOR, 2026-10-02: "First we need to take the pcg that is in /vendor and use it to
// build a random number generator... the generator in /vendor has a 16386k variant that I
// wanted to use, it's 32_16386 but I wanted to make it 512_16386 so it's a 512-bit x 16386
// generator".
//
// ONE GENERATOR, pcg512_k16384 (pcg_512.hpp): pcg32_k16384 at 512 bits -- a 1024-bit LCG,
// 512 bits out, 16384 words of 512 bits XORed over them. fast, normal and ultra all draw from
// it; what makes them three grades is how long they throw draws away first (random_spin.hpp).
//
// SEEDED FROM THE KERNEL, getrandom(2), the WHOLE state -- 1,048,832 bytes of it -- never a
// 64-bit seed stretched over a table. 003 made that choice and said why (its random.cpp: "the
// seed fills the whole table: about half a million bits of kernel entropy"); it is kept, and
// it is why a generator is made once a thread and never per call.
//
// ONE GENERATOR A THREAD, MADE ON FIRST USE. A program's threads (satellite.variable.thread)
// each get their own, seeded on their own, so no lock stands on a draw and two threads never
// read one stream; a thread that never draws never holds the table. satl's 1024 start-up
// threads tokenise and never draw.
//
// THE SEAM IS 64 BITS WIDE, uniform over the whole limb, which is the one property the
// sampler's argument rests on (random_draw.hpp). 001 and 003 drew the same line at 32 bits
// and proved it generator-agnostic with a stub; 004's numbers are 64-bit limbs, so the seam
// is one limb. The generator's 512-bit outputs are handed out eight limbs at a time, least
// significant first -- the same stream, consumed a limb at a time.
//
// NOTHING ABOVE THIS LINE NAMES A PCG TYPE. random_source.cpp is the one translation unit of
// satl that includes the Apache-2.0 headers (and random_cases.cpp, the harness), so the
// dependency has one file to be replaced in, as 003's rule had it.
//
// Written 2026-10-02.

#include <cstddef>
#include <memory>

namespace satellite004 {

// The seam. next_limb() is uniform over 0 .. 2^64 - 1, every call, from one stream.
class LimbSource {
public:
    virtual ~LimbSource();
    virtual unsigned long long int next_limb() = 0;
};

// THIS THREAD'S generator: made and seeded from the kernel the first time this thread asks,
// kept until the thread ends.
LimbSource &random_source();

// A generator seeded from one number: the same seed gives the same limbs in the same order,
// every run, every machine and every compiler -- the table, state and stream are built from
// the seed in a defined order and handed to pcg whole (random_source.cpp says why not pcg's
// own one-number constructor). The harness drives the sampler through this and pins the
// first limb of one seed; satellite.random.seeded, when the author shapes it, is this
// behind a word.
std::unique_ptr<LimbSource> seeded_random_source(unsigned long long int seed);

// What the generator is made of, for the doc, --debug and the harness: read from the type,
// never typed twice.
struct RandomGeneratorFacts {
    const char *generator;      // "pcg512_k16384"
    unsigned output_bits;       // what one call of the generator answers: 512
    std::size_t period_pow2;    // log2 of the period: the joint one, 16 + 16384 * 512 (pcg_512.hpp says why
                                // pcg's own period_pow2() states 1024 + 16384 * 512 and overcounts)
    std::size_t state_bytes;    // sizeof the generator
};
RandomGeneratorFacts random_generator_facts();

// Bytes from the kernel: getrandom(2) in a loop, so it is never short and never an
// exception; ENOSYS falls back to /dev/urandom (random_source.cpp says why and when).
void kernel_random_bytes(void *into, std::size_t count);

} // namespace satellite004
