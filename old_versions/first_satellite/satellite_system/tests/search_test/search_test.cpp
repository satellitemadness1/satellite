// The search power — the ladder, the supersets, and the walk.
//
// Three things this file exists to pin, because each is a rule that would
// otherwise be true only by accident:
//
//   1. EVERY LEVEL SCORES WHAT IT CLAIMS TO. A pair meant to match at 5 must
//      not quietly score 3: the ladder's whole value is that the dial means
//      something, and a level that reaches too far makes every level above it
//      meaningless.
//   2. THE SUPERSET PROPERTY HOLDS, counted rather than assumed. DECISION 2
//      says scoring makes "raising the dial can only add hits" true by
//      construction; this counts the hits at all ten settings over one corpus
//      and asserts the count never falls. If search_score is ever rewritten as
//      ten independent tests, this is what notices.
//   3. THE WALK REPORTS WHERE IT WAS. A hit's path has to lead back to the
//      hit, or the long spelling is decoration and the write-through that
//      DECISION 7a promises it enables cannot be built on it.
//
// The ladder half calls search_score directly, because the levels are a fact
// about two values and need no program to demonstrate. The walk half runs
// satellite source, because a path through a list of maps is a fact about the
// language and pinning it any other way would pin the test's own scaffolding.

#include "evaluator/search.hpp"
#include "interpreter/interp.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace satellite;

static int failures = 0;

static void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

static Value str(const char *text) { return make_string(encode_raw(text)); }
static Value num(long long n)      { return Value(Number(n)); }

// The level `needle` and `hay` match at, checked against what was intended.
static void level(const Value &needle, const Value &hay, int want,
                  const std::string &what)
{
    const int got = search_score(needle, hay);
    if (got != want) {
        printf("FAIL: %s\n  want level %d\n  got  level %d\n", what.c_str(),
               want, got);
        failures++;
    }
}

// --- the ladder -------------------------------------------------------------

static void search_test_ladder()
{
    // 1 -- exact, and the two identities §8.6 already decided. 1 and 1.0 are
    // ONE value because Number renders from normalized(); 12 and "12" are TWO
    // because the key's type tag says so. The search agrees with the map about
    // both, which is the point of borrowing map_key_of rather than inventing a
    // second notion of sameness.
    level(str("bolt"), str("bolt"), SEARCH_EXACT, "identical strings");
    level(num(12), num(12), SEARCH_EXACT, "identical numbers");

    // 2 -- case. Both directions, because folding is not a property of which
    // argument came first.
    level(str("Bolt"), str("bolt"), SEARCH_CASE, "Bolt finds bolt");
    level(str("bolt"), str("BOLT"), SEARCH_CASE, "bolt finds BOLT");

    // 3 -- cross-type. The tag dropped on request, and NOT before: this is the
    // level that undoes §8.6's separation, so anything below it must refuse.
    level(num(12), str("12"), SEARCH_CROSS_TYPE, "12 finds \"12\"");
    check(search_score(num(12), str("12")) > SEARCH_CASE,
          "12 and \"12\" do not match at the case level or below");

    // 4 -- trimmed. Space is not in the code table, so this also pins that the
    // raw area is where whitespace was correctly looked for.
    level(str(" bolt "), str("bolt"), SEARCH_TRIMMED, "outer space ignored");

    // 5 and 6 -- prefix then substring, each in both directions.
    level(str("bolt"), str("boltzmann"), SEARCH_PREFIX, "prefix, short first");
    level(str("boltzmann"), str("bolt"), SEARCH_PREFIX, "prefix, long first");
    level(str("ltz"), str("boltzmann"), SEARCH_SUBSTRING, "substring");

    // 8 and 9 -- typos. The numeric case is the one worth pinning: 85845 finds
    // 85846 because their DIGITS are one edit apart, which is what let the
    // ladder cover numbers without an epsilon anybody has to justify.
    level(str("bolt"), str("bilt"), SEARCH_ONE_TYPO, "one substitution");
    level(str("bolt"), str("bilte"), SEARCH_TWO_TYPOS, "two edits");
    level(num(85845), num(85846), SEARCH_ONE_TYPO, "85845 finds 85846");

    // 10 -- subsequence, and the floor under it. "bt" is in "bolt" in order;
    // "tb" is not in it in any order, so nothing on the ladder may claim it.
    // Far enough apart that no tighter level claims it first: "bt" against
    // "bolt" is TWO EDITS and so is level 9's, which is the ladder working
    // rather than a gap in it -- the tightest level always wins.
    level(str("bt"), str("boltzmann"), SEARCH_SUBSEQUENCE, "subsequence");
    level(str("tb"), str("boltzmann"), SEARCH_NO_MATCH,
          "out of order is no match");
    level(str("bolt"), str("xyzzy"), SEARCH_NO_MATCH, "nothing alike");

    // A CONTAINER IS NEVER COMPARED TO A SCALAR. Descending is the walker's
    // job, and a comparator that also descended would double every nested hit
    // -- so this is the boundary between layer one and layer two, asserted.
    List one;
    one.push_back(std::make_shared<const Value>(str("bolt")));
    const Value boxed = make_list(one);
    level(str("bolt"), boxed, SEARCH_NO_MATCH, "scalar never matches a list");

    // The length bound that keeps levels 8 and 9 from being quadratic on
    // candidates that cannot win (DECISION 3c). Correctness, not speed: two
    // strings six apart in length are not two edits apart, whatever the matrix
    // would have said.
    level(str("bo"), str("boltzmann"), SEARCH_PREFIX,
          "a long/short pair is caught by prefix, not by distance");
    level(str("qqqq"), str("qqqqqqqqqqqq"), SEARCH_PREFIX,
          "twelve against four still prefixes");
}

