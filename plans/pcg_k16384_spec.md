# pcg32_k16384 — derived spec for an in-tree reimplementation

Written 2026-08-24 by three subagents reading the vendored pcg-cpp-0.98 for
UNDERSTANDING only (no source copied) plus probes run against it. The fourth
agent — which implements this spec and diffs 10000 outputs against the real
pcg32_k16384 — had not finished when this was written, so treat the algorithm
section as unverified until that bit-exactness test is run.

Goal: drop the Apache-2.0 vendored dependency so the top-level MIT LICENSE is
accurate. Legal basis: copyright covers pcg-cpp expression, not the algorithm.
Rule for whoever implements it — write from this description, never
transliterate the header.

---

##### a42ecdb00ad644168
The vendored pcg-cpp is consumed in exactly ONE translation unit, at ten sites, through ONE type and ONE seed adaptor: `pcg32_k16384` (typedef at pcg-cpp-0.98/include/pcg_random.hpp:1748, `ext_setseq_xsh_rr_64_32<14,16,true>`) and `pcg_extras::seed_seq_from<std::random_device>`. Nothing outside src/random_numbers/random.cpp names a pcg entity in code — every other mention in the tree is a comment, a doc paragraph, a build flag, or a licence stanza. The drop-in contract is therefore small and fully enumerable: seed-sequence construction, single-uint64 construction, an unbounded 32-bit draw, and a bounded [0,bound) draw, on a type that is in-place constructible inside std::optional. The seam the whole thing feeds is `satellite::Bits32` (src/satellite_number/bignum.hpp:50-57), a two-member abstract class whose only virtual is `unsigned next()` — the bignum sampler has no other requirement, and the test suite already drives it with a home-grown splitmix32 stub, proving the seam is generator-agnostic. Build impact is three deletions and one collapse back to the generic pattern rule, with byte-identical resulting compile commands. Licensing impact is the point of the exercise: deleting the `Files: pcg-cpp-0.98/*` stanza, its standalone Apache-2.0 licence paragraph, and the "the two routes do NOT agree" caveat makes debian/copyright and the top-level LICENSE state the same terms for the first time, which closes the gap debian/copyright:11-14 documents WITHOUT editing LICENSE at all. Measured reference behaviour for the spec: sizeof 65552, seed sequence requests 16392 words (524,544 bits), entropy-seeded construction ~1.6 ms, value-seeded ~0.026 ms, folded throughput ~137,000 draws/ms here.

-[blocker] The complete drop-in contract: one type, one adaptor, four operations, ten call sites
   anchor: src/random_numbers/random.cpp:94
   Every reference to pcg in compiled code, exhaustively:

- random.cpp:10-11 — `#include <pcg_extras.hpp>`, `#include <pcg_random.hpp>` (angle-bracket, resolved by -isystem)
- random.cpp:94 — `pcg_extras::seed_seq_from<std::random_device> seed_source;`
- random.cpp:95 — `pcg32_k16384 rng(seed_source);` — SEED-SEQUENCE construction
- random.cpp:102 — `rng(uint32_t)` — BOUNDED draw, picks the spin length in [0, max-min+1)
- random.cpp:104 — `rng(uint32_t)` — BOUNDED draw, picks the watchdog cap
- random.cpp:112 — `rng()` — UNBOUNDED draw, inside the fold loop
- random.cpp:138 — `rng_.emplace(spun_seed(tier_))` — construction from a single `uint64_t`
- random.cpp:139 — `(*rng_)()` — UNBOUNDED draw, returned as `unsigned`
- random.cpp:144 — `std::optional<pcg32_k16384> rng_;` — storage

That is the whole surface. The replacement must offer: (a) construction from an entropy source that fills the entire extension table plus base state; (b) construction from one 64-bit value (note the accumulator is uint64_t but random.cpp:132-134 guarantees its value is under 2^32 — the parameter width, not the value range, is what the ctor must accept); (c) `operator()()` returning a 32-bit result; (d) `operator()(uint32_t bound)` giving unbiased [0,bound); (e) destructible + in-place constructible so std::optional::emplace works. Nothing here needs `min()`/`max()`, `discard()`, `seed()`, stream selection, equality, or streaming — no std::uniform_int_distribution or std::shuffle is ever applied to it, so the UniformRandomBitGenerator concept is NOT a requirement. Note the bounded form is used only on the FIRST generator; the second answers only through `operator()()`.
   => Implement exactly these five capabilities and no more. Resist rebuilding a general-purpose std-compatible engine — the consumed contract is four operations wide, and a smaller owned type is easier to audit and to state the licence of.

-[blocker] Bits32 is the real seam, and it is two lines wide — the sampler is already generator-agnostic
   anchor: src/satellite_number/bignum.hpp:50
   `class Bits32` at src/satellite_number/bignum.hpp:50-57 is a virtual destructor plus one pure virtual: `virtual unsigned next() = 0;` — documented at bignum.hpp:54-56 as "Uniform over the whole 32-bit range. A generator that cannot promise that is not one this sampler's uniformity argument holds for."

That is the entire interface between the generator and the arbitrary-precision half. src/satellite_number/random.cpp consumes it in exactly one place: `draw_below(Bits32 &bits, unsigned bound)` at src/satellite_number/random.cpp:27-37, which does its own rejection sampling (threshold = 2^32 mod bound, computed as `(0u - bound) % bound`) and calls `bits.next()` in a loop until a draw lands at or above the threshold. `BigInt::random_below` (src/satellite_number/random.cpp:41-76) then calls `draw_below` once per limb — full base 10^9 for every limb but the top, [0, top+1) for the top — and rejects until the assembled value is below the bound.

The consequence for the replacement is strong and worth stating plainly: the bignum side does NOT use the generator's own bounded draw. It only ever asks for full-width 32-bit values and does its own debiasing. So the ONLY property the sampler's uniformity argument depends on is that `next()` is uniform over the full 32-bit range. src/satellite_number/random.cpp:5-6 says so itself: "Nothing in this file knows what a PCG is; it asks a Bits32 for 32 bits at a time."
   => Leave Bits32 and both files of the sampler completely untouched. The swap is contained to src/random_numbers/random.cpp — only the comment at bignum.hpp:44-49 (which explains WHY Bits32 lives in bignum.hpp, citing "the PCG headers, which are a vendored third-party tree") needs rewording, and its underlying reason survives the swap unchanged.

-[important] The public surface in random.hpp exposes nothing from pcg — the swap is invisible to every caller
   anchor: src/random_numbers/random.hpp:32
   src/random_numbers/random.hpp declares four things and no generator type at all:

- `enum class RandomTier { Fast, Normal, Ultra };` (random.hpp:32-36) — an index into the private window table, nothing more. It is used as `static_cast<int>(tier)` at random.cpp:97 and random.cpp:168 to index `kWindows[]` (random.cpp:38-42), so the enumerator ORDER is load-bearing and the enum has no explicit values.
- `bool random_tier(const std::string &word, RandomTier &out)` (random.hpp:41) — string-to-tier, three literal comparisons at random.cpp:149-164. The false return is what makes `satellite.random.quick` a "no such module function" (random.hpp:38-40).
- `void random_window(RandomTier, long long &min_ms, long long &max_ms)` (random.hpp:44) — exists for errors and tests only.
- `bool random_digits(RandomTier, int digits, Number &out)` (random.hpp:54) and `bool random_range(RandomTier, const Number &low, const Number &high, Number &out)` (random.hpp:59).

The only include in the header is "satellite_number/bignum.hpp" (random.hpp:3) plus <string>. `Draws`, `Window`, `kWindows`, `spun_seed`, `clock_type`, `CAP_LOW/CAP_HIGH`, `BATCH` are all inside an anonymous namespace (random.cpp:20-147). The sole consumer is src/evaluator/modules.cpp:486-564, which touches only RandomTier, random_tier, random_digits and random_range.
   => No header change is needed for the swap beyond the two comment lines that name PCG (random.hpp:10, random.hpp:15). random.hpp:15-19's refusal to call any tier "secure" must survive verbatim — an own-implementation PCG-character generator makes no more of a cryptographic claim than the vendored one did, and the temptation to soften that paragraph because the code is now "ours" should be named and refused.

-[important] Threading: no shared state anywhere — every generator is a per-call stack object, and that property must survive
   anchor: src/random_numbers/random.cpp:178
   There is no thread-local, no singleton, no lock, and no mutable global in the whole path. `Draws bits(tier);` is a plain stack local at random.cpp:178 (random_digits) and random.cpp:189 (random_range), constructed fresh per call and destroyed at return. `spun_seed` (random.cpp:92) constructs its `seed_seq_from<std::random_device>` and its first generator as stack locals too. The only namespace-scope data is `constexpr Window kWindows[]` (random.cpp:38), `constexpr long long CAP_LOW/CAP_HIGH` (random.cpp:58-59) and `constexpr int BATCH` (random.cpp:65) — all read-only.

The interpreter's evaluation is single-threaded in practice: `grep -rn "std::thread" src/` outside tests hits only src/console_output/console.hpp:174 (the printer thread) and src/system_facts/system.cpp:59,332. "Spacesuits" are a LANGUAGE construct (src/environment/spacesuits.cpp:1 — "collecting suits, linking supers, and flattening the layout"), not threads, so satellite.random is not concurrently reachable today. But random.cpp is compiled into library_test_tsan (Makefile:477-479 via TESTSRCS at Makefile:244), so any function-local static or file-scope mutable the replacement introduces would be under ThreadSanitizer's eye the moment two tests race it.

The `std::random_device` instance is likewise per-call, owned by the seed adaptor at random.cpp:94.
   => Keep the no-shared-state shape exactly. In particular do NOT 'optimise' the new generator into a lazily-initialised file-scope singleton or a thread_local to avoid re-seeding — that would trade a property the tree currently has for free (and that TSAN checks) against a cost §18 has already decided to pay on purpose.

-[important] The generator is stored by value inside std::optional, and that costs ~136 KB of stack per draw
   anchor: src/random_numbers/random.cpp:144
   random.cpp:143-144: `RandomTier tier_; std::optional<pcg32_k16384> rng_;` — by value, not by pointer. Measured on this machine with the project compiler: sizeof(pcg32_k16384) = 65552 (a 64 KiB extension table plus 16 bytes of base state), alignof 8, sizeof(std::optional<pcg32_k16384>) = 65560.

Compiling random.cpp with -Wframe-larger-than=2048 reports actual frames:
  random.cpp:173 random_digits — 65640 bytes
  random.cpp:182 random_range  — 65720 bytes
  random.cpp:129 Draws::next   — 70632 bytes (spun_seed inlined into it)
So a draw peaks near 136 KB of stack. Only ONE table is ever CONSTRUCTED at a time, which is what random.cpp:70-71 claims ("keeps one 64 KB extension table alive at a time") and the claim holds: spun_seed at random.cpp:138 is fully evaluated and its frame gone before `emplace` builds the second generator, so the optional's 64 KiB buffer is untouched reserved stack during the spin. But the RESERVATION is real, and the optional's storage is on the stack even for calls the sampler refuses before drawing a bit.

The laziness at random.cpp:137-138 (`if (!rng_) rng_.emplace(...)`) is the reason for the optional and is documented at random.cpp:121-124: a refused bound must not cost three seconds first. §18 measured that at 0.196 s for an over-wide .range against 2-3 s (DESIGN.md:3147-3150).
   => The replacement can keep the by-value optional (same footprint, zero behaviour change) or heap-allocate the 16384-word table behind a unique_ptr (drops the frames to nothing and removes the reservation on refused calls). The second is a genuine improvement but it IS a change — decide it deliberately and record it, rather than letting it fall out of how the new class happens to be written. Whichever is chosen, the lazy-construction contract at random.cpp:137-138 must survive intact: it is a documented, measured §18 property.

-[important] Measured reference behaviour, to pin the spec (observations, not source)
   anchor: src/random_numbers/random.cpp:95
   Run against the vendored copy with /home/madness/opt/clang-24/bin/clang++ -std=c++20 -O2 in the scratchpad:

