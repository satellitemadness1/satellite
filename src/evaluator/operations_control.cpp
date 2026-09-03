// The machine's statement and control-flow arms. See operations.cpp for the
// expression half and machine.hpp for the contract.
//
// EVERY ARM HERE LEAVES NO VALUE ON THE VALUE STACK, which is the other half of
// this machine's type system and is what makes a block a loop over its
// children with nothing to clean up between them.
//
// A LOOP DOES NOT GROW THE WORK STACK, AND THAT IS THE SENTENCE
// SCRATCH.md/NO_LIMITS.md §2.2 SPENT A SECTION ON. "A loop costs zero stack
// depth -- the frame is reused every iteration -- and that program is 103
// `while` loops. A million calls that each RETURN is depth 1." op_while below is
// that fact in code: it asks to be resumed at step 0, so the same work item is
// reused and the stack is the same height on iteration one and iteration a
// billion. Depth only grows where calls have not returned yet, which is
// op_call.

#include "evaluator/evaluator_internal.hpp"

#include "satellite_value/render.hpp"

#include <utility>

namespace satellite {
namespace eval {

namespace {

// A condition, or a refusal naming what was written instead.
//
// SATELLITE HAS NO TRUTHINESS AND THIS IS WHERE THAT IS ENFORCED. DESIGN §1.1's
// rule is never to do anything behind the user's back, and a number quietly
// standing in for a test is the oldest way a language does exactly that. Three
// arms need the check and they get it from one place, so the sentence is one
// sentence.
bool condition(Machine &m, bool *out)
{
    const Value value = m.pop_value();
    if (truth_of(value, out))
        return true;
    m.refuse(errors::make<errors::Code::EVAL_NOT_A_CONDITION>(
        m.span_of(m.here()), type_name(value)));
    return false;
}

} // namespace

void op_block(Machine &m, const Op &op, uint32_t step)
{
    // ONE WORK ITEM FOR THE WHOLE BLOCK, whatever its length. `step` is which
    // statement is next, so a body of a thousand statements is one frame here
    // and not a thousand -- the same property a recursive walker would have had
    // by looping inside one C++ frame.
    if (step >= m.program().list_size(op.a)) {
        m.done();
        return;
    }
    m.again(step + 1);
    m.push(m.program().list_at(op.a, step));
}

void op_expression(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        m.again(1);
        m.push(op.a);
        return;
    }
    // THE VALUE IS DROPPED HERE AND NOWHERE ELSE, which is what keeps the
    // value stack's height a property of the program rather than of how far it
    // has got. An expression statement is the one place a value is computed and
    // not wanted.
    m.done();
    m.pop_value();
}

void op_store(Machine &m, const Op &op, uint32_t step)
{
    // A DECLARATION WITH NO INITIALISER STORES NOTHING, AND IT STORES IT EVERY
    // TIME. DESIGN §7.4 gives a name its own slot and never reuses one across
    // scopes, so a declaration inside a loop body is the same slot on every
    // iteration -- and a variable declared without a value must be nothing on
    // iteration two as well as on iteration one. Leaving the slot alone would
    // make it hold the previous round's answer, which is the kind of thing that
    // reads as working until a loop runs twice.
    if (op.b == kNoOp) {
        m.done();
        m.set_local(op.a, Value::nothing());
        return;
    }
    if (step == 0) {
        m.again(1);
        m.push(op.b);
        return;
    }
    m.done();
    m.set_local(op.a, m.pop_value());
}

void op_store_global(Machine &m, const Op &op, uint32_t step)
{
    if (op.b == kNoOp) {
        m.done();
        m.set_global(op.a, Value::nothing());
        return;
    }
    if (step == 0) {
        m.again(1);
        m.push(op.b);
        return;
    }
    m.done();
    m.set_global(op.a, m.pop_value());
}

void op_return(Machine &m, const Op &op, uint32_t step)
{
    // NO done() ON EITHER PATH, and that is not an omission. unwind() truncates
    // the work stack to below the CALL op that started this activation, which
    // removes this op along with everything else the body had left to do -- a
    // `satellite.return` from inside three nested loops and an `if` drops all
    // four in one resize. A recursive evaluator needs an exception or a status
    // code threaded through every arm to do the same thing.
    if (op.a == kNoOp) {
        m.unwind(Value::nothing());
        return;
    }
    if (step == 0) {
        m.again(1);
        m.push(op.a);
        return;
    }
    m.unwind(m.pop_value());
}

void op_if(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        m.again(1);
        m.push(op.a);
        return;
    }
    bool taken = false;
    if (!condition(m, &taken))
        return;

