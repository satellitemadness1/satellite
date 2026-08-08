// Capsule calls, construction, and object method dispatch.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

namespace satellite {

namespace {

// Everything that belongs to the CALLER, saved for the length of one
// activation and put back afterwards: its frame, the capsule that frame
// belongs to, and any satellite.return it had pending.
//
// RAII rather than three assignments after the body, because an error inside
// the body leaves through the same path a clean return does, and a frame left
// current after its C++ storage has gone is a dangling pointer.
struct Activation {
    Activation(Frame *&current, const CapsuleInfo *&capsule, ValuePtr &returned,
               ObjectPtr &self, const SpacesuitInfo *&suit,
               Frame *frame, const CapsuleInfo *info, ObjectPtr receiver,
               const SpacesuitInfo *lexical)
        : current_(current), capsule_(capsule), returned_(returned),
          self_(self), suit_(suit),
          saved_frame_(current), saved_capsule_(capsule),
          saved_return_(std::move(returned)), saved_self_(std::move(self)),
          saved_suit_(suit)
    {
        current_ = frame;
        capsule_ = info;
        returned_ = nullptr;
        self_ = std::move(receiver);
        suit_ = lexical;
    }

    ~Activation()
    {
        current_ = saved_frame_;
        capsule_ = saved_capsule_;
        returned_ = std::move(saved_return_);
        self_ = std::move(saved_self_);
        suit_ = saved_suit_;
    }

    Activation(const Activation &) = delete;
    Activation &operator=(const Activation &) = delete;

private:
    Frame *&current_;
    const CapsuleInfo *&capsule_;
    ValuePtr &returned_;
    ObjectPtr &self_;
    const SpacesuitInfo *&suit_;

