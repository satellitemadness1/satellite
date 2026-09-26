// Phase 5 — frequency. "we look at the likelihood of what we have made from the
// previous phases, and the likelihood of the result being just something else."
// (orbit.txt)
//
// IT PRODUCES NO CANDIDATES. Every value it ranks was put there by one of the
// four phases before it, which is what makes it a reducer rather than a fifth
// opinion -- and it is the reason DECISION 2 refuses to let the pipeline exit
// early. Frequency weighs candidates AGAINST EACH OTHER; handed one unopposed
// candidate it can only ever rate it 100%, which is not a confidence, it is a
// restatement.
//
// IT PRODUCES TWO NUMBERS AND NOT ONE, and that is the whole of the result
// type's design landing here. `confidence` used to be a blend of tightness,
// agreement, occurrence and memory, so a direct hit that occurs twice and a
// neighbour the engine invented that three phases liked could arrive at the same
// number by different routes with nothing in the answer saying which happened.
//
//     attention   COUNTED. How many times the answer is literally in the
//                 corpus. Computed from the corpus and the walk, at level 1,
//                 and it may never read a phase's opinion.
//     weight      JUDGED. What orbit thinks, 0..100: tightness, agreement,
//                 memory, halved when the engine proposed rather than found.
//
// The pair DISAGREEING is the product. "High attention, low weight" is "it is
// definitely in there and I do not think it is what you meant"; "high weight,
// low attention" is "I am fairly sure and there is nothing under it", and that
// second sentence is the one a search engine cannot otherwise say about itself.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace satellite {
namespace {

// THE THREE WEIGHTS, AND WHY EACH IS THE SIZE IT IS. They sum to exactly 100 at
// their maxima, so a perfect score is reachable and the number means something
// rather than being a ratio of an arbitrary total.
//
// THERE WERE FOUR. Occurrence was one of them and is not any more: it left to
// become attention, which is the behaviour change the split is FOR and not a
// side effect of it. What is left is exactly the judged part -- how alike, how
// many agreed, and what memory says -- and the three were renormalised to 100
// rather than left summing to 85, because a maximum nobody can reach is not a
// scale, it is a bug that never gets reported.
//
// TIGHTNESS is half of everything, because the ladder is the one signal that
// says the needle and the answer are actually alike. An exact hit that no other
// phase corroborated should still outrank a two-typo hit that three did.
constexpr int TIGHTNESS_MAX = 50;

// AGREEMENT is next, and it is still capped at TWO EXTRA PHASES rather than
// four, because the phases are not independent: guess re-runs direct's walk at a
// wider dial, so those two agreeing is nearly automatic. Three distinct phases
// reaching one answer is the real signal, and paying more for a fourth would be
// paying for the correlation. Occurrence's 15 went here and to prior, so the
// STEP grew from 10 to 15 and the cap stayed where the argument put it.
constexpr int AGREEMENT_STEP = 15;
constexpr int AGREEMENT_MAX = 30;

// PRIOR is the smallest, because memory can be loud without being right: a
// record seen fifty times may be fifty repetitions of one mistake. It is also
// the only one of the three that is not about THIS search, which is the second
// reason it does not get to dominate one.
constexpr int PRIOR_MAX = 20;

// Diminishing returns, spelled as a table rather than a curve because there are
// three steps and a table can be read.
int banded(int count, int cap)
{
    if (count <= 0)
        return 0;
    if (count == 1)
        return cap / 3;
    if (count <= 3)
        return (cap * 2) / 3;
    return cap;
}

int weight_of(const Finding &finding, int distinct_phases)
{
    int score = 0;

    // 50 at an exact match, 5 at the loosest rung. A finding with no ladder
    // score at all contributes nothing here, which is right: it was not reached
    // by comparing two values.
    if (finding.score != SEARCH_NO_MATCH)
        score += (SEARCH_LOOSEST + 1 - finding.score) * (TIGHTNESS_MAX / SEARCH_LOOSEST);

    score += std::min(AGREEMENT_MAX, (distinct_phases - 1) * AGREEMENT_STEP);
    score += banded(finding.prior, PRIOR_MAX);

    // NO ATTENTION TERM HERE, AND THAT IS THE POINT OF THE WHOLE FILE. Adding
    // one would put the concrete count back inside the judgement, and the two
    // numbers would share an input -- at which point they could no longer
    // disagree, and the pair would be two spellings of one opinion rather than
    // an opinion and the evidence beside it.

    // A CONSTRUCTED ANSWER IS HALVED, and this is the arithmetic of DECISION 6
    // rather than a tuning choice. Phase 3's force half answers a question the
    // user did not ask: it may well be the right answer -- orbit.txt's own
    // example asks for exactly that -- but it cannot be as likely as the same
    // evidence for the question that WAS asked. Halving says so in the one
    // number a caller is most likely to read, so a user who never reads `why`
    // still gets told.
    if (finding.constructed)
        score /= 2;

    return std::min(100, score);
}

} // namespace

