// The table: which registry number is which fact. See
// satellite_arguments/rows.hpp for why it is not in handlers.cpp.

#include "satellite_arguments/rows.hpp"

#include "satellite_arguments/arguments.hpp"
#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace satellite::arguments {

namespace {

using facts::MachineAnswers;
using words::NodeId;

// The body behind the process-wide value, or nullptr before start().
const Arguments *body()
{
    if (const Arg *handle = std::get_if<Arg>(&object()))
        return handle->get();
    return nullptr;
}

} // namespace

const Row kRows[kRowCount] = {
    // --- machine `1 14 1 1 1` -----------------------------------------------
    {NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE_CORES, nullptr, &MachineAnswers::cores},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE_CPU, &MachineAnswers::cpu, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE_THREADS, nullptr, &MachineAnswers::threads},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE_ARCHITECTURE, &MachineAnswers::architecture, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE_BYTE_ORDER, &MachineAnswers::byte_order, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE_PAGE_SIZE, nullptr, &MachineAnswers::page_size},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_MACHINE_POINTER_BITS, nullptr, &MachineAnswers::pointer_bits},

    // --- memory `1 14 1 1 2` ------------------------------------------------
    {NodeId::LIBRARY_MAIN_ARGUMENTS_MEMORY_TOTAL, nullptr, &MachineAnswers::memory_total_mb},

    // --- username `1 14 1 1 3` ----------------------------------------------
    {NodeId::LIBRARY_MAIN_ARGUMENTS_USERNAME, &MachineAnswers::username, nullptr},

    // --- system `1 14 1 1 4` ------------------------------------------------
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SYSTEM_NAME, &MachineAnswers::name, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SYSTEM_KERNEL, &MachineAnswers::kernel, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SYSTEM_KERNEL_VERSION, &MachineAnswers::kernel_version, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SYSTEM_DISTRIBUTION, &MachineAnswers::distribution, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SYSTEM_DISTRIBUTION_ID, &MachineAnswers::distribution_id, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SYSTEM_DISTRIBUTION_VERSION, &MachineAnswers::distribution_version, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SYSTEM_HOSTNAME, &MachineAnswers::hostname, nullptr},

    // --- build `1 14 1 1 5` -------------------------------------------------
    {NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD_COMPILER, &MachineAnswers::compiler, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD_COMPILER_VERSION, &MachineAnswers::compiler_version, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD_STANDARD, &MachineAnswers::standard, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD_FLAGS, &MachineAnswers::flags, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD_MAKE, &MachineAnswers::make, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD_STANDARD_LIBRARY, &MachineAnswers::standard_library, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD_C_LIBRARY, &MachineAnswers::c_library, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_BUILD_BUILT, &MachineAnswers::built, nullptr},

    // --- interpreter `1 14 1 1 6` -------------------------------------------
    // THE BARE WORD IS THE PATH, which is PLAN M20's own shape table and makes
    // this the one group word in the object that is a fact rather than a set.
    {NodeId::LIBRARY_MAIN_ARGUMENTS_INTERPRETER, &MachineAnswers::interpreter, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_INTERPRETER_VERSION, &MachineAnswers::version, nullptr},

    // --- process `1 14 1 1 7` -----------------------------------------------
    {NodeId::LIBRARY_MAIN_ARGUMENTS_PROCESS_ID, nullptr, &MachineAnswers::process_id},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_PROCESS_PARENT, nullptr, &MachineAnswers::parent_process_id},

    // --- session `1 14 1 1 8` -----------------------------------------------
    // Four of the five. `directory` is the OBJECT's -- it is what the program
    // was started in, and `satellite.directory.change` can move underneath it.
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SESSION_SHELL, &MachineAnswers::shell, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SESSION_TERMINAL, &MachineAnswers::terminal, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SESSION_LANGUAGE, &MachineAnswers::language, nullptr},
    {NodeId::LIBRARY_MAIN_ARGUMENTS_SESSION_HOME, &MachineAnswers::home, nullptr},
};

static_assert(sizeof kRows / sizeof kRows[0] == kRowCount,
              "satellite_arguments/rows.hpp: kRowCount and the table disagree "
              "-- a fact was appended and the count did not move");

