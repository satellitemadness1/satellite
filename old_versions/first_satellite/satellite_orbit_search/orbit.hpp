#pragma once

// Satellite Orbit — the five phases, and the state they settle into.
//
// LAYER FOUR. plans/search_power.txt built three and search.hpp names them:
// the comparator scores two values, the walker finds them, the application
// spells them as `lm[...]` and `.search()`. Orbit sits above all three and adds
// nothing beneath them — every phase here scores through search_score and finds
// through search_walk, and not one of them opens a Value and compares it
// itself. The ladder is already the language's answer to "how alike are two
// things"; a phase with a second answer would make the dial mean two things.
//
// FREE OF THE Evaluator CLASS, on the contract search.hpp opens with. No
// fail(), no Span, no session state, so the whole engine is testable without
// building a program — which is what orbit_test does.
//
// Phase 4's file lives next door in orbit_memory.hpp, included here so that a
// translation unit still reaches the whole of orbit through one path. The seam
// is real: what orbit REMEMBERS is a format on disk with its own compatibility
// promise, and what orbit DOES is five phases over values in memory.
//
// satellite_orbit_search/orbit_plan.txt carries the decisions and the reasons.
// satellite_orbit_search/orbit.txt is the intent it was written from.

#include "evaluator/search.hpp"
#include "satellite_orbit_search/orbit_memory.hpp"

#include <string>
#include <vector>

namespace satellite {

// The five, in the order they run. orbit.txt: direct, guess, predict, index,
// frequency — "and that is the seaRCH pipeline built for the AI".
//
// THEY ACCUMULATE. Each is handed everything found so far and appends to it;
// none of them stops the next one. That is DECISION 2 and it is not a
// preference: frequency weighs "the likelihood of what we have made from the
// previous phases", and a pipeline that exited at the first hit would hand it
// one unopposed candidate it could only ever rate 100%.
enum OrbitPhase : int {
    ORBIT_DIRECT    = 1,  // it is in there, at this path
    ORBIT_GUESS     = 2,  // the dial run upward on the query's behalf
    ORBIT_PREDICT   = 3,  // logic (the one-hop link) and force (perturbation)
    ORBIT_INDEX     = 4,  // memory on disk: have we resolved this before
    ORBIT_FREQUENCY = 5,  // how likely is any of it, and what else could it be
};

inline constexpr int ORBIT_FIRST_PHASE = ORBIT_DIRECT;
inline constexpr int ORBIT_LAST_PHASE  = ORBIT_FREQUENCY;

// The phase's name as the language spells it. One table, because the name goes
// into a result the user reads AND into the memory file on disk, and two
// spellings of "predict" would make yesterday's records unreadable.
const char *orbit_phase_name(OrbitPhase phase);

// A ladder level as a word: 1 is "exact", 8 is "one typo". The `why` sentence
// on a finding is read by a person, and "matched at level 8" is a number where
// "one typo away" is the fact the number stands for.
//
// Orbit's own rather than search.hpp's, because the ladder's SPELLING is a
// property of how orbit explains itself and the ladder is not orbit's to
// change. search.hpp keeps the levels; this keeps the words for them.
const char *orbit_level_name(int score);

// One thing a phase concluded.
//
// EVERY FINDING CARRIES ITS PHASE AND ITS REASON. DECISION 6, and it is the
// condition under which this feature is allowed to exist in this language at
// all: a phase-1 finding is a fact about the corpus, and a phase-3 FORCE
// finding is something the engine made up. A user who cannot tell those apart
// has an engine acting on them without saying so.
//
// So there is no default constructor. A Finding is built with a phase and a
// sentence or it does not compile.
struct Finding {
    Finding(ValuePtr value_in, ValuePtr key_in, List path_in, int score_in,
            OrbitPhase phase_in, std::string why_in)
        : value(std::move(value_in)), key(std::move(key_in)),
          path(std::move(path_in)), score(score_in), phase(phase_in),
          why(std::move(why_in))
    {
    }

    // What was found. For a map entry this is the entry's VALUE even when the
    // key matched, which is search_walk's rule and is kept here so a hit means
    // the same thing whichever layer produced it.
    ValuePtr value;

    // The map key it sat under, or null for a list element or the root.
    ValuePtr key;