void orbit_frequency(const OrbitQuery &query, std::vector<Finding> &inout)
{
    if (inout.empty())
        return;

    // --- merge -------------------------------------------------------------
    //
    // The four phases are EXPECTED to produce the same finding more than once.
    // That duplication is the agreement signal and not a defect: direct finding
    // something and memory remembering it are two reasons to believe it, and
    // collapsing them without counting would throw away the evidence.
    //
    // IT IS ALSO WHERE THE FIVE PHASES BECOME ONE STATE. Every copy of an answer
    // leaves its sentence behind in said[its phase], so what comes out is not
    // the winning finding with a count beside it -- it is one answer carrying
    // what each of the five concluded about it, which is what a result was asked
    // to be. Nothing else in the pipeline sees more than its own phase's copy.
    std::vector<Finding> merged;
    std::map<std::string, size_t> at;
    std::map<std::string, std::set<int>> phases;

    for (const Finding &finding : inout) {
        const std::string id = finding_key(finding);
        phases[id].insert(static_cast<int>(finding.phase));

        auto found = at.find(id);
        if (found == at.end()) {
            at[id] = merged.size();
            merged.push_back(finding);
            merged.back().said[finding.phase] = finding.why;
            continue;
        }

        // THE TIGHTEST WINS, AND ITS SENTENCE COMES WITH IT. When direct found
        // something at level 1 and guess found the same thing at 8, the answer
        // is that it is an exact match -- and `why` has to be the exact match's
        // sentence, or the result would carry a weight built from one finding
        // and an explanation belonging to another.
        //
        // What does NOT get overwritten is said[]: the loser's own sentence is
        // kept under its own phase, because "guess would have reached this at
        // level 8" stays true and stays worth reading after direct reached it
        // at 1. The tightest finding wins the summary, not the record.
        Finding &kept = merged[found->second];
        if (finding.score != SEARCH_NO_MATCH &&
            (kept.score == SEARCH_NO_MATCH || finding.score < kept.score)) {
            const int prior = std::max(kept.prior, finding.prior);
            std::string said[ORBIT_LAST_PHASE + 1];
            for (int p = ORBIT_FIRST_PHASE; p <= ORBIT_LAST_PHASE; p++)
                said[p] = kept.said[p];
            kept = finding;
            kept.prior = prior;
            for (int p = ORBIT_FIRST_PHASE; p <= ORBIT_LAST_PHASE; p++)
                if (!said[p].empty())
                    kept.said[p] = said[p];
        } else {
            kept.prior = std::max(kept.prior, finding.prior);
        }
        if (kept.said[finding.phase].empty())
            kept.said[finding.phase] = finding.why;
    }

    // --- count, and then weigh ----------------------------------------------
    //
    // THE COUNT FIRST AND SEPARATELY, because attention is a fact about the
    // corpus and weight is an opinion about the answer, and the order they are
    // computed in is the clearest place to say that the second never reads the
    // first. Cached by the value's rendering: several findings can be the same
    // value at different paths, and the corpus does not change between them.
    //
    // The cost is stated rather than hidden: one walk per DISTINCT value found,
    // where the old occurrence map was one walk in total. That is what counting
    // windows costs -- a window is not a node, so no single pass over the
    // alphabet can see one -- and it is the price of the number being right.
    std::map<std::string, int> sightings;

    for (Finding &finding : merged) {
        const std::string id = finding_key(finding);
        const int distinct = static_cast<int>(phases[id].size());
        finding.agreement = distinct;

        if (finding.value) {
            const std::string rendered = to_string(*finding.value);
            auto seen = sightings.find(rendered);
            if (seen == sightings.end())
                seen = sightings.emplace(
                    rendered, orbit_sightings(query.root, finding.value,
                                              query.max_depth)).first;
            finding.attention = seen->second;
        }

        finding.weight = weight_of(finding, distinct);

        if (distinct > 1)
            finding.why += ", and " + std::to_string(distinct) +
                           " phases agree on it";
    }

    // --- rank --------------------------------------------------------------
    //
    // ON WEIGHT, AND ATTENTION IS READ BESIDE IT RATHER THAN ADDED TO IT. A
    // ranking that averaged the two would destroy exactly the case the split
    // was built to expose -- the answer orbit is sure of with nothing under it
    // would be quietly pulled down to the middle and stop being visible as the
    // warning it is.
    //
    // Attention DOES break a tie, after the judged fields have finished
    // disagreeing. That is lexicographic and not a blend: of two answers orbit
    // rates identically, the one that is really in there more often goes first,
    // and no arithmetic anywhere lets a large count buy a better weight.
    //
    // stable_sort, and search_walk's reason: two findings that tie all the way
    // down have to come back in the same order twice, or a program that reads
    // the second one gets a different answer on a different day.
    std::stable_sort(merged.begin(), merged.end(),
                     [](const Finding &a, const Finding &b) {
                         if (a.weight != b.weight)
                             return a.weight > b.weight;
                         if (a.score != b.score)
                             return a.score < b.score;
                         if (a.attention != b.attention)
                             return a.attention > b.attention;
                         return !a.constructed && b.constructed;
                     });

    // --- and what THIS phase concluded --------------------------------------
    //
    // Frequency produces no candidates, so it would otherwise be the one of the
    // five with nothing to say in a result that claims to carry all five. It
    // does have something to say, and it is the sentence orbit.txt asked for:
    // where this landed among the others, which is "the likelihood of the
    // result being just something else" stated as data rather than as a caveat.
    for (size_t i = 0; i < merged.size(); i++)
        merged[i].said[ORBIT_FREQUENCY] =
            "ranked " + std::to_string(i + 1) + " of " +
            std::to_string(merged.size()) + " on weight " +
            std::to_string(merged[i].weight) + ", with attention " +
            std::to_string(merged[i].attention);

    inout = std::move(merged);
}

} // namespace satellite