Value answer_for(const Row &row)
{
    const MachineAnswers &answers = facts::machine_answers();
    if (row.count != nullptr)
        return Value::number(Number::from_u64(answers.*row.count));
    return Value::string(encode_raw(answers.*row.text));
}

bool answer_of(words::PathId path, Value *out)
{
    for (std::size_t i = 0; i < kRowCount; i++)
        if (static_cast<words::PathId>(kRows[i].path) == path) {
            *out = answer_for(kRows[i]);
            return true;
        }

    const Arguments *held = body();
    if (path == static_cast<words::PathId>(
                    NodeId::LIBRARY_MAIN_ARGUMENTS_SESSION_DIRECTORY)) {
        // THE OBJECT'S AND NOT THE MACHINE'S, because it is where the program
        // was STARTED and `satellite.directory.change` `1 18 1` can move the
        // process underneath it -- value_arguments.hpp carries the argument.
        // `kUnrecorded` AND NOT AN EMPTY STRING when start() has not run,
        // which is a test binary that installed the rows without a command
        // line. arguments_facts.hpp's rule: a blank value would let a program
        // believe the machine had been asked and had said nothing.
        *out = Value::string(encode_raw(
            held != nullptr ? held->directory : std::string(facts::kUnrecorded)));
        return true;
    }
    if (path ==
        static_cast<words::PathId>(NodeId::LIBRARY_MAIN_ARGUMENTS_LENGTH)) {
        // IT DESCRIBES THE COMMAND LINE RATHER THAN BEING PART OF IT, so it
        // counts the words and is not one of them -- which is also why it is a
        // sibling of `machine` and not one of the numbered positions.
        *out = Value::number(
            Number::from_u64(held != nullptr ? held->words.size() : 0));
        return true;
    }
    return false;
}

namespace {

// Every fact under `arguments`, as name and value, in REGISTRY order and never
// from a list written here. words.def is what says `machine` has seven
// children; a second list in this file would be the thing that goes stale the
// next time one is appended -- read_group()'s argument in handlers.cpp, and the
// mistake the M6 draft's `kValidProps[]` made.
//
// THE RECURSION IS TWO DEEP AND CANNOT BE MORE, which is why it is recursion at
// all: DESIGN §7.5 forbids a C++ stack over a depth the USER chooses, and this
// one is over words.def, whose shape is fixed at compile time. WORD_NUMBERS §4
// calls this subtree the deepest in the language at six numbers, and
// `arguments` sits at four of them.
void collect(NodeId parent, const std::string &prefix,
             std::vector<Entry> &into)
{
    for (words::PathId child = words::first_child(parent);
         child != words::kNoPath; child = words::next_sibling(child)) {
        const std::string_view spelling =
            words::spelling_of(static_cast<NodeId>(child));
        // The `()` row takes no position among its siblings and names no fact.
        if (spelling.empty() || spelling == "()")
            continue;

        const std::string name = prefix + std::string(spelling);
        Value value;
        // answer_of() is what leaves out the rows a later milestone owns, so
        // `interpreter.library_path` is absent from `keys()` exactly as it is
        // absent from the printed lines -- one rule, read in both places.
        if (answer_of(child, &value))
            into.push_back({name, std::move(value)});

        // A GROUP'S CHILDREN ARE ENTRIES AND THE GROUP IS NOT, because a
        // group's own answer is a map of exactly these entries -- listing both
        // would say everything twice. `interpreter` is the exception in both
        // directions: it has an answer of its own AND children, so the line
        // above emitted it and this line walks into it.
        collect(static_cast<NodeId>(child), name + ".", into);
    }
}

} // namespace

std::vector<Entry> entries_of(const Arguments &body)
{
    std::vector<Entry> out;

    // THE COMMAND LINE FIRST, IN ARGV ORDER, which is the order it was typed
    // in and the order `[i]` reads it in. `program`, then `argument_1`...,
    // so a name and an index name the same word.
    for (const CommandLineWord &word : body.words)
        out.push_back({word.name, word.value});

    collect(NodeId::LIBRARY_MAIN_ARGUMENTS, std::string(), out);
    return out;
}

} // namespace satellite::arguments