- The typedef under replacement is at pcg-cpp-0.98/include/pcg_random.hpp:1748 and names `pcg_engines::ext_setseq_xsh_rr_64_32<14,16,true>`. The first parameter is the table size as a power of two: 2^14 = 16384 words, which is where the k16384 in the name comes from. The sibling k-typedefs at pcg_random.hpp:1698-1731 differ only in that first parameter, confirming its meaning.
- sizeof = 65552 bytes = 16384 x 4-byte words + 16 bytes of base state.
- result_type is uint32_t; min() = 0, max() = 4294967295.
- Seeding from a seed sequence requests 16392 32-bit words = 524,544 bits. That is 16384 table words plus 8 more words = 256 bits for the base state (two 64-bit quantities — a state and a sequence/increment selector). DESIGN.md:3051-3052's "on the order of half a million bits" is this number.
- Entropy-seeded construction costs ~1.60 ms wall (three trials: 1.636 / 1.602 / 1.564), which is 16392 std::random_device draws.
- Value-seeded construction costs ~0.026 ms — about 60x cheaper, so the table on that path is DERIVED from the base generator rather than filled from entropy. Confirmed behaviourally: the first 16384 outputs of the extended generator seeded with 42 match plain pcg32 seeded with 42 in 0 of 16384 positions, so the table is neither zero nor bypassed.
- Folded-draw throughput (`acc = (acc + rng()) / 2`, 20M iterations) ~137,000 draws/ms here. DESIGN.md:3080 records ~153,600/ms bare and ~104,000/ms stored on the Xeon E5-2670 v3 §18 was measured on; the same order, different machine.
- Type traits: copy-constructible, move-constructible, default-constructible, constructible from uint64_t and from uint32_t.

The -isystem rationale is live and reproducible: compiling random.cpp with -I instead of -isystem yields exactly one warning, "parameter 'target' set but not used [-Wunused-but-set-parameter]" at pcg_extras.hpp:223 (a stream extraction operator taking uint8_t by value); with -isystem the compile is silent. That is what Makefile:105-108 and DESIGN.md:3186-3189 describe.
   => Two behaviours the spec must decide explicitly rather than inherit by accident: (1) what the single-value constructor does to the 16384 table words — the reference derives them from the freshly seeded base generator, so a replacement that leaves them zero would be a different generator wearing the same name; (2) what default stream/increment the single-argument constructor selects, since the reference's two-argument form takes it and the one-argument form supplies a default. Neither is visible in the tree's own code, and both change the output sequence.

-[blocker] Makefile: PCGFLAGS deletes cleanly and $(RANDOM)/random.o collapses back into the generic pattern rule with a byte-identical command
   anchor: Makefile:109
   Three edits, and the collapse is exact.

(1) Delete Makefile:102-110 — the PCGFLAGS comment block and `PCGFLAGS = -isystem pcg-cpp-0.98/include` (line 109), plus the trailing blank line 110. Line 111 begins the unrelated LDFLAGS comment.

(2) Delete Makefile:429-435 — the comment at 429-432 and the rule:
    $(RANDOM)/random.o: $(RANDOM)/random.cpp
    \t$(CXX) $(CXXFLAGS) $(PCGFLAGS) -I$(SRC) -c -o $@ $(RANDOM)/random.cpp
random.o then falls to the generic pattern rule at Makefile:413-414:
    $(SRC)/%.o: $(SRC)/%.cpp
    \t$(CXX) $(CXXFLAGS) -I$(SRC) -c -o $@ $<
which is the explicit recipe minus $(PCGFLAGS), character for character. The PREREQUISITE set is also unchanged: the deleted rule listed only random.cpp, and $(RANDOM)/random.o is a member of OBJS (Makefile:225), so Makefile:472's `$(OBJS): $(HDRS) .cxxflags-stamp` already supplies $(HDRS) and the flags stamp to it and continues to. There is no behavioural difference of any kind.

(3) Delete `$(PCGFLAGS)` from Makefile:478, the library_test_tsan link — it is the only other user, and only because TESTSRCS (Makefile:244) compiles $(RANDOM)/random.cpp from source rather than linking the prebuilt object.

One trap: .cxxflags-stamp records only '$(CXX) $(CXXFLAGS)' (Makefile:459-460), and PCGFLAGS was never part of it, so removing PCGFLAGS does not by itself invalidate anything — make does not track recipe changes. In practice random.cpp is being rewritten in the same commit so random.o rebuilds anyway, but a Makefile-only edit would silently keep the stale object.

Also verified: clang accepts a -isystem pointing at a deleted directory silently (exit 0, no diagnostic), so a half-finished edit fails quietly rather than loudly — another reason to do all three at once.
   => Do all three edits in the same commit as the random.cpp rewrite. If the replacement adds new files, they need registering: a new .cpp must join OBJS (Makefile:225) and TESTSRCS (Makefile:244) or it will not link; a new private header included only by random.cpp should follow the reasoning already recorded at Makefile:236-240 for format.hpp — adding it to HDRS (Makefile:228-235) makes one edit to it rebuild the entire interpreter, so an explicit prerequisite on a restored random.o rule may be the better trade. That is the ONE case where the rule at 429-435 would be kept rather than deleted, with its comment rewritten from 'sees the vendored PCG headers' to naming the real dependency.

-[blocker] debian/copyright: three stanzas go, and the documented LICENSE gap closes without touching LICENSE
   anchor: debian/copyright:11
   Read in full (70 lines). What changes:

- DELETE lines 10-14, the second paragraph of the file's opening Comment: "The two routes do NOT agree about pcg-cpp-0.98, and that is a known gap rather than an oversight: this file carries its Apache-2.0 stanza and the top-level LICENSE does not mention it, so a tarball install states only the Expat terms. The .deb is correct; `make install` is the route that needs deciding." This paragraph is the exact gap the whole exercise exists to close, and removing pcg-cpp closes it by making the sentence false rather than by answering the question it poses.
- DELETE lines 20-31 entirely, the `Files: pcg-cpp-0.98/*` stanza — Copyright 2014 Melissa O'Neill, License: Apache-2.0, and its two Comment paragraphs. Line 26-27's claim ("This is the ONLY part of the tree that is not the upstream author's own work") becomes true of nothing and goes with the stanza. Lines 29-31's warning against restating 0.98 as dual-licensed on the strength of later pcg-cpp releases goes with it too.
- DELETE lines 37-51, the standalone `License: Apache-2.0` paragraph including its /usr/share/common-licenses/Apache-2.0 pointer. It exists solely to give the deleted stanza a licence body; leaving it behind is a lintian complaint (unused-license-paragraph-in-dep5-copyright).

What stays untouched: `Files: *` / Expat (16-18), `Files: debian/*` / Expat (33-35), and the full `License: Expat` text (53-70). After the deletions the file states one licence, for one copyright holder, over the whole tree.

The top-level LICENSE (21 lines, "MIT License (Expat)", Copyright 2026 Terran Satellite) needs NO edit at all — it becomes correct by subtraction. Makefile:670 installs it as $(docdir)/copyright and Makefile:666-669 explains that dh_installdocs writes debian/copyright to the same path so "the two routes must state the same terms"; after this change they finally do.

Also deleted with the directory: pcg-cpp-0.98/LICENSE.txt, a 201-line Apache License 2.0 text. Nothing in the Makefile install rules ever copied it (Makefile:655-681 installs example/, DESIGN.md, LICENSE, README.md and the man pages only) — which is precisely the mechanism of the gap at copyright:11-14.

Checked and clean: debian/rules, debian/control, debian/satellite.install, debian/satellite-term.install, debian/satellite.dirs and debian/README.packaging contain no pcg reference.
   => Make the three deletions together — an orphaned Apache-2.0 licence paragraph or a surviving 'known gap' comment would each be worse than the current honest state, because both would misdescribe a tree that no longer contains third-party code. Consider adding a short Comment noting that the generator is an independent implementation of a published algorithm, so a later reader does not re-add a stanza out of caution.

-[important] Documentation: DESIGN.md §18 needs six specific edits, README one paragraph
   anchor: DESIGN.md:3021
   DESIGN.md, all inside §18 (2973-3206):
- 3006-3009: "PCG makes no cryptographic claim and its state is recoverable from its output. **No tier here is secure...**" — the claim stays TRUE of an own implementation of the same algorithm. Reword the attribution, keep the refusal.
- 3021 and 3024: steps 1 and 4 of the mechanism name `pcg32_k16384` by its vendored typedef; both become the new type's name.
- 3051-3052: "`seed_seq_from<std::random_device>` fills all 16384 extension words from kernel entropy — on the order of half a million bits" — the number survives (measured: 16392 words, 524,544 bits) but the adaptor's name does not.
- 3110-3111: "which knows nothing about PCG and asks a `Bits32`" — still true, reword.
- 3186-3189: the `-isystem, not -I` bullet becomes obsolete in full. Its verified consequence ("a from-scratch build is silent, and `ldd satl` still lists six shared objects") must be RE-verified after the swap and restated, since it is a §9 property.
- 3190-3193: the `discard()` bullet is a warning about the vendored library's behaviour on k-variants. It documents a real defect in code that is leaving the tree. Either delete it, or keep it as a note that the replacement deliberately does not offer `discard()` at all (which the consumed contract does not need).
- 3194-3195: "One object sees the PCG headers" — the whole point disappears; random.o becomes an ordinary object.
- 3200-3201: open question 2, "Whether a tier ever moves to `getrandom(2)`", is untouched by this work and should stay open.

README.md:279-284: "The generator underneath is PCG, which makes no cryptographic claim and whose state is recoverable from its output, so nothing here is described as secure". Same treatment — the property holds, the attribution changes.

plans/random.md is a historical plan document (dated 2026-08-19 at line 28) and arguably should not be retro-edited at all. Worth knowing it contains a factual error that debian/copyright:29-31 was written to correct: line 235-236 says "`pcg-cpp-0.98/` is in the tree, MIT/Apache-2.0", but 0.98 is Apache-2.0 ALONE — the dual option came in later releases. If the plan is amended rather than left as history, that is the line to fix.
   => §18 is the specification, and the tree treats it that way (random.hpp:7, random.cpp:1, and the tests all cite it). Add a subsection recording WHY the generator became the project's own — the licensing motive — and what was measured to confirm the character survived, so the swap reads as a decision with evidence rather than as a name substitution. §18's own habit is to record what each step was measured to be worth; this change should meet that bar.

-[important] pcg_test/ is a prototype of the SUPERSEDED mechanism and breaks the moment the headers go — but the tree's own recorded reasoning argues for updating it, not deleting it
   anchor: .gitignore:39
   pcg_test/pcg_test.cpp is 74 lines, the only file in the directory, tracked, last modified 2026-08-19. It includes the vendored headers by RELATIVE PATH — `#include "../pcg-cpp-0.98/include/pcg_extras.hpp"` and pcg_random.hpp at pcg_test/pcg_test.cpp:9-10 — not through -isystem, so deleting pcg-cpp-0.98/ breaks it immediately and silently (it is not a Makefile product, so no build catches it).

It prototypes an OLDER mechanism than the one that shipped. Its own header comment at lines 4-7 says each tier "draws its answer from the SAME generator. There is no reseeding step: the seed_seq_from<random_device> below fills all 16384 extension words, and reseeding from a single 32-bit draw would funnel every one of them through 32 bits." That is DESIGN.md:3050-3054's recorded-but-not-taken recommendation. It has no fold, no watchdog cap and no bignum path — plans/random.md:259-262 confirms exactly that.

Its status is deliberate and documented: .gitignore:39-45 keeps the binary out of git and explains that pcg_test "is a PROTOTYPE and not a Makefile product, so it is deliberately outside the list above rather than another name in it... What it prototyped has landed as random.cpp and random_test, and it is kept as the standalone harness the §18 timing numbers came out of — one that needs no interpreter to run is worth keeping around the next time they have to be taken again." Makefile:903 also removes pcg_test/pcg_test in `clean`, under a comment (896-902) about build products that survive clean ending up in somebody's tarball.

