// Methods that mutate through a storage slot.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

namespace satellite {

// The read-modify-write, once. See eval.hpp for why this is not three copies.
//
// A cell that holds nothing is handed to the transform as nil rather than
// rejected here, so the "cannot append to nil" wording stays with the mutator
// that knows what it was trying to do.
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
