#include "evaluator/eval_internal.hpp"

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
