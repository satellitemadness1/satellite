// The machine's dispatching arms -- `handlers[path_id]` at run time, in three
// ops that share one core. See operations.cpp for the rest of the expression
// half and evaluator/dispatch.hpp for what a Handler is.
//
// THREE OPS BECAUSE THERE ARE THREE ANSWERS TO "WHERE DOES A CHANGED RECEIVER
// GO". op_dispatch is a call on a module path -- `satellite.console.display`
// -- or a module constant read bare -- `satellite.bool.true` -- and has no
// receiver anywhere, so its fourth operand is the callee's compiled spelling.
// op_method and op_method_global are DESIGN §6.4's sugar compiled down:
// `s.upper()` is `satellite.variable.string.upper(s)` in the table, the
// receiver rides as argument 0, and the fourth operand is the SLOT the
// receiver lives in -- which is what a row whose `mutates` flag is set writes
// the answer back to, and the only thing separating the two method ops is
// whether that slot is in the frame or among the globals.
//
// A MUTATING METHOD'S ANSWER IS ITS RECEIVER'S NEW VALUE -- M11's decision,
// recorded here because this is the line that enacts it. The handler answers
// what the receiver became, the core stores that same value into the slot,
// and the expression's value is the new string -- so `s.append("!")` in
// statement position mutates and drops the copy, while `t = s.clear()` means
// what it reads as. One value, two destinations, no second contract.

#include "evaluator/evaluator_internal.hpp"

#include "evaluator/dispatch.hpp"

#include <string>

namespace satellite {
namespace eval {

namespace {

// Where a changed receiver would be written -- or, for the two Place arms,
// where the ANSWER goes: M14's `input(prompt, target)` `1 5 4` writes the
// line it read into a slot the compiler already resolved, and the expression
// yields nothing. words.def's place list is the declaration; the compiler is
// what keeps a non-name out of the slot operand, so by the time either Place
// arm runs, `op.d` names storage the way a method's receiver does.
// WHERE A MUTATING METHOD PUBLISHES ITS ANSWER BACK TO -- DESIGN §6.4's
// storage-slot rule, and `Field` is M26's fourth answer to it. A frame slot, a
// global, and now a FIELD of the spacesuit whose method is running: the object
// is at slot 0 (resolve puts it there) and the index was decided before the
// program started, so writing back is two indirections and no lookup.
enum class Target : uint8_t {
    None, Local, Global, Field, PlaceLocal, PlaceGlobal
};

// What the refusal sentences call the callee. op_dispatch compiled its
// spelling into the text table; the method and place ops read the selector
// off their own node, because their fourth operand is spent on the slot.
std::string callee(Machine &m, const Op &op, Target target)
{
    if (target == Target::None)
        return m.program().text(op.d);
    return std::string(m.text_of(m.here()));
}

void dispatch(Machine &m, const Op &op, uint32_t step, Target target)
{
    if (step == 0) {
        // ARGUMENTS IN REVERSE SO THEY EVALUATE FORWARDS -- op_call's note.
        // For a method the compiler put the receiver FIRST in this list, so it
        // is pushed last, runs first, and sits under its arguments exactly
        // where DESIGN §6.4's written-out form says it goes.
        m.again(1);
        for (uint32_t i = m.program().list_size(op.b); i > 0; i--)
            m.push(m.program().list_at(op.b, i - 1));
        return;
    }

    const uint32_t count = m.program().list_size(op.b);
    const words::PathId path = op.a;

    // THE INLINE CACHE -- PLAN §2.4. The guard is the receiver's type tag, or
    // 0 when nothing is bound; at this milestone every site is monomorphic by
    // construction -- resolve folded the selector through the DECLARED type --
    // so the cell's whole job is that the second execution of a call site does
    // no lookup at all, which is what "permanently retires the seven-arm chain
    // of §1.1" means.
    Cache &cache = m.cache(op.c);
    const Handler *handler = nullptr;
    if (cache.filled) {
        handler = static_cast<const Handler *>(cache.handler);
    } else {
        handler = Handlers::table().find(path);
        if (handler != nullptr) {
            cache.handler = handler;
            cache.guard = 0;
            cache.filled = true;
        }
    }

    if (handler == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_NO_HANDLER>(
            m.span_of(m.here()), callee(m, op, target), "a later milestone"));
        return;
    }

    // A METHOD REACHED WITHOUT A RECEIVER IS THE WRITTEN-OUT SPELLING, which
    // DESIGN §6.4 keeps off the surface: "the right-hand side is NOT surface
    // syntax -- no program may write it". The compiler cannot enforce that --
    // it does not know which rows bind a receiver -- so the row itself does,
    // here, and S0718 says how the method is actually asked.
    if (handler->binds_receiver && target == Target::None) {
        m.refuse(errors::make<errors::Code::EVAL_NEEDS_RECEIVER>(
            m.span_of(m.here()), callee(m, op, target)));
        return;
    }