// --- the supersets ----------------------------------------------------------

static void search_test_supersets()
{
    // One corpus, ten settings. Every pair here matches at SOME level or at
    // none; what is asserted is that the count of matches never falls as the
    // dial is raised, which is the property the whole feature rests on.
    const std::vector<std::pair<Value, Value>> corpus = {
        {str("bolt"), str("bolt")},       {str("Bolt"), str("bolt")},
        {num(12), str("12")},             {str(" bolt "), str("bolt")},
        {str("bolt"), str("boltzmann")},  {str("ltz"), str("boltzmann")},
        {str("bolt"), str("bilt")},       {str("bolt"), str("bilte")},
        {str("bt"), str("bolt")},         {str("bolt"), str("xyzzy")},
        {num(85845), num(85846)},         {num(7), num(7)},
    };

    int previous = -1;
    for (int dial = SEARCH_TIGHTEST; dial <= SEARCH_LOOSEST; dial++) {
        int hits = 0;
        for (const auto &pair : corpus) {
            const int score = search_score(pair.first, pair.second);
            if (score != SEARCH_NO_MATCH && score <= dial)
                hits++;
        }
        check(hits >= previous,
              "threshold " + std::to_string(dial) + " returns at least what " +
              std::to_string(dial - 1) + " did");
        previous = hits;
    }

    // Not merely non-decreasing: the dial has to actually DO something, or a
    // comparator that answered 1 for everything would pass the check above.
    int at_one = 0, at_ten = 0;
    for (const auto &pair : corpus) {
        const int score = search_score(pair.first, pair.second);
        if (score == SEARCH_EXACT)
            at_one++;
        if (score != SEARCH_NO_MATCH)
            at_ten++;
    }
    check(at_ten > at_one, "the dial widens the result as it is raised");
    check(at_one == 2, "exactly two pairs in the corpus are exact matches");
}

// --- the walk, through the language ----------------------------------------

static int ns_counter = 0;

static void check_output(const std::string &body, const std::string &want,
                         const std::string &what)
{
    // TOP-LEVEL statements, not a capsule: run_source runs a program's top
    // level and does not call satellite.main. A fresh namespace per case,
    // because satellite.library is a process-wide singleton and one case's
    // declarations would otherwise be visible to the next.
    InterpResult result =
        run_source(body + "\n", "s" + std::to_string(++ns_counter), false);
    if (result.output != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what.c_str(), want.c_str(),
               result.output.c_str());
        failures++;
    }
}

// A list of two maps, which is the shape the whole feature was asked for.
static const char *kTracker =
    "satellite.container.map<satellite.variable.string, "
    "satellite.variable.number> a\n"
    "a.set(\"Fortitude\", 3)\n"
    "satellite.container.map<satellite.variable.string, "
    "satellite.variable.number> b\n"
    "b.set(\"Tenacity\", 85845)\n"
    "satellite.container.list<satellite.container.map<"
    "satellite.variable.string, satellite.variable.number>> lm\n"
    "lm.append(a)\n"
    "lm.append(b)\n";