The argument in .gitignore:41-44 gets STRONGER, not weaker, with this change: replacing the generator is precisely "the next time they have to be taken again", and a harness that measures throughput and spin behaviour without an interpreter in the way is the cheapest instrument for proving the new generator kept the old character.
   => UPDATE it, do not delete it, and use it as the before/after instrument for the swap — take its numbers against the vendored copy before removing the directory, since afterwards there is nothing to compare against. Then repoint its includes at the project's own generator header, rename the directory to something not named after a departed dependency, and update the three places that name the old path: .gitignore:39-45 (the stanza and the ignored binary path), Makefile:903 (the clean line), and plans/random.md:261 if that document is amended at all — its build line still reads `-isystem ../pcg-cpp-0.98/include`. Deleting the directory instead is defensible, but it discards the only bench that produced §18's numbers, and it should be an explicit decision rather than tidying.

-[important] Nothing in the test suite tests the generator itself — the replacement arrives with no coverage and needs its own
   anchor: src/random_numbers/random_test.cpp:41
   src/random_numbers/random_test.cpp (375 lines) contains no pcg reference and no generator test. Its own header at lines 3-9 explains the split: the SAMPLER is driven by a stub "that does not spin at all", and the TIERS are exercised end to end "a handful of times on `fast`". Lines 11-14: "Nothing here asserts a particular VALUE, because there is no particular value to assert."

What it actually checks of the generator path is only timing and range: test_spin (238-272) asserts a fast draw takes at least its 50 ms floor and that results land in the requested interval; test_tiers (217-236) checks the three window pairs against random_window. Everything statistical — uniformity to 2% over 300000 draws, both interval ends reachable, two-limb rejection, the one-in-ten leading-zero property, the refusals — runs against `class Stub` at random_test.cpp:41-56, a splitmix32 the test file implements ITSELF (state += 0x9e3779b9, two xor-multiply-shift rounds with 0x21f0aaad and 0x735a2d97). So the existing suite would pass unchanged against a replacement generator of ANY quality, including a broken one, so long as it returned 32 bits and the spin took 50 ms.

Build-wise the test is already clean: Makefile:515-516 compiles random_test.cpp with plain TESTFLAGS and links $(LIBOBJS) — it never needed PCGFLAGS, because it links the prebuilt random.o rather than compiling random.cpp. Only library_test_tsan (Makefile:477-479) compiles random.cpp from source.

Two useful notes for the replacement: Stub is a working model of the Bits32 contract implemented from scratch inside this repo, and it demonstrates the deterministic-seed testing style §18 wants (random_test.cpp:38-40: "deterministic from its seed, so a failure here reproduces exactly rather than being a distribution that was unlucky once").
   => Add generator-level tests that the current suite has no equivalent of, since the vendored library's own test suite is what is being given up: a known-answer test fixing the output sequence for a fixed seed (which is what makes a future refactor safe), a check that seeding fills all 16384 table words rather than a prefix, equidistribution across the 32-bit range, and a check that the value-seeded and entropy-seeded paths produce independent streams. Without a known-answer vector the replacement has no regression net at all.

##### abca50d653a2daacd
The seeding half of `satellite.random`, pinned down by reading the vendored headers and by instrumented probes against the reference. Headline numbers: constructing a `pcg32_k16384` from `pcg_extras::seed_seq_from<std::random_device>` draws exactly **16392 32-bit words = 65568 bytes**, of which 4 words (128 bits) are drawn and thrown away and 1 more bit is lost to the odd-increment shift; 524415 bits actually land in the state, which makes DESIGN.md:3051's "on the order of half a million bits" accurate. It is paid **per `satellite.random` call** — verified end-to-end, three `fast` calls in one program take 0.25 s against 0.07-0.08 s for one. Two blockers came out of it. First, DESIGN.md:3051, random.hpp:10 and random.cpp:67/86 all say "kernel entropy" and on this machine that is false: under strace the built `./satl` issues exactly one `getrandom` in the whole process and it is glibc's own 8-byte start-up call; the 65568 bytes come from the **RDRAND instruction** via libstdc++'s default token, never touching the kernel. Second, that path can **throw** — libstdc++ retries RDRAND 100 times then raises `std::runtime_error`, and there is no `catch` anywhere under `src/`, so the failure mode of a language builtin is SIGABRT. Recommendation is `getrandom(2)` with flags 0 in a loop: it makes the "kernel entropy" sentence true by construction, turns the failure into an errno instead of an exception, keeps `ldd satl` at six because it lives in libc, and is **12.4x faster** (0.133 ms for the full 65568 bytes against 1.649 ms measured for the current path). Exactly one value constraint exists in the whole seeding ceremony: the base LCG increment must be odd. The 16384 extension words are unconstrained — all-zero and all-ones tables both verified working.

-[important] Exact seeding draw: 16392 words / 65568 bytes, with a 4-word discard and a documented layout
   anchor: src/random_numbers/random.cpp:94-95
   Constructing a `pcg32_k16384` from `pcg_extras::seed_seq_from<std::random_device>` (src/random_numbers/random.cpp:94-95) pulls exactly **16392** 32-bit values = **65568 bytes**. Measured by substituting a counting stub for the wrapped engine (scratchpad/count.cpp); a plain `pcg32` from the same adaptor pulls 8.

The word-to-field layout, established by feeding a scripted stream of distinguishable words and reading the constructed state back through the reference's stream-insertion operator (scratchpad/order.cpp):

- words 0-1: drawn, **discarded**
- words 2-3: base LCG state seed, low half first (little-endian pairing, verified by setting one word to all-ones at a time)
- words 4-5: base stream seed, low half first
- words 6-7: drawn, **discarded**
- words 8-16391: the 16384 extension words, one word each, in order (table[0] = word 8, table[16383] = word 16391)

So 128 bits are drawn from the entropy source and never used. The waste is structural in the reference: the base engine's two seeds each come from a helper that materialises a two-element array of 64-bit values (4 words) and returns one of them, and it is invoked twice.

Entropy actually installed: 64 (state) + 63 (stream, see the 63-bit finding) + 16384*32 (table) = **524415 bits**, against 524544 drawn. `sizeof(pcg32_k16384)` = 65552 bytes; `period_pow2()` = 524352 = 64 + 16384*32; `pcg32::streams_pow2()` = 63.
   => The spec should call for 16384 words for the table plus one 64-bit state seed plus one 63-bit stream seed = **65552 bytes**, all of it used. Do not reproduce the 4-word discard; it is an artifact of the reference's generic seed-sequence plumbing, nothing depends on it, and there is no byte-for-byte compatibility requirement anywhere in the repo (nothing serialises or reproduces a seeded generator).

-[blocker] BLOCKER: "kernel entropy" is false as built — the seed comes from RDRAND, not the kernel
   anchor: DESIGN.md:3051
   DESIGN.md:3051, src/random_numbers/random.hpp:10, src/random_numbers/random.cpp:67 and src/random_numbers/random.cpp:86 all state that the seed comes from *kernel* entropy. On the machine §18 was measured on, it does not.

Evidence, in order of directness:

1. `strace -f -e trace=getrandom,openat ./satl` on a program whose first line is `satellite.random.fast(12)` shows exactly **one** `getrandom` call in the entire process — `getrandom("...", 8, GRND_NONBLOCK) = 8` — which is glibc's own start-up draw for the stack guard, identical to the one a hello-world binary makes. No `/dev/urandom`, no `/dev/random`, no second `getrandom`.
2. `strace -c` on a standalone probe doing 16392 `std::random_device` draws shows **zero** syscalls attributable to them.
3. `objdump -d` on this build's libstdc++ (`/home/madness/opt/gcc-17/lib64/libstdc++.so.6`, `__GLIBCXX__` 20251022) contains `__x86_rdrand` and `__x86_rdseed`; the default token resolves to RDRAND when the CPU advertises it. This CPU (Intel Xeon E5-2670 v3, the §17/§18 machine) advertises `rdrand` and **not** `rdseed` — the explicit `"rdseed"` token throws `device not available: Function not implemented`.

So the 65568 bytes come off the CPU's on-die DRNG. Which source `std::random_device` picks is a runtime property of the CPU, not of the source code: the same binary on a CPU without RDRAND would silently switch to `/dev/urandom` and become 8.4x slower (13.95 ms vs 1.67 ms per 16392 draws, measured), with 16392 four-byte `read()` syscalls.
   => Either the sentence is rewritten to something unfalsifiable ("whatever `std::random_device` picks"), or the implementation moves to `getrandom(2)` and the existing sentence becomes true by construction on every Linux build. Since the entire point of the exercise is to own the code, take `getrandom` — it costs nothing (see the cost finding) and it retires an inaccuracy in three files at the same time.

-[blocker] BLOCKER: the seeding path can throw, and nothing under src/ catches it
   anchor: src/random_numbers/random.cpp:95
   Two throwing paths reach `spun_seed`:

1. **Constructor failure.** `std::random_device`'s constructor throws when its chosen source is unavailable — verified: the `"rdseed"` token on this CPU throws `random_device::random_device(const std::string&): device not available: Function not implemented`.
2. **Runtime RDRAND failure.** The disassembled `__x86_rdrand` loads 0x64 (**100**) into its retry counter, retries while the carry flag is clear, and branches to a cold path that raises `std::runtime_error`; `strings` on the library confirms the message `random_device: rdrand failed`.

Either exception is raised inside the `pcg32_k16384` construction at src/random_numbers/random.cpp:95, propagates out of `spun_seed()`, out of `Draws::next()` (random.cpp:137-139), out of `random_digits`/`random_range` (random.cpp:178, :189), through the evaluator call sites at src/evaluator/modules.cpp:518 and :556, and out of `main` — `grep -rn "catch (" src/` returns nothing but a comment, and src/programs/main.cpp has no `try`, no `catch`, no `set_terminate`. Result: `std::terminate` / SIGABRT, not a satellite diagnostic. A language builtin that can hang is not one (random.cpp:47); neither is one that can abort.

Worse, there is a **silent** variant the retry loop cannot catch. The loop tests the carry flag, so hardware that returns a stuck value with CF *set* is accepted unchecked — which is exactly the AMD Ryzen 3000 firmware bug where RDRAND returned all-ones, and the AMD family 15h/16h behaviour where RDRAND stops producing entropy after resume from suspend (the kernel clears the CPUID bit for these; that blacklist does nothing for a userspace RDRAND executed inside libstdc++). On such a part all 16384 extension words would be filled with the same constant and nothing in satellite would notice.
   => The from-scratch seeding function must not be able to throw out of a builtin. `getrandom(2)` reports failure as an errno, so make the seeding function return `bool` and let it fail the call the way the code already knows how to — `random_digits` at random.cpp:178-179 and `random_range` at random.cpp:189-192 already thread a bool up to modules.cpp:518/:556, which turns it into the language's own "could not draw" message. No exception, no abort, no new machinery.

-[important] What DESIGN §18 promises about seeding, and which promises survive the rewrite
   anchor: DESIGN.md:3019-3025
   The seeding promises, with line numbers, from DESIGN.md:3017-3065 and their echoes:

