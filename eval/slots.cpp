// Storage: frame slots, spacesuit fields, satellite.library globals.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

namespace satellite {

Evaluator::Slot Evaluator::slot_of(const Expr &expr)
{
    // A bare name carries its own answer: resolve() decided, statically,
    // whether it is a capsule local (a frame slot) or a top-level variable
    // (satellite.library). Nothing is looked up by string here.
    if (const Name *name = std::get_if<Name>(&expr)) {
        if (name->slot >= 0)
            return Slot{name->slot, std::string(), name->text, true};
        if (is_field_slot(name->slot))
            return Slot{name->slot, std::string(), name->text, true};
        if (name->slot == SLOT_CAPSULE || name->slot == SLOT_METHOD ||
            name->slot == SLOT_SUIT)
            return Slot{};              // none of the three names storage
        return Slot{SLOT_GLOBAL, ns_, name->text, true};
    }

    std::vector<std::string> path;
    if (flatten_path(expr, path) && path.size() == 4 && path[1] == "library")
        return Slot{SLOT_GLOBAL, path[2], path[3], true};

    return Slot{};
}

// Frames are indexed, never searched, so this is the whole of "find a local".
// The bounds check is not defensive programming for its own sake: the
// alternative to a wrong answer is a null dereference, and resolve() stamping
// a slot the frame does not have would be a bug worth naming rather than
// crashing on.
ValuePtr *Evaluator::frame_cell(const Slot &slot, const std::string &shown,
                                Span span)
{
    if (!current_ ||
        static_cast<size_t>(slot.index) >= current_->slots.size()) {
        fail(span, "internal: " + shown + " resolved to frame slot " +
                   std::to_string(slot.index) + " with no such frame");
        return nullptr;
    }
    return &current_->slots[static_cast<size_t>(slot.index)];
}

// A field is indexed into the current receiver exactly as a local is indexed
// into the current frame. The bounds check guards the same thing for the same
// reason: resolve() sized the layout, so a slot the object does not have would
// be a resolver bug worth naming rather than a null dereference.
std::atomic<ValuePtr> *Evaluator::field_cell(const Slot &slot,
                                             const std::string &shown, Span span)
{
    const size_t index = static_cast<size_t>(field_index(slot.index));
    if (!current_self_ || index >= current_self_->field_count) {
        fail(span, "internal: " + shown + " resolved to field " +
                   std::to_string(index) + " with no such receiver");
        return nullptr;
    }
    return &current_self_->fields[index];
}

ValuePtr Evaluator::read_slot(const Slot &slot, const std::string &shown,
                              Span span)
{
    if (slot.in_field()) {
        std::atomic<ValuePtr> *cell = field_cell(slot, shown, span);
        if (!cell)
            return nullptr;
        // A lock-free load, exactly as satellite.library's get() is: a reader
        // racing a writer sees the old value or the new one, never a torn one.
        ValuePtr value = cell->load();
        if (!value) {
            // Unreachable once construction has run: every field is written,
            // from its initialiser or from its type's default.
            fail(span, "internal: " + shown +
                       " is read before construction sets it");
            return nullptr;
        }
        return value;
    }

    if (slot.in_frame()) {
        ValuePtr *cell = frame_cell(slot, shown, span);
        if (!cell)
            return nullptr;
        if (!*cell) {
            // Unreachable through resolve(), which rejects a read of a name
            // before its declaration, and reports it before anything runs.
            fail(span, "internal: " + shown +
                       " is read before its declaration runs");
            return nullptr;
        }
        return *cell;
    }

    ValuePtr v = Library::instance().get(slot.ns, slot.name);
    if (!v) {
        // Two distinct non-dispatchable states, and they need different
        // messages (§7): no such variable at all, versus a declared variable
        // that holds nil. Collapsing them sends the reader hunting for a typo
        // that is not there.
        fail(span, "no such variable: " + shown);
        return nullptr;
    }
    return v;
}

bool Evaluator::write_slot(const Slot &slot, ValuePtr value, Span span)
{
    if (slot.in_field()) {
        std::atomic<ValuePtr> *cell = field_cell(slot, slot.name, span);
        if (!cell)
            return false;
        // The store alone is atomic; the lock is what makes it exclusive
        // against a mutator's read-modify-write on the same field, which is the
        // pair satellite.library's set() and update() form one level up.
        std::lock_guard<std::mutex> guard(current_self_->write_lock);
        cell->store(std::move(value));
        return true;
    }

    if (slot.in_frame()) {
        ValuePtr *cell = frame_cell(slot, slot.name, span);
        if (!cell)
            return false;
        // No lock, and none needed: a frame is reachable from exactly one
        // thread. That is the whole difference from satellite.library, and it
        // is what turns §6's 1585/1600 into 0/1600.
        *cell = std::move(value);
        return true;
    }

    Library::instance().set(slot.ns, slot.name, *value);
    return true;
}

const Type *Evaluator::declared_type(const Slot &slot) const
{
    if (slot.in_field()) {
        // Read off the RECEIVER's layout rather than the declaring suit's. The
        // two agree by construction — a subclass lays its parent's fields down
        // first, so index i names the same field in both — and taking it from
        // the object is what stays right if that ever stops being true.
        if (!current_self_ || !current_self_->suit)
            return nullptr;
        const size_t index = static_cast<size_t>(field_index(slot.index));
        if (index >= current_self_->suit->fields.size())
            return nullptr;
        return &current_self_->suit->fields[index].type;
    }

    if (slot.in_frame()) {
        if (!current_capsule_ ||
            static_cast<size_t>(slot.index) >=
                current_capsule_->slot_types.size())
            return nullptr;
        return &current_capsule_->slot_types[static_cast<size_t>(slot.index)];
    }
    auto found = declared_.find(slot.key());
    return found == declared_.end() ? nullptr : &found->second;
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

} // namespace satellite
