// The rows the evaluator dispatches: thirty-two facts, the two the object
// knows, six groups that display their children, and two that are M25's. See
// satellite_arguments/arguments.hpp for the seam and rows.hpp for the table.
//
// THE HANDLERS TAKE NO RECEIVER, which arguments.hpp argues: a fact under
// `arguments` compiles to the module-constant `op_dispatch`, there is one
// command line and one machine per run, and the object is process-wide.
//
// ONE TEMPLATE BODY AND THIRTY-TWO FUNCTION POINTERS. A handler is
// `bool (*)(Machine &, const Value *, uint32_t, Value *)` and is handed no
// path to look itself up by, so a single shared body could not tell which row
// it was dispatched for. The template's index is how each row knows which one
// it is, and it is resolved at compile time -- which is what keeps this from
// being thirty-two near-identical four-line functions.

#include "satellite_arguments/arguments.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_arguments/rows.hpp"
#include "satellite_containers/containers.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <string>
#include <utility>

namespace satellite::arguments {

namespace {

using words::NodeId;

template <std::size_t I>
bool read_fact(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = answer_for(kRows[I]);
    return true;
}

// The two the OBJECT knows rather than the machine. They go through
// answer_of() rather than repeating its two clauses, so a group's map and a
// direct read cannot disagree about what `session.directory` is.
template <NodeId Path>
bool read_of_object(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    return answer_of(static_cast<words::PathId>(Path), answer);
}

// A GROUP DISPLAYS ITS CHILDREN -- the author, 2026-09-11, asked with the
// alternatives laid out. `arguments.machine` answers all seven of its facts as
// a map, `arguments.session` all five of its, and so on for the six.
//
// THE COST WAS NAMED AND TAKEN. DESIGN §7.7 wrote `arguments.memory` as FREE
// memory and `arguments.machine` as what the processor is, and this decision
// corrects both lines: free memory is reachable through
// `satellite.system.memory.free()`, which M20 built three commits ago, and the
// processor through `arguments.machine.cpu`, which §7.7 already spells as "the
// same, asked for directly". `interpreter` is not one of the six, because PLAN
// M20's own shape table had already settled it as the path.
//
// THE CHILDREN COME OUT OF THE REGISTRY, IN REGISTRY ORDER, and never from a
// list here. words.def is what says `machine` has seven children; a second
// list in this file would be the thing that goes stale the next time one is
// appended -- the mistake the M6 draft's `kValidProps[]` made, and which
// names.cpp's arguments_member() already declines to repeat.
template <NodeId Group>
bool read_group(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    MapBody built;
    for (words::PathId child = words::first_child(Group);
         child != words::kNoPath; child = words::next_sibling(child)) {
        const std::string_view spelling =
            words::spelling_of(static_cast<NodeId>(child));
        Value value;
        // The `()` row takes no position among its siblings and names no fact;
        // answer_of() is what leaves out the rows a later milestone owns.
        if (spelling.empty() || spelling == "()" || !answer_of(child, &value))
            continue;

        // map_with RATHER THAN A DIRECT push_back, because bodies.cpp owns the
        // index's invariant and a second builder here would be a second place
        // that has to keep it. The copy it makes is over at most eight
        // entries, on a path a program reaches only by asking.
        MapBody next;
        containers::map_with(built,
                             Value::string(encode_raw(std::string(spelling))),
                             value, next);
        built = std::move(next);
    }
    *answer = Value::map(std::move(built));
    return true;
}

// M25's TWO, REFUSED WITH THE REASON RATHER THAN GUESSED AT. PLAN M20 splits
// v1's system.cpp and sends `library_path()` and its `-DSATELLITE_LIB_DIR`
// build coupling to M25, "which is where `satellite.include` of another file
// lands" -- and this tree's Makefile has no install prefix yet, so an answer
// here would be a fact about a mechanism that does not exist. The row says
// which milestone owns it, which is what `retune_min_free_mb` does one module
// over and for the same reason.
bool read_library_path(eval::Machine &machine, const Value *, uint32_t, Value *)
{
    machine.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
        machine.span_of(machine.here()), "the interpreter's library path",
        "where satl looks for another file is M25's, with `satellite.include` "
        "of one -- there is no library path to report until something reads "
        "it"));
    return false;
}

template <std::size_t... I>
void install_facts(std::index_sequence<I...>)
{
    (eval::Handlers::table().install(
         static_cast<words::PathId>(kRows[I].path),
         {read_fact<I>, false, 0, "M20"}),
     ...);
}

void install_one(NodeId path, eval::HandlerFn fn)
{
    eval::Handlers::table().install(static_cast<words::PathId>(path),
                                    {fn, false, 0, "M20"});
}

} // namespace

void install_handlers()
{
    install_facts(std::make_index_sequence<kRowCount>{});

    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_SESSION_DIRECTORY,
                read_of_object<NodeId::LIBRARY_MAIN_ARGUMENTS_SESSION_DIRECTORY>);
    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_COUNT,
                read_of_object<NodeId::LIBRARY_MAIN_ARGUMENTS_COUNT>);

    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE,
                read_group<NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE>);
    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_MEMORY,
                read_group<NodeId::LIBRARY_MAIN_ARGUMENTS_MEMORY>);
    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_SYSTEM,
                read_group<NodeId::LIBRARY_MAIN_ARGUMENTS_SYSTEM>);
    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD,
                read_group<NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD>);
    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_PROCESS,
                read_group<NodeId::LIBRARY_MAIN_ARGUMENTS_PROCESS>);
    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_SESSION,
                read_group<NodeId::LIBRARY_MAIN_ARGUMENTS_SESSION>);

    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_INTERPRETER_LIBRARY_PATH,
                read_library_path);
    install_one(NodeId::LIBRARY_MAIN_ARGUMENTS_INTERPRETER_LIBRARY_PATH_SOURCE,
                read_library_path);
}

} // namespace satellite::arguments