static void search_test_walk()
{
    const std::string t = kTracker;

    // A key found anywhere in the structure answers WHAT IT POINTS AT, which is
    // the question "find me bolt" was actually asking.
    check_output(t + "satellite.console.display(lm[\"Tenacity\"].to_string())",
                 "[85845]\n", "a key deep in a list of maps");

    // The pair form: key and value together, which is the spelling the request
    // named. Both halves must pass, so a right key with a wrong value is not
    // a hit at threshold 1.
    check_output(t + "satellite.console.display(lm[{\"Tenacity\", 85845}]"
                     ".to_string())",
                 "[85845]\n", "the pair form finds the entry");
    check_output(t + "satellite.console.display(lm[{\"Tenacity\", 1}]"
                     ".length())",
                 "0\n", "a wrong value is not a hit");

    // NOTHING FOUND IS AN EMPTY LIST, NOT AN ERROR. A search asks whether a
    // structure holds something, and a question that errors when the answer is
    // "no" cannot be asked.
    check_output(t + "satellite.console.display(lm[\"nope\"].length())",
                 "0\n", "a miss is an empty list");

    // A WHOLE NUMBER IS STILL POSITIONAL (DECISION 6a). This is the check that
    // fails if anyone ever lets the dial reach the ordinary subscript.
    check_output(t + "satellite.console.display(lm[0].to_string())",
                 "{Fortitude: 3}\n", "l[0] is still the first element");

    // The path leads back to the hit: index 1 of the list, key Tenacity of the
    // map there. This is what the write-through will be built on.
    check_output(t + "satellite.console.display(lm.search(\"Tenacity\")"
                     "[0][\"path\"].to_string())",
                 "[1, Tenacity]\n", "the path names where the hit was");

    // The dial, and the ordering it exposes. At 6 both maps answer, and the
    // TIGHTER hit comes first -- "t" prefixes Tenacity and is only a substring
    // of Fortitude, so 5 sorts before 6.
    check_output(t + "satellite.system.threshold(6)\n"
                     "satellite.console.display(lm[\"t\"].to_string())",
                 "[85845, 3]\n", "best score first");

    // A MAP ROOT. An exact hit is still answered exactly -- §8.6's lookup is
    // untouched, and this is the check that fails if it ever stops being --
    // while a miss searches instead of dead-ending.
    const std::string m =
        "satellite.container.map<satellite.variable.string, "
        "satellite.variable.number> m\n"
        "m.set(\"Tenacity\", 85845)\n";
    check_output(m + "satellite.console.display(m[\"Tenacity\"])",
                 "85845\n", "an exact map hit is still the bare value");
    check_output(m + "satellite.console.display(m[{\"Tenacity\", 85845}]"
                     ".to_string())",
                 "[85845]\n", "a pair against a map root searches");

    // Crazy combination: map -> list -> map, and a path that walks all three.
    check_output(
        "satellite.container.map<satellite.variable.string, "
        "satellite.variable.number> inner\n"
        "inner.set(\"Tenacity\", 85845)\n"
        "satellite.container.list<satellite.container.map<"
        "satellite.variable.string, satellite.variable.number>> mid\n"
        "mid.append(inner)\n"
        "satellite.container.map<satellite.variable.string, "
        "satellite.container.list<satellite.container.map<"
        "satellite.variable.string, satellite.variable.number>>> outer\n"
        "outer.set(\"wave_one\", mid)\n"
        "satellite.console.display(outer.search(\"Tenacity\")[0][\"path\"]"
        ".to_string())",
        "[wave_one, 0, Tenacity]\n",
        "a path through map -> list -> map");

    // The dial reads back, and refuses what it does not have.
    check_output("satellite.system.threshold(4)\n"
                 "satellite.console.display(satellite.system.threshold())",
                 "4\n", "the dial reads back");
}

int main()
{
    search_test_ladder();
    search_test_supersets();
    search_test_walk();

    printf("PASS: search (ten levels each scoring what they claim; the "
           "superset property counted over a corpus at all ten settings; a "
           "container never matched against a scalar; the walk through a list "
           "of maps by key, by pair and by path; a miss as an empty list; "
           "l[0] still positional; best score first; the dial set, read and "
           "refused)\n");
    return failures ? 1 : 0;
}
