// satellite.container.result's own method table — the four questions a map
// cannot answer, plus every field by name.
//
// Part of src/evaluator/, and on the RUNTIME side of the seam eval_internal.hpp
// draws: this is method surface a bytecode VM calls unchanged.
//
// A FREE FUNCTION and not an Evaluator member, on the argument search.hpp opens
// with: the search power and the result it settles into are deliberately free of
// the Evaluator class. It also keeps eval_evaluator.hpp, which is AT the
// 325-line ceiling, out of this feature entirely -- the same pressure that made
// the search dial a free function rather than a member.
//
// std::nullopt means "not one of mine, try the map's read surface". An engaged
// optional means this answered; a null ValuePtr inside it means it refused, and
// `error` says why.

#include "evaluator/eval_internal.hpp"

#include <optional>

namespace satellite {
namespace {

// A field by name. Linear over sixteen entries and deliberately not through the
// side index: that index is keyed by map_key_of's canonical bytes, so using it
// would mean encoding a SatString for every lookup to answer a question about a
// name the compiler already knows.
const ValuePtr *field_named(const MapBody &fields, const std::string &name)
{
    for (const MapEntry &entry : fields.entries)
        if (entry.key && to_string(*entry.key) == name)
            return &entry.value;
    return nullptr;
}

std::string field_text(const MapBody &fields, const char *name)
{
    const ValuePtr *found = field_named(fields, name);
    return (found && *found) ? to_string(**found) : "";
}

// THE REPORT, for a person. `.lines()` is what a list and the arguments object
// already spell "the human listing" -- to_string() is the value as one line, and
// this is the value as a reading -- so a result answering the same two questions
// with the same two names is the language's own convention rather than a new one.
//
// WEIGHT AND ATTENTION ON ONE LINE, SIDE BY SIDE AND NOT SUMMED. The pair
// disagreeing is what the split was built to expose: "weight 88, attention 0"
// has to be readable at a glance as orbit speculating, and a report that
// blended them into one number would hide exactly the case the type exists for.
std::string result_report(const ResultBody &result, const MapBody &fields)
{
    std::string out = field_text(fields, "value");
    out += "\n  weight " + field_text(fields, "weight") +
           "   attention " + field_text(fields, "attention") +
           "   rank " + field_text(fields, "rank") + " of " +
           field_text(fields, "candidates");

    // WHICH PHASE SETTLED IT, on the same line as the two numbers, because
    // "constructed by predict" and "found by direct" are the difference between
    // a fact about the corpus and the engine's own proposal -- and a reader who
    // takes in the numbers without that word has been told half of it.
    out += "   " + field_text(fields, "phase");
    if (field_text(fields, "constructed") == "true")
        out += " (constructed)";
    out += "\n  " + field_text(fields, "why");

    // WHERE IT WAS, and the empty path means two different things depending on
    // which side of `constructed` it is on. On a found result it is the ROOT --
    // the whole structure matched -- and on a constructed one there is nowhere
    // it came from at all, so the line is left off rather than printed as an
    // empty pair of brackets that a reader would have to interpret.
    const ValuePtr *path = field_named(fields, "path");
    const List *steps = (path && *path) ? as_list(**path) : nullptr;
    if (steps && !steps->empty())
        out += "\n  at " + to_string(**path);
    else if (steps && field_text(fields, "constructed") != "true")
        out += "\n  the whole structure";

    // WHAT EACH OF THE FIVE CONCLUDED, in phase order, and only the ones that
    // reached this answer. The five together are the state of the search; the
    // silent ones are as much a part of it as the ones that spoke, and a reader
    // sees which those were by which lines are missing.
    const ValuePtr *phases = field_named(fields, "phases");
    if (phases && *phases) {
        if (const MapBody *said = as_map(**phases)) {
            size_t width = 0;
            for (const MapEntry &entry : said->entries)
                if (entry.key && to_string(*entry.key).size() > width)
                    width = to_string(*entry.key).size();
            for (const MapEntry &entry : said->entries) {
                if (!entry.key || !entry.value ||
                    std::holds_alternative<std::monostate>(*entry.value))
                    continue;
                const std::string name = to_string(*entry.key);
                out += "\n  " + name +
                       std::string(width - name.size() + 2, ' ') +
                       to_string(*entry.value);
            }
        }
    }

    if (result.ranked && result.ranked->size() > 1) {
        const size_t others = result.ranked->size() - 1;
        out += "\n  " + std::to_string(others) +
               (others == 1 ? " alternative" : " alternatives");
    }
    return out;
}

} // namespace

std::optional<ValuePtr> result_method(const ValuePtr &recv,
                                      const std::string &name,
                                      const std::vector<ValuePtr> &argv,
                                      std::string &error)
{
    const ResultBody *self = as_result(*recv);
    if (!self || !self->fields)
        return std::nullopt;
    const MapBody *fields = as_map(*self->fields);
    if (!fields)
        return std::nullopt;

    auto arity = [&](size_t want) {
        if (argv.size() == want)
            return true;
        error = arity_message("satellite.container.result", name, want,
                              argv.size());
        return false;
    };

    // WHAT ELSE IT COULD HAVE BEEN -- the one thing about a result that cannot
    // be reached any other way, which is the whole argument for spending the
    // type's own spelling on it. A result handed to a capsule ON ITS OWN still
    // knows what the search's other candidates were, and that is what "a result
    // carries the whole state of the search" means when it is not a slogan.
    //
    // ONE LEVEL DEEP. The results in `ranked` are bare, so an alternative
    // answers an empty list here -- see ResultBody, where the reason is that a
    // result whose alternatives carried alternatives would close a shared_ptr
    // cycle. An empty list and not an error, on the search power's own rule: a
    // question that fails when the answer is "none" cannot be asked.
    if (name == "alternatives") {
        if (!arity(0))
            return ValuePtr(nullptr);
        List out;
        if (self->ranked) {
            out.reserve(self->ranked->size());
            for (size_t i = 0; i < self->ranked->size(); i++)
                if (i != self->rank)
                    out.push_back((*self->ranked)[i]);
        }
        return make_value(std::move(out));
    }

    if (name == "lines") {
        if (!arity(0))
            return ValuePtr(nullptr);
        return make_value(encode_raw(result_report(*self, *fields)));
    }

    // EVERY FIELD IS A METHOD, and the table is the fields map itself. A field
    // the builder adds is a method the same day, and `r.why()` and `r["why"]`
    // cannot come to mean different things because there is only one of them.
    //
    // The map's own read surface is consulted BEFORE this (methods_containers.
    // cpp), which is the arguments object's rule one level over: a method wins
    // over a name, so a field that collided with `.length()` would be
    // unreachable as a method rather than ambiguous. None of the sixteen does.
    if (const ValuePtr *found = field_named(*fields, name)) {
        if (!arity(0))
            return ValuePtr(nullptr);
        return *found ? *found : make_value(std::monostate{});
    }

    return std::nullopt;
}

} // namespace satellite
