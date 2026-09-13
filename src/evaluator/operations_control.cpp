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

#include "evaluator/dispatch.hpp"

#include "satellite_spacesuit/suit_object.hpp"
#include "satellite_thread/thread_handle.hpp"
#include "satellite_value/render.hpp"

#include <memory>
#include <string>
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

    // THE STATEMENT BOUNDARY, WHICH IS WHERE CTRL-C LANDS -- M11, and v1's
    // promise ported with its handler: "the first SIGINT sets the flag and
    // lets the walk stop itself at the next statement, which is what makes an
    // interrupted program report the line it was on." The caret goes under the
    // statement that did NOT run.
    const OpIndex next = m.program().list_at(op.a, step);
    if (m.interrupted(next))
        return;

    m.again(step + 1);
    m.push(next);
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

void op_retune(Machine &m, const Op &op, uint32_t step)
{
    // M15's RETUNE -- an assignment whose target is a numbered language path.
    // `a` is the PathId, `b` the value's op, `c` the canonical spelling for
    // the sentence below (compile_expressions' op_dispatch takes the same
    // care and says why the spelling is the registry's).
    //
    // THE LOOKUP HAPPENS AT RUN TIME, op_refuse's argument yet again: a
    // retune in a branch that never runs is a program that runs. A path with
    // no write row answers S0724, which is S0721's write-side twin and can
    // loosen the same way -- dispatch.hpp's Assigners note carries the
    // mechanism's whole argument.
    if (step == 0) {
        m.again(1);
        m.push(op.b);
        return;
    }

    const Assigner *row =
        Assigners::table().find(static_cast<words::PathId>(op.a));
    if (row == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_NOT_RETUNABLE>(
            m.span_of(m.here()), m.program().text(op.c)));
        return;
    }

    const Value &value = m.value_from_top(0);
    Value unused;
    if (!row->fn(m, &value, 1, &unused))
        return; // the row refused and said why, with this op's span

    m.done();
    m.pop_value();
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
        // ONCE PER ITERATION, BECAUSE STEP 0 COMES ROUND ONCE PER ITERATION --
        // the loop below resumes itself at 0, so this line is the boundary
        // that stops `while` bodies whose statements are too quick to catch,
        // and the first walk in this tree long enough to be stopped at all
        // (PLAN §8's M11 entry). An empty body loops through here too, which
        // is what makes `while` with nothing in it interruptible rather than
        // immortal.
        if (m.interrupted(m.here()))
            return;
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
        // THE ITERATION BOUNDARY, THE SAME LINE op_while HAS AND FOR THE SAME
        // REASON -- every iteration passes through case 1 whether or not the
        // loop has a condition, so this is the one place that catches both.
        if (m.interrupted(m.here()))
            return;
        // A `for` WITH NO CONDITION RUNS FOREVER, which is the language's
        // answer rather than a hole: DESIGN §6's grammar makes every one of the
        // three parts optional, so `for (;;)` is writable and means what it
        // says. What ends it is a `satellite.return`, or Ctrl-C -- built at
        // M11, on this line.
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

// op_call WITH THE ENTER REMOVED -- M23, and the whole of DESIGN §13's deferred
// call at run time.
//
//     satellite.variable.thread t = satellite.thread.new(capsule_test(word))
//
// `word` is evaluated HERE, on this thread, at this moment; `capsule_test` is
// not entered. What goes on the value stack is a `satellite.variable.capsule`
// `1 6 16` holding the capsule index and the argument values, frozen -- DESIGN
// §13: "the handler evaluates the arguments and stores (capsule number,
// argument values) for the thread to run later."
//
// SO IT IS op_call's STEP 0 AND op_call's ARITY CHECK, AND THEN NOT op_call's
// enter(). Reading the two side by side is the point: every line they share is
// a line a deferred call has to agree with a performed one about, and the one
// they do not share is the one word that makes this milestone. The arity check
// is S0722 for op_call's reason exactly -- it is the same question about the
// same capsule, and a second code would be a second sentence for one mistake.
//
// AND THERE IS NO FRAME, WHICH IS WHY THIS OP IS SAFE TO BE WRONG ABOUT. If
// nothing ever starts the thread, all that happened is that some arguments were
// evaluated and a value was built. A packaged call that is never run costs what
// its arguments cost and nothing else -- no slot is taken, no frame is pushed,
// and unwind() never has to know this op existed.
void op_package(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        // ARGUMENTS IN REVERSE SO THEY EVALUATE FORWARDS -- op_call's line and
        // op_call's reason, thirty lines up.
        m.again(1);
        for (uint32_t i = m.program().list_size(op.b); i > 0; i--)
            m.push(m.program().list_at(op.b, i - 1));
        return;
    }

    const Capsule &target = m.program().capsules()[op.a];
    const uint32_t count = m.program().list_size(op.b);
    if (count != target.parameters) {
        m.refuse(errors::make<errors::Code::EVAL_ARGUMENT_COUNT>(
            m.span_of(m.here()), m.program().text(op.c),
            arity_text(target.parameters), std::to_string(count)));
        return;
    }

    // TAKEN OFF IN ORDER, WHICH IS BACKWARDS FROM THE STACK. The last argument
    // is on top, so the vector is filled from its end -- enter() does the same
    // thing into slots and this is that loop with a different destination.
    auto packaged = std::make_shared<thread::Deferred>();
    packaged->capsule = op.a;
    packaged->name = std::string(m.program().text(op.c));
    packaged->arguments.resize(count);
    for (uint32_t i = count; i > 0; i--)
        packaged->arguments[i - 1] = m.pop_value();

    m.done();
    m.push_value(Value(Cap(std::move(packaged))));
}

