#pragma once

#include "ast.hpp"
#include "value.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Call frames and the pass that fills them in — milestone M3, DESIGN.md §6.
//
// §6 is the design's most important decision, and it is backed by measurement
// rather than taste: routing capsule locals through satellite.library makes a
// recursive fact() return 1 for every input, and makes eight threads running a
// capsule with no recursion and no shared state produce 1585 wrong results out
// of 1600. The Library gives atomicity; locals need isolation. A frame is the
// isolation.
//
// WHERE THIS PASS LIVES, and why it is not in the parser
// ------------------------------------------------------
// §6 says locals resolve to integer slot indices "at parse time". §5 and
// parser.hpp say, at length, that the parser stays resolution-free with no
// feedback edge from a symbol table. Those read as a contradiction and are not
// one: §6 means "statically, once, before execution" — as against looking a
// name up on every read — not "inside the parser".
//
// The parser could not do it anyway. A capsule may call one defined further
// down the file, and mutual recursion (is_even/is_odd) is unresolvable in
// single-pass recursive descent; eval.cpp already collects capsule names in a
// pre-pass for exactly this reason. So resolution is a separate walk, and it
// keeps two useful properties: the parser's purity survives intact, and a
// resolver bug reports "unknown variable at line N" BEFORE anything runs
// instead of producing a wrong answer somewhere inside the tree walk.

namespace satellite {

// One capsule activation.
//
// No mutex and no atomic, deliberately: a frame is reachable from exactly one
// thread. That is the entire difference from satellite.library, and it is what
// turns §6's 1585/1600 into 0/1600.
struct Frame {
    std::vector<ValuePtr> slots;
};

struct ResolveError {
    std::string message;
    Span span;
};

// Everything known about a capsule before it ever runs.
struct CapsuleInfo {
    const Capsule *capsule = nullptr;

    // Parameters occupy slots [0, param_count); locals follow.
    size_t param_count = 0;
    size_t slot_count = 0;

    // Parallel to a Frame's slots, and STATIC — a declared type belongs to the
    // capsule, not to an activation. Building a vector<Type> per call would
    // roughly double the fixed cost of calling and carry no information that
    // differs between calls. §7 says to store the declared TypeNode alongside
    // the binding; here the binding is a slot, so this is alongside it.
    std::vector<Type> slot_types;
    std::vector<std::string> slot_names;   // diagnostics only
};

// --- spacesuits ------------------------------------------------------------

// One field, in the layout of the spacesuit that owns the layout — not
// necessarily the one that declared it. `owner` is the declaring suit, which is
// what an access check has to compare against.
struct FieldInfo {
    std::string name;
    Type type;
    Access access = Access::Protected;

    // The initialiser, borrowed from the tree. Null when the field has none, in
    // which case construction uses the declared type's default, exactly as an
    // uninitialised local does.
    const Expr *init = nullptr;

    const SpacesuitInfo *owner = nullptr;
};

// A method is a capsule plus a receiver and an access. Everything about calling
// it — slot counts, declared types, the body — is the CapsuleInfo, so a method
// call and a capsule call go down the same path with one extra binding.
//
// The CapsuleInfo is SHARED rather than held by value, and that is load-bearing
// rather than an optimisation. Inheritance flattens by copying a superclass's
// method table into its subclass, and layouts are all built before any body is
// resolved — so a by-value CapsuleInfo would be copied while still empty, and
// the subclass would call the inherited method with a frame of no slots and no
// declared types. Sharing means the owner's body resolves into the one object
// every descendant already points at.
struct MethodInfo {
    Access access = Access::Protected;
    const SpacesuitInfo *owner = nullptr;
    std::shared_ptr<CapsuleInfo> info;
};

// Everything known about a spacesuit before any instance of it exists.
//
// Both tables are FLATTENED at resolve time — a superclass's fields and methods
// are copied in — so nothing walks a superclass chain at run time. That is the
// same trade §6 made for locals: do the search once, statically, and leave an
// index (for a field) or one hash lookup (for a method) at the point of use.
struct SpacesuitInfo {
    const Spacesuit *suit = nullptr;
    std::string name;
    const SpacesuitInfo *super = nullptr;    // null when there is no superclass

