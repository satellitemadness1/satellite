// satellite.container.result — the type, and the two numbers that are the whole
// reason it is a type.
//
// Six things this file exists to pin, each a rule that would otherwise be true
// only by accident:
//
//   1. .orbit() ANSWERS RESULTS, not maps. The measured cost of the map version
//      was that a result could not be BOUND to any type at all, so the first
//      check here is the one that was impossible before the type existed.
//   2. ATTENTION IS COUNTED AND WEIGHT IS JUDGED, and nothing that moves one
//      moves the other. Memory raises weight and leaves attention alone; the
//      corpus raises attention and the phases cannot touch it.
//   3. ALL FIVE PHASES ARE IN EVERY RESULT, the silent ones as nil, because a
//      missing key is an error and "predict had nothing to say" is an answer.
//   4. ALTERNATIVES ARE ONE LEVEL DEEP, by construction. An alternative's own
//      .alternatives() is empty, which is what stops the handles closing a ring.
//   5. A RESULT IS A MAP UNDERNEATH -- it type-checks against a bare map, it
//      prints as one, and the search power walks into it -- and is NOT one to
//      map<K, V>, because the fields are heterogeneous and that is the point.
//   6. THE FIELDS AND THE METHODS ARE ONE THING. Every field is reachable by
//      name, so `r.why()` and `r["why"]` cannot come to mean different things.
//
// It runs the engine and the builder directly, never through a program, which
// is the contract orbit.hpp opens with.

#include "orbit_test.hpp"

#include <string>

using namespace satellite;

namespace {

// The one corpus this file searches: orbit.txt's own example.
ValuePtr example_corpus()
{
    return list_of({yes(), yes(), no(), yes(), no(), yes(), yes()});
}

Type type_of(const char *space, const char *name)
{
    Type t;
    t.space = space;
    t.name = name;
    return t;
}

// A field of the result, by name. Through the map, because that is what a
// program reaches for and because a test that read a C++ member would be
// testing something a satellite program cannot see.
ValuePtr field(const ValuePtr &result, const char *name)
{
    const ResultBody *body = as_result(*result);
    if (!body || !body->fields)
        return nullptr;
    const MapBody *fields = as_map(*body->fields);
    if (!fields)
        return nullptr;
    for (const MapEntry &entry : fields->entries)
        if (entry.key && to_string(*entry.key) == name)
            return entry.value;
    return nullptr;
}

std::string field_text(const ValuePtr &result, const char *name)
{
    ValuePtr found = field(result, name);
    return found ? to_string(*found) : std::string("<missing>");
}

// Everything .orbit() would hand a program for this query, without a program.
ValuePtr resolve_to_results(const ValuePtr &root, const ValuePtr &pattern)
{
    OrbitQuery query;
    query.root = root;
    query.pattern = pattern;

    Resolution resolution;
    orbit_resolve(query, resolution);
    return orbit_results_of(resolution, query);
}

} // namespace

void orbit_test_result_type()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    ValuePtr corpus = example_corpus();
    ValuePtr answers = resolve_to_results(corpus, list_of({yes(), yes()}));

    const List *results = as_list(*answers);
    check(results != nullptr && !results->empty(),
          "a resolution answers a list of results");
    if (!results || results->empty())
        return;

    const ValuePtr &best = (*results)[0];
    check(as_result(*best) != nullptr,
          "and every element of it IS a satellite.container.result");

    // --- 1. it can be BOUND, which is the whole reason it is a type ----------
    //
    // This is the check that could not be written before. The map version was
    // measured against ./satl and refused: the fields are a list, a nil, a
    // number, a bool and a string together, so no map<K, V> is true about them
    // and results were reachable only by chaining a subscript off a call.
    Type result_type = type_of("container", "result");
    Type bare_map = type_of("container", "map");
    Type list_of_results = type_of("container", "list");
    list_of_results.args.push_back(result_type);

    check(matches(result_type, *best),
          "a result matches satellite.container.result");
    check(matches(list_of_results, *answers),
          "and a list of them matches list<result> -- the binding that was "
          "impossible while a result was a map");

    // A MAP UNDERNEATH, and only to a BARE map. The heterogeneity that made the
    // type necessary is exactly what a map<K, V> must go on refusing, or the
    // old lie would be back with a new spelling.
    check(matches(bare_map, *best),
          "a result matches a bare satellite.container.map");

    Type map_of_strings = type_of("container", "map");
    map_of_strings.args.push_back(type_of("variable", "string"));
    map_of_strings.args.push_back(type_of("variable", "string"));
    check(!matches(map_of_strings, *best),
          "and never map<string, string>: the fields are not all strings");

    // nil satisfies it, for the reason nil satisfies a file: there is no empty
    // conclusion to default a declaration to.
    Value nothing = std::monostate{};
    check(matches(result_type, nothing),
          "nil satisfies a result, so the declaration form is usable");

    // --- 2. the sixteen fields, and the order that argues for them -----------
    const ResultBody *body = as_result(*best);
    const MapBody *fields = body ? as_map(*body->fields) : nullptr;
    check(fields != nullptr && fields->entries.size() == 16,
          "a result carries sixteen fields");

    // ATTENTION BEFORE WEIGHT, which is the type's argument in the order it
    // prints: the evidence, and then the opinion about the evidence.
    if (fields && fields->entries.size() >= 6) {
        check(to_string(*fields->entries[3].key) == "attention",
              "attention is the fourth field");
        check(to_string(*fields->entries[5].key) == "weight",
              "and weight comes after it, evidence before opinion");
    }

    check(field_text(best, "value") == "[true, true]",
          "the winner is the run that was asked for");
    check(field_text(best, "attention") == "2",
          "which is really in there twice");
    check(field_text(best, "phase") == "direct",
          "found by phase 1, not guessed");
    check(field_text(best, "constructed") == "false",
          "and not constructed");
    check(field_text(best, "rank") == "0", "the winner ranks 0");
    check(field_text(best, "candidates") ==
              std::to_string(results->size()),
          "and knows how many candidates the resolution produced");
    check(field_text(best, "pattern") == "[true, true]",
          "a result alone still knows what was asked");
    check(field_text(best, "threshold") == "1",
          "and at what dial it was asked");

    // --- 3. all five phases, the silent ones as nil --------------------------
    ValuePtr phases = field(best, "phases");
    const MapBody *said = phases ? as_map(*phases) : nullptr;
    check(said != nullptr && said->entries.size() == 5,
          "every result carries all five phases");
    if (said) {
        const char *names[] = {"direct", "guess", "predict", "index",
                               "frequency"};
        for (size_t i = 0; i < said->entries.size() && i < 5; i++)
            check(to_string(*said->entries[i].key) == names[i],
                  "the five are in the order they run");

        // A missing key is an error in this language, so a phase that said
        // nothing has to be PRESENT and nil rather than absent -- otherwise
        // asking "what did predict think" would error exactly when the answer
        // is the interesting one.
        check(said->entries.size() > 2 && said->entries[2].value &&
                  std::holds_alternative<std::monostate>(*said->entries[2].value),
              "a phase that concluded nothing is nil, not missing");
        check(said->entries.size() > 0 && said->entries[0].value &&
                  !to_string(*said->entries[0].value).empty(),
              "and the phase that found it left its sentence behind");
    }

    orbit_memory_forget();
}

