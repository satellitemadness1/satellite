#include "evaluator/eval_internal.hpp"
#include "evaluator/search.hpp"
#include "satellite_orbit_search/orbit.hpp"

#include <optional>

// The method table for list, the arguments object, map.
//
// One arm of Evaluator::call_method, which was 715 lines in a single function
// before the 2026-08-24 split. The branch bodies below are UNCHANGED.
//
// std::nullopt means "this receiver is not mine"; an ENGAGED optional means the
// arm answered, and the ValuePtr inside may be null because that is how a failed
// method call reports itself once fail() has run. Every `return <expr>;` in the
// moved code therefore became an engaged optional with no edit at all.
//
// `arity` and `number_arg` were lambdas in the old prologue. They are still
// lambdas with the SAME NAMES and the same call syntax, so not one call site
// changed; each now forwards to the one shared implementation in methods.cpp.

namespace satellite {

std::optional<ValuePtr> Evaluator::method_containers(
    const ValuePtr &recv, const std::string &name,
    const std::vector<ValuePtr> &argv, const char *module, Span span)
{
    auto arity = [&](size_t want) {
        return method_arity(module, name, argv, want, span);
    };

    // --- search, over either container ---------------------------------------
    //
    // ONE ARM FOR BOTH, before either container's own table, because .search()
    // is not a list method that a map happens to share: it is the search power
    // reaching the domain, and the walker does not care which of the two it was
    // handed. Writing it twice would be two places to fix a ladder change.
    //
    // This is the RICH spelling (DECISION 7a) -- a map per hit with the value,
    // the key, the path and the score. The subscript form is the short one and
    // lives in subscripts.cpp.
    // map_view() and not as_map(), so a RESULT is searchable too. The result of
    // a search being searchable by the power that produced it is the promise the
    // type was shaped to keep, and this is the arm where a program spends it:
    // `found[0].search("typo")` asks which of one result's sixteen fields
    // mentions one, with no new code anywhere beneath.
    if (name == "search" && (as_list(*recv) || map_view(*recv))) {
        if (!arity(1))
            return nullptr;
        std::string error;
        ValuePtr found = search_collect(recv, argv[0], true, max_depth_, error);
        if (!found)
            fail(span, error);
        return found;
    }

    // --- orbit, over either container ---------------------------------------
    //
    // Satellite Orbit: the same question .search() answers, run through five
    // phases instead of one walk. ONE ARM FOR BOTH containers on .search()'s
    // own argument -- the walker does not care which of the two it was handed.
    //
    // A SECOND SPELLING AND NOT A REPLACEMENT. .search() is "what is in there"
    // and is one walk; .orbit() is "what do you think it is" and costs five
    // phases and a touch of the disk. Two questions, two spellings, and the
    // user picks which one they are asking -- which is also why nothing about
    // .search() or `lm[...]` moves.
    if (name == "orbit" && (as_list(*recv) || map_view(*recv))) {
        if (!arity(1))
            return nullptr;
        std::string error;
        ValuePtr found = orbit_collect(recv, argv[0], max_depth_, error);
        if (!found)
            fail(span, error);
        return found;
    }

    // --- list --------------------------------------------------------------
    if (const List *self = as_list(*recv)) {
        if (name == "length")
            return arity(0) ? make_value(Number(self->size())) : nullptr;
        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;
        // .to_string() is the list as one value -- "[a, b]" -- and .lines() is
        // the list as a listing: one element per line, with what each element
        // is when it names something on disk. Two renderings because they
        // answer two questions, and the REPL echoes a list with this one.
        if (name == "lines")
            return arity(0) ? make_value(encode_raw(list_lines(*self))) : nullptr;
        if (name == "first" || name == "last") {
            if (!arity(0))
                return nullptr;
            if (self->empty()) {
                fail(span, "satellite.container.list." + name +
                           " on an empty list");
                return nullptr;
            }
            return name == "first" ? self->front() : self->back();
        }
        if (name == "contains") {
            if (!arity(1))
                return nullptr;
            for (const ValuePtr &item : *self)
                if (item && value_equals(*item, *argv[0]))
                    return make_value(true);
            return make_value(false);
        }
    }

    // --- the arguments object -----------------------------------------------
    //
    // BEFORE the list block would have matched, and it never would: an
    // Arguments is its own variant alternative, so as_list() answers null for
    // one. The list methods are re-answered here over the COMMAND LINE HALF
    // only, which is the whole of DECISION 4 in plans/arguments.txt: every
    // program that takes arguments writes
    // `for (i = 1; i < args.length(); i++)`, and if .length() counted the
    // thirty environment entries that loop would start reading the kernel
    // release as though the user had typed it.
    //
    // So .length(), [i], .first(), .last() and .contains() answer exactly what
    // they answered before this type existed, and everything the object adds
    // is reached BY NAME.
    if (const Arguments *self = as_arguments(*recv)) {
        const List command_line = arguments_command_line(*self);

        if (name == "length")
            return arity(0) ? make_value(Number(command_line.size())) : nullptr;

        // Every entry, command line and environment together. The name is
        // different from `length` on purpose -- two counts that differ need
        // two words, and a program that means "all of them" should have to say
        // so.
        if (name == "count")
            return arity(0) ? make_value(Number(self->entries.size())) : nullptr;

        // Every name, in display order, as an ordinary list<string> -- so a
        // program can walk what it was given rather than knowing the list in
        // advance. This is what makes the object discoverable from inside the
        // language and not only from the help.
        if (name == "names") {
            if (!arity(0))
                return nullptr;
            List out;
            out.reserve(self->entries.size());
            for (const ArgumentEntry &e : self->entries)
                out.push_back(make_value(encode_raw(e.name)));
            return make_value(make_list(std::move(out)));
        }

        // The whole object, one entry per line, names beside the data. Not the
        // command line: `.to_string()` on the arguments object is what
        // satellite.console.display(args) calls, and showing only the command
        // line there would hide the thirty entries that are the point.
        if (name == "to_string" || name == "lines")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;

        if (name == "has") {
            if (!arity(1))
                return nullptr;
            const SatString *key = as_string(*argv[0]);
            if (!key) {
                fail(span, "satellite arguments .has takes a name, not " +
                           to_string(*argv[0]));
                return nullptr;
            }
            return make_value(self->index.count(decode(*key)) != 0);
        }

        if (name == "get") {
            if (!arity(1))
                return nullptr;
            const SatString *key = as_string(*argv[0]);
            if (!key) {
                fail(span, "satellite arguments .get takes a name, not " +
                           to_string(*argv[0]));
                return nullptr;
            }
            return argument_named(*self, decode(*key), span);
        }

        if (name == "first" || name == "last") {
            if (!arity(0))
                return nullptr;
            if (command_line.empty()) {
                fail(span, "satellite.container.list." + name +
                           " on an empty list");
                return nullptr;
            }
            return name == "first" ? command_line.front() : command_line.back();
        }

        if (name == "contains") {
            if (!arity(1))
                return nullptr;
            for (const ValuePtr &item : command_line)
                if (item && value_equals(*item, *argv[0]))
                    return make_value(true);
            return make_value(false);
        }
    }


    // --- settle, over either container ---------------------------------------
    //
    // Layer five: resolve, then ask again with the answer, until the question
    // stops changing. `.orbit()` is one resolution and this is a sequence of
    // them, so it is a third spelling rather than a flag on the second -- the
    // user is asking a different question ("what does this settle on") and pays
    // a different price for it, which is several full resolutions and one write
    // to memory.
    //
    // .settle(p) runs the default rounds; .settle(p, n) names the number, which
    // is the two-arity shape satellite.directory.list already has. n is refused
    // BY NAME when it is out of range, on the dial's DECISION 5b: a clamp would
    // make .settle(p, 5000) silently mean 64 and the program would never learn
    // it had asked for something the language does not have.
    if (name == "settle" && (as_list(*recv) || map_view(*recv))) {
        if (argv.empty() || argv.size() > 2) {
            fail(span, std::string(module) + ".settle takes a pattern, and "
                       "optionally how many rounds to run; got " +
                       std::to_string(argv.size()) + " arguments");
            return nullptr;
        }

        int rounds = ORBIT_SETTLE_ROUNDS;
        if (argv.size() == 2) {
            long long asked = 0;
            if (!as_index(*argv[1], asked)) {
                fail(span, std::string(module) + ".settle wants a whole "
                           "satellite.variable.number of rounds, got " +
                           to_string(*argv[1]));
                return nullptr;
            }
            rounds = static_cast<int>(
                asked > ORBIT_SETTLE_MAX ? ORBIT_SETTLE_MAX + 1 : asked);
        }

        std::string error;
        ValuePtr found =
            orbit_settle_collect(recv, argv[0], rounds, max_depth_, error);
        if (!found)
            fail(span, error);
        return found;
    }

    // --- the result --------------------------------------------------------
    //
    // satellite.container.result, what .orbit() hands back. Its own four
    // questions -- and every one of its sixteen fields by name -- live in
    // methods_result.cpp; the MAP'S read surface is borrowed here, over the
    // result's own fields, so .keys(), .values(), .get() and .length() are one
    // implementation rather than two that have to keep agreeing.
    //
    // THE MAP IS ASKED FIRST, which is the arguments object's rule one level
    // over: a method wins over a name, so a field that collided with .length()
    // would be unreachable rather than ambiguous. None of the sixteen collides,
    // and the order is kept anyway because "they cannot overlap" is a property
    // a field added later could quietly take away.
    //
    // .length() therefore counts FIELDS and not alternatives, and the surprise
    // is worth naming: `found.length()` is how many results came back, and
    // `r.length()` is how many fields one result has. The other reading would
    // make a result a map that lies about its own size while every other map
    // method went on counting fields. `r.alternatives().length()` is how a
    // program says which number it means.
    if (const ResultBody *self = as_result(*recv)) {
        if (is_map_read_method(name))
            return call_map_method(self->fields, name, argv, span);

        std::string error;
        if (std::optional<ValuePtr> answer =
                result_method(recv, name, argv, error)) {
            if (!*answer)
                fail(span, error);
            return *answer;
        }
        fail(span, "satellite.container.result has no method " + name);
        return nullptr;
    }

    // --- map ---------------------------------------------------------------
    // Delegated to src/evaluator/maps.cpp, which owns the key contract. The mutating
    // half (.set / .remove) never arrives here: is_mutator routes it to
    // call_mutator before the receiver is even evaluated, because it has to
    // write back through the receiver's storage slot.
    if (as_map(*recv))
        return call_map_method(recv, name, argv, span);
    return std::nullopt;
}

} // namespace satellite
