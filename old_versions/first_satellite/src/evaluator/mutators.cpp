// Methods that mutate through a storage slot.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// The read-modify-write, once. See eval.hpp for why this is not three copies.
//
// A cell that holds nothing is handed to the transform as nil rather than
// rejected here, so the "cannot append to nil" wording stays with the mutator
// that knows what it was trying to do.
// l[i] = x
//
// A list is BUILT, THEN FROZEN (value.hpp), so this is a copy-and-replace and
// not an in-place poke: the transform builds a new List with one element
// changed and update_through_slot publishes it, under whatever lock that slot
// needs. Every other list mutation in the language already works this way —
// .append() is the same three lines — so this adds a spelling rather than a
// mechanism, and it inherits the write protocol instead of inventing a second
// one.
//
// O(n) per assignment, and that is the honest cost of the immutability
// contract. Filling n slots by index is O(n^2); .append() in a loop is O(n)
// amortised and is still the way to build a list.
bool Evaluator::assign_index(const Index &node, const Expr &value_expr,
                             Span span)
{
    if (!node.target || !node.subscript) {
        fail(span, "malformed index");
        return false;
    }

    // The CONTAINER's slot, not the element's — an element is not storage of
    // its own. This is also what refuses `foo()[0] = 1`: a temporary names no
    // slot, so there is nothing for the new list to be written back into.
    Slot slot = slot_of(*node.target);
    if (!slot.valid) {
        fail(node.target->span,
             "cannot assign to this expression: an index assignment writes the "
             "whole container back, so what is indexed has to be a variable");
        return false;
    }

    // Both operands BEFORE the transform, because the transform runs under a
    // lock and nothing that runs satellite code may happen inside it — which is
    // exactly what update_through_slot's own comment promises.
    ValuePtr sub = eval(*node.subscript);
    if (failed() || !sub)
        return false;
    ValuePtr value = eval(value_expr);
    if (failed() || !value)
        return false;

    return update_through_slot(
        slot, span,
        [&](const Value &current, Value &next, std::string &error) {
            // A map keeps its old answer, and keeps the old WORDS: §8.6 gives
            // it .set(key, value), and a subscript store would be a second way
            // to spell one thing. The phrase is preserved because the message
            // is what a test pins.
            if (as_map(current)) {
                error = "cannot assign to this expression: a map is written "
                        "with .set(key, value)";
                return false;
            }

            const List *list = as_list(current);
            if (!list) {
                error = "cannot assign to an index of " + to_string(current);
                return false;
            }

            long long i = 0;
            if (!as_index(*sub, i)) {
                error = "index must be a whole satellite.variable.number, got " +
                        to_string(*sub);
                return false;
            }

            // The same rules the READ side uses, deliberately identical: a
            // negative index counts from the end, and out of range is an error
            // rather than a grow. An index assignment that extended the list
            // would make `l[5] = x` on an empty list a way to create four nil
            // elements nobody asked for.
            const long long len = static_cast<long long>(list->size());
            if (i < 0)
                i += len;
            if (i < 0 || i >= len) {
                error = "index " + to_string(*sub) +
                        " is outside a list of length " + std::to_string(len);
                return false;
            }

            List copy = *list;
            copy[static_cast<size_t>(i)] = value;
            next = make_list(std::move(copy));
            return true;
        });
}

bool Evaluator::update_through_slot(
    const Slot &slot, Span span,
    const std::function<bool(const Value &, Value &, std::string &)> &transform)
{
    const Value nil{std::monostate{}};
    Value next;
    std::string error;

    if (slot.in_field()) {
        std::atomic<ValuePtr> *cell = field_cell(slot, slot.name, span);
        if (!cell)
            return false;

        // The lock covers load, transform and store. Nothing inside it runs
        // satellite code, so it cannot re-enter itself.
        std::lock_guard<std::mutex> guard(current_self_->write_lock);
        ValuePtr current = cell->load();
        if (!transform(current ? *current : nil, next, error)) {
            fail(span, error);
            return false;
        }
        cell->store(make_value(std::move(next)));
        return true;
    }

    if (slot.in_frame()) {
        ValuePtr *cell = frame_cell(slot, slot.name, span);
        if (!cell)
            return false;
        if (!transform(*cell ? **cell : nil, next, error)) {
            fail(span, error);
            return false;
        }
        *cell = make_value(std::move(next));
        return true;
    }

    Library::instance().update(
        slot.ns, slot.name, [&](const Value *current) -> Value {
            if (!current) {
                error = "no such variable: " + slot.name;
                return std::monostate{};
            }
            Value produced;
            // Re-checked under the lock rather than before it: another thread
            // may have replaced the value since we looked.
            if (!transform(*current, produced, error))
                return *current;
            return produced;
        });

    if (!error.empty()) {
        fail(span, error);
        return false;
    }
    return true;
}