    if (handler->arity != kAnyArity && handler->arity != count) {
        // THE SENTENCE COUNTS WHAT THE USER WROTE. A receiver-bound row's
        // arity includes argument 0, and "takes 2 arguments" about a method
        // the user called with one WRITTEN argument would send them counting
        // the wrong things -- so the hidden argument comes off both numbers.
        const uint32_t hidden = handler->binds_receiver ? 1 : 0;
        m.refuse(errors::make<errors::Code::EVAL_ARGUMENT_COUNT>(
            m.span_of(m.here()), callee(m, op, target),
            arity_text(handler->arity - hidden),
            std::to_string(count > hidden ? count - hidden : 0)));
        return;
    }

    if (handler->mutates && target == Target::None) {
        // DESIGN §6.4's last line, enforced by the op rather than remembered
        // by every install site: "a mutating method needs a receiver that
        // names a storage slot ... there is nowhere to write back."
        m.refuse(errors::make<errors::Code::EVAL_NOWHERE_TO_WRITE>(
            m.span_of(m.here()), callee(m, op, target)));
        return;
    }

    Value answer;
    if (!m.call_handler(handler, count, &answer))
        return;

    m.done();

    if (target == Target::PlaceLocal || target == Target::PlaceGlobal) {
        // THE WRITE IS SKIPPED WHEN THE ANSWER IS NOTHING, and that is the
        // interrupt contract rather than a convenience: a Ctrl-C at the
        // prompt answers nothing and the walk stops at the next boundary --
        // overwriting the place on the way out would destroy a value the
        // person cancelled INTO. A real empty line is an empty string, not
        // nothing, so return-pressed still writes. And the expression yields
        // nothing always -- "writes a place and returns nothing" -- with
        // S1003 refusing at compile every position that could read it.
        if (!answer.is_nothing()) {
            if (target == Target::PlaceLocal)
                m.set_local(op.d, std::move(answer));
            else
                m.set_global(op.d, std::move(answer));
        }
        m.push_value(Value::nothing());
        return;
    }

    if (handler->mutates) {
        if (target == Target::Local) {
            m.set_local(op.d, answer);
        } else if (target == Target::Field) {
            // THE GUARD IS op_field's GUARD AND IS NOT DEFENSIVE. Slot 0 holds
            // the receiver because a method can only be entered through a call
            // that pushed one, so a non-suit here is a miscompile rather than a
            // program's mistake -- and the alternative is dereferencing
            // whatever is there.
            // `const Sui *` AND THE MUTATION IS STILL LEGAL, which is
            // op_field_store's trick one file over: the handle is const, the
            // object it points at is not. That IS reference semantics -- the
            // slot is not being rewritten, the thing in it is being changed,
            // and every other name holding that object sees it.
            const Sui *held = std::get_if<Sui>(&m.local(0));
            if (held == nullptr || !*held || op.d >= (*held)->fields.size()) {
                m.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
                    m.span_of(m.here()),
                    "a method on a field outside a spacesuit method",
                    "no milestone -- resolve puts the receiver at slot 0"));
                return;
            }
            (*held)->fields[op.d] = answer;
        } else {
            m.set_global(op.d, answer);
        }
    }
    m.push_value(std::move(answer));
}

} // namespace

void op_dispatch(Machine &m, const Op &op, uint32_t step)
{
    dispatch(m, op, step, Target::None);
}

void op_method(Machine &m, const Op &op, uint32_t step)
{
    dispatch(m, op, step, Target::Local);
}

void op_method_global(Machine &m, const Op &op, uint32_t step)
{
    dispatch(m, op, step, Target::Global);
}

// A METHOD ON A FIELD OF THE SPACESUIT THIS METHOD BELONGS TO -- M26.
// `class_dna.append(x)` inside one of the suit's own capsules, which is how
// every suit in the author's infinity_data_main.satl is written and what a
// spacesuit is FOR: a list nobody outside can reach, changed by the capsules
// that own it. Without this the call compiled to a bare dispatch with no
// receiver and answered S0718 -- "`append` is a method and is asked on a
// value" -- six frames into the program.
void op_method_field(Machine &m, const Op &op, uint32_t step)
{
    dispatch(m, op, step, Target::Field);
}

void op_place(Machine &m, const Op &op, uint32_t step)
{
    dispatch(m, op, step, Target::PlaceLocal);
}

void op_place_global(Machine &m, const Op &op, uint32_t step)
{
    dispatch(m, op, step, Target::PlaceGlobal);
}

} // namespace eval
} // namespace satellite
