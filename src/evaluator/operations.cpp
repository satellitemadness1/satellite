// The machine's expression arms. See evaluator/machine.hpp for the contract
// every one of them is written against, and closure.hpp for what an Op holds.
//
// EVERY ARM HERE LEAVES EXACTLY ONE VALUE ON THE VALUE STACK, and that is the
// whole type system of this machine. operations_control.cpp holds the arms that
// leave none. Two files rather than one because that is the line -- an
// expression answers and a statement acts -- and it is the same seam DESIGN §6
// draws between its two halves of the grammar.
//
// AN ARM NEVER CALLS ANOTHER ARM. That is the rule the explicit stack exists to
// keep: `a + b` does not evaluate `a`, it PUSHES `a` and asks to be resumed.
// The one thing that looks like an exception is a refusal, which does not
// return to its caller because nothing runs after it.
//
// AND done() COMES AFTER THE LAST THING THAT CAN REFUSE, in every arm, which is
// not a style choice. Machine::here() reads the top of the work stack, so an
// arm that pops itself before raising a diagnostic draws its caret under its
// PARENT. The order below is what puts the caret under the operator.

#include "evaluator/evaluator_internal.hpp"

#include "evaluator/dispatch.hpp"
#include "satellite_value/render.hpp"

#include <utility>

namespace satellite {
namespace eval {

namespace {

// The two operands of a binary op, in the order they were written.
//
// READ AND NOT POPPED, and both halves of that matter. The stack gives them
// back in reverse -- the arm below pushed left on top so it would run first --
// so getting the ORDER wrong is invisible for `+` and wrong for `-`, which is
// why it is one function. And they stay on the stack until the answer exists,
// because Machine::fold() then turns the two slots into one without a push:
// pop, pop, push is three vector operations where fold is an assignment and a
// pop, and the push is the one that has to ask about the ceiling.
struct Pair {
    const Value &left;
    const Value &right;
};

Pair operands(Machine &m)
{
    return {m.value_from_top(1), m.value_from_top(0)};
}

// Both operands are numbers, or the arm refuses naming the one that is not.
bool both_numbers(Machine &m, BinaryOp op, const Pair &pair, errors::Code code)
{
    const Value *wrong = nullptr;
    if (!pair.left.is_number())
        wrong = &pair.left;
    else if (!pair.right.is_number())
        wrong = &pair.right;
    if (wrong == nullptr)
        return true;

    // THE SENTENCE NAMES THE OPERATOR AND THE TYPE, NOT THE VALUE. A number
    // printed into an error message can be fifty thousand digits long (M8
    // deleted the clamp that stopped it), so what a person needs is "this one
    // is a string" rather than the string.
    errors::Diagnostic problem =
        code == errors::Code::EVAL_NOT_A_NUMBER
            ? errors::make<errors::Code::EVAL_NOT_A_NUMBER>(
                  m.span_of(m.here()), text_of(op), type_name(*wrong))
            : errors::make<errors::Code::EVAL_NOT_COMPARABLE>(
                  m.span_of(m.here()), text_of(op), type_name(*wrong));
    m.refuse(std::move(problem));
    return false;
}

} // namespace

void op_no_op(Machine &m, const Op &, uint32_t)
{
    // OP ZERO, AND IT IS THE EMPTY STATEMENT. closure.hpp keeps index 0 as a
    // no-op so kNoOp is safe to push, the way ast.hpp keeps node 0 a None node.
    // An EXPRESSION position is never kNoOp -- the compiler emits op_refuse
    // rather than nothing -- so this arm leaving no value cannot starve one.
    m.done();
}

void op_constant(Machine &m, const Op &op, uint32_t)
{
    m.done();
    m.push_value(m.program().constant(op.a));
}

void op_local(Machine &m, const Op &op, uint32_t)
{
    // DESIGN §7.2 IN ONE LINE: an integer index into this activation's own
    // storage, decided before anything ran. §7.1's receipt for what the first
    // satellite did instead is that a recursive `fact` returned 1 for every
    // input and eight threads produced 1585 wrong results out of 1600.
    m.done();
    m.push_value(m.local(op.a));
}

void op_global(Machine &m, const Op &op, uint32_t)
{
    m.done();
    m.push_value(m.global(op.a));
}

void op_unary(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        m.again(1);
        m.push(op.a);
        return;
    }

    const UnaryOp which = static_cast<UnaryOp>(op.b);
    const Value &value = m.value_from_top(0);

    if (which == UnaryOp::Negate) {
        if (!value.is_number()) {
            m.refuse(errors::make<errors::Code::EVAL_NOT_A_NUMBER>(
                m.span_of(m.here()), text_of(which), type_name(value)));
            return;
        }
        Number negated = std::get<Number>(value).negated();
        m.done();
        m.set_top(Value::number(std::move(negated)));
        return;
    }