// A SPACESUIT, CONSTRUCTED -- M26. `a` is the layout, `b` is one op per field:
// the initialiser the suit declared, or an op pushing nothing.
//
// ONE OP PER FIELD AND NOT A LOOP OVER "has an initialiser", which is what
// makes this arm short. A field with no `=` gets an op that pushes nothing, so
// the count on the stack is the field count by construction and the arm below
// has no case to get wrong -- the same trick op_call uses by evaluating every
// argument rather than only the interesting ones.
//
// AND THE INITIALISERS RUN AT CONSTRUCTION, ONCE PER OBJECT. They are ordinary
// expressions compiled in the suit's own scope, so `satellite.variable.number
// data_id = 0` is a zero stored into slot 0 of every object built, and two
// objects never share it. That is the difference DESIGN §7.1 is a receipt for,
// one level up from the frames: the first satellite's registry gave every
// instance one storage cell between them.
void op_construct(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        // IN REVERSE SO THEY EVALUATE FORWARDS -- op_call's line and op_call's
        // reason, and it matters here for the same reason: a field initialiser
        // may call something that prints.
        m.again(1);
        for (uint32_t i = m.program().list_size(op.b); i > 0; i--)
            m.push(m.program().list_at(op.b, i - 1));
        return;
    }

    const suit::Layout &layout = m.program().suit_at(op.a);
    const uint32_t count = m.program().list_size(op.b);

    auto made = std::make_shared<suit::SuitObject>();
    made->layout = &layout;
    made->fields.resize(count);
    for (uint32_t i = count; i > 0; i--)
        made->fields[i - 1] = m.pop_value();

    m.done();
    m.push_value(Value(Sui(std::move(made))));
}

// A FIELD, READ THROUGH THE RECEIVER AT SLOT 0 -- M26.
//
// TWO INDIRECTIONS AND NO NAME ANYWHERE, which is DESIGN §7.1's argument
// arriving one level up from where it was made. Resolve turned `n` into a
// field index before the program started, and slot 0 is the receiver because
// resolve put it there -- so reading a field is a slot read and a vector index,
// with nothing hashed and nothing compared.
//
// THE GUARD IS NOT DEFENSIVE. Slot 0 of a method's frame is filled by the call
// that entered it, and a method can only be entered through a call that pushed
// a receiver -- so a non-suit here would be a miscompile rather than a
// program's mistake. It is checked because the alternative is dereferencing
// whatever is there, and because `satellite_value/value.hpp`'s as_list note
// makes the same argument about a producer this module cannot see.
void op_field(Machine &m, const Op &op, uint32_t)
{
    const Sui *held = std::get_if<Sui>(&m.local(0));
    if (held == nullptr || !*held || op.a >= (*held)->fields.size()) {
        m.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
            m.span_of(m.here()), "a field read outside a spacesuit method",
            "no milestone -- resolve puts the receiver at slot 0"));
        return;
    }
    m.done();
    m.push_value((*held)->fields[op.a]);
}

// A FIELD, WRITTEN THROUGH THE RECEIVER -- M26, and THIS IS WHERE REFERENCE
// SEMANTICS ACTUALLY HAPPENS.
//
// IT WRITES THROUGH THE HANDLE AND NOT BACK INTO A SLOT, which is the whole
// difference from every mutating method the language already has. A list is
// frozen and a mutation is "a copy published whole through the receiver's
// storage slot" (DESIGN §6.4) -- so `l.append(x)` changes what the CALLER's
// name holds and nobody else's. A spacesuit is a reference type, so this
// changes the object, and every name holding that object sees it. That is
// PLAN §8's M26 done-when in one line: "passes one into a capsule that mutates
// it, and proves the caller sees the mutation".
void op_field_store(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0 && op.b != kNoOp) {
        m.again(1);
        m.push(op.b);
        return;
    }

    const Sui *held = std::get_if<Sui>(&m.local(0));
    if (held == nullptr || !*held || op.a >= (*held)->fields.size()) {
        m.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
            m.span_of(m.here()), "a field write outside a spacesuit method",
            "no milestone -- resolve puts the receiver at slot 0"));
        return;
    }

    (*held)->fields[op.a] = op.b == kNoOp ? Value::nothing() : m.pop_value();
    m.done();
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
    if (fn == op_package)      return "package";
    if (fn == op_construct)    return "construct";
    if (fn == op_field)        return "field";
    if (fn == op_field_store)  return "field_store";
    if (fn == op_dispatch)     return "dispatch";
    if (fn == op_method)       return "method";
    if (fn == op_method_global) return "method_global";
    if (fn == op_method_field)  return "method_field";
    if (fn == op_refuse)       return "refuse";
    if (fn == op_no_question)  return "no_question";
    if (fn == op_misuse)       return "misuse";
    if (fn == op_options)      return "options";
    if (fn == op_place)        return "place";
    if (fn == op_place_global) return "place_global";
    if (fn == op_block)        return "block";
    if (fn == op_expression)   return "expression";
    if (fn == op_store)        return "store";
    if (fn == op_store_global) return "store_global";
    if (fn == op_to_float)     return "to_float";
    if (fn == op_retune)       return "retune";
    if (fn == op_return)       return "return";
    if (fn == op_if)           return "if";
    if (fn == op_while)        return "while";
    if (fn == op_for)          return "for";
    return "?";
}

} // namespace eval
} // namespace satellite