    // THE `if` IS FINISHED BEFORE ITS BRANCH STARTS, which is what makes a
    // chain of `else if` cost one work item rather than one per link. The
    // branch is pushed in this op's place, not on top of it.
    m.done();
    if (taken)
        m.push(op.b);
    else if (op.c != kNoOp)
        m.push(op.c);
}

void op_while(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        m.again(1);
        m.push(op.a);
        return;
    }
    bool again = false;
    if (!condition(m, &again))
        return;
    if (!again) {
        m.done();
        return;
    }
    // BACK TO STEP 0, ON THE SAME WORK ITEM. See the file note: this is the
    // line that makes a loop cost no depth.
    m.again(0);
    m.push(op.b);
}

void op_for(Machine &m, const Op &op, uint32_t step)
{
    switch (step) {
    case 0:
        // The initialiser, once. DESIGN §6's `for` opens a scope for it, and
        // resolve has already given anything declared there a slot of its own.
        m.again(1);
        if (op.a != kNoOp)
            m.push(op.a);
        return;

    case 1:
        // A `for` WITH NO CONDITION RUNS FOREVER, which is the language's
        // answer rather than a hole: DESIGN §6's grammar makes every one of the
        // three parts optional, so `for (;;)` is writable and means what it
        // says. What ends it is a `satellite.return`, or M11's Ctrl-C.
        if (op.b == kNoOp) {
            m.again(3);
            m.push(op.d);
            return;
        }
        m.again(2);
        m.push(op.b);
        return;

    case 2: {
        bool again = false;
        if (!condition(m, &again))
            return;
        if (!again) {
            m.done();
            return;
        }
        m.again(3);
        m.push(op.d);
        return;
    }

    default:
        // The body has run; the step expression, then the condition again.
        m.again(1);
        if (op.c != kNoOp)
            m.push(op.c);
        return;
    }
}

void op_call(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        // ARGUMENTS IN REVERSE SO THEY EVALUATE FORWARDS. The stack hands them
        // back in the order they were pushed on top of each other, so the last
        // argument goes down first -- M8.5 §3.3 found the same inversion in the
        // printers and answered it the same way, by naming the pieces in source
        // order in exactly one place.
        m.again(1);
        for (uint32_t i = m.program().list_size(op.b); i > 0; i--)
            m.push(m.program().list_at(op.b, i - 1));
        return;
    }

    if (step == 1) {
        const Capsule &target = m.program().capsules()[op.a];
        const uint32_t count = m.program().list_size(op.b);
        if (count != target.parameters) {
            m.refuse(errors::make<errors::Code::EVAL_ARGUMENT_COUNT>(
                m.span_of(m.here()), m.program().text(op.c),
                arity_text(target.parameters),
                std::to_string(count)));
            return;
        }
        m.again(2);
        m.enter(op.a, count, m.program().node_of(m.here()));
        return;
    }

    // THE BODY RAN OFF ITS END, so the capsule returned nothing. This op is
    // still here because unwind() is what removes it, and a real
    // `satellite.return` reaches unwind() first and this arm never runs.
    m.unwind(Value::nothing());
}

void op_enter(Machine &m, const Op &op, uint32_t step)
{
    // op_call WITH THE ARGUMENTS ALREADY EVALUATED -- see Capsule::entry.
    // Machine::call() puts them on the value stack and pushes this.
    if (step == 0) {
        m.again(1);
        m.enter(op.a, m.program().capsules()[op.a].parameters, kNoNode);
        return;
    }
    m.unwind(Value::nothing());
}

const char *op_name(OpFn fn)
{
    // A CHAIN AND NOT A TABLE, because the key is a function ADDRESS and there
    // is nothing to index by. It is read by `satl --compile` and by
    // tests/eval_test, which asserts every arm declared in
    // evaluator_internal.hpp has a row here -- that assertion is what a
    // -Wswitch would have given if this could have been a switch.
    if (fn == op_no_op)        return "no_op";
    if (fn == op_constant)     return "constant";
    if (fn == op_local)        return "local";
    if (fn == op_global)       return "global";
    if (fn == op_unary)        return "unary";
    if (fn == op_binary)       return "binary";
    if (fn == op_call)         return "call";
    if (fn == op_enter)        return "enter";
    if (fn == op_dispatch)     return "dispatch";
    if (fn == op_refuse)       return "refuse";
    if (fn == op_block)        return "block";
    if (fn == op_expression)   return "expression";
    if (fn == op_store)        return "store";
    if (fn == op_store_global) return "store_global";
    if (fn == op_return)       return "return";
    if (fn == op_if)           return "if";
    if (fn == op_while)        return "while";
    if (fn == op_for)          return "for";
    return "?";
}

} // namespace eval
} // namespace satellite