    bool flag = false;
    if (!truth_of(value, &flag)) {
        m.refuse(errors::make<errors::Code::EVAL_NOT_A_CONDITION>(
            m.span_of(m.here()), type_name(value)));
        return;
    }
    m.done();
    m.set_top(Value::boolean(!flag));
}

void op_binary(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        // LEFT ON TOP SO IT RUNS FIRST. DESIGN §6.6 makes every level
        // left-associative and satellite has no operator with a side effect
        // yet, but evaluation ORDER is a thing a language either decides or
        // discovers later -- and C++17's indeterminate sequencing is exactly
        // what M8.5 found in the `.satc` writer's comment column.
        m.again(1);
        m.push(op.b);
        m.push(op.a);
        return;
    }

    const BinaryOp which = static_cast<BinaryOp>(op.c);
    const Pair pair = operands(m);

    // EQUALITY WORKS ON ANYTHING AND ORDERING DOES NOT, which is DESIGN §8's
    // table read honestly: two values of different types are not equal, and
    // there is no answer at all to whether a string is less than a bool.
    if (which == BinaryOp::Equal || which == BinaryOp::NotEqual) {
        const bool equal = same(pair.left, pair.right);
        m.done();
        m.fold(Value::boolean(which == BinaryOp::Equal ? equal : !equal));
        return;
    }

    if (which == BinaryOp::Less || which == BinaryOp::Greater ||
        which == BinaryOp::LessEqual || which == BinaryOp::GreaterEqual) {
        if (!both_numbers(m, which, pair, errors::Code::EVAL_NOT_COMPARABLE))
            return;
        const int order = Number::compare(std::get<Number>(pair.left),
                                          std::get<Number>(pair.right));
        bool answer = false;
        switch (which) {
        case BinaryOp::Less:         answer = order < 0; break;
        case BinaryOp::Greater:      answer = order > 0; break;
        case BinaryOp::LessEqual:    answer = order <= 0; break;
        default:                     answer = order >= 0; break;
        }
        m.done();
        m.fold(Value::boolean(answer));
        return;
    }

    if (!both_numbers(m, which, pair, errors::Code::EVAL_NOT_A_NUMBER))
        return;

    const Number &left = std::get<Number>(pair.left);
    const Number &right = std::get<Number>(pair.right);

    // DIVISION BY ZERO IS S0601 AND NOT A ROW OF ITS OWN, which is
    // FORMAT/CXX.md §1's second rule about the one place a fact lives.
    // `satl --number 1 / 0` already says this sentence and programs/
    // number_command.cpp already checks this way; a second row would be a
    // second sentence about one thing.
    if ((which == BinaryOp::Divide || which == BinaryOp::Modulo) && right.is_zero()) {
        m.refuse(errors::make<errors::Code::NUMBER_DIVIDE_BY_ZERO>(
            m.span_of(m.here()), left.to_string()));
        return;
    }

    Number answer;
    switch (which) {
    case BinaryOp::Add:      answer = Number::add(left, right); break;
    case BinaryOp::Subtract: answer = Number::sub(left, right); break;
    case BinaryOp::Multiply: answer = Number::mul(left, right); break;
    case BinaryOp::Divide:
        answer = Number::divide(left, right, m.policy().division_digits);
        break;
    default:                 answer = Number::modulo(left, right); break;
    }

    m.done();
    m.fold(Value::number(std::move(answer)));
}

void op_refuse(Machine &m, const Op &op, uint32_t)
{
    // A COMPILE-TIME DECISION RAISED AT RUN TIME, and the delay is the point.
    // A program with a `satellite.include` in a branch that never runs is a
    // program that runs; refusing it at compile time would make M9 narrower
    // than M4's parser for no reason a user could act on.
    m.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
        m.span_of(m.here()), m.program().text(op.a), m.program().text(op.b)));
}

void op_dispatch(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        m.again(1);
        const OpListId args = op.b;
        for (uint32_t i = m.program().list_size(args); i > 0; i--)
            m.push(m.program().list_at(args, i - 1));
        return;
    }

    const uint32_t count = m.program().list_size(op.b);
    const words::PathId path = op.a;

    // THE INLINE CACHE -- PLAN §2.4. The guard is the receiver's type tag, or 0
    // when nothing is bound, because a method's answer depends on what it was
    // called ON and nothing else at this milestone. The second execution of a
    // call site does no lookup at all, which is what "permanently retires the
    // seven-arm chain of §1.1" means.
    Cache &cache = m.cache(op.c);
    const uint32_t guard = 0;

    const Handler *handler = nullptr;
    if (cache.filled && cache.guard == guard) {
        handler = static_cast<const Handler *>(cache.handler);
    } else {
        handler = Handlers::table().find(path);
        if (handler != nullptr) {
            cache.handler = handler;
            cache.guard = guard;
            cache.filled = true;
        }
    }

    if (handler == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_NO_HANDLER>(
            m.span_of(m.here()), m.program().text(op.d), "a later milestone"));
        return;
    }

    if (handler->arity != kAnyArity && handler->arity != count) {
        m.refuse(errors::make<errors::Code::EVAL_ARGUMENT_COUNT>(
            m.span_of(m.here()), m.program().text(op.d),
            std::to_string(handler->arity) + " arguments", std::to_string(count)));
        return;
    }

    Value answer;
    if (!m.call_handler(handler, count, &answer))
        return;

    m.done();
    m.push_value(std::move(answer));
}

} // namespace eval
} // namespace satellite