ValuePtr Evaluator::call_mutator(const Expr &recv_expr, const std::string &name,
                                 const std::vector<ValuePtr> &argv_in,
                                 Span span,
                                 const std::vector<ExprPtr> *arg_exprs)
{
    // A LOCAL COPY, because a brace literal may be reshaped by the type it is
    // being handed to and the transforms below read whatever ends up here.
    // Copying a vector of ValuePtr is a refcount bump per element; the argument
    // count is 1 or 2.
    std::vector<ValuePtr> argv = argv_in;
    // A mutating method writes back, so its receiver must name storage. There
    // is nowhere to put the result of foo().append(x) (§7).
    Slot slot = slot_of(recv_expr);
    if (!slot.valid) {
        fail(recv_expr.span, "." + name +
                             "() must be called on a variable, because it "
                             "writes back through its receiver");
        return nullptr;
    }
    // Each selector belongs to exactly one container — `append` to the list,
    // `set` and `remove` to the map — but which container the RECEIVER is is a
    // question only the receiver can answer. Deriving it from the method name
    // instead was a heap-buffer-overflow read: `l.set(1, 2)` on a
    // satellite.container.list<T> took the map path and indexed args[1] on a
    // one-argument vector. Everything below keys off `declared`, and the name
    // is used only to pick which selector is being asked for.
    const Type *declared = declared_type(slot);
    const bool declared_container = declared && declared->space == "container";
    const bool declared_map = declared_container && declared->name == "map";
    const bool map_selector = (name == "set" || name == "remove");

    // A selector the receiver's container does not own is settled here, so it
    // reads the same as any other missing method rather than failing somewhere
    // deep in the transform.
    if (declared_container && declared_map != map_selector) {
        fail(span, "satellite.container." + declared->name + " has no method " +
                   name);
        return nullptr;
    }

    const char *module =
        map_selector ? "satellite.container.map" : "satellite.container.list";
    const size_t want = (name == "set") ? 2 : 1;
    if (argv.size() != want) {
        fail(span, arity_message(module, name, want, argv.size()));
        return nullptr;
    }

    // The declared element types are checked at INSERTION, which is the only
    // place a generic argument can be violated (§7).
    //
    // args[1] is read ONLY under `declared_map`, and there Resolver::check_type
    // guarantees the count is 0 or 2 — with args.empty() already excluded, that
    // means exactly 2. The guarantee is about a MAP type and was never about a
    // list, which is what the earlier form of this code got wrong.
    // `{"str1", 99}` IS A MAP HERE, and a list one line away in a
    // list<list<string>>. literals.cpp has the whole argument; what matters at
    // this site is that it runs BEFORE matches() rather than instead of it --
    // the shape is offered, and the declared type still has the last word on
    // whether what came back is acceptable.
    //
    // Gated on the argument having been WRITTEN as `{ ... }`. A list that
    // arrived through a variable is left alone, because a value that changed
    // type on the way into a call the program can read and see it did not is
    // the kind of magic §1 spends the whole language avoiding.
    if (declared_container && !declared->args.empty() && arg_exprs) {
        for (size_t i = 0; i < argv.size() && i < arg_exprs->size(); i++) {
            // Which declared argument this position is checked against: a map's
            // key is args[0] and its value args[1]; a list has only args[0].
            const size_t which = (declared_map && i == 1) ? 1 : 0;
            if (which >= declared->args.size())
                continue;
            if (!is_brace_literal((*arg_exprs)[i].get()))
                continue;
            std::string error;
            ValuePtr shaped = shape_literal(declared->args[which], argv[i],
                                            error);
            if (!shaped) {
                fail(span, error);
                return nullptr;
            }
            argv[i] = std::move(shaped);
        }
    }

    if (declared_container && !declared->args.empty()) {
        if (declared_map) {
            if (!matches(declared->args[0], *argv[0])) {
                fail(span, "cannot use " + to_string(*argv[0]) +
                           " as a key in " + unparse(*declared) + " " +
                           slot.name);
                return nullptr;
            }
            if (name == "set" && !matches(declared->args[1], *argv[1])) {
                fail(span, "cannot store " + to_string(*argv[1]) + " in " +
                           unparse(*declared) + " " + slot.name);
                return nullptr;
            }
        } else if (!matches(declared->args[0], *argv[0])) {
            fail(span, "cannot append " + to_string(*argv[0]) + " to " +
                       unparse(*declared) + " " + slot.name);
            return nullptr;
        }
    }

    // One transform per selector. Each is pure C++ over the current value, and
    // update_through_slot decides which lock, if any, it runs under.
    std::function<bool(const Value &, Value &, std::string &)> transform;

    if (name == "append") {
        transform = [&](const Value &current, Value &next,
                        std::string &error) {
            const List *list = as_list(current);
            if (!list) {
                error = "cannot append to " + to_string(current);
                return false;
            }
            List copy = *list;
            copy.push_back(argv[0]);
            next = make_list(std::move(copy));
            return true;
        };
    } else if (name == "set" || name == "remove") {
        transform = [&](const Value &current, Value &next,
                        std::string &error) {
            const MapBody *map = as_map(current);
            if (!map) {
                error = "cannot ." + name + "() on " + to_string(current);
                return false;
            }
            MapBody copy;
            const bool ok =
                (name == "set")
                    ? map_with(*map, argv[0], argv[1], copy, error)
                    : map_without(*map, argv[0], copy, error);
            if (!ok)
                return false;
            next = make_map(std::move(copy));
            return true;
        };
    } else {
        fail(span, "no mutating method " + name);
        return nullptr;
    }

    if (!update_through_slot(slot, span, transform))
        return nullptr;

    // A mutator yields nothing: the new value is already in the variable, and
    // returning it would make every append echo the whole list in the REPL.
    return make_value(std::monostate{});
}

} // namespace satellite
