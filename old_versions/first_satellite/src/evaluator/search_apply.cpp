// Layer three: the dial, and what [ ... ] and .search() hand back.
//
// The only file that knows both that a search exists and that lists and maps
// are the domain it was asked for. search.cpp and search_walk.cpp know neither,
// which is what lets the ladder grow without touching the language surface and
// the surface change without touching the ladder.
//
// EVERYTHING HERE IS A FREE FUNCTION. search.hpp opens by saying the power is
// deliberately free of the Evaluator class, and this is where that claim was
// most tempting to break: a `threshold_` member and an `eval_search` method
// would both have worked, and both would have made the header's first paragraph
// a lie. They also would not have fit -- eval_evaluator.hpp was at 323 lines
// against the 325 ceiling before this work started, which is a second, blunter
// reason and the one that forced the question.

#include "evaluator/search.hpp"
#include "evaluator/eval_internal.hpp"

namespace satellite {
namespace {

// The dial itself. THREAD-LOCAL, per DECISION 5a: per-thread by construction is
// the same answer §6 gives for a Frame, and it costs no lock in the one
// operation that will run in a loop.
//
// The cost is honest and belongs beside the storage: a threshold set on one
// thread does not move another thread's. That is the right reading for a knob
// meaning "how should MY next search behave", and it is exactly the reading a
// per-thread frame already has.
//
// Starts at SEARCH_EXACT, so a program that never touches the dial gets the
// exact match every program written before this feature already assumed.
thread_local int g_threshold = SEARCH_EXACT;

// One hit, as a map a program can read: value, key, path, score.
//
// Built through map_with rather than by filling a MapBody directly, so the side
// index is maintained by the one function that owns that invariant -- value.hpp
// is explicit that a body is built then frozen, and a second place that knew
// how would be a second place that could get it wrong.
ValuePtr hit_as_map(const SearchHit &hit)
{
    MapBody body, next;
    std::string error;
    auto put = [&](const char *name, const ValuePtr &value) {
        if (map_with(body, make_value(encode_raw(name)),
                     value ? value : make_value(std::monostate{}), next, error))
            body = std::move(next);
        next = MapBody{};
    };

    put("value", hit.value);
    put("key", hit.key);
    put("path", make_value(hit.path));
    put("score", make_value(Number(hit.score)));
    return make_value(std::move(body));
}

} // namespace

int search_threshold() { return g_threshold; }

void set_search_threshold(int level) { g_threshold = level; }

ValuePtr search_collect(const ValuePtr &target, const ValuePtr &pattern,
                        bool rich, int max_depth, std::string &error)
{
    std::vector<SearchHit> hits;
    if (!search_walk(target, pattern, g_threshold, max_depth, hits)) {
        error = "this structure nests deeper than satellite.library.system."
                "max_depth (" + std::to_string(max_depth) +
                "), so a search cannot finish walking it";
        return nullptr;
    }

    List out;
    out.reserve(hits.size());
    for (const SearchHit &hit : hits)
        out.push_back(rich ? hit_as_map(hit)
                           : (hit.value ? hit.value
                                        : make_value(std::monostate{})));
    return make_value(std::move(out));
}

// satellite.system.threshold(n) -- how loose a search may be, 1..10.
//
// A FUNCTION and not a satellite.library variable, which is the spelling that
// was asked for and also the right one: system.max_depth is a datum read once
// at construction, and this is a dial a program moves mid-run.
//
// Spelled under satellite.system because that is where the request put it and
// where a knob belongs beside max_depth -- but dispatched from modules.cpp
// rather than from module_system, because it is not a system FACT: uname and
// getpwuid answer what the machine is, and this sets how the search behaves.
int search_threshold_call(const std::string &full,
                          const std::vector<ValuePtr> &argv, ValuePtr &result,
                          std::string &error)
{
    if (full != "satellite.system.threshold")
        return 0;

    const std::string range = std::to_string(SEARCH_TIGHTEST) + " to " +
                              std::to_string(SEARCH_LOOSEST);

    // No argument READS it, so a program can ask what the dial is set to
    // without a second word for the question.
    if (argv.empty()) {
        result = make_value(Number(g_threshold));
        return 1;
    }

    if (argv.size() != 1) {
        error = arity_message("satellite.system", "threshold", 1, argv.size());
        return -1;
    }

    long long level = 0;
    if (!as_index(*argv[0], level)) {
        error = "satellite.system.threshold wants a whole "
                "satellite.variable.number from " + range + ", got " +
                to_string(*argv[0]);
        return -1;
    }

    // OUT OF RANGE IS AN ERROR, NOT A CLAMP (DECISION 5b). A clamp would make
    // threshold(11) silently mean 10, and the program would never learn it had
    // asked for something the language does not have.
    if (level < SEARCH_TIGHTEST || level > SEARCH_LOOSEST) {
        error = "satellite.system.threshold is " + range + " -- " +
                std::to_string(SEARCH_TIGHTEST) + " is an exact match and " +
                std::to_string(SEARCH_LOOSEST) +
                " is anything remotely alike -- got " + to_string(*argv[0]);
        return -1;
    }

    g_threshold = static_cast<int>(level);
    // Answers nothing, the way a mutator does: the dial is set, and echoing it
    // back would make every threshold() line print in the REPL.
    result = make_value(std::monostate{});
    return 1;
}

} // namespace satellite