- **DESIGN.md:3019-3021** — "Per call, in order: 1. Seed a `pcg32_k16384` from `pcg_extras::seed_seq_from<std::random_device>`." This names the vendored type *and* the standard-library type. Also echoed at plans/random.md:68.
- **DESIGN.md:3050-3054** (and mirrored verbatim in the code comment at random.cpp:85-87) — "`seed_seq_from<std::random_device>` fills **all 16384 extension words** from kernel entropy — **on the order of half a million bits** — and steps 3 and 4 funnel every one of them through a 32-bit accumulator." This is load-bearing: it is the entire justification for the recorded-but-not-taken recommendation that follows, that answering straight from the first generator beats any fold. Both halves are measurably true today (16392 words drawn, 524415 bits installed).
- **DESIGN.md:3004-3015** — the seeding is explicitly **not** a security property. "PCG makes no cryptographic claim and its state is recoverable from its output. No tier here is secure and none of them is described that way." And: "The word 'secure' in a language's documentation is load-bearing: someone will key something on it."
- **DESIGN.md:3013-3014** — `getrandom(2)` is already on the record as "the available answer, not taken" for a hypothetical secure tier, costed at "256 bits of kernel entropy in about a microsecond, and `ldd satl` stays at six." **Both figures check out**: measured `getrandom` of 256 bytes is 0.8 us median (32 bytes is below timer resolution), and `ldd satl` today lists exactly six entries (linux-vdso, libstdc++, libm, libgcc_s, libc, ld-linux) — `getrandom` is in libc, so it adds nothing.
- **DESIGN.md:3019** — "Per call" is itself a promise, and it holds (see the cost finding).
- **DESIGN.md:3198-3201** — "What is not decided" item 2 keeps a `getrandom(2)` move open, so switching the *seeding* primitive reverses no settled decision.

Three promises the rewrite must keep: (a) a 16384-word extension table, (b) **every word filled independently from the entropy source** — deriving them from a smaller seed would silently falsify DESIGN.md:3051 and gut the recommendation at 3050-3054, (c) no security claim anywhere, including in whatever replaces the naming at 3021.

One promise the rewrite **cannot** keep: DESIGN.md:3021 and plans/random.md:68 name `pcg_extras::seed_seq_from<std::random_device>` by type, and that type is the thing being deleted. Those two lines must be rewritten whichever primitive is chosen.
   => Rewrite DESIGN.md:3021 and plans/random.md:68 to name the mechanism rather than the vendored type — "Seed a pcg32 base with a 16384-word extension table, every word drawn from `getrandom(2)`". Keep DESIGN.md:3050-3054 exactly as it stands; it stays true and stays checkable. When rewriting, guard DESIGN.md:3011-3015: moving the seeding to `getrandom` must not be allowed to drag the word "secure" into §18 — the tiers remain a statistical character, because the generator is still a PCG whose state is recoverable from its output no matter how good the seed was.

-[important] Recommended primitive: getrandom(2) with flags 0, in a loop; /dev/urandom only on ENOSYS; never std::random_device
   anchor: src/random_numbers/random.cpp:94
   **Semantics that matter**, from `getrandom(2)` as installed here (Linux man-pages 6.06) and confirmed by probe:

- flags 0 draws from the urandom source and blocks **only** until the CRNG is initialised. After initialisation it never blocks. This is the whole reason to prefer it over `/dev/urandom`, which hands out non-random bytes before initialisation without saying so.
- "If the urandom source has been initialized, reads of **up to 256 bytes** will always return as many bytes as requested and will not be interrupted by signals. **No such guarantees apply for larger buffer sizes.**" 65552 bytes is far above 256, so **the loop is mandatory**: a signal handler can produce a partial return or `EINTR`. Measured one-shot `getrandom(65568)` returned in full 50/50 times here — which proves nothing, since the short return is signal-dependent, not load-dependent.
- Errors: `EAGAIN` only with `GRND_NONBLOCK`; `EINTR` per above; `ENOSYS` if the kernel predates 3.17 (glibc here is 2.39, whose wrapper reports it); `EFAULT`/`EINVAL` are programmer errors.
- `GRND_RANDOM` (0x2) is wrong here — it caps at 512 bytes per call and can block for no benefit on a modern kernel. `GRND_INSECURE` (0x4, present in this `<sys/random.h>`) is wrong here too — it defeats the one guarantee flags 0 buys.
- `getentropy(3)` is the same syscall with a 256-byte cap and all-or-nothing semantics. 257 calls measured at 159.6 us against 133.3 us one-shot — acceptable, but pointless when the `getrandom` loop is three lines.
- No file descriptor is involved, which the man page calls out as a reason to prefer it: it survives `chroot` and fd exhaustion.

**Exact chain:**
1. Loop `getrandom(buf + got, want - got, 0)` until `got == want`. Treat a negative return with `errno == EINTR` as continue; treat any positive short return as progress and go round again.
2. On `ENOSYS` **only**, fall back to `open("/dev/urandom", O_RDONLY | O_CLOEXEC)` plus a read loop (handling short reads and `EINTR`), then close.
3. On any other errno, or a failed fallback, **return false** and let the builtin fail with the language's own message. Do not retry forever. Do not fabricate a clock-derived seed — a silently-weak seed is worse than a refused call, and §18 has no promise that a draw always succeeds.

**Do not use `std::random_device`.** Beyond the two blockers: the standard does not require it to be non-deterministic (an implementation may use a pseudo-random engine when it cannot do better, reporting `entropy() == 0`), and the historic MinGW/libstdc++ case where it was a fixed Mersenne Twister is the canonical reminder that the type carries no guarantee. Even on Linux its source is chosen at runtime from CPU features, so the same binary's entropy provenance varies by machine.