    Frame *saved_frame_;
    const CapsuleInfo *saved_capsule_;
    ValuePtr saved_return_;
    ObjectPtr saved_self_;
    const SpacesuitInfo *saved_suit_;
};

} // namespace

ValuePtr Evaluator::call_capsule(const CapsuleInfo &info,
                                 const std::string &name,
                                 const std::vector<ValuePtr> &argv, Span span,
                                 ObjectPtr self, const SpacesuitInfo *suit)
{
    if (!info.capsule || !info.capsule->body) {
        fail(span, "capsule " + name + " has no body");
        return nullptr;
    }

    // resolve() checks this statically for a call it can see; the entry point
    // is called by the runtime, which it cannot.
    if (argv.size() != info.param_count) {
        fail(span, name + " takes " + std::to_string(info.param_count) +
                   (info.param_count == 1 ? " argument, got "
                                          : " arguments, got ") +
                   std::to_string(argv.size()));
        return nullptr;
    }

    // THE frame: a plain local, so one activation costs one C++ stack frame
    // and one vector, and both die with the call. No mutex and no atomic —
    // it is reachable from exactly one thread, which is the entire difference
    // from satellite.library and what turns §6's 1585/1600 into 0/1600.
    //
    // resolve() counted the slots, so this sizes once and never grows.
    Frame frame;
    frame.slots.resize(info.slot_count);

    // Parameters are slots [0, param_count), in source order — the whole of
    // "bind the arguments".
    for (size_t i = 0; i < info.param_count; i++) {
        if (!argv[i] || !matches(info.slot_types[i], *argv[i])) {
            fail(span, "cannot pass " +
                       (argv[i] ? to_string(*argv[i]) : std::string("nothing")) +
                       " as " + unparse(info.slot_types[i]) + " " +
                       info.slot_names[i]);
            return nullptr;
        }
        frame.slots[i] = argv[i];
    }

    // The receiver rides alongside the frame and is saved and restored with
    // it: a method called from a method must see its own object again when the
    // inner one returns.
    Activation activation(current_, current_capsule_, returned_, current_self_,
                          current_suit_, &frame, &info, std::move(self), suit);

    Flow flow = exec(*info.capsule->body);
    if (failed())
        return nullptr;

    // Falling off the end returns nil, so a capsule with no satellite.return
    // still has a value — and satellite.return() with no operand already
    // produced one.
    if (flow == Flow::Return && returned_)
        return returned_;
    return make_value(std::monostate{});
}

// ---------------------------------------------------------------------------
// Spacesuits
// ---------------------------------------------------------------------------

ValuePtr Evaluator::construct(const SpacesuitInfo &suit,
                              const std::vector<ValuePtr> &argv, Span span)
{
    if (depth_ >= max_depth_) {
        // Reachable by a field initialiser that builds its own spacesuit, which
        // is a program that describes an infinitely deep object.
        fail(span, "constructing " + suit.name + " nests deeper than "
                   "satellite.library.system.max_depth (" +
                   std::to_string(max_depth_) + ")");
        return nullptr;
    }
    DepthGuard guard(depth_);

    ObjectPtr object = std::make_shared<Object>(suit.fields.size());
    object->suit = &suit;

    {
        // Initialisers run with the new object as receiver and NO frame: a
        // field is not a local of anything, and construction happens at a
        // declaration rather than inside a call. Superclass fields come first
        // in the layout, so a superclass initialises before its descendant —
        // the only order in which an inherited initialiser cannot read a field
        // its subclass has not written yet.
        Activation activation(current_, current_capsule_, returned_,
                              current_self_, current_suit_, nullptr, nullptr,
                              object, &suit);

        for (size_t i = 0; i < suit.fields.size(); i++) {
            const FieldInfo &field = suit.fields[i];

            // The declaring suit, not `suit`: an inherited initialiser is code
            // written inside its own spacesuit and sees what that suit sees.
            current_suit_ = field.owner ? field.owner : &suit;

            Value value = default_of(field.type);
            if (field.init) {
                ValuePtr init = eval(*field.init);
                if (failed() || !init)
                    return nullptr;
                value = *init;
                if (!matches(field.type, value)) {
                    fail(field.init->span,
                         "cannot initialise " + unparse(field.type) + " " +
                         field.name + " with " + to_string(value));
                    return nullptr;
                }
            }
            // No lock: nothing else can reach the object yet.
            object->fields[i].store(make_value(std::move(value)));
        }
    }

    if (suit.ctors.empty() && !argv.empty()) {
        // resolve() catches this at a call it can see.
        fail(span, suit.name + " has no constructor, so it takes no arguments");
        return nullptr;
    }

    // Constructors run after EVERY field is initialised, superclass first. A
    // constructor therefore sees the whole object already at its declared
    // values, including its subclass's fields — which is exactly what C++'s
    // interleaved order cannot offer, and the reason not to copy that order.
    static const std::vector<ValuePtr> none;
    for (size_t i = 0; i < suit.ctors.size(); i++) {
        const MethodInfo &ctor = suit.ctors[i];

        // Only the most derived constructor is handed the site's arguments;
        // resolve() has already required every earlier one to take none.
        const std::vector<ValuePtr> &args =
            (i + 1 == suit.ctors.size()) ? argv : none;

        if (ctor.access != Access::Public) {
            const bool related =
                current_suit_ && (suit.is_a(current_suit_->name) ||
                                  current_suit_->is_a(suit.name));
            if (!related) {
                fail(span, suit.name +
                           "'s constructor is satellite.protected, so an "
                           "instance can only be built from inside " +
                           (ctor.owner ? ctor.owner->name : suit.name) +
                           " or a spacesuit that inherits from it");
                return nullptr;
            }
        }

        call_capsule(*ctor.info, suit.name + "'s constructor", args, span,
                     object, ctor.owner);
        if (failed())
            return nullptr;
    }

    return make_value(Value(std::move(object)));
}

ValuePtr Evaluator::call_object_method(const ObjectPtr &object,
                                       const std::string &name,
                                       const std::vector<ValuePtr> &argv,
                                       Span span)
{
    if (!object || !object->suit) {
        fail(span, "nil has no methods, so ." + name + "() has no receiver");
        return nullptr;
    }

    // Dispatch on the OBJECT's spacesuit. The table is flattened, so an
    // inherited method is found in one lookup and an override replaces it — the
    // whole of virtual dispatch, with no chain walked at the call.
    const MethodInfo *method = object->suit->find_method(name);
    if (!method) {
        fail(span, object->suit->name + " has no method " + name);
        return nullptr;
    }

    // A protected method is reachable from code inside a RELATED spacesuit:
    // the receiver's own, an ancestor of it, or a descendant. Judged on the
    // suits rather than on the method, because after an override the method
    // found belongs to a subclass the caller may never have heard of, and it
    // is the name the caller could see that decides what it may call.
    if (method->access != Access::Public) {
        const bool related =
            current_suit_ && (object->suit->is_a(current_suit_->name) ||
                              current_suit_->is_a(object->suit->name));
        if (!related) {
            fail(span, name + " is satellite.protected in " +
                       object->suit->name + ", so it cannot be called from " +
                       (current_suit_ ? "spacesuit " + current_suit_->name
                                      : std::string("outside a spacesuit")));
            return nullptr;
        }
    }

    return call_capsule(*method->info, object->suit->name + "." + name, argv,
                        span, object, method->owner);
}

// ---------------------------------------------------------------------------
// Indexing and slicing
// ---------------------------------------------------------------------------

} // namespace satellite