    // Indices and keys from the root. An ordinary satellite list, so a result
    // hands it straight back — and so a result can be searched by the same
    // power that produced it.
    //
    // EMPTY AND MEANINGLESS for a constructed finding: phase 3's force half
    // proposes a value that is not in the corpus, so there is no path to it.
    // `constructed` is how a reader tells that empty apart from the root's.
    List path;

    // The ladder level it matched at, 1..10, or SEARCH_NO_MATCH when the phase
    // reached its conclusion by something other than comparing two values —
    // which only the constructed findings do.
    int score;

    OrbitPhase phase;

    // One sentence, in words, for the user. Not a debugging aid: it is the
    // difference between "it was at [2, name]" and "nothing matched, so this
    // was proposed by changing one element of what you asked for".
    std::string why;

    // NOTHING IN THE CORPUS SAID THIS. Set by phase 3's force half and by
    // nothing else. A result that is `constructed` may still be the right
    // answer — orbit.txt's own example asks for exactly that — but it is an
    // answer of a different kind and the type says so.
    bool constructed = false;

    // WHAT EACH OF THE FIVE CONCLUDED ABOUT THIS ANSWER. said[p] is phase p's
    // own sentence, empty where p never reached it; slot 0 is unused so the
    // index IS the OrbitPhase. Filled by phase 5's merge, the only place that
    // ever sees every phase's copy of one answer.
    //
    // This is what makes a result carry the STATE of the search and not only its
    // conclusion. `why` is the winning sentence; these are all of them, and the
    // empty ones say as much -- a value only direct found is a different kind of
    // answer from one direct, guess and memory all reached.
    std::string said[ORBIT_LAST_PHASE + 1];

    // ATTENTION IS COUNTED. WEIGHT IS JUDGED, and the pair DISAGREEING is the
    // signal: "it is in there four times and orbit does not think it is what
    // you meant" and "orbit is sure and there is nothing under it" are the two
    // answers one `confidence` could not tell apart, and the second is the one
    // worth being warned about. src/satellite_value/value_result.hpp argues it.

    // How many times it is LITERALLY in the corpus, as a node or as a window of
    // a list. A raw count and never a percentage, counted at level 1 whatever
    // the dial says, from the corpus alone: attention may never read a phase's
    // opinion, because two numbers that shared inputs could no longer disagree
    // and the pair would be two spellings of one judgement.
    int attention = 0;

    // Filled by phase 5, which produces no candidates and weighs these three.
    int agreement = 0;   // how many phases independently produced it
    int prior     = 0;   // how many times memory has seen it resolve so
    int weight    = 0;   // 0..100. Tightness, agreement, memory; halved if made
};

// What a resolution is: every finding, ranked, plus what went wrong.
struct Resolution {
    std::vector<Finding> findings;  // best first once frequency has run
    bool too_deep = false;
};

// The question. `threshold` is the dial as the search power already defines it
// (search_threshold()), and `max_depth` bounds the walk the way every other
// recursion in the evaluator is bounded.
struct OrbitQuery {
    ValuePtr root;
    ValuePtr pattern;
    int threshold = SEARCH_EXACT;
    int max_depth = 64;

