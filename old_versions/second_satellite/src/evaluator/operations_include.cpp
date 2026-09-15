// `satellite.include(ship(args))`, when control reaches it -- PLAN M25, built
// 2026-09-13. See evaluator/compile_includes.cpp for what the compiler decided
// and satellite_spaceship/shape.hpp for the spellings.
//
// THE AUTHOR'S THREE RULES, IN THE ORDER THIS OP KEEPS THEM:
//
//   "Match the arguments first"   -- every satellite.capsule.launch in the file
//                                    is asked whether the values fit it, before
//                                    any of them runs. Arguments that fit none
//                                    are S0734, and nothing has happened yet.
//   "then run the first launch,   -- every launch that fits runs, one after
//    then the second launch, in      another, in the order the file declares
//    that order"                     them, each handed its own copy of the
//                                    arguments.
//   loaded once                   -- the file's globals were set up when the
//                                    run started; the includes at its top run
//                                    the first time the file is included, by
//                                    whichever walk gets there first.
//
// AND NO C++ RECURSION ANYWHERE, which DESIGN §7.5 requires of every walker in
// this tree. A launch is entered through its capsule's entry op, so its frame
// sits on the machine's own stack; a launch that includes a file whose launch
// includes this one is a deeper stack and not a deeper C++ frame.

#include "evaluator/evaluator_internal.hpp"

#include "satellite_value/value.hpp"

#include <string>