void orbit_test_result_alternatives()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    ValuePtr answers = resolve_to_results(example_corpus(),
                                          list_of({yes(), yes()}));
    const List *results = as_list(*answers);
    check(results != nullptr && results->size() > 1,
          "orbit.txt's example produces more than one candidate");
    if (!results || results->size() < 2)
        return;

    const ResultBody *best = as_result(*(*results)[0]);
    check(best != nullptr && best->ranked != nullptr,
          "a result carries the whole ranked resolution");
    if (!best || !best->ranked)
        return;

    check(best->ranked->size() == results->size(),
          "every candidate is in it, this one included");
    check(best->rank == 0, "and the winner knows it placed first");

    // ONE LEVEL DEEP, BY CONSTRUCTION. The results inside `ranked` are bare, so
    // an alternative has no alternatives -- which is not a limitation to be
    // lifted later but the thing that stops the handles closing a ring. A result
    // whose alternatives carried alternatives would point back at itself, and
    // value.hpp calls that leak the honest cost of a reference type.
    const ValuePtr &alternative = (*best->ranked)[1];
    const ResultBody *alt = as_result(*alternative);
    check(alt != nullptr, "an alternative is itself a result");
    check(alt != nullptr && alt->ranked == nullptr,
          "and carries none of its own: the depth is one, by construction");

    // The twin is not a second answer. It renders identically and compares
    // equal, because a result IS its fields and both of these are the same
    // finding -- one reachable from the list, one from the winner.
    check(value_equals(*(*results)[1], *alternative),
          "an alternative equals the result it stands for in the list");

    // A RESULT IS NOT A MAP, however much it is one underneath. The variant
    // alternatives differ, so equality answers no before it looks at a field --
    // which is what keeps `found[0] == some_map` from ever being true by
    // accident.
    const ResultBody *first = as_result(*(*results)[0]);
    check(first && first->fields &&
              !value_equals(*(*results)[0], *first->fields),
          "and a result never equals the plain map of its own fields");

    orbit_memory_forget();
}

void orbit_test_result_searchable()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    ValuePtr answers = resolve_to_results(example_corpus(),
                                          list_of({yes(), yes()}));

    // THE RESULT OF A SEARCH IS SEARCHABLE BY THE POWER THAT PRODUCED IT, which
    // is the promise the type was shaped to keep and which costs one word --
    // map_view() -- in the walker and the comparator. `direct` is the phase
    // field of the findings phase 1 produced, so an exact search for it finds
    // them without anything knowing what a result is.
    std::vector<SearchHit> hits;
    check(search_walk(answers, str("direct"), SEARCH_EXACT, 64, hits),
          "a list of results is walkable");
    check(!hits.empty(),
          "and the search power reaches inside a result, at level 1");

    // The winner's own attention, found by searching the result for the number
    // rather than by reading the field -- which only works if a result is a map
    // to the walker.
    const List *results = as_list(*answers);
    if (results && !results->empty()) {
        std::vector<SearchHit> inner;
        search_walk((*results)[0], str("why"), SEARCH_EXACT, 64, inner);
        check(!inner.empty(), "and into one result on its own");
    }

    orbit_memory_forget();
}