    // MAY THIS RESOLUTION WRITE TO MEMORY? True for a resolution a user asked
    // for, and FALSE FOR EVERY ROUND OF A SETTLE, which is not a style choice:
    // rounds 2 and up ask questions the ENGINE invented, and a memory that
    // recorded its own guesses would feed them back as evidence next time --
    // weight compounding on nothing, which is the failure orbit_engine.cpp's
    // record site is arranged to avoid. A settle records exactly once, itself,
    // pairing the question the USER asked with what the whole run settled on.
    bool remember = true;
};

// ---------------------------------------------------------------------------
// Layer five: the answer becomes the question
// ---------------------------------------------------------------------------

// One round of a settle: what was asked, and everything that came back.
struct OrbitRound {
    ValuePtr pattern;
    Resolution resolution;
};

// How many rounds a settle runs when nobody says. Small on purpose: each round
// is a WHOLE resolution -- five phases and several walks of the corpus -- and a
// chain longer than this is one no person is going to read. A settle almost
// always stops long before it, because it stops the moment a question repeats.
inline constexpr int ORBIT_SETTLE_ROUNDS = 8;

// The most a program may ask for, refused BY NAME rather than clamped, on the
// dial's own DECISION 5b: a clamp would make .settle(p, 5000) silently mean 64
// and the program would never learn it asked for something the language does
// not have.
inline constexpr int ORBIT_SETTLE_MAX = 64;

// RESOLVE, THEN ASK AGAIN WITH THE ANSWER. Round 1 asks what the user asked;
// round 2 asks what round 1 concluded; and so on until a question repeats.
//
// This is the one place orbit can improve its own answer without anybody
// writing another rule, and it is why it exists. A one-typo hit is a WEAK
// answer -- weight 15 on the ladder -- but the value it found is a REAL string
// in the corpus, and asking for that instead is an exact question with an exact
// answer. The trajectory shows the weight rise, and nothing learned anything:
// the second question was simply better than the first.
//
// It is also how orbit reaches past the ONE-HOP bound phase 3 imposes on
// itself. The link rule is deliberately a single hop, because an unbounded
// chase inside the walker is a hang on a structure the language permits. A hop
// per ROUND is the same chase made safe: bounded, counted, stoppable, and with
// every step on the record as a result of its own.
//
// STOPS AT A REPEATED QUESTION, which is a fixpoint when the answer is the
// question just asked and a CYCLE when it is an earlier one -- a and b that
// point at each other close after two rounds rather than running to the cap.
// Also stops on a round that found nothing, and at `rounds`.
//
// False when the structure was deeper than max_depth, as orbit_resolve is.
bool orbit_settle(const OrbitQuery &query, int rounds,
                  std::vector<OrbitRound> &out);

// The trajectory as a satellite program sees it: ONE RESULT PER ROUND, in
// order, each the winner of its own round and carrying that round's other
// candidates in .alternatives().
//
// NOTHING IS REINTERPRETED to make this work, and that is deliberate. Each
// element is exactly the result `.orbit()` would have answered that round, with
// `pattern` saying what the round asked -- so the trajectory reads as a
// conversation the engine had with itself, and `rank` and `candidates` go on
// meaning where a result stood among the candidates of its OWN round.
//
// Null with `error` set when the structure was too deep or `rounds` is out of
// range.
ValuePtr orbit_settle_collect(const ValuePtr &target, const ValuePtr &pattern,
                              int rounds, int max_depth, std::string &error);

// ---------------------------------------------------------------------------
// The phases. One shape, five times.
// ---------------------------------------------------------------------------
//
// Each is handed the query and everything found so far, and APPENDS. Reading
// `out` is the point — guess skips what direct already has, predict links from
// what the first two found, and frequency reads all of it. That shared vector
// is what "building up on the direct search" means in code.

// Phase 1. search_walk at the dial, plus the run scan.
//
// THE RUN SCAN IS NEW MACHINERY and orbit.txt is why. Its example searches
// {T,T,F,T,F,T,T} for {T,T}, and nothing in the language finds that today: a
// two-element brace is an ENTRY pattern for a map or a nested two-element
// list, and no code anywhere slides a window along a flat list. So a list
// pattern of length k over a list root scores every window of length k,
// elementwise, through the same ladder.
//
// In DIRECT and not in GUESS because a run that is literally present is found,
// not guessed.
void orbit_direct(const OrbitQuery &query, std::vector<Finding> &out);

// Phase 2. The dial, run upward on the query's behalf.
//
// Levels 5..10 already are what "guess without brute force" honestly means —
// prefix, substring, one typo, two typos, subsequence. What was missing is that
// the USER had to pick the level, so a needle that would have matched at 8
// returned nothing at 1 and said nothing about how close it came. This widens
// to SEARCH_LOOSEST, keeps what phase 1 did not have, and records the level it
// took.
void orbit_guess(const OrbitQuery &query, std::vector<Finding> &out);

// Phase 3. Logic and force, which orbit.txt names as two things because they
// are two things.
//
// LOGIC is the one-hop link: a value found at one path whose text is also a KEY
// at another resolves to what that key points at. It is the one inference a
// flat search structurally cannot make, and it is bounded to a single hop for
// the reason the walker is bounded at all.
//
// FORCE is neighbour construction: perturb the query by ONE element and ask
// whether the perturbed query occurs. This is what makes orbit.txt's own answer
// expressible — searching {T,T} and being told it may be {T,F} is a constructed
// candidate, and no amount of searching produces it because it is not what was
// asked for. One element, because k elements over an alphabet of n is n^k and
// one is n*k: the bound is the whole reason this is not a brute force.
void orbit_predict(const OrbitQuery &query, std::vector<Finding> &out);

// Phase 4. Memory, on disk.
//
// A remembered answer is offered ONLY IF IT IS PRESENT IN THE CURRENT CORPUS.
// Memory proposes; it never invents. Without that rule a stale record would put
// a value into a result that the searched structure does not contain, and a
// result whose `value` is not in the thing searched is a lie whatever its
// confidence says.
void orbit_index(const OrbitQuery &query, std::vector<Finding> &out);

// Phase 5. It produces no candidates. It ranks them, on four numbers the other
// four phases have already paid for, and fills in `confidence`.
void orbit_frequency(const OrbitQuery &query, std::vector<Finding> &inout);

// All five, in order, accumulating. False when the structure was deeper than
// max_depth, which the caller reports by naming the knob.
bool orbit_resolve(const OrbitQuery &query, Resolution &out);

// Every finding as satellite.container.result, ranked, each carrying the whole
// state of the search that made it: the value, where it was, what it is worth,
// how much is really under it, what each of the five said, what was asked, at
// what dial, and what else it could have been.
//
// A LIST, EMPTY WHEN NOTHING RESOLVED -- the search power's own rule one layer
// down, right for the same reason: a question that errors when the answer is
// "no" cannot be asked.
//
// Null with `error` set when the structure was deeper than `max_depth`.
ValuePtr orbit_collect(const ValuePtr &target, const ValuePtr &pattern,
                       int max_depth, std::string &error);

// The resolution as the language sees it: one satellite.container.result per
// finding, ranked, each holding the others as its alternatives. Exposed rather
// than static because orbit_test builds a resolution and then asks what a
// program would have been handed -- the only way to test the surface without a
// parser, which is the contract at the top of this file.
ValuePtr orbit_results_of(const Resolution &resolution, const OrbitQuery &query);

// ---------------------------------------------------------------------------
// Shared machinery, used by more than one phase
// ---------------------------------------------------------------------------

// A finding's identity, for dedup and for counting agreement: where it was and
// what it is. Two phases reaching the same value at the same path are two votes
// for one answer, not two answers — which is the number frequency needs.
std::string finding_key(const Finding &finding);

// Every window of `needle.size()` in `hay` that scores at or tighter than
// `threshold`, elementwise. The run scan, factored out of phase 1 so that
// phase 3's force half can ask the same question about a perturbed needle.
//
// `at` is the index the window starts at. The score is the LOOSEST elementwise
// score in the window, because a run is only as tight as its weakest element —
// which is score_both's rule, applied across k elements instead of two.
struct OrbitRun {
    size_t at;
    int score;
};
void orbit_scan_runs(const List &hay, const List &needle, int threshold,
                     std::vector<OrbitRun> &out);

// HOW MANY TIMES `value` IS LITERALLY IN `root` -- attention, and the whole of
// how it is computed. Two kinds of sighting, counted once each:
//
//   a NODE     it is an element, a key or a value somewhere in there
//   a WINDOW   it is a list, and those elements sit adjacent inside a longer
//              one -- orbit.txt's {T,T} inside {T,T,F,T,F,T,T}
//
// A window spanning a WHOLE list is not counted: that list is already a node,
// and counting both would report one sighting as two. EXACT, never at the dial,
// because a count is a fact and a fact does not move when a knob about how alike
// two things must be is turned.
//
// The asymmetry it fixes was measured: a run phase 1 FOUND scored occurrence 0
// -- a window is not a node, so the alphabet never held it -- while a run phase
// 3 CONSTRUCTED scored 2, because force counted its own windows on the way past.
// The found thing looked less present than the invented one.
int orbit_sightings(const ValuePtr &root, const ValuePtr &value, int max_depth);

// Every distinct value that appears anywhere in `root`, with how many times.
// The corpus's own alphabet — what phase 3 perturbs toward, and what phase 5
// counts occurrences against. Rendered form to count, one example value each.
struct OrbitTerm {
    ValuePtr value;
    int count;
};
void orbit_alphabet(const ValuePtr &root, int max_depth,
                    std::vector<OrbitTerm> &out);

} // namespace satellite