namespace satellite {
namespace eval {

namespace {

// WHETHER ONE VALUE IS ONE A PARAMETER OF `type` CAN HOLD. Nothing fits every
// type, because DESIGN §6.4 q3 gives every type a nothing state; a parameter
// with no type the resolver could name -- `satellite`, or a type the value
// model has no arm for -- fits anything, since there is nothing to check it
// against and refusing would be inventing a rule.
bool holds(const Compiled &program, const Value &value, words::PathId type,
           const std::vector<uint32_t> &layouts)
{
    if (value.is_nothing() || type == words::kNoPath)
        return true;
    if (!layouts.empty()) {
        const Sui *object = std::get_if<Sui>(&value);
        if (object == nullptr || !*object)
            return false;
        for (const uint32_t layout : layouts)
            if ((*object)->layout == &program.suit_at(layout))
                return true;
        return false;
    }
    using words::NodeId;
    switch (static_cast<NodeId>(type)) {
    case NodeId::VARIABLE_NUMBER:     return value.is_number();
    case NodeId::VARIABLE_STRING:     return value.is_string();
    case NodeId::VARIABLE_BOOL:       return value.is_bool();
    // A FLOAT PARAMETER TAKES A NUMBER, which is what a literal `1.5` is --
    // compile_statements.cpp's into_declared() converts on the way in, and a
    // launch's parameter is a declaration like any other.
    case NodeId::VARIABLE_FLOAT:      return value.is_float() || value.is_number();
    case NodeId::VARIABLE_TIME:       return value.is_time();
    case NodeId::VARIABLE_BINARY:     return value.is_binary();
    case NodeId::VARIABLE_HEX:        return value.is_hex();
    case NodeId::VARIABLE_FILE:       return value.is_file();
    case NodeId::VARIABLE_THREAD:     return value.is_thread();
    case NodeId::VARIABLE_CAPSULE:    return value.is_capsule();
    case NodeId::VARIABLE_VARIANT:    return true;
    case NodeId::CONTAINER_LIST:      return value.is_list() || value.is_arguments();
    case NodeId::CONTAINER_MAP:       return value.is_map();
    case NodeId::CONTAINER_ARGUMENTS: return value.is_arguments();
    default:                          return true;
    }
}

// The top `count` values, in order, against one launch.
bool fits(const Machine &m, const Launch &launch, uint32_t count)
{
    if (launch.types.size() != count)
        return false;
    for (uint32_t i = 0; i < count; i++)
        if (!holds(m.program(), m.value_from_top(count - 1 - i), launch.types[i],
                   launch.layouts[i]))
            return false;
    return true;
}

// "(number, string)" -- what an include handed over, or what a launch takes.
std::string shape_of_values(const Machine &m, uint32_t count)
{
    std::string out = "(";
    for (uint32_t i = 0; i < count; i++) {
        if (i > 0)
            out += ", ";
        out += type_name(m.value_from_top(count - 1 - i));
    }
    return out + ")";
}

std::string shape_of_launch(const Compiled &program, const Launch &launch)
{
    std::string out = "(";
    for (size_t i = 0; i < launch.types.size(); i++) {
        if (i > 0)
            out += ", ";
        if (!launch.layouts[i].empty())
            out += program.suit_at(launch.layouts[i].front()).name;
        else if (launch.types[i] == words::kNoPath)
            out += "satellite";
        else
            out += std::string(
                words::spelling_of(static_cast<words::NodeId>(launch.types[i])));
    }
    return out + ")";
}

std::string what_it_takes(const Compiled &program, const File &file)
{
    if (file.launches.empty())
        return "it declares no satellite.capsule.launch to hand them to";
    std::string out = file.launches.size() == 1 ? "its one launch takes "
                                                : "its launches take ";
    for (size_t i = 0; i < file.launches.size(); i++) {
        if (i > 0)
            out += i + 1 == file.launches.size() ? " and " : ", ";
        out += shape_of_launch(program, file.launches[i]);
    }
    return out;
}

} // namespace

// `a` is the file, `b` the argument ops, `c` how many there are. Steps:
//
//   0          evaluate the arguments
//   1          match them; run the file's top-level includes the first time
//   2          those includes have run -- let other walks see the file loaded
//   3 + 2k     ask launch k, and every one after it, whether it fits
//   4 + 2k     launch k has returned -- drop its answer, ask from k + 1
//
// LOADED MEANS LOADED FOR EVERY WALK -- found by review. Once a program can
// start a thread, the walk that includes a file first holds the file's loading
// access until its top-level includes have run, and any other walk including
// the same file waits for it; the first version let the second walk run the
// launches while the first was still part way through. A walk including a
// file it is itself loading takes the hold again at once -- the access counts
// -- and a wait that would close a cycle between threads is refused (S1408).

void op_include(Machine &m, const Op &op, uint32_t step)
{
    const Compiled &program = m.program();
    const uint32_t count = op.c;

    if (step == 0) {
        m.again(1);
        for (uint32_t i = count; i > 0; i--)
            m.push(program.list_at(op.b, i - 1));
        return;
    }

    const File &file = program.files()[op.a];

    if (step == 1) {
        if (count > 0) {
            bool any = false;
            for (const Launch &launch : file.launches)
                any = any || fits(m, launch, count);
            if (!any) {
                m.refuse(errors::make<errors::Code::EVAL_NO_LAUNCH_FITS>(
                    m.span_of(m.here()), file.name, shape_of_values(m, count),
                    what_it_takes(program, file)));
                return;
            }
        }
        if (!m.hold_loading(op.a))
            return; // refused: a wait between threads that would never end
        if (m.globals()->claim(op.a) && file.includes != kNoOp) {
            m.again(2);
            m.push(file.includes);
            return;
        }
        step = 2;
    }

    if (step == 2) {
        m.release_loading(op.a);
        step = 3;
    }

    size_t next = 0;
    if (step % 2 == 0) {
        m.pop_value(); // what the launch answered, which nothing receives
        next = (step - 4) / 2 + 1;
    } else {
        next = (step - 3) / 2;
    }

    for (; next < file.launches.size(); next++) {
        const Launch &launch = file.launches[next];
        if (!fits(m, launch, count))
            continue;
        // A COPY OF EACH ARGUMENT, FIRST TO LAST. The originals stay below for
        // the next launch; every push moves them one further from the top, so
        // the same distance always reaches the next one.
        for (uint32_t i = 0; i < count; i++)
            m.push_value(m.value_from_top(count - 1));
        m.again(static_cast<uint32_t>(4 + 2 * next));
        // THE INCLUDE IS THE LAUNCH'S CALL SITE -- found by review, when a
        // refusal inside a launch printed a stack with no line for the
        // include that ran it.
        m.call_from(program.node_of(m.here()), program.file_of(m.here()));
        m.push(program.capsules()[launch.capsule].entry);
        return;
    }

    for (uint32_t i = 0; i < count; i++)
        m.pop_value();
    m.done();
}

} // namespace eval
} // namespace satellite
