// Methods that mutate through a storage slot.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

namespace satellite {

ValuePtr Evaluator::call_mutator(const Expr &recv_expr, const std::string &name,
                                 const std::vector<ValuePtr> &argv, Span span)
{
    // A mutating method writes back, so its receiver must name storage. There
    // is nowhere to put the result of foo().append(x) (§7).
    Slot slot = slot_of(recv_expr);
    if (!slot.valid) {
        fail(recv_expr.span, "." + name +
                             "() must be called on a variable, because it "
                             "writes back through its receiver");
        return nullptr;
    }
    if (argv.size() != 1) {
        fail(span, arity_message("satellite.container.list", name, 1,
                                 argv.size()));
        return nullptr;
    }

    // The declared element type is checked at INSERTION, which is the only
    // place a list's generic argument can be violated (§7).
    const Type *declared = declared_type(slot);
    if (declared && !declared->args.empty() &&
        !matches(declared->args[0], *argv[0])) {
        fail(span, "cannot append " + to_string(*argv[0]) + " to " +
                   unparse(*declared) + " " + slot.name);
        return nullptr;
    }

    // A field is read-modify-written under the object's own write_lock, which
    // is what makes the sequence indivisible against a plain assignment to the
    // same field. No satellite code runs inside it — argv was fully reduced
    // above — so §7's rule that nothing re-enters a held lock still holds.
    if (slot.in_field()) {
        std::atomic<ValuePtr> *cell = field_cell(slot, slot.name, span);
        if (!cell)
            return nullptr;

        std::lock_guard<std::mutex> guard(current_self_->write_lock);
        ValuePtr current = cell->load();
        const List *list = current ? as_list(*current) : nullptr;
        if (!list) {
            fail(span, "cannot append to " +
                       (current ? to_string(*current) : std::string("nil")));
            return nullptr;
        }
        List next = *list;
        next.push_back(argv[0]);
        cell->store(make_value(std::move(next)));
        return make_value(std::monostate{});
    }

    // A frame slot is read-modify-written in the open, with no lock and no
    // atomic. The Library's update() exists to make that sequence indivisible
    // between threads; a frame is reachable from one thread, so there is
    // nothing to make indivisible.
    if (slot.in_frame()) {
        ValuePtr *cell = frame_cell(slot, slot.name, span);
        if (!cell)
            return nullptr;
        const List *list = *cell ? as_list(**cell) : nullptr;
        if (!list) {
            fail(span, "cannot append to " +
                       (*cell ? to_string(**cell) : std::string("nil")));
            return nullptr;
        }
        List next = *list;
        next.push_back(argv[0]);
        *cell = make_value(std::move(next));
        return make_value(std::monostate{});
    }

    // Everything above ran before update(); the lambda below runs no satellite
    // code, only C++, so re-entering the same variable's write lock is
    // impossible by construction.
    std::string type_error;
    Library::instance().update(
        slot.ns, slot.name, [&](const Value *current) -> Value {
            if (!current) {
                type_error = "no such variable: " + slot.name;
                return std::monostate{};
            }
            const List *list = as_list(*current);
            if (!list) {
                // Re-checked under the lock rather than before it: another
                // thread may have replaced the value since we looked.
                type_error = "cannot append to " + to_string(*current);
                return *current;
            }
            List next = *list;
            next.push_back(argv[0]);
            return make_list(std::move(next));
        });

    if (!type_error.empty()) {
        fail(span, type_error);
        return nullptr;
    }
    // A mutator yields nothing: the new value is already in the variable, and
    // returning it would make every append echo the whole list in the REPL.
    return make_value(std::monostate{});
}

} // namespace satellite