    // Superclass fields FIRST, then this suit's own. That order is the whole
    // of inheritance at the field level: an inherited method was resolved
    // against its own suit's indices, and prefixing means those indices still
    // name the same fields in a descendant's instance.
    std::vector<FieldInfo> fields;

    // Keyed by method name, with this suit's own definition replacing an
    // inherited one. Dispatch looks this up on the OBJECT's suit rather than
    // the calling method's, which is what makes an override visible to the
    // inherited method that calls it.
    std::unordered_map<std::string, MethodInfo> methods;

    // The constructors that run when an instance of this suit is built, in the
    // order they run: superclass first, this suit's own last. A separate list
    // rather than an entry in `methods` because a constructor is never called
    // by name, and because a subclass does not OVERRIDE its parent's — both
    // run, which is what makes an inherited field's setup happen at all.
    //
    // Field initialisers all run before any of these. That is deliberately not
    // C++'s interleaving of base-construction with derived-field-initialisation:
    // running every initialiser first means a constructor sees every field of
    // the whole object already at its declared value, which is the property the
    // interleaved order famously fails to give.
    std::vector<MethodInfo> ctors;

    // The parameters the construction site supplies. They go to the LAST
    // constructor in the chain — the most derived one — so every earlier
    // constructor has to take none, which resolve() checks.
    size_t ctor_params() const
    {
        return ctors.empty() || !ctors.back().info
                   ? 0
                   : ctors.back().info->param_count;
    }

    // True for this suit and every ancestor of it — the whole of the subtype
    // test, and what makes a spacesuit type match an instance of a subclass.
    bool is_a(const std::string &other) const;

    const MethodInfo *find_method(const std::string &name) const;

    // The field's index in `fields`, or -1. Callers want the index, not the
    // FieldInfo, because the index is what an Object is addressed by.
    int find_field(const std::string &name) const;
};

struct ResolveResult {
    std::unordered_map<std::string, CapsuleInfo> capsules;

    // Node-based on purpose: a SpacesuitInfo holds a `super` pointer into this
    // same map, and unordered_map keeps element addresses stable across the
    // rehash that inserting the next spacesuit causes.
    std::unordered_map<std::string, SpacesuitInfo> suits;

    std::vector<ResolveError> errors;

    bool ok() const { return errors.empty(); }

    // Keyed by the name as written: `fact`, or `satellite.main` for a reserved
    // capsule. Null when there is no such capsule.
    const CapsuleInfo *find(const std::string &name) const;

    // Null when there is no such spacesuit.
    const SpacesuitInfo *find_suit(const std::string &name) const;
};

// Resolves every capsule body, stamping Name::slot and VarDecl::slot.
//
// Never throws. On an error it records it and keeps going, so one bad name
// does not hide every later one.
//
// MUST run to completion, single-threaded, before any evaluation of the same
// Program — see the contract on Name::slot in ast.hpp.
//
// Top-level statements are NOT resolved to slots. `satellite.variable.number
// x = 1` at the top level stays observable as satellite.library.main.x, per
// §6 (which demotes capsule locals only) and §10's main.x ruling. Top level
// keeps SLOT_GLOBAL, and an unknown bare name there is not an error, because
// the REPL declares a variable on one line and reads it on the next — two
// separate Programs that no single resolve() call can see at once.
//
// Inside a capsule the opposite holds: an unknown bare name is a resolve-time
// error rather than an implicit global read. That is what makes capsules
// lexically closed, and it is what makes §6's 8-thread result mean anything.
//
// A method body resolves the same way with one scope inserted between the
// locals and the error: the enclosing spacesuit's fields and methods. So a
// local shadows a field, a field shadows nothing (there is no implicit global
// read to shadow), and a field is still not reachable from outside the suit —
// which is what makes `satellite.protected` a statement about the language
// rather than a comment.
ResolveResult resolve(const Program &program);

std::string format_error(const ResolveError &error, const std::string &source);

} // namespace satellite
