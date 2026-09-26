// A finding, as satellite.container.result -- the whole state of the search
// that made it, in the sixteen fields a program reads.
//
// The only file in this module that knows what a RESULT is. The five phases do
// not, and orbit_apply.cpp knows only that a resolution exists, so a field added
// here moves nothing above or below it -- the same seam search_apply.cpp draws
// one layer down, for the same reason.
//
// THE ORDER OF THE FIELDS IS THE ARGUMENT THE TYPE MAKES. A result prints in
// insertion order, so what a reader sees first is what it settled on, then how
// much is REALLY under it, then what orbit THINKS of it. attention before
// weight, every time: the evidence, and then the opinion about the evidence.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

namespace satellite {
namespace {

// A field, written through map_with so the side index is maintained by the one
// function that owns that invariant. value.hpp is explicit that a body is built
// then frozen, and a second place that knew how would be a second place that
// could get it wrong.
struct Fields {
    MapBody body;

    void put(const char *name, ValuePtr value)
    {
        MapBody next;
        std::string error;
        if (map_with(body, make_value(encode_raw(name)),
                     value ? value : make_value(std::monostate{}), next, error))
            body = std::move(next);
    }
    void put_text(const char *name, const std::string &text)
    {
        put(name, make_value(encode_raw(text)));
    }
    void put_number(const char *name, long long n)
    {
        put(name, make_value(Number(n)));
    }
};

// WHAT EACH OF THE FIVE CONCLUDED, all five present, nil where that phase never
// reached this answer.
//
// ALL FIVE, AND NOT ONLY THE ONES THAT SPOKE. A missing key in a map is an
// error (§8.6), so a phases map that held only the phases that concluded
// something would make `r.phases()["predict"]` an error exactly when the
// interesting answer is "predict had nothing to say about this". The silence of
// a phase is a fact about the search, and a fact a program has to be able to
// ask for without risking an error is a fact that has to be stored.
ValuePtr phases_of(const Finding &finding)
{
    Fields said;
    for (int p = ORBIT_FIRST_PHASE; p <= ORBIT_LAST_PHASE; p++) {
        const char *name = orbit_phase_name(static_cast<OrbitPhase>(p));
        if (finding.said[p].empty())
            said.put(name, nullptr);
        else
            said.put_text(name, finding.said[p]);
    }
    return make_value(std::move(said.body));
}

// One finding's fields. Shared by a result and by the bare twin of it that sits
// in `ranked`, because every one of the sixteen is true of both -- see
// `candidates`, which is deliberately a count of what the RESOLUTION produced
// rather than of what this result can hand back.
ValuePtr fields_of(const Finding &finding, const OrbitQuery &query, size_t rank,
                   size_t total)
{
    Fields fields;

    // --- what it settled on, and where -------------------------------------
    fields.put("value", finding.value);
    fields.put("key", finding.key);
    fields.put("path", make_value(finding.path));

    // --- the concrete ------------------------------------------------------
    //
    // A COUNT, not a percentage, and it stands before the opinion for the reason
    // in the banner. `constructed` belongs to this half rather than to the
    // judged one: whether the corpus contains this value AT ALL is a fact about
    // the corpus, and it is the fact that says how to read the count beside it.
    fields.put_number("attention", finding.attention);
    fields.put("constructed", make_value(Value(finding.constructed)));

    // --- the judged --------------------------------------------------------
    fields.put_number("weight", finding.weight);
    fields.put_number("score", finding.score);
    fields.put_text("phase", orbit_phase_name(finding.phase));
    fields.put_number("agreement", finding.agreement);
    fields.put_number("prior", finding.prior);
    fields.put_text("why", finding.why);
    fields.put("phases", phases_of(finding));

    // --- the question this is an answer to ----------------------------------
    //
    // A RESULT HANDED TO A CAPSULE ON ITS OWN still knows what was asked and at
    // what dial. Without these two a result is an answer with no question, and a
    // program that stored one and read it later could not say what it had been
    // an answer TO -- which is most of what "it carries the whole state of the
    // search" has to mean if it means anything.
    fields.put("pattern", query.pattern);
    fields.put_number("threshold", query.threshold);

    // --- where it stands among the others ------------------------------------
    fields.put_number("rank", static_cast<long long>(rank));
    fields.put_number("candidates", static_cast<long long>(total));
    return make_value(std::move(fields.body));
}

} // namespace

ValuePtr orbit_results_of(const Resolution &resolution, const OrbitQuery &query)
{
    const size_t total = resolution.findings.size();

    // One fields map per finding, built once and shared by the two results that
    // stand for it below.
    std::vector<ValuePtr> fields;
    fields.reserve(total);
    for (size_t i = 0; i < total; i++)
        fields.push_back(fields_of(resolution.findings[i], query, i, total));

    // THE BARE RESULTS, which are what an alternative is.
    //
    // Two results per finding, and the duplication is the answer to a cycle
    // rather than an oversight. Every result carries every other, so a result
    // alone can still say what else the search considered; if the ones it
    // carried also carried it, the handles would point in a ring and the whole
    // resolution would leak -- the same shared_ptr cycle value.hpp calls the
    // honest cost of an Object, arrived at by a different road.
    //
    // So the ones in here have no `ranked` of their own, `.alternatives()` on an
    // alternative is empty, and the depth is one by construction rather than by
    // a visited set that has to be maintained.
    List bare;
    bare.reserve(total);
    for (size_t i = 0; i < total; i++) {
        ResultBody body;
        body.fields = fields[i];
        body.rank = i;
        bare.push_back(make_value(make_result(std::move(body))));
    }
    ListRef ranked = std::make_shared<const List>(std::move(bare));

    // THE RESULTS THEMSELVES. One shared handle to the ranked list each, so the
    // storage is linear in the number of findings and not square in it -- which
    // it would be if every result held its own copy of the others.
    List out;
    out.reserve(total);
    for (size_t i = 0; i < total; i++) {
        ResultBody body;
        body.fields = fields[i];
        body.ranked = ranked;
        body.rank = i;
        out.push_back(make_value(make_result(std::move(body))));
    }
    return make_value(std::move(out));
}

} // namespace satellite
