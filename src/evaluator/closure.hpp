#pragma once

// The compiled closure tree -- PLAN M9. PLAN §2.3 is the specification and
// §2.5 is the constraint that decides its shape.
//
// A COMPILED OP IS FIVE WORDS, WHICH IS THE SHAPE ast.hpp ALREADY HAS. A Node
// is a kind and four payload words in 24 bytes; an Op is a FUNCTION POINTER and
// the same four payload words, in the same 24. That symmetry is not decoration:
// closure compilation is PLAN §2.3's "every decision that CAN be made before
// execution IS", and what it replaces is precisely the kind byte -- a tag the
// evaluator would have had to test at every node becomes the address it would
// have jumped to. "At runtime a node is one indirect call with no tag test and
// no re-resolution" is that sentence, and this struct is where it is kept.
//
// AND IT IS NOT BYTECODE, which PLAN §2.3 spends a section on. There is no
// linear instruction stream -- an op names its children by index and they are
// nowhere in particular. There is no decode loop, no serialised form of this
// arena, and no compile step a user waits for. What does the job an opcode
// table does is DESIGN §4's numbering, which exists before any program does.
//
// THE MACHINE THAT RUNS THIS KEEPS ITS OWN STACK, AND THAT IS THE WHOLE REASON
// THE OPS ARE AN ARENA. DESIGN §7.5: the language has no depth limit. A closure
// tree of `unique_ptr<IExprClosure>` with a virtual `eval(ctx)` -- which is what
// the M7 draft in prototype/ built, and the obvious C++ answer -- recurses on
// the C++ stack once per level and dies at a depth `ulimit -s` chooses. M8.5
// did the same rewrite for the four static passes; this milestone inherits the
// rule rather than rediscovering it. An op is a POD, the arena is a vector, and
// the work stack is on the heap.
//
// WHICH MEANS AN OP FUNCTION IS NOT `Value f(children)`. It cannot call its
// children -- that is the recursion. It is handed the machine and a STEP
// number, and it either pushes its children and asks to be resumed at the next
// step, or it takes their answers off the value stack and pushes its own.
// evaluator/machine.hpp is that loop and the contract is written there.

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include "satellite_spacesuit/suit_object.hpp"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace satellite::eval {

class Machine;

// An index into the op arena. 0 is not an op, for ast.hpp's reason exactly: an
// absent child is a legal answer everywhere here -- a `satellite.return` with no
// value, a `for` with no step, an `if` with no else -- and a sentinel that is
// also a valid index is how those three become one bug.
using OpIndex = uint32_t;

inline constexpr OpIndex kNoOp = 0;

// A handle to a list of child ops, or 0 for the empty list. The count is stored
// with the list, which is ast.hpp's trick and is what keeps an op at four
// payload words when a `for` needs an init, a condition, a step and a body.
using OpListId = uint32_t;

inline constexpr OpListId kNoOpList = 0;

struct Op;

// WHAT AN OP DOES WHEN THE MACHINE REACHES IT.
//
// `step` IS WHAT REPLACES THE RETURN ADDRESS a recursive evaluator got from the
// C++ stack. `a + b` is three visits to one op: push b then a and ask for step
// 1; be resumed at step 1 with two answers waiting on the value stack. The
// number of steps is a property of the op and lives in its own function.
using OpFn = void (*)(Machine &machine, const Op &op, uint32_t step);

// ONE OP. The function, and four payload words that mean different things in
// each -- the price of a POD arena, paid here in one table rather than by a
// reader who has to guess. This is ast.hpp's table with the kind column
// replaced by an address.
//
//   fn              a                b               c              d
//   -------------------------------------------------------------------------
//   op_constant     constant index   -               -              -
//   op_local        frame slot       -               -              -
//   op_global       global index     -               -              -
//   op_capsule      capsule index    -               -              -
//   op_unary        operand          operator index  -              -
//   op_binary       left             right           operator index -
//   op_call         capsule index    argument list   -              -
//   op_package      capsule index    argument list   text index     -
//   op_construct    suit index       field init list -              -
//   op_field        field index      -               -              -
//   op_field_store  field index      value or none   -              -
//   op_dispatch     PathId           argument list   cache index    text index
//   op_options      the wrapped op   names (texts)   -              -
//   op_method       PathId           argument list   cache index    frame slot
//   op_method_global PathId          argument list   cache index    global index
//   op_refuse       errors::Code     text index      -              -
//   op_block        statement list   -               -              -
//   op_expression   expression       -               -              -
//   op_store        frame slot       value or none   -              -
//   op_to_float     value            -               -              -
//   op_retune       PathId           value           text index     -
//   op_store_global global index     value or none   -              -
//   op_return       value or none    -               -              -
//   op_if           condition        then block      else or none   -
//   op_while        condition        body            -              -
//   op_for          init or none     condition/none  step or none   body
//
// AN OP'S SOURCE NODE IS A SIDE TABLE AND NOT A FIELD, which is PLAN §2.2's
// decision about `Name::slot` applied one layer on. A span is needed when a
// diagnostic is raised and when `satl --compile` prints, and on neither of those
// paths does one more indirection matter; on the walk it is four bytes of every
// cache line spent on something the walk never reads. Compiled::node_of() is
// the table.
struct Op {
    OpFn fn = nullptr;
    uint32_t a = 0;
    uint32_t b = 0;
    uint32_t c = 0;
    uint32_t d = 0;
};

// 24 BYTES, THE SAME AS A Node, and the assert is here for ast.hpp's reason:
// a field added without thinking should be a compile error naming this line
// rather than an arena that quietly grew by a third.
static_assert(sizeof(Op) == 24,
              "closure.hpp: an Op is five words -- a function pointer where a "
              "Node has a kind, and the same four payload words. See the table "
              "above and ast.hpp's, which it is deliberately shaped like");

// One capsule, compiled. DESIGN §7.2's frame, decided before anything runs.
//
// `slots` IS THE WHOLE FRAME AND `parameters` IS ITS FIRST PART, which is
// resolve.hpp's Frame with the names dropped: slots [0, parameters) are the
// argument list in order and everything after is a local. Nothing here is
// looked up by name at runtime, which is DESIGN §7.1's entire point -- the
// first satellite keyed a global registry by "<capsule>.<variable>" and a
// recursive capsule returned 1 for every input.
struct Capsule {
    words::PathId path = words::kNoPath;
    OpIndex body = kNoOp;

    // THE OP THAT ENTERS THIS CAPSULE FROM OUTSIDE, and it exists so that
    // Machine::call() and op_call take the same road. A call from C++ has no
    // call op sitting on the work stack to be resumed when the body ends, and
    // Frame::work_floor is an index INTO that stack -- so without this there
    // would be a second way to build a frame, and DESIGN §7.1 is a receipt for
    // what a second way to get a frame wrong costs. op_enter is op_call with
    // the argument evaluation already done.
    OpIndex entry = kNoOp;

    uint32_t slots = 0;
    uint32_t parameters = 0;
    NodeIndex node = kNoNode;

    // A SPACESUIT'S METHOD, whose receiver is slot 0 -- what Machine::enter()
    // puts on the thread's access list for the call (THREAD.md T2).
    bool method = false;

    // WHICH FILE DECLARED IT -- M25. `node` indexes that file's tree.
    uint32_t file = 0;
};

// ONE `satellite.capsule.launch`, AS AN INCLUDE MATCHES ARGUMENTS TO IT -- M25,
// the author's "smart arg passing -- we look for where the arguments fit". A
// launch fits when the include hands it one value per parameter and every value
// is one its parameter's declared type can hold.
struct Launch {
    uint32_t capsule = 0;

    // Each parameter's declared type, and -- for a spacesuit parameter -- the
    // layouts an object may have been built from: the suit's own and every
    // suit that extends it, found by review when a `dog` did not fit an
    // `animal` parameter. Empty for any other type; kNoPath fits anything.
    std::vector<words::PathId> types;
    std::vector<std::vector<uint32_t>> layouts;
};

// ONE FILE OF THE PROGRAM -- M25. File 0 is the one satl was given; file n is
// the nth spaceship the loader met. What runs when a file is included is here.
struct File {
    std::string name;          // the spaceship's name, empty for file 0
    const Ast *ast = nullptr;  // the tree its ops' nodes index

    // THE FILE'S GLOBALS, SET UP ONCE AT THE START OF THE RUN, and the includes
    // written at its top, run the first time the file itself is included. File
    // 0's two are inside `top()` in the order they were written, as always.
    OpIndex setup = kNoOp;
    OpIndex includes = kNoOp;

    std::vector<Launch> launches;
};

// An inline cache cell -- PLAN §2.4. One per dispatching call site.
//
// MUTABLE AND IN A SIDE TABLE, so the op arena stays immutable and shareable
// across threads (DESIGN §10.5) while the cache is per-run. What it caches is
// the handler a PathId resolved to and the receiver type it was resolved FOR;
// the guard is the type tag, because a method call's answer depends on what it
// is called on and nothing else at this milestone.
struct Cache {
    const void *handler = nullptr;
    uint32_t guard = 0;
    bool filled = false;
};

// A whole program, compiled. Built once per run, read many times, never written
// to disk -- PLAN §2.3: "no serialised form of the closure tree", because
// writing one down would freeze an implementation. `.satc` serialises the layer
// above this and is a cache.
class Compiled {
public:
    OpIndex add(OpFn fn, NodeIndex node, uint32_t a = 0, uint32_t b = 0,
                uint32_t c = 0, uint32_t d = 0, uint32_t file = 0);

    OpListId add_list(const std::vector<OpIndex> &items);

    uint32_t add_constant(Value value);

    const Op &operator[](OpIndex index) const { return ops_[index]; }

    uint32_t list_size(OpListId list) const { return lists_[list]; }
    OpIndex list_at(OpListId list, uint32_t i) const { return lists_[list + 1 + i]; }

    const Value &constant(uint32_t index) const { return constants_[index]; }

    NodeIndex node_of(OpIndex index) const { return nodes_[index]; }

    // WHICH FILE AN OP CAME FROM -- M25, the side table beside `nodes_`, and
    // the tree its node indexes. A program of one file answers 0 and itself.
    uint32_t file_of(OpIndex index) const
    {
        return index < op_files_.size() ? op_files_[index] : 0;
    }
    const Ast &ast_of(uint32_t file, const Ast &own) const
    {
        return file == 0 || file >= files_.size() || files_[file].ast == nullptr
                   ? own
                   : *files_[file].ast;
    }

    const std::vector<File> &files() const { return files_; }
    std::vector<File> &files() { return files_; }

    const std::vector<Capsule> &capsules() const { return capsules_; }
    std::vector<Capsule> &capsules() { return capsules_; }

    // What `satl --compile` and the machine both want: the file's top level,
    // which at this grammar is every global's initialiser in order. DESIGN §6
    // has no top-level statement, so there is nothing else it can be.
    OpIndex top() const { return top_; }
    void set_top(OpIndex top) { top_ = top; }

    // AN INCLUDE OF FILE 0 WITH NOTHING HANDED OVER -- M25. What
    // Machine::run_launches() pushes, so the file satl was given runs its
    // launches through the one op every include runs through.
    OpIndex launch_op() const { return launch_op_; }
    void set_launch_op(OpIndex op) { launch_op_ = op; }

    uint32_t globals() const { return globals_; }
    uint32_t add_global() { return globals_++; }

    // WHETHER THE PROGRAM CAN START A THREAD -- THREAD.md T2. A program that
    // does is shared from its first line (Machine's constructor), so no method
    // call or statement already under way when the first `start()` runs is
    // left off its thread's access list. Found by T2's review: `o.run()`
    // starting a worker that calls `o.bump()` left run() unheld, and both
    // threads' increments raced.
    bool starts_threads() const { return starts_threads_; }
    void set_starts_threads() { starts_threads_ = true; }

    // WHETHER THE STATEMENT THAT ENDS AT `op` CAN WRITE `satellite.library` --
    // THREAD.md T2, and the fix for the regression dark_mechanicum found on
    // revision 04: every statement that merely READ a global took the
    // exclusive hold, so eight threads reading a constant queued on each other
    // (0.80 s against T1's 0.04 s). A statement that only reads needs no hold
    // -- each read is whole under Globals' mutex -- and one that writes holds
    // the globals from its first touch, so `n = n + 1` stays exact. Decided by
    // the compiler, over every op the statement compiled to; a statement
    // containing another (an `if` with a body that writes) counts as writing,
    // which only ever holds more, never less.
    bool statement_writes(OpIndex op) const
    {
        return op < writes_.size() && writes_[op] != 0;
    }
    void set_writes(std::vector<uint8_t> writes) { writes_ = std::move(writes); }

    // THE SPACESUIT LAYOUTS -- M26. One per suit the file declares, held by the
    // program because that is what outlives every object of it; every
    // `SuitObject` carries a bare pointer to its own, which
    // satellite_spacesuit/suit_object.hpp says is safe for the arena's reason.
    //
    // A `deque` AND NOT A `vector`, WHICH IS THE ONE CONTAINER CHOICE IN THIS
    // FILE THAT IS NOT THE OBVIOUS ONE. Objects hold `const Layout *`, and a
    // vector that reallocates on its next push would leave every object built
    // so far pointing at freed memory. A program declaring a suit, constructing
    // one, and then declaring a second suit is not exotic -- it is
    // `example/spacesuits.satl`. A deque never moves what it already holds.
    uint32_t add_suit(suit::Layout layout)
    {
        suits_.push_back(std::move(layout));
        return static_cast<uint32_t>(suits_.size() - 1);
    }

    const suit::Layout &suit_at(uint32_t index) const { return suits_[index]; }
    size_t suits() const { return suits_.size(); }

    uint32_t caches() const { return caches_; }
    uint32_t add_cache() { return caches_++; }

    // The refusal texts op_refuse names by index -- what is not built yet, in
    // the words a person reads. Kept here rather than in the op so that an Op
    // stays a POD and the arena stays trivially copyable.
    uint32_t add_text(std::string text);
    const std::string &text(uint32_t index) const { return texts_[index]; }

    size_t size() const { return ops_.size(); }

private:
    // Index 0 is a no-op so that kNoOp is safe to dereference, and lists_[0] is
    // the empty list's count. Both are ast.hpp's tricks, kept for its reasons.
    std::vector<Op> ops_{Op{}};
    std::vector<NodeIndex> nodes_{kNoNode};
    std::vector<uint32_t> op_files_{0};
    std::vector<File> files_;
    std::vector<OpIndex> lists_{0};
    std::vector<Value> constants_;
    std::vector<Capsule> capsules_;
    std::vector<std::string> texts_;
    std::deque<suit::Layout> suits_;
    OpIndex top_ = kNoOp;
    OpIndex launch_op_ = kNoOp;
    uint32_t globals_ = 0;
    uint32_t caches_ = 0;
    bool starts_threads_ = false;
    std::vector<uint8_t> writes_;
};

} // namespace satellite::eval