Sources: [getrandom(2)](https://man7.org/linux/man-pages/man2/getrandom.2.html), [PCG streams and seeding](https://www.pcg-random.org/posts/critiquing-pcg-streams.html).
   => Specify `getrandom(2)` flags 0 in a retry loop as the primary, `/dev/urandom` on `ENOSYS` only, and a `bool` return on everything else. Include `<sys/random.h>`; nothing links, it is libc, so `ldd satl` stays at six as DESIGN.md:3014 requires.

-[important] Cost: paid per satellite.random call, and 64 KB from getrandom is 12.4x cheaper than what it replaces
   anchor: src/random_numbers/random.cpp:178
   **Per call, not per program and not per tier.** `Draws` is constructed fresh at src/random_numbers/random.cpp:178 (`random_digits`) and :189 (`random_range`); each is reached once per builtin invocation from src/evaluator/modules.cpp:518 and :556. `Draws::next()` (random.cpp:137-139) lazily calls `spun_seed` once and caches the second generator in the `std::optional` at random.cpp:144 — so one invocation buys one entropy seeding, one spin, and one accumulator-seeded generator, and the seeding is amortised across all the 32-bit words a single wide draw consumes but **not** across calls.

Verified end to end against the built `./satl`: one `satellite.random.fast(12)` = 0.07-0.08 s wall; **three** `fast` calls in one program = 0.25 s; one `ultra(12)` = 2.36 s. Three calls draw 3 x 65568 bytes.

**Measured** (Xeon E5-2670 v3, median of 20+, scratchpad/timing.cpp and gr.cpp):

| what | median |
|---|---|
| `seed_seq_from<random_device>` -> `pcg32_k16384` (current path) | **1.649 ms** (min 1.561) |
| 16392 bare `std::random_device` draws | 1.625 ms |
| `getrandom` 65568 bytes, one shot | **0.133 ms** |
| `getrandom` 65568 bytes, chunked at 256 | 0.160 ms |
| `pcg32_k16384` from a single uint64 (step 4's selfinit) | 0.025 ms |
| `random_device` with the `/dev/urandom` token, 16392 draws | 13.95 ms |

So essentially all of the current 1.649 ms is the entropy source, not the table fill. `getrandom` runs at ~490 MB/s here (0.8 us at 256 B, 8.5 us at 4 KB, 2.21 ms at 1 MB).

**Against the tier windows**, seeding as a share of a call drops from 1.65-3.30% to 0.13-0.27% for `fast` (50-100 ms), 0.55-0.66% to 0.044-0.053% for `normal`, and 0.055-0.082% to 0.0044-0.0067% for `ultra`.

**No kernel-side depletion concern to weigh against it.** Since Linux 5.18 the urandom source is a ChaCha20 CSPRNG with no entropy accounting on the read path; this kernel (6.12.0-211.47.1.el10_2) reports `poolsize` 256 and `entropy_avail` 256, which is the initialised constant rather than a level that drains.
   => Draw the full 65552 bytes per call. Do not cache a generator across calls to save it — the per-call reseed is what DESIGN.md:3019 promises, and the saving would be 0.13 ms against a 50-3000 ms spin. The 256-byte chunked loop costs 20% more than one-shot (27 us) and is not worth choosing over a plain retry loop on the full buffer, but either is fine to specify.

-[important] Seed-value constraints: exactly one, and it is not on the table
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:308
   **The base LCG increment must be ODD.** The modulus is 2^64; by Hull-Dobell full period requires the increment coprime to the modulus, which for a power-of-two modulus means odd. pcg-random.org states it in one line: *"Increments have just one rule: they must be odd. That's it."* The reference enforces it structurally — it forms the increment by shifting the 64-bit stream seed left one and setting bit 0 — which is *why* the stream is 63 bits and not 64. An implementation that took a raw entropy word as the increment directly would get an even increment half the time and wreck the base period on those runs. **This is the one trap.**

**The multiplier must be ≡ 1 (mod 4)**, same full-period condition. The reference's 64-bit multiplier is 6364136223846793005; its low hex digit is D, so it is 1 mod 4. This constrains the constant, not the entropy.

**The base state has no constraint.** Any 64-bit value is a legal LCG state — an odd-increment LCG mod 2^64 has full period from every start. (The reference does run the seed through one extra step before storing — add increment, multiply, add increment. That is a stream-labelling detail, not a correctness requirement.)

**The 16384 extension words have no constraint whatsoever.** Zero is legal, all-zero is legal, all-identical is legal. The published description is explicit that the combined state is the base state crossed with the full k-tuple product of r-bit values, and that the output is the selected table word XORed with the base output — so every table is a reachable state. Verified by probe: an all-zero table with zero state and zero stream gave **99998 distinct values in 100000 draws** (birthday expectation ~1.2 collisions); an all-0xFFFFFFFF table with all-ones state and stream gave **100000 distinct**.

**One thing that looks like a constraint and is not:** the reference's table-advance propagates a carry to the next word when a stepped word's new value is exactly 0. Zero is the odometer's carry position — a state the generator reaches on its own and handles. A word seeded to 0 simply starts at a carry boundary.

**The one genuinely load-bearing rule is a schedule, not a value.** From the PCG paper §7.1: *"The only requirement is that we advance the state of the extension array every time the primary generator crosses zero so that over the full period of the combined generator every possible extension array state is married with every possible base-generator state."* The reference advances far more often than the minimum — for this variant, every 2^16 base steps, stepping every word on each advance. That belongs to the algorithm half of the spec, not the seeding half, but the seeding section must not be read as covering it.

Sources: [PCG streams/increments](https://www.pcg-random.org/posts/critiquing-pcg-streams.html), [the PCG paper §7.1](https://www.cs.hmc.edu/tr/hmc-cs-2014-0905.pdf).
   => The seeding spec needs exactly two sentences of constraint: force the increment odd (draw 63 bits, or draw 64 and set bit 0 yourself), and treat the 16384 table words plus the 64-bit base state as unconstrained raw bytes with no validation, no rejection, and no reseed-on-zero. Any "must be non-zero" check would be cargo cult and would bias the table.

-[nice-to-know] The stream seed is 63 bits, not 64 — the top bit is discarded
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:308
   The reference builds the increment by shifting the 64-bit stream seed left one and setting the low bit, so the seed's top bit falls off the end. Verified directly: a generator seeded with stream 0 and one seeded with stream 2^63 compare **equal**, and `pcg32::streams_pow2()` reports 63.

Combined with the four fully discarded words, the current path draws 524544 bits from the source and installs 524415 (64 state + 63 stream + 524288 table). DESIGN.md:3051's "on the order of half a million bits" describes the installed figure and is correct either way.
   => State in the spec which convention the new code uses — draw a 63-bit stream value and set the low bit, or draw 64 bits and accept one wasted bit. Either is fine; saying which keeps the entropy accounting behind DESIGN.md:3051 checkable by anyone who wants to re-derive it.

-[nice-to-know] Which entropy word becomes state and which becomes stream is compiler-dependent, so there is nothing to be bug-compatible with
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:488-497
   The reference passes the two base-generator seeds as the two arguments of one constructor call, and each argument independently pulls four words from the sequence. C++ leaves the order of function-argument evaluation **unspecified**, so which four-word batch becomes the state and which becomes the stream is a property of the compiler, not of the algorithm.

Under this project's compiler (clang-24) the first batch supplies the state (words 2-3) and the second the stream (words 4-5) — verified by the scripted-word probe. A compiler evaluating right-to-left would swap them.

This is harmless for randomness, since all the words are i.i.d. But it means the vendored code has **no reproducible entropy-to-field mapping across toolchains**, so there is no seeding behaviour a from-scratch implementation is obliged to match.
   => Assign the fields explicitly in a defined order and say so in the spec. It is one sentence, and it retires a genuine (if benign) unspecified-behaviour dependency at the same time as the licence problem.

-[nice-to-know] seed_seq_from is a pass-through, not a seed sequence — the replacement is smaller than it looks
   anchor: pcg-cpp-0.98/include/pcg_extras.hpp:557-585
   The reference's adaptor does **not** implement the standard's SeedSequence initialisation algorithm. Its `generate()` fills the destination range with successive raw outputs of the wrapped engine, one 32-bit value per element, and its `size()` returns the wrapped engine's `max()` — measured as **4294967295**, which is not a count of anything and would be nonsense to a conforming consumer. It is a pass-through, not a mixer.

Practical consequence for the rewrite: nothing has to reimplement `std::seed_seq` semantics, and nothing should. The entirety of what the adaptor does is *"fill this buffer with fresh 32-bit values from the source"*, which a bulk `getrandom` loop does directly and better — in one call rather than a word at a time.

A layout detail on the same object: the adaptor holds the source engine **by value**, so one `std::random_device` is constructed and destroyed per `spun_seed` call at src/random_numbers/random.cpp:94. On a machine where the default token resolves to `/dev/urandom` that is an `open`/`close` pair plus 16392 four-byte `read()` calls per `satellite.random` call (16396 reads counted under `strace -c`).
   => Specify a single free function of the shape `bool fill_entropy(void *buf, size_t len)` and have the generator's constructor take a filled 65552-byte buffer. No seed-sequence concept, no template plumbing, no adaptor type — which is also the smallest possible surface to write from scratch and defend as independent work.

-[nice-to-know] 64 KB of generator lives on the stack, once at a time
   anchor: src/random_numbers/random.cpp:144
   `sizeof(pcg32_k16384)` measured at **65552 bytes**. src/random_numbers/random.cpp:95 puts one on `spun_seed`'s frame; src/random_numbers/random.cpp:144 puts another inside the `std::optional` member of `Draws`, which is itself a stack local at random.cpp:178 and :189.

The comment at random.cpp:69-71 is correct that only one is live at a time — `spun_seed`'s generator is destroyed when it returns the seed, before the second is emplaced — so the peak is ~64 KB of stack, not 128 KB, against an 8 MB default. Fine as it stands, but it is inherited rather than chosen.
   => Keep the one-at-a-time property and say in the spec that it is deliberate. If the entropy is drawn straight into the table's storage there is no separate 64 KB staging buffer, so the getrandom move does not add to the peak — worth stating explicitly so nobody later adds a `std::vector<uint8_t>` intermediate and doubles it.

##### aa7ef51efb78b5819
pcg32_k16384 is fully specified below and a from-scratch implementation CAN be made bit-exact. I read the vendored headers to derive the algorithm, then wrote an independent implementation from that derivation and ran it against the reference: it matches bit-for-bit on every path satellite uses (base pcg32 over 1e6 draws; the 32-bit table-word permutation and its carry flag over 2e5 random cases plus a constructed wrap-to-zero case; the full generator over 4e5 draws spanning 6 table advances, on both the 64-bit-seed/self-init path and the std::seed_seq path; the bounded draw; the default-ctor seed; and the kdd=false c-variant as a cross-check of the index/tick rules). Probes are in /tmp/claude-1000/-home-madness-code-cxx-satellite/52d0a23c-9839-4b74-b2ec-a2e9ba2707ff/scratchpad/{probe,probe2,probe3,cnt}.cpp. Headline shape: a 64-bit LCG (mult 6364136223846793005, odd increment) whose XSH-RR permutation gives a 32-bit base output; that output is XORed with one word of a 16384-entry uint32 table; the table word is chosen by the LOW 14 bits of the pre-step LCG state; the whole table is advanced by an odometer of 16384 independent 32-bit RXS-M-XS generators exactly once per 65536 draws, keyed on the low 16 bits of the state being zero. Period 2^524352, 16384-dimensional equidistribution. Two unspecified-evaluation-order warts in the reference are called out, and DESIGN.md:3190's discard() bug is explained (extended does not override discard, so discard steps the base LCG past table advances without performing them) and confirmed by measurement. Web access worked; citations to pcg-random.org and Wikipedia are real, and I note where the paper PDF could not be text-extracted.

-[important] Template decomposition of pcg32_k16384, confirmed from the header
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:1748
   pcg-cpp-0.98/include/pcg_random.hpp:1748 defines `pcg32_k16384` as `pcg_engines::ext_setseq_xsh_rr_64_32<14,16,true>`, exactly as the task states.

Unwrapping the aliases:
- pcg_random.hpp:1653 -> `ext_setseq_xsh_rr_64_32<T,A,K>` = `ext_std32<T, A, setseq_xsh_rr_64_32, K>`
- pcg_random.hpp:1631 -> `ext_std32<T,A,B,K>` = `extended<T, A, B, oneseq_rxs_m_xs_32_32, K>`
- pcg_random.hpp:1552 -> `setseq_xsh_rr_64_32` = `setseq_base<uint32_t, uint64_t, xsh_rr_mixin>`, i.e. `engine<uint32_t, uint64_t, xsh_rr_mixin<uint32_t,uint64_t>, output_previous = (sizeof(uint64_t) <= 8) = TRUE, specific_stream<uint64_t>>`
- pcg_random.hpp:1565 -> `oneseq_rxs_m_xs_32_32` = `oneseq_base<uint32_t, uint32_t, rxs_m_xs_mixin>` (fixed increment, no stream selector)

So the five parameters of `extended<...>` (declared at pcg_random.hpp:1141) resolve to:
1. `table_pow2 = 14`  -> table of 2^14 = 16384 entries
2. `advance_pow2 = 16` -> table advanced once per 2^16 base outputs
3. `baseclass = setseq_xsh_rr_64_32` (that is plain `pcg32`)
4. `extvalclass = oneseq_rxs_m_xs_32_32` (the generator each TABLE WORD is a member of)
5. `kdd = true`  -> "keep it dimensionally distributed": low-bit indexing and phase-locked ticking

Measured footprint: `sizeof(pcg32_k16384) == 65552` bytes = 65536 (table) + 8 (LCG state) + 8 (LCG increment). `pcg32_k16384::period_pow2() == 524352`.

Note there is a SECOND consumer in the tree besides src/random_numbers/random.cpp: pcg_test/pcg_test.cpp:9-10 includes the headers by relative path. Any replacement has to cover it, and Makefile:109 (`PCGFLAGS = -isystem pcg-cpp-0.98/include`), Makefile:434 and Makefile:478 go away with it.

Licensing motivation confirmed: LICENSE line 1 is "MIT License (Expat)", pcg_random.hpp:6 is "Licensed under the Apache License, Version 2.0", and pcg-cpp-0.98/LICENSE.txt is the only other licence file in the tree.
   => Implement `extended<14,16,pcg32,rxs_m_xs_32_32,kdd=true>` as one concrete non-templated class. None of pcg-cpp's genericity is used by satellite, so the from-scratch version should be a single struct with hard-coded constants — that is both simpler and unambiguously an independent expression. Delete PCGFLAGS from the Makefile and port pcg_test/pcg_test.cpp at the same time.

-[important] The base generator: 64-bit LCG, stream selection, and why the increment must be odd
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:288
   State is a single `uint64_t`. The recurrence, all arithmetic mod 2^64 (natural uint64 wraparound):

    cursor <- cursor * BASE_MULT + step_add

with BASE_MULT = 6364136223846793005 (pcg_random.hpp:146) and step_add the per-stream additive constant. Wikipedia's PCG article gives the same multiplier for pcg32 and the same increment example 1442695040888963407 (https://en.wikipedia.org/wiki/Permuted_congruential_generator).

STREAM SELECTION (`specific_stream`, pcg_random.hpp:288-330). The generator stores the increment, not the stream number. A stream selector `k` becomes

    step_add = (k << 1) | 1

and the inverse (`stream()`) is `step_add >> 1`. The top bit of `k` is therefore discarded; there are 2^63 distinct streams (`streams_pow2()` returns 63). When no stream is given, step_add = BASE_INC0 = 1442695040888963407 (pcg_random.hpp:147), which is odd.

WHY ODD. For an LCG modulo 2^n, the Hull-Dobell conditions reduce to: the increment must be coprime to 2^n (i.e. odd), and multiplier - 1 must be divisible by every prime factor of 2^n and by 4 (i.e. multiplier = 1 mod 4). 6364136223846793005 mod 4 = 1, so the multiplier half is satisfied permanently and the only thing a user can get wrong is the increment. An even increment would drop the period below 2^64 and, worse, would break the low-bit structure the extension depends on (see the tick rule finding). `(k << 1) | 1` makes the mistake unrepresentable. pcg-random.org states this directly: "Increments have just one rule: they must be odd. That's it. It doesn't matter at all what the increment is... so long as the increment is odd, there are no bad increments." (https://www.pcg-random.org/posts/critiquing-pcg-streams.html)

A consequence used later: because the multiplier is 1 mod 4 and the increment is odd, the low k bits of `cursor` are themselves a full-period LCG mod 2^k. They cycle through all 2^k values in exactly 2^k steps, for every k.

SEEDING (pcg_random.hpp:458): the constructor does not assign the seed to the state. It computes

    cursor = (seed + step_add) * BASE_MULT + step_add

i.e. one LCG step applied to (seed + increment). This matches the reference C `pcg32_srandom_r` (zero the state, step, add initstate, step). The default seed when none is given is 0xcafef00dd15ea5e5 (verified against my implementation).

OUTPUT ORDERING (`output_previous = true`, pcg_random.hpp:405 and 387-402): a draw returns the permutation of the state BEFORE the step, then steps. This matters for bit-exactness and for the index rule, because the table index is taken from that same pre-step state.
   => Store the increment, not the stream number, and normalise with `(k << 1) | 1` in the one place a stream can enter. Keep the (seed + inc) * mult + inc seeding form verbatim — it is what makes our output match the widely published pcg32 test vectors, which is a free correctness check independent of the vendored code.

-[important] The base output function: XSH-RR, 64 -> 32, at bit level
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:795
   Derived from pcg_random.hpp:795-822 by evaluating its compile-time size arithmetic for xtype=uint32_t, itype=uint64_t:

    bits = 64, xtypebits = 32, sparebits = 32
    wantedopbits = 5 (because xtypebits >= 32)
    opbits = 5 (sparebits 32 >= 5), so amplifier = 0 and the rotation is used unamplified
    mask = 31, topspare = 5, bottomspare = 32 - 5 = 27
    xshift = (topspare + xtypebits) / 2 = (5 + 32) / 2 = 18

Giving, for a 64-bit state s:

    rot  = (uint32)(s >> 59)          # the top 5 bits choose the rotation, 0..31
    mid  = s ^ (s >> 18)              # 64-bit xorshift right by 18
    word = (uint32)(mid >> 27)        # keep bits 27..58 of mid as the 32-bit result
    out  = rotate_right_32(word, rot)

This is exactly the canonical published pcg32: Wikipedia gives count = x >> 59, x ^= x >> 18, (uint32_t)(x >> 27), rotr32 by count (https://en.wikipedia.org/wiki/Permuted_congruential_generator).

Two bit-level notes for the implementer:

1. The xorshift by 18 leaves bits 46..63 untouched (everything shifted in from above is zero there), so reading `rot` before or after the xorshift is equivalent. The reference reads it before; either is bit-exact.

2. The rotate must be written so that rot = 0 does not shift a uint32 by 32 (undefined behaviour). `(word >> rot) | (word << ((32 - rot) & 31))` is correct: at rot = 0 both halves are `word`, and OR gives `word`.

The design intent, per O'Neill: the top 5 bits (which are the highest-quality bits of an LCG) are spent choosing a data-dependent rotation, and the 32 output bits are drawn from the middle after an xorshift that folds high entropy downward. That is what breaks the LCG's low-bit-lattice structure.

Verified: 1,000,000 consecutive outputs of my implementation match `pcg32(12345, 67890)` exactly (probe.cpp section 1: "base pcg32 match: YES").
   => Write XSH-RR as a single free function over uint64 -> uint32 with the four constants 59, 18, 27 and the 31-mask written literally. Do not reproduce pcg-cpp's compile-time sparebits/topspare derivation; it exists only to serve sizes satellite never instantiates, and copying its structure is the part that would look like a transliteration.

-[blocker] The extension table: shape, indexing, XOR combination, and the tick rule
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:1176
   From pcg_random.hpp:1149-1200, evaluated for table_pow2 = 14, advance_pow2 = 16, state_type = uint64_t (stypebits = 64), result_type = uint32_t:

    table_size  = 1 << 14 = 16384 entries
    element type = uint32_t (the base generator's result type)
    table bits  = 16384 * 32 = 524288 bits = 64 KiB exactly
    table_shift = 64 - 14 = 50
    table_mask  = 0x3FFF
    may_tick    = (16 < 64) && (16 < 64) = TRUE
    tick_shift  = 64 - 16 = 48
    tick_mask   = 0xFFFF
    may_tock    = (64 < 64) = FALSE   <-- no "tock" path for this instantiation

INDEX. With kdd = true, `get_extended_value()` (pcg_random.hpp:1176) takes

    index = cursor & 0x3FFF

the LOW 14 bits of the CURRENT (pre-step) LCG state — the same state value the base output is computed from, because output_previous is true. (The MCG low-bit shift at pcg_random.hpp:1179 is dead here: setseq is not an MCG.)

TICK. Still with kdd = true:

    tick when (cursor & 0xFFFF) == 0

Because the low 16 bits of the state are a full-period LCG mod 2^16, this fires exactly once every 65536 draws, at a phase fixed by the seed. Measured: 6 ticks in 400,000 draws, first at draw 47483, spaced exactly 65536 apart (probe.cpp section 6).

ORDER OF OPERATIONS, and it is load-bearing. The index is computed from the state first, then the tick check runs and may call advance_table(), and only THEN is data_[index] read (the function returns a reference, dereferenced by the caller after the advance). So on a tick the caller sees the POST-advance word. This is only observable at index 0 — a tick implies the low 16 bits are zero, hence the low 14 are too — but getting it backwards diverges on the very first tick. My implementation advances then reads, and matches.

COMBINATION (pcg_random.hpp:1206-1211): XOR, not addition.

    result = base_output XOR table[index]

COVERAGE. Within each 65536-draw block the low 14 bits cycle through all 16384 values exactly 4 times, so every table word is XORed into exactly 4 outputs per block, and the table is refreshed between blocks.
   => Implement next_u32() as: snapshot the state; if (state & 0xFFFF) == 0 advance the whole table; read table[state & 0x3FFF]; compute the base output from the snapshot; step the state; return base XOR word. Add a unit test that runs at least 200,000 draws so it crosses at least three ticks — a test of 1000 draws will pass against a wrong tick rule (which is precisely how DESIGN.md:3190's discard() bug hid at n=1000).

-[important] kdd = true versus kdd = false, and what the third parameter buys
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:1179
   pcg_random.hpp:1176-1198 branches on `kdd` in exactly two places, and both branches are about WHICH BITS of the base state drive the extension:

    kdd = true  (the "k" variants, e.g. pcg32_k16384):
        index = state & 0x3FFF                 # LOW 14 bits
        tick  = (state & 0xFFFF) == 0          # LOW 16 bits all zero

    kdd = false (the "c" variants, e.g. pcg32_c1024 at pcg_random.hpp:1734):
        index = state >> 50                    # HIGH 14 bits
        tick  = (state >> 48) == 0             # HIGH 16 bits all zero

I confirmed the kdd = false rules by building `ext_setseq_xsh_rr_64_32<14,16,false>` and matching my own implementation against it for 300,000 draws (probe3.cpp: "kdd=false (index = state>>50, tick = (state>>48)==0): MATCH").

WHY IT MATTERS. The equidistribution proof needs the extension to behave like a positional counter: the index must sweep every slot exactly once per cycle, and the table must advance exactly once per complete sweep. The low bits of an LCG mod 2^64 do exactly that — the low 14 bits are a full-period LCG mod 2^14, and the low 16 bits hit zero exactly once per 65536 steps, so index and tick are perfectly phase-locked. That is what "kdd" abbreviates: keep it dimensionally distributed.

The high bits give up that structure. `state >> 50` does not cycle uniformly over short windows and `(state >> 48) == 0` fires on 2^48 of the 2^64 states scattered irregularly rather than periodically, so a c-variant advances the table at unpredictable moments. It loses the equidistribution guarantee and, per the header comment at pcg_random.hpp:1705-1710, is offered instead as "better cryptographic security" with the parenthetical "(just how good the cryptographic security is is an open question)".

A third consequence: `extended::advance()` (pcg_random.hpp:1518) contains a `static_assert(kdd, "Efficient advance is too hard for non-kdd extension.")`. Only kdd generators can be jumped.

Since satellite makes no cryptographic claim at all (DESIGN.md's own "The tiers are a statistical character, not a security property"), kdd = true is the right and only sensible choice, and our implementation should hard-code it rather than carry the flag.
   => Hard-code the kdd = true behaviour and do not implement the c-variant. If the mechanism is documented, say plainly that the low bits are used BECAUSE they are the ones with clean periodic structure, which is the opposite of the usual advice about LCG low bits and will otherwise read as a bug to the next person.

-[important] advance_pow2 = 16: what the second parameter means
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:1159
   `advance_pow2` sets how often the extension table is advanced, expressed as a power of two of base outputs. It is turned into `tick_mask = (1 << advance_pow2) - 1 = 0xFFFF` at pcg_random.hpp:1161-1166, and the tick fires when the low `advance_pow2` bits of the base state are zero. So advance_pow2 = 16 means: advance the whole table once every 2^16 = 65536 base outputs.

The constraint the value must satisfy is advance_pow2 >= table_pow2. Here 16 >= 14, with two bits of slack: the index sweeps the 16384 slots four times between advances. If advance_pow2 were LESS than table_pow2 the table would be advanced before the index had visited every slot, and the equidistribution argument would collapse.

Why 16 rather than 14 (the tight choice) is a cost/benefit call, not a correctness one: advancing every 2^14 draws would quadruple the amortised cost of the odometer. Note the sibling `pcg32_k16384_fast` at pcg_random.hpp:1749 uses advance_pow2 = 32, i.e. one advance per 4 billion draws — far cheaper, and it is a oneseq/XSH-RS variant.

`may_tick` is true only when advance_pow2 < state bits (64) and < 64; both hold. `may_tock` (pcg_random.hpp:1168) is `state_bits < 64`, which is FALSE for a 64-bit base, so the "tock" path — an extra advance when the whole state hits zero — is compiled out entirely for pcg32_k16384. Do not implement it; it exists for small-state instantiations where may_tick is false and the table would otherwise never advance.

Amortised cost: one advance touches all 16384 slots per 65536 draws, i.e. 0.25 slot-steps per output, each slot-step being one inverse permutation, one 32-bit LCG step and one forward permutation. DESIGN.md measures the resulting throughput at ~153,600 draws/ms bare on the Xeon E5-2670 v3 (DESIGN.md section 18, "Throughput is ~153,600 draws/ms bare").
   => Hard-code 0xFFFF as the tick mask with a comment giving the two facts that pin it: 2^16 outputs between advances, and 16 >= 14 so every slot is visited (four times) between advances. Omit the tock path — for a 64-bit base it is unreachable, and carrying it invites someone to 'fix' it later.

-[blocker] How one table word advances: the inside-out RXS-M-XS stepper, at bit level
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:1108
   Each of the 16384 table entries is not a plain counter. It is the OUTPUT of a distinct 32-bit `oneseq_rxs_m_xs_32_32` generator, and advancing it means: invert the output permutation to recover that generator's state, take one LCG step, re-apply the permutation. pcg-cpp calls this trick `inside_out` (pcg_random.hpp:1100-1121); the static_assert there names the requirement — the output function has to be a permutation of the state, which RXS-M-XS is and XSH-RR is not.

The forward permutation, derived from pcg_random.hpp:896-916 with xtype = itype = uint32_t (so xtypebits = bits = 32, opbits = 4, shift = 0, mask = 15, final xorshift = (2*32+2)/3 = 22):

    function word_forward(uint32 s) -> uint32:
        r = s >> 28                 # top 4 bits choose the shift, 0..15
        s = s ^ (s >> (4 + r))      # random xorshift, 4..19 places
        s = s * 277803737           # mod 2^32   (mcg multiplier, pcg_random.hpp:883)
        return s ^ (s >> 22)        # fixed xorshift

The inverse (pcg_random.hpp:917-936):

    function word_inverse(uint32 v) -> uint32:
        v = undo_xorshift(v, 22)
        v = v * 2897767785          # mod 2^32   (the modular inverse of 277803737, pcg_random.hpp:884)
        r = v >> 28
        return undo_xorshift(v, 4 + r)

where undo_xorshift inverts y = x ^ (x >> s) over 32 bits. A clean iterative form (mine, not pcg-cpp's recursive masking one):

    function undo_xorshift(uint32 y, uint32 s) -> uint32:
        x = y
        repeat 8 times: x = y ^ (x >> s)
        return x

Eight iterations always suffice because s >= 4 here, so ceil(32/4) = 8.

WHY THE INVERSE CAN RECOVER r: the shift is at least 4 places, so `s ^ (s >> (4+r))` never disturbs the top 4 bits. That is the entire reason opbits is 4 and the shift has a +4 floor.

Each slot j has its own increment. From the loop at pcg_random.hpp:1435-1447 the 1-based position i = j+1 is passed into `external_step`, which at pcg_random.hpp:1112 adds i*2 to the base increment. Concretely, for 0-based slot j:

    slot_inc(j) = (2891336453 + 2*(j+1))  mod 2^32

with 2891336453 = default_increment<uint32_t> (pcg_random.hpp:144) and 747796405 = default_multiplier<uint32_t> (pcg_random.hpp:143). Adding an even number keeps it odd, so every slot is a full-period LCG mod 2^32, and every slot is a DIFFERENT stream. So:

    function step_slot(j) -> bool wrapped:
        s = word_inverse(table[j])
        s = s * 747796405 + (2891336453 + 2*(j+1))     # mod 2^32
        table[j] = word_forward(s)
        return table[j] == 0

The carry flag is `new output == 0`. Because RXS-M-XS is a bijection with word_forward(0) = 0, that is exactly "this slot's underlying LCG state reached zero", i.e. this slot completed a full 2^32 cycle. (The `is_mcg` branch at pcg_random.hpp:1116 is dead here — the ext value class is a oneseq LCG, not an MCG, so `zero` is literally 0.)

Verified two ways: 200,000 random (word, slot) pairs match pcg-cpp's `external_step` in both the new word and the carry flag; and a constructed case where the slot's next state is exactly 0 — computed as (-slot_inc) * multiplier^-1 mod 2^32 via Newton iteration — gives carry = true and new word = 0 from both implementations (probe.cpp sections 2 and 3).
   => Store the table as OUTPUT words, exactly as the reference does, not as underlying LCG states. Storing states would need a forward permutation on every read (1 per draw) instead of an inverse+forward on every slot-step (0.25 of each per draw) — four times the work. Test undo_xorshift exhaustively for every shift 4..19 and 22 against a brute-force check; it is the one piece here where a subtly wrong loop bound produces output that still looks random.

-[blocker] The table advance: an odometer where every digit ticks, plus a carry
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:1435
   From pcg_random.hpp:1435-1447. The rule is NOT the obvious one ("increment slot 0, carry into slot 1 on wrap"). Every slot advances on every table advance, and a carry from the slot below causes ONE EXTRA step:

    function advance_table():
        carry = false
        for j = 0 .. 16383:
            if carry:
                carry = step_slot(j)      # the extra step, and it can itself carry
            c = step_slot(j)              # every slot always steps at least once
            carry = carry or c

Read the carry composition carefully: when a carry arrives, the incoming carry variable is OVERWRITTEN by the result of the extra step, and then OR-ed with the result of the mandatory step. So a slot propagates a carry if either of its (one or two) steps landed on zero. A slot with no incoming carry takes exactly one step.

This is what makes the state space traversal cover all 2^(32*16384) table configurations rather than cycling in lockstep with period 2^32: slot j's advance rate is 1 per tick plus the wrap rate of slot j-1, so slot j effectively runs at a rate that grows down the array, giving the positional-counter behaviour the equidistribution proof needs.

Cost: 16384 slot-steps per advance in the common case (carries into slot 0 never happen — there is no slot -1 — and a carry out of slot j requires that slot to have completed 2^32 steps, i.e. 2^32 * 2^16 = 2^48 draws, so in any realistic run advance_table is exactly 16384 step_slot calls). Every 65536 draws.

Because the carry path is unreachable in normal operation, it is untestable end-to-end and must be tested at the unit level. I did exactly that: constructing a word whose next state is zero and checking that both the reference and my implementation report carry = true (probe.cpp section 3, "carry case: ref carry=1 val=0 | mine carry=1 val=0 | OK").
   => Write the loop exactly as above and do not 'simplify' it to a conventional odometer — an odometer that only steps slot 0 is a different generator with a much shorter period. Unit-test the carry by seeding a slot with the word that maps to state (-slot_inc) * mult^-1 mod 2^32, since 2^48 draws is not a test anyone can run.

-[important] Seeding path A: from a seed sequence (the path random.cpp:94-95 uses)
   anchor: src/random_numbers/random.cpp:94
   src/random_numbers/random.cpp:94-95 constructs

    pcg_extras::seed_seq_from<std::random_device> seed_source;
    pcg32_k16384 rng(seed_source);

which hits the seed-sequence constructor at pcg_random.hpp:1286-1290. Tracing what actually happens:

1. `seed_seq_from::generate` (pcg_extras.hpp:557-580) simply fills each destination word with `uint_least32_t(random_device())`. No mixing, no std::seed_seq initialisation. Raw kernel words go straight through.

2. The base class is constructed from the sequence via `engine(generate_one<uint64_t,1,2>(seq), generate_one<uint64_t,0,2>(seq))`. Each `generate_one<uint64_t, i, 2>` runs a SEPARATE 4-word `generate()` call (generate_to computes FROM_ELEMS = 2 * ceil(8/4) = 4), assembles two uint64s little-endian-first (uneven_copy, pcg_extras.hpp:409-435: value |= dest_t(word) << shift, shift += 32), and returns element i. So the seed is the SECOND uint64 of one 4-word draw and the stream is the FIRST uint64 of another 4-word draw. Four of the eight words are discarded.

3. The table is filled by `generate_to<16384>(seq, data_)`, and because the destination is exactly uint32 wide this takes the direct path (pcg_extras.hpp:494-501 -> pcg_extras.hpp:458-462): ONE `generate()` call producing 16384 words written verbatim into the table.

Measured with a counting stand-in for std::random_device: construction consumes exactly 16392 device words = 524,544 bits. 16384 of them become the table untouched. That is DESIGN.md section 18's "on the order of half a million bits" made exact — it is 524,288 bits of kernel entropy in the table plus 64 in the seed plus 63 effective in the stream.

So the model is:

    function seed_from_sequence(seq):
        seed     = next 64-bit word from seq
        stream   = next 64-bit word from seq
        step_add = (stream << 1) | 1
        cursor   = (seed + step_add) * BASE_MULT + step_add
        for j = 0..16383: table[j] = next 32-bit word from seq

No self-init runs on this path, and no base draws are consumed.

PORTABILITY WART, and it is a real one. The two `generate_one` calls are arguments to the same constructor, and C++ leaves their relative evaluation order unspecified. I measured it: under clang-24 the SEED argument is drawn first (device words 3-4 become the seed, words 5-6 become the stream); under gcc-17 the STREAM argument is drawn first (words 1-2 become the stream, words 7-8 become the seed). Both compile the same source to different bit-level seedings. With std::random_device this is statistically invisible, and with a real std::seed_seq it is invisible too (generate is a pure function of the seed data, so both calls return the same 4 words and both compilers land on the same seed/stream) — but it means "bit-exact against pcg-cpp on the random_device path" is a compiler-dependent statement.
   => Take the seed material as an explicit, ordered stream in our implementation — read seed, then stream, then the table, in one documented order — so the result is compiler-independent. Do not reproduce pcg-cpp's generate_one/uneven_copy machinery: satellite only ever feeds it std::random_device, so the honest replacement is a small function that draws 2 uint64s and 16384 uint32s from a device in a fixed order. Note in DESIGN.md that this drops the 4 wasted device words pcg-cpp discards.

-[important] Seeding path B: from a single 64-bit value, and the self-init table fill (random.cpp:144)
   anchor: src/random_numbers/random.cpp:144
   src/random_numbers/random.cpp:144 keeps a `std::optional<pcg32_k16384>` that is filled at random.cpp:129 with `rng_.emplace(spun_seed(tier_))` — a `uint64_t`. That reaches the single-argument constructor at pcg_random.hpp:1261, which calls `selfinit()` (pcg_random.hpp:1335-1351).

Two things this path does that path A does not:

1. NO STREAM IS SET. The one-argument constructor initialises only the base state, so the increment stays at its default member value, BASE_INC0 = 1442695040888963407. Every generator satellite creates on this path is on the same stream.

2. THE TABLE IS FILLED FROM THE BASE GENERATOR, not from entropy:

        xdiff = first_base_draw - second_base_draw     # wrapping uint32 subtraction
        for j = 0..16383: table[j] = base_draw() XOR xdiff

   Those 16386 draws call the BASE generator directly, bypassing the extension, so no table advance runs during seeding even if the state's low 16 bits pass zero. My implementation reproduces this and matches for 400,000 subsequent draws (probe.cpp section 4).

   The header's own comment (pcg_random.hpp:1337-1345) is candid that this is a fallback — "we have very little provided data... (use a seed sequence, folks!)" — and lists the mitigations: XOR differences rather than raw values, and the fact that the table is not read back in the order it was written.

This is the mechanism behind DESIGN.md section 18's recorded criticism. The first generator holds ~524,288 bits of kernel entropy; steps 3-5 funnel all of it through a single 32-bit accumulator (random.cpp:112), and the second generator's entire 64 KiB table is then a deterministic function of those ~30 measured bits.

SECOND PORTABILITY WART: `xdiff` is computed as `baseclass::operator()() - baseclass::operator()()` on one line. For the built-in binary minus, C++ leaves the operands unsequenced, so which draw is the minuend is unspecified. I tested both clang-24 and gcc-17 at -O0 and -O2: all four evaluate the LEFT operand first, giving xdiff = first - second. That is what my implementation uses and what matches. But it is still unspecified behaviour sitting in the seeding of a generator, and it should not be inherited.
   => Reproduce the self-init rule exactly (including xdiff = first - second) if bit-exactness against the vendored generator is wanted as a correctness test, but write the two draws as separate named statements so the order is ours and not the compiler's. Separately: this path is where DESIGN.md section 18's 'answering straight from the first generator is stronger than any fold' recommendation bites — the rewrite is a natural moment to revisit it, since deleting steps 3-5 would remove the need for the self-init path altogether.

-[nice-to-know] Period and equidistribution: what k16384 buys over plain pcg32
   anchor: pcg-cpp-0.98/include/pcg_random.hpp:1201
   PLAIN pcg32 (setseq_xsh_rr_64_32): state is 64 bits, period 2^64 per stream, with 2^63 streams. It is 1-dimensionally equidistributed at 32 bits — over one full period each of the 2^32 possible outputs appears exactly 2^32 times. It is NOT 2-dimensionally equidistributed: the pair (x, x) can occur at most... more usefully, the 64-bit state cannot encode all 2^64 successive pairs plus a position, so successive-pair coverage is not guaranteed.

pcg32_k16384: `period_pow2()` (pcg_random.hpp:1201-1204) returns base_period_pow2 + table_size * ext_period_pow2 = 64 + 16384*32 = 64 + 524288 = 524352. I confirmed the value at runtime: `pcg32_k16384::period_pow2() == 524352`. So the period is

    2^524352  (roughly 10^157,826)

and the total state is 65552 bytes (measured sizeof), of which 65536 is the table.

EQUIDISTRIBUTION: 16384-dimensional at 32 bits. Every possible sequence of 16384 consecutive 32-bit outputs occurs over the full period, each exactly 2^64 times. The header states this in its own idiom at pcg_random.hpp:1743-1746: "These generators have an insanely huge period (2^524352)... suitable for silly party tricks, such as dumping out 64 KB ZIP files at an arbitrary point in the future. [Actually, over the full period of the generator, it will produce every 64 KB ZIP file 2^64 times!]" — 16384 words x 4 bytes is exactly 64 KiB, so that sentence IS the equidistribution claim.

The arithmetic is consistent: 2^524288 distinct 16384-tuples x 2^64 occurrences each = 2^524352 = the period.

Published corroboration for the family: pcg-random.org's using-pcg-cpp page describes pcg32_k64 as a "32-bit 64-dimensionally equidistributed generator, 2^2112 period" and pcg32_k1024 with "2^32832 period" (https://www.pcg-random.org/using-pcg-cpp.html). Both fit 2^(64 + 32k) exactly: 64 + 64*32 = 2112 and 64 + 1024*32 = 32832. The general statement of the scheme, from the PCG paper's summary: a k-dimensionally equidistributed generator can be built for any k, and if the base period is 2^b the combined period is 2^(kb) — see https://www.pcg-random.org/paper.html and the paper PDF at https://www.pcg-random.org/pdf/hmc-cs-2014-0905.pdf.

HONEST NOTE ON SOURCING: WebSearch and WebFetch both work in this environment, and the pcg-random.org and Wikipedia quotations above are real fetches. I could NOT extract usable text from the paper PDF itself — the fetch returned corrupted/compressed text and no readable section on extended generators — so I have cited the paper only for the general k-dimensional claim that the summary pages restate, and every bit-level claim in this specification comes from reading the vendored header plus my own measurements, not from the paper.

WHAT IT BUYS SATELLITE, honestly: nothing a satellite program can observe. DESIGN.md section 18 already says the tiers differ in "nothing a program can see except how long they take", and the 40-digit draws satellite makes consume ~5 words at a time. The 16384-dimensional guarantee is a property of the full 2^524352 period. The real reasons to keep the same generator character are (a) the seeding path pours 524,288 bits of kernel entropy into a table where they are all reachable, and (b) DESIGN.md specifies this generator, so changing it silently would change the specification.
   => State the period as 2^524352 and the equidistribution as 16384-dimensional in the new module's header comment, with the derivation (64 + 16384*32) alongside so nobody has to trust the number. Do not restate any cryptographic claim: DESIGN.md's refusal to use the word 'secure' should carry over verbatim to the from-scratch implementation.

-[blocker] DESIGN.md:3190 — what pcg-cpp's discard() actually gets wrong, and what to do instead
   anchor: DESIGN.md:3190
   DESIGN.md:3190-3193 records: "pcg-cpp's discard() is wrong for extended generators. Measured: pcg32_k16384 matches n individual draws at n=10 and n=1000 and diverges at n=100,000 and above; pcg32 matches at every n tested. Nothing here uses discard(), and nothing should on a k-variant."

THE CAUSE, from the headers. `extended` (pcg_random.hpp:1142) declares its own `advance(state_type distance, bool forwards)` (pcg_random.hpp:1226) which correctly advances both the base LCG and the table — it computes how many ticks the jump crosses and calls the bulk `advance_table(ticks, forwards)` at pcg_random.hpp:1450. But `extended` NEVER OVERRIDES `discard`. `discard` is inherited from `engine` (pcg_random.hpp:441-444), whose body is `advance(delta)`. That name resolves at definition time, inside `engine`'s scope, to `engine::advance` — the base-only, non-virtual one. The derived class's `advance` is invisible to it.

So `rng.discard(n)` on a pcg32_k16384 jumps the 64-bit LCG state forward by n and leaves the 16384-word table exactly where it was. Every table advance the jump skipped over is simply lost.

THAT EXPLAINS THE MEASUREMENT PRECISELY. A table advance fires once per 65536 draws. If the jump does not cross a tick, discard() happens to be right; if it crosses one, it is wrong. n=10 and n=1000 are far below 65536 and so pass with probability about 1 - n/65536 — n=1000 had roughly a 1.5% chance of failing and did not. n=100,000 exceeds 65536 and must cross at least one tick, so it always fails. Plain pcg32 has no table and is correct at every n.

I reproduced and sharpened it (probe2.cpp). With seed 42:

    n=1, 10, 1000        discard: state SAME, outputs SAME   | advance: SAME, SAME
    n=65535              discard: state DIFF, outputs DIFF   | advance: SAME, SAME
    n=65536              discard: state DIFF, outputs DIFF   | advance: SAME, SAME
    n=100000, 300000     discard: state DIFF, outputs DIFF   | advance: SAME, SAME

    first n where discard(n) diverges (seed 42) = 55963

Two things worth recording that DESIGN.md does not yet say. First, the threshold is NOT 65536 — it is wherever the seed's tick phase happens to fall, which for seed 42 is 55963. So the boundary is seed-dependent and a test that probes n = 65536 will not reliably characterise it. Second, `extended::advance(n)` IS correct at every n I tested, including 65535, 65536, 100000 and 300000 (identical state and identical next three outputs). The bug is confined to the inherited `discard` name.

This is a genuine defect in the vendored library, not a design decision, and it is the kind of thing that silently produces a different-but-still-random stream — exactly the failure mode DESIGN.md elsewhere calls 'invisible to any test that only checks the range'.
   => In the from-scratch implementation, either (a) give the class exactly one jump entry point that always advances both the LCG and the table, and no second spelling of it, or (b) provide no jump at all — satellite uses none, and 'not implemented' cannot be silently wrong. Do NOT reproduce a discard/advance pair. Then update DESIGN.md:3190 to say what was wrong (discard resolves to the base class's advance, which never touches the extension table), add that the divergence threshold is the seed's tick phase rather than a fixed 65536 (55963 for seed 42), and record that extended::advance() was correct — the current text reads as if the whole jump facility were broken.

-[blocker] The complete pseudocode, in our own structure — the implementation artifact
   anchor: src/random_numbers/random.cpp:102
   This is the artifact to implement from. All arithmetic is unsigned and wraps at the stated width. Naming is deliberately ours.

CONSTANTS
    BASE_MULT   = 6364136223846793005      # uint64
    BASE_INC0   = 1442695040888963407      # uint64, the default stream, odd
    WORD_MULT   = 747796405                # uint32
    WORD_INC0   = 2891336453               # uint32, odd
    SCRAMBLE    = 277803737                # uint32, odd
    UNSCRAMBLE  = 2897767785               # uint32, SCRAMBLE^-1 mod 2^32
    TABLE_LEN   = 16384
    INDEX_MASK  = 0x3FFF                   # low 14 bits
    TICK_MASK   = 0xFFFF                   # low 16 bits

STATE
    uint64 cursor        # the LCG state
    uint64 step_add      # the LCG increment, always odd
    uint32 table[16384]  # 64 KiB

--- base output permutation, XSH-RR 64 -> 32 ---
    function scramble64to32(uint64 s) -> uint32:
        rot  = uint32(s >> 59)                          # top 5 bits pick the rotation
        mid  = s XOR (s >> 18)
        word = uint32(mid >> 27)                        # bits 27..58
        return (word >> rot) OR (word << ((32 - rot) AND 31))

--- one base step; OUTPUT FIRST, then advance ---
    function base_draw() -> uint32:
        out = scramble64to32(cursor)
        cursor = cursor * BASE_MULT + step_add
        return out

--- the 32-bit bijection each table word lives in (RXS-M-XS) ---
    function undo_xorshift(uint32 y, uint32 s) -> uint32:     # inverts y = x XOR (x >> s), s >= 4
        x = y
        repeat 8 times: x = y XOR (x >> s)
        return x

    function word_forward(uint32 s) -> uint32:
        r = s >> 28                                     # top 4 bits pick the shift
        s = s XOR (s >> (4 + r))
        s = s * SCRAMBLE
        return s XOR (s >> 22)

    function word_inverse(uint32 v) -> uint32:
        v = undo_xorshift(v, 22)
        v = v * UNSCRAMBLE
        r = v >> 28
        return undo_xorshift(v, 4 + r)

--- advance one slot; report whether it wrapped ---
    function step_slot(j) -> bool:
        s = word_inverse(table[j])
        s = s * WORD_MULT + (WORD_INC0 + 2*(j+1))       # uint32; increment stays odd
        table[j] = word_forward(s)
        return table[j] == 0

--- advance the whole table: every slot steps, a carry adds one extra step ---
    function advance_table():
        carry = false
        for j = 0 .. 16383:
            if carry: carry = step_slot(j)
            c = step_slot(j)
            carry = carry OR c

--- the draw ---
    function next_u32() -> uint32:
        s = cursor                                      # snapshot BEFORE stepping
        if (s AND TICK_MASK) == 0: advance_table()      # once per 65536 draws
        mix = table[s AND INDEX_MASK]                   # read AFTER the advance
        return base_draw() XOR mix

--- uniform on [0, bound), unbiased; needed by random.cpp:102 and :104 ---
    function next_below(uint32 bound) -> uint32:
        threshold = uint32(0 - bound) MOD bound         # = 2^32 mod bound
        loop forever:
            r = next_u32()
            if r >= threshold: return r MOD bound

--- seeding A: from an entropy source ---
    function seed_from_entropy(source):
        seed     = 64 bits from source
        stream   = 64 bits from source
        step_add = (stream << 1) OR 1
        cursor   = (seed + step_add) * BASE_MULT + step_add
        for j = 0 .. 16383: table[j] = 32 bits from source

--- seeding B: from one 64-bit value (self-init) ---
    function seed_from_u64(uint64 seed):
        step_add = BASE_INC0
        cursor   = (seed + step_add) * BASE_MULT + step_add
        first  = base_draw()
        second = base_draw()
        xdiff  = uint32(first - second)
        for j = 0 .. 16383: table[j] = base_draw() XOR xdiff
        # 16386 base draws, none of which tick the table

The bounded draw above is the exact behaviour of pcg_extras.hpp:517-529 specialised to min = 0 and max = 2^32-1: its `(max - min + 1 - bound) % bound` evaluates in uint32 to (2^32 - bound) % bound = 2^32 mod bound. I verified equivalence over 200,000 bounded draws against the reference (probe3.cpp: "bounded_rand equivalence: MATCH"). This matters because random.cpp:102 and random.cpp:104 both call `rng(uint32_t)`, and DESIGN.md section 18 is explicit that modulo alone skews 2:1 in the measured buckets.
   => Implement exactly this. Keep the four literal masks (59/18/27/31 for XSH-RR, 28/22/4 for RXS-M-XS, 0x3FFF, 0xFFFF) as named constants with the derivation in a comment, because none of them is guessable from the others. Do not omit next_below — random.cpp:102 and :104 depend on it and a plain modulo there would reintroduce the exact bias DESIGN.md measured.

-[important] Bit-exactness is achievable and already demonstrated — use it as the correctness test
   anchor: src/random_numbers/random.cpp:95
   YES. A from-scratch implementation can be made bit-for-bit identical to pcg32_k16384 for the same seed, and I have already built one and proved it. Results (all from the scratchpad probes, built with /home/madness/opt/clang-24/bin/clang++ -std=c++20 -O2 against the vendored headers):

    base pcg32, 1,000,000 draws, seed (12345, 67890)         MATCH
    word permutation + carry, 200,000 random cases            MATCH
    constructed wrap-to-zero carry case                       MATCH (carry set, word 0, both sides)
    full k16384, 64-bit seed 0x0123456789abcdef, 400,000      MATCH  (6 table advances crossed)
    full k16384, std::seed_seq{1,2,3,4}, 400,000              MATCH
    full k16384, seed 0xDEADBEEF with stream 0x1234, 300,000   MATCH
    default-constructed (seed 0xcafef00dd15ea5e5), 100,000     MATCH
    kdd=false c-variant cross-check, 300,000                   MATCH
    bounded draws, 200,000 with three different bounds          MATCH

WHAT HAS TO MATCH, exhaustively:

1. BASE_MULT = 6364136223846793005 and the increment normalisation (stream << 1) | 1, default 1442695040888963407.
2. Seeding form cursor = (seed + step_add) * BASE_MULT + step_add — NOT cursor = seed.
3. Output-previous ordering: the returned value is the permutation of the state BEFORE the step, and the table index comes from that SAME pre-step state.
4. XSH-RR exactly: rot from bits 59..63, xorshift right 18, take bits 27..58, rotate right.
5. Index = low 14 bits; tick when low 16 bits are zero; ADVANCE THEN READ (only observable at index 0, but fatal if reversed).
6. Combination is XOR.
7. Slot stepper: word_inverse -> LCG(747796405, 2891336453 + 2*(j+1)) -> word_forward, with carry = (new word == 0), and the RXS-M-XS constants 28 / 4 / 277803737 / 22 forward and 22 / 2897767785 / 28 / 4 inverse.
8. The odometer: every slot steps once, plus one extra step on an incoming carry, carry composed with OR.
9. Table fill: either the entropy words verbatim (path A) or self-init with xdiff = first_draw - second_draw and 16386 base draws that bypass the extension (path B).
10. Bounded draw: threshold = 2^32 mod bound, reject below threshold, return r mod bound.

TWO CAVEATS, both about the REFERENCE and not about us:

(a) The self-init `xdiff` is computed from two calls that C++ leaves unsequenced. clang-24 and gcc-17 both evaluate left-first here (giving first - second), so the match holds on this machine with both compilers, but it is unspecified behaviour and a future compiler could disagree.

(b) The seed-sequence constructor's seed and stream arguments are also unsequenced, and clang-24 and gcc-17 DO disagree: with a counting entropy source, clang maps device words 3-4 to the seed and 5-6 to the stream, gcc maps words 1-2 to the stream and 7-8 to the seed. This is invisible for a real std::seed_seq (both generate calls return the same array) and statistically irrelevant for std::random_device, but it means bit-exactness on the random_device path can only be asserted per-compiler.

Neither caveat threatens the plan: paths (a) and (b) only affect how seed material is mapped in, and the tests that matter — the 64-bit-seed path and the std::seed_seq path — are deterministic and compiler-stable.
   => Land the correctness test as a temporary test target that links BOTH the new implementation and the vendored headers and asserts equality over at least 400,000 draws from a fixed 64-bit seed and a fixed std::seed_seq — enough to cross six table advances. Keep pcg-cpp in the tree until that test is green, then delete the vendored directory, PCGFLAGS (Makefile:109, :434, :478), and the test target in one commit, replacing the test with a golden-vector test (a few hundred outputs at known offsets, including offsets either side of the first tick) so the property survives with no Apache-licensed code present. Also update DESIGN.md section 18's '-isystem, not -I' note (DESIGN.md around :3185) — that whole rationale disappears with the vendored headers, and the from-scratch-rebuild-is-silent property then just holds.

