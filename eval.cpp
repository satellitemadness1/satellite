#include "eval.hpp"

#include "library.hpp"
#include "system.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace satellite {
namespace {

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

// Bounds the C++ recursion the tree walk costs, so a pathological tree raises
// a satellite error instead of segfaulting the C++ stack (§6).
struct DepthGuard {
    explicit DepthGuard(int &depth) : depth_(depth) { depth_++; }
    ~DepthGuard() { depth_--; }

    DepthGuard(const DepthGuard &) = delete;
    DepthGuard &operator=(const DepthGuard &) = delete;

private:
    int &depth_;
};

// satellite.library.main.x -> {"satellite", "library", "main", "x"}.
// False for anything that is not a pure SatelliteLit/Member chain, which is
// what separates a language path from an expression with members on it.
bool flatten_path(const Expr &expr, std::vector<std::string> &out)
{
    if (std::holds_alternative<SatelliteLit>(expr)) {
        out.push_back("satellite");
        return true;
    }
    if (const Member *m = std::get_if<Member>(&expr)) {
        if (!m->target || !flatten_path(*m->target, out))
            return false;
        out.push_back(m->name);
        return true;
    }
    return false;
}

std::string join_path(const std::vector<std::string> &path)
{
    std::string out;
    for (size_t i = 0; i < path.size(); i++) {
        if (i)
            out += ".";
        out += path[i];
    }
    return out;
}

// A declaration with no initialiser still gets a value of its declared type,
// so `satellite.container.list<...> l` is an empty list you can append to
// rather than a nil you cannot. The bare `satellite` type has no such value
// and stays nil.
//
// A SPACESUIT type is nil here, and the declaration statement builds the
// instance instead (exec, below). The split is not arbitrary: a spacesuit's
// default is an ALLOCATION, so this function would have to be able to fail and
// to run satellite code, and a field of a suit's own type would default-
// construct forever. A variable is not part of any object's layout, so it has
// no such regress and gets a real instance; a field is, so it starts as nil and
// a method fills it with my_class_name().
Value default_of(const Type &type)
{
    if (type.is_singleton() || type.is_spacesuit())
        return std::monostate{};
    if (type.space == "variable") {
        if (type.name == "bool")
            return false;
        if (type.name == "number")
            return Number();
        if (type.name == "string")
            return make_string(SatString{});
        // The epoch. A time has no "empty" the way a string does, and the epoch
        // is the one instant that is a fact rather than a choice.
        if (type.name == "time")
            return Time{};
        // A file is nil until it is opened, and nil is a real answer rather
        // than a placeholder: satellite.variable.file is a reference type
        // (§8.3), and a declaration cannot open anything because it has no path
        // to open and nowhere to report that opening failed.
    }
    if (type.space == "container" && type.name == "list")
        return make_list(List{});
    return std::monostate{};
}

// Structural, not pointer, equality: two lists holding equal values are equal
// even when they share no children. The variant's own operator== would compare
// the shared_ptrs inside a List, so the List arm has to come first.
//
// An OBJECT is the exception, and it needs no arm: the variant's own operator==
// compares the ObjectPtrs, which is identity, and identity is what equality
// means for a reference type. Two instances with equal fields are two
// instances — a spacesuit that wants them equal says so with a method, because
// only it knows which of its fields are part of what it means to be equal.
bool value_equals(const Value &a, const Value &b)
{
    if (a.index() != b.index())
        return false;
    if (const List *la = as_list(a)) {
        const List &lb = *as_list(b);
        if (la->size() != lb.size())
            return false;
        for (size_t i = 0; i < la->size(); i++) {
            const ValuePtr &x = (*la)[i];
            const ValuePtr &y = lb[i];
            if (!x || !y) {
                if (x != y)
                    return false;
                continue;
            }
            if (!value_equals(*x, *y))
                return false;
        }
        return true;
    }
    return static_cast<const ValueBase &>(a) == static_cast<const ValueBase &>(b);
}

// An index must be a whole number: 1.5 is a bug in the program, not a
// silently truncated 1.
bool as_index(const Value &v, long long &out)
{
    // Number::to_integer answers both halves at once — is it whole, and does it
    // fit — so there is no isfinite/floor dance any more, and no way for a
    // value that merely rounds to an integer to pass as one.
    const Number *n = std::get_if<Number>(&v);
    return n && n->to_integer(out);
}

// Half-open, negative bounds Python-style, out-of-range clamps, hi < lo is
// empty (§7).
void clamp_range(long long &lo, long long &hi, long long len)
{
    if (lo < 0)
        lo += len;
    if (hi < 0)
        hi += len;
    lo = std::max(0LL, std::min(lo, len));
    hi = std::max(0LL, std::min(hi, len));
    if (hi < lo)
        hi = lo;
}

ValuePtr make_value(Value v)
{
    return std::make_shared<const Value>(std::move(v));
}

// A string and a list are stored behind a handle (value.hpp), so neither
// converts to a Value implicitly any more. These two overloads keep that a
// detail of the value model rather than something every call site restates.
ValuePtr make_value(SatString s)
{
    return make_value(make_string(std::move(s)));
}

ValuePtr make_value(List items)
{
    return make_value(make_list(std::move(items)));
}


// The whole language, on one screen.
//
// That is only possible because the language IS one screen: two dozen methods,
// seven module functions, four statement forms. A reference that fits is worth
// more than a reference that is complete, and here they are the same thing --
// so this is checked against the code below rather than written once and left
// to rot.
std::string help_overview()
{
    return
"satellite 0.1 -- the whole language\n"
"\n"
"  declare    satellite.variable.number x = 1        bool number string time file\n"
"             satellite.container.list<satellite.variable.number> l\n"
"             my_class thing            a spacesuit; thing(\"arg\") to construct\n"
"  values     satellite.bool.true  satellite.bool.false  satellite (the runtime)\n"
"  operators  + - * / %   == != < <= > >=   !   -x    l[i]  l[a:b]  l[:b]  l[a:]\n"
"\n"
"  capsule    satellite.capsule name(satellite.variable.number n)\n"
"                 satellite.returns(satellite.variable.number) { ... }\n"
"  spacesuit  satellite.spacesuit my_class(superclass) {\n"
"                 satellite.protected { ...fields... }\n"
"                 satellite.public    { my_class(args) {...}  ...capsules... } }\n"
"  control    satellite.statement.if (c) {} satellite.statement.else {}\n"
"             satellite.statement.while (c) {}   satellite.statement.for (i;c;s) {}\n"
"  return     satellite.return(value)\n"
"\n"
"  console    satellite.console.display(value)\n"
"  time       satellite.time.now()                  .minus(t) .nanoseconds()\n"
"  file       satellite.file.open(path, \"read\"|\"write\"|\"append\")\n"
"                 .ok() .read() .write(s) .close() .error() .path()\n"
"  directory  satellite.directory.current()  .change(dir)  .exists(dir)\n"
"  globals    satellite.library.<namespace>.<name>\n"
"\n"
"  help       satellite.help          this text (the () is optional)\n"
"             satellite.help(value)   the methods that value answers to\n";
}

// What a particular value can do. The type table below is the same one
// call_method dispatches on, so a method that exists is a method that is
// listed -- a help text that drifts from the code is worse than none.
std::string help_for(const Value &value)
{
    switch (value.index()) {
    case 1:  return "satellite.variable.bool\n"
                    "  .negate()  .and(b)  .or(b)  .to_string()\n";
    case 2:  return "satellite.variable.number\n"
                    "  .plus(n) .minus(n) .times(n) .divided_by(n) .modulo(n)\n"
                    "  .abs() .floor() .ceil() .round() .to_string()\n";
    case 3:  return "satellite.variable.string\n"
                    "  .length() .concat(s) .contains(s) .starts_with(s)\n"
                    "  .ends_with(s) .to_string()          s[i]  s[a:b]\n";
    case 4:  return "satellite.container.list\n"
                    "  .length() .append(v) .first() .last() .contains(v)\n"
                    "  .to_string()                        l[i]  l[a:b]\n";
    case 6:  return "satellite.variable.time\n"
                    "  .minus(t) -> nanoseconds   .nanoseconds()  .to_string()\n";
    case 7:  return "satellite.variable.file\n"
                    "  .ok() .read() .write(s) .close() .error() .path()\n";
    default: break;
    }
    return "nil has no methods\n";
}

// A language-owned VALUE reached by a module path, as against a language-owned
// FUNCTION. §5 anticipated the shape when it warned not to make a trailing '('
// mandatory after a module path, "or module constants like satellite.math.pi
// become errors".
//
// satellite.bool.true and satellite.bool.false are the two satellite.variable.
// bool values, and they are the only way to write either one. The spelling
// costs no parser rule, no lexer keyword and no second reserved word: segment-1
// dispatch already sends `bool` down the module path like any other name.
//
// It cannot be satellite.variable.bool.true, however much that reads like the
// type it belongs to, because §4 reserves satellite.variable.* as a TYPE
// namespace in which no path is ever a value expression — and that reservation
// is what keeps '<' unambiguous. `bool` as a module segment and `bool` as a
// type name are different things, and keeping them apart is the point.
//
// The obvious alternative, bare `true` and `false`, would be the language's
// second and third reserved words. §1 has exactly one.
ValuePtr module_constant(const std::vector<std::string> &path)
{
    // Deliberately reachable WITHOUT parentheses. Someone who needs help is by
    // definition someone who may not remember the calling syntax, and making
    // them get it right first is the one place a language can least afford to.
    if (path.size() == 2 && path[0] == "satellite" && path[1] == "help")
        return make_value(encode_raw(help_overview()));

    if (path.size() == 3 && path[0] == "satellite" && path[1] == "bool") {
        if (path[2] == "true")
            return make_value(true);
        if (path[2] == "false")
            return make_value(false);
    }
    return nullptr;
}

// Methods that write back through their receiver. They are the reason a
// receiver has to name a storage slot.
bool is_mutator(const std::string &name)
{
    return name == "append";
}

// What the depth guard is protecting, measured rather than guessed.
//
// One capsule activation costs exactly 3 depth units — eval(Call) -> eval_call
// -> call_capsule -> exec(body) -> exec_block -> exec(return) -> eval — and
// 3169 bytes of C++ stack at -O2 (clang 24, x86-64). Three units is the fewest
// any body can cost, so that is the WORST ratio the guard has to survive: a
// longer body spends more units per byte, not fewer. The cost does not grow
// with the number of locals either, because a frame's slots are a vector.
//
// Where the C++ stack actually ends, bisected on an 8 MB stack with the guard
// disabled:
//
//     -O2    2600 levels return, 2800 segfault   -> ~7950 units
//     -O0    1200 levels return, 1300 segfault   -> ~3750 units
//
// §6's suggested default of 10000 was written when nothing recursed, and it is
// past BOTH cliffs — so the guard could never have fired, and a runaway
// recursion was a segfault rather than the error the guard exists to produce.
// 2000 units is a quarter of the optimised stack and half the unoptimised one.
// It is also what the resolver walks to (MAX_RESOLVE_DEPTH, env.cpp), so
// neither pass is the one that dies first.
constexpr int DEFAULT_MAX_DEPTH = 2000;

// The ceiling on satellite.library.system.max_depth. The knob raises the limit
// toward the stack and cannot raise it past: above the cliff a larger setting
// does not buy deeper recursion, it buys a segfault instead of an error
// message. 3000 stays under the -O0 cliff as well as the -O2 one, because an
// unoptimised build is exactly where losing the error message costs most.
constexpr int MAX_MAX_DEPTH = 3000;

int read_max_depth()
{
    ValuePtr v = Library::instance().get("system", "max_depth");
    long long set = 0;
    if (v)
        if (const Number *n = std::get_if<Number>(v.get()))
            if (n->floor().to_integer(set) && set >= 1)
                return static_cast<int>(std::min<long long>(set, MAX_MAX_DEPTH));
    return DEFAULT_MAX_DEPTH;
}

// The significant digits a non-terminating division keeps (§8.1). A knob for
// the same reason max_depth is one: the right answer depends on the program,
// and the default is only a default.
int read_division_digits()
{
    ValuePtr v = Library::instance().get("system", "division_digits");
    long long set = 0;
    if (v)
        if (const Number *n = std::get_if<Number>(v.get()))
            if (n->floor().to_integer(set) && set >= 1)
                return static_cast<int>(
                    std::min<long long>(set, Number::MAX_DIVISION_DIGITS));
    return Number::DEFAULT_DIVISION_DIGITS;
}

} // namespace

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

bool matches(const Type &type, const Value &value)
{
    if (type.is_singleton())
        return std::holds_alternative<std::monostate>(value);

    // A spacesuit type accepts an instance of itself or of any descendant —
    // is_a() is the whole subtype rule — and it accepts nil, because objects
    // are a reference type and a reference may name nothing. The second half is
    // what makes a field of spacesuit type legal before a method fills it.
    if (type.is_spacesuit()) {
        if (std::holds_alternative<std::monostate>(value))
            return true;
        const ObjectPtr *object = std::get_if<ObjectPtr>(&value);
        return object && *object && (*object)->suit &&
               (*object)->suit->is_a(type.name);
    }

    if (type.space == "variable") {
        if (type.name == "bool")
            return std::holds_alternative<bool>(value);
        if (type.name == "number")
            return std::holds_alternative<Number>(value);
        if (type.name == "string")
            return (as_string(value) != nullptr);
        if (type.name == "time")
            return std::holds_alternative<Time>(value);
        // nil satisfies a file for the same reason it satisfies a spacesuit:
        // both are reference types, and a reference may name nothing.
        if (type.name == "file")
            return std::holds_alternative<FilePtr>(value) ||
                   std::holds_alternative<std::monostate>(value);
        return false;
    }

    if (type.space == "container" && type.name == "list") {
        const List *list = as_list(value);
        if (!list)
            return false;
        if (type.args.empty())
            return true;
        for (const ValuePtr &item : *list)
            if (!item || !matches(type.args[0], *item))
                return false;
        return true;
    }

    return false;
}

const char *module_of(const Value &value)
{
    switch (value.index()) {
    case 0: return nullptr;                         // nil owns no methods
    case 1: return "satellite.variable.bool";
    case 2: return "satellite.variable.number";
    case 3: return "satellite.variable.string";
    case 4: return "satellite.container.list";
    case 5: {
        // The one entry that is not a literal: an instance's methods belong to
        // its spacesuit, so the "module path" is the suit's name. Borrowed from
        // the SpacesuitInfo, which the ResolveResult owns and which therefore
        // outlives any message this ends up in.
        const ObjectPtr &object = std::get<ObjectPtr>(value);
        return suit_name(object ? object->suit : nullptr).c_str();
    }
    case 6: return "satellite.variable.time";
    case 7: return "satellite.variable.file";
    default: return nullptr;
    }
}

std::string format_error(const EvalError &error, const std::string &source)
{
    size_t at = std::min(error.span.start, source.size());
    size_t begin = source.rfind('\n', at == 0 ? 0 : at - 1);
    begin = (begin == std::string::npos) ? 0 : begin + 1;
    size_t end = source.find('\n', at);
    if (end == std::string::npos)
        end = source.size();

    std::string out = "satellite: " + error.message + " (line " +
                      std::to_string(error.span.line) + ")\n";
    out += "    " + source.substr(begin, end - begin) + "\n";
    out += "    " + std::string(at - begin, ' ') + "^\n";
    return out;
}

// ---------------------------------------------------------------------------
// Evaluator
// ---------------------------------------------------------------------------

Evaluator::Evaluator(const ResolveResult &resolved, std::string ns, bool echo)
    : resolved_(resolved), ns_(std::move(ns)), echo_(echo),
      max_depth_(read_max_depth()), division_digits_(read_division_digits())
{
}

void Evaluator::fail(Span span, std::string message)
{
    errors_.push_back(EvalError{std::move(message), span});
}

void Evaluator::run(const Program &program)
{
    // No pre-pass over capsule names any more: resolve() already collected
    // every one of them, before any body was walked. That is what makes a call
    // to a capsule defined further down the file — and mutual recursion —
    // resolvable at all (§6).

    for (const TopLevel &item : program.items) {
        if (failed())
            return;

        // satellite.include is parsed and validated, then ignored: there is no
        // module system in v1 and saying so is better than implying one.
        if (std::holds_alternative<Include>(item))
            continue;
        if (std::holds_alternative<Capsule>(item))
            continue;
        // A spacesuit is a declaration, not a statement: resolve() already
        // built its layout and its method table, and nothing runs until an
        // instance is constructed.
        if (std::holds_alternative<Spacesuit>(item))
            continue;

        const StmtPtr &stmt = std::get<StmtPtr>(item);
        if (!stmt)
            continue;

        // A bare expression at the top level echoes its value, which is what
        // makes `x` print 1 in the REPL. A statement that produced nil stays
        // silent, so satellite.console.display does not print twice.
        if (echo_) {
            if (const ExprStmt *e = std::get_if<ExprStmt>(stmt.get())) {
                if (!e->expr)
                    continue;
                ValuePtr v = eval(*e->expr);
                if (failed())
                    return;
                if (v && !std::holds_alternative<std::monostate>(*v))
                    output_ += to_string(*v) + "\n";
                continue;
            }
        }

        if (exec(*stmt) == Flow::Return)
            return;
    }
}

void Evaluator::run_entry(const Program &program, const List &args)
{
    run(program);
    if (failed())
        return;
    // A satellite.return at the top level already stopped the program; running
    // main afterwards would ignore it.
    if (returned_)
        return;

    const CapsuleInfo *info = resolved_.find("satellite.main");
    const Capsule *entry = info ? info->capsule : nullptr;

    // A program that is only top-level statements is legal — the REPL produces
    // nothing else — so a missing satellite.main is not an error.
    if (!entry)
        return;

    if (entry->params.size() > 1) {
        fail(entry->span, "satellite.main takes at most one parameter, got " +
                          std::to_string(entry->params.size()));
        return;
    }

    std::vector<ValuePtr> argv;

    if (entry->params.size() == 1) {
        const Param &param = entry->params[0];

        // The signature is fixed by §2, so check the declared shape rather than
        // only matches(): an empty argument list matches list<anything>, and
        // accepting satellite.main(list<number> argz) because nothing was
        // passed today would break the first time something was.
        const Type &type = param.type;
        bool shaped = type.space == "container" && type.name == "list" &&
                      type.args.size() == 1 &&
                      type.args[0].space == "variable" &&
                      type.args[0].name == "string";
        if (!shaped) {
            fail(param.span,
                 "satellite.main's parameter must be "
                 "satellite.container.list<satellite.variable.string>, not " +
                 unparse(type));
            return;
        }

        argv.push_back(make_value(make_list(args)));
    }

    // From here it is an ordinary call: argz lands in slot 0 of main's frame
    // like any other parameter, which is what §11 said M3 would replace the
    // namespace binding with.
    ValuePtr result = call_capsule(*info, "satellite.main", argv, entry->span);
    if (failed())
        return;

    // The entry point's return value is the program's, and call_capsule has
    // already restored the (empty) top-level one.
    returned_ = result;
}

Flow Evaluator::exec_block(const Block &block)
{
    for (const StmtPtr &stmt : block.statements) {
        if (failed())
            return Flow::Normal;
        if (!stmt)
            continue;
        if (exec(*stmt) == Flow::Return)
            return Flow::Return;
    }
    return Flow::Normal;
}

bool Evaluator::condition(const Expr &expr, bool &out)
{
    ValuePtr v = eval(expr);
    if (failed() || !v)
        return false;
    const bool *b = std::get_if<bool>(v.get());
    if (!b) {
        // No implicit truthiness. Every value in satellite has a declared
        // type, so "is 0 false?" is a question the language never has to
        // answer — and every language that did answer it regrets the answer.
        fail(expr.span, "condition must be a satellite.variable.bool, got " +
                        std::string(module_of(*v) ? module_of(*v) : "nil"));
        return false;
    }
    out = *b;
    return true;
}

Flow Evaluator::exec(const Stmt &stmt)
{
    if (depth_ >= max_depth_) {
        fail(stmt.span, "satellite stack overflow (satellite.library.system."
                        "max_depth = " + std::to_string(max_depth_) + ")");
        return Flow::Normal;
    }
    DepthGuard guard(depth_);

    if (const VarDecl *decl = std::get_if<VarDecl>(&stmt)) {
        // `satellite` is the one reserved word, and the binding site is where
        // that is enforced (§1).
        if (decl->name == "satellite") {
            fail(stmt.span, "satellite is reserved and cannot be declared");
            return Flow::Normal;
        }

        Value value = default_of(decl->type);
        if (decl->init) {
            ValuePtr init = eval(*decl->init);
            if (failed() || !init)
                return Flow::Normal;
            value = *init;
            if (!matches(decl->type, value)) {
                fail(decl->init->span,
                     "cannot initialise " + unparse(decl->type) + " " +
                     decl->name + " with " + to_string(value));
                return Flow::Normal;
            }
        } else if (decl->type.is_spacesuit()) {
            // `my_class_name my_object` builds one. This is the construction
            // site the language has instead of a constructor call, which is
            // what lets the declaration read exactly like every other one.
            const SpacesuitInfo *suit = resolved_.find_suit(decl->type.name);
            if (!suit) {
                // resolve() rejects an unknown spacesuit before anything runs.
                fail(stmt.span, "internal: no such spacesuit: " +
                                decl->type.name);
                return Flow::Normal;
            }
            ValuePtr made = construct(*suit, {}, stmt.span);
            if (failed() || !made)
                return Flow::Normal;
            value = *made;
        }

        // A capsule local declares into its frame; a top-level variable
        // declares into satellite.library, where the declared type has to be
        // remembered alongside it. A frame slot needs no such record: its type
        // is in the capsule, one copy for every activation.
        if (decl->slot >= 0) {
            Slot slot{decl->slot, std::string(), decl->name, true};
            write_slot(slot, make_value(std::move(value)), stmt.span);
            return Flow::Normal;
        }

        Slot slot{SLOT_GLOBAL, ns_, decl->name, true};
        declared_[slot.key()] = decl->type;
        Library::instance().set(slot.ns, slot.name, std::move(value));
        return Flow::Normal;
    }

    if (const Assign *assign = std::get_if<Assign>(&stmt)) {
        if (!assign->target || !assign->value)
            return Flow::Normal;

        Slot slot = slot_of(*assign->target);
        if (!slot.valid) {
            fail(assign->target->span, "cannot assign to this expression");
            return Flow::Normal;
        }

        ValuePtr value = eval(*assign->value);
        if (failed() || !value)
            return Flow::Normal;

        const Type *declared = declared_type(slot);
        if (declared && !matches(*declared, *value)) {
            fail(assign->value->span,
                 "cannot assign " + to_string(*value) + " to " +
                 unparse(*declared) + " " + slot.name);
            return Flow::Normal;
        }

        write_slot(slot, std::move(value), assign->target->span);
        return Flow::Normal;
    }

    if (const ExprStmt *e = std::get_if<ExprStmt>(&stmt)) {
        if (e->expr)
            eval(*e->expr);
        return Flow::Normal;
    }

    if (const Return *ret = std::get_if<Return>(&stmt)) {
        if (ret->value) {
            ValuePtr v = eval(*ret->value);
            if (failed())
                return Flow::Normal;
            returned_ = v;
        } else {
            returned_ = make_value(std::monostate{});
        }
        return Flow::Return;
    }

    if (const Block *block = std::get_if<Block>(&stmt))
        return exec_block(*block);

    if (const If *node = std::get_if<If>(&stmt)) {
        bool test = false;
        if (!node->condition || !condition(*node->condition, test))
            return Flow::Normal;
        if (test)
            return node->then_branch ? exec(*node->then_branch) : Flow::Normal;
        return node->else_branch ? exec(*node->else_branch) : Flow::Normal;
    }

    if (const While *node = std::get_if<While>(&stmt)) {
        for (;;) {
            bool test = false;
            if (!node->condition || !condition(*node->condition, test))
                return Flow::Normal;
            if (!test)
                return Flow::Normal;
            if (node->body && exec(*node->body) == Flow::Return)
                return Flow::Return;
            if (failed())
                return Flow::Normal;
        }
    }

    if (const For *node = std::get_if<For>(&stmt)) {
        if (node->init && exec(*node->init) == Flow::Return)
            return Flow::Return;
        for (;;) {
            if (failed())
                return Flow::Normal;
            // A missing condition means "always true", so for(;;) loops.
            bool test = true;
            if (node->condition && !condition(*node->condition, test))
                return Flow::Normal;
            if (!test)
                return Flow::Normal;
            if (node->body && exec(*node->body) == Flow::Return)
                return Flow::Return;
            if (failed())
                return Flow::Normal;
            if (node->step && exec(*node->step) == Flow::Return)
                return Flow::Return;
        }
    }

    fail(stmt.span, "unhandled statement");
    return Flow::Normal;
}

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

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

ValuePtr Evaluator::eval(const Expr &expr)
{
    if (depth_ >= max_depth_) {
        fail(expr.span, "expression nests deeper than satellite.library."
                        "system.max_depth (" + std::to_string(max_depth_) + ")");
        return nullptr;
    }
    DepthGuard guard(depth_);

    if (const NumberLit *n = std::get_if<NumberLit>(&expr))
        return make_value(n->value);

    if (const StringLit *s = std::get_if<StringLit>(&expr))
        return make_value(s->value);

    if (std::holds_alternative<SatelliteLit>(expr))
        return make_value(std::monostate{});   // the runtime singleton

    if (const Name *name = std::get_if<Name>(&expr)) {
        if (name->slot == SLOT_CAPSULE) {
            fail(expr.span, "capsule " + name->text +
                            " cannot be used as a value");
            return nullptr;
        }
        if (name->slot == SLOT_METHOD) {
            fail(expr.span, "method " + name->text +
                            " cannot be used as a value; call it with " +
                            name->text + "()");
            return nullptr;
        }
        if (name->slot == SLOT_SUIT) {
            fail(expr.span, "spacesuit " + name->text +
                            " cannot be used as a value; build one with " +
                            name->text + "()");
            return nullptr;
        }
        return read_slot(slot_of(expr), name->text, expr.span);
    }

    if (const Member *m = std::get_if<Member>(&expr))
        return eval_member(*m, expr.span);
    if (const Call *c = std::get_if<Call>(&expr))
        return eval_call(*c, expr.span);
    if (const Index *i = std::get_if<Index>(&expr))
        return eval_index(*i, expr.span);
    if (const Slice *s = std::get_if<Slice>(&expr))
        return eval_slice(*s, expr.span);
    if (const Unary *u = std::get_if<Unary>(&expr))
        return eval_unary(*u, expr.span);
    if (const Binary *b = std::get_if<Binary>(&expr))
        return eval_binary(*b, expr.span);

    fail(expr.span, "unhandled expression");
    return nullptr;
}

ValuePtr Evaluator::eval_member(const Member &node, Span span)
{
    // Flattened from the parts rather than by rebuilding an Expr around the
    // node, which would allocate on every member access.
    std::vector<std::string> path;
    if (node.target && flatten_path(*node.target, path)) {
        path.push_back(node.name);

        if (path.size() >= 2 && path[1] == "library") {
            if (path.size() != 4) {
                fail(span, "a library path is satellite.library."
                           "<namespace>.<name>, got " + join_path(path));
                return nullptr;
            }
            return read_slot(Slot{SLOT_GLOBAL, path[2], path[3], true},
                             join_path(path), span);
        }

        if (ValuePtr constant = module_constant(path))
            return constant;

        // satellite.console.display without the call parentheses, and any
        // other module path used as a value.
        fail(span, join_path(path) + " is a module path, not a value");
        return nullptr;
    }

    // Bare field access is deliberately not in the language: accessor methods
    // only, so `my_window.height()` is the one spelling (§8.3, §12).
    fail(span, "no bare field access in satellite; call " + node.name +
               "() instead");
    return nullptr;
}

bool Evaluator::eval_args(const std::vector<ExprPtr> &args,
                          std::vector<ValuePtr> &argv)
{
    argv.reserve(args.size());
    for (const ExprPtr &arg : args) {
        if (!arg)
            return false;
        ValuePtr v = eval(*arg);
        if (failed() || !v)
            return false;
        argv.push_back(std::move(v));
    }
    return true;
}

ValuePtr Evaluator::eval_call(const Call &node, Span span)
{
    if (!node.target) {
        fail(span, "call has no target");
        return nullptr;
    }

    // A capsule call. Which name is a capsule was settled by resolve(), so a
    // local of the same name shadows one, exactly as it shadows anything else.
    if (const Name *name = std::get_if<Name>(node.target.get())) {
        if (name->slot >= 0 || is_field_slot(name->slot)) {
            fail(span, name->text + " is a variable, not a capsule");
            return nullptr;
        }

        // my_class_name(...): construction. The one call whose target is a
        // type, and what `my_class_name x(...)` is written into by the parser.
        if (name->slot == SLOT_SUIT) {
            const SpacesuitInfo *suit = resolved_.find_suit(name->text);
            if (!suit) {
                fail(span, "no such spacesuit: " + name->text);
                return nullptr;
            }
            std::vector<ValuePtr> ctor_argv;
            if (!eval_args(node.args, ctor_argv))
                return nullptr;
            return construct(*suit, ctor_argv, span);
        }

        // A bare call inside a method is a call on the SAME receiver, so it
        // dispatches on that receiver's spacesuit like any other method call —
        // an override in a subclass is what an inherited method reaches.
        if (name->slot == SLOT_METHOD) {
            if (!current_self_) {
                fail(span, "internal: " + name->text +
                           " resolved to a method with no receiver");
                return nullptr;
            }
            std::vector<ValuePtr> method_argv;
            if (!eval_args(node.args, method_argv))
                return nullptr;
            // Copied, not referenced: the call may reassign the field the
            // receiver came from, and the object has to survive that.
            ObjectPtr self = current_self_;
            return call_object_method(self, name->text, method_argv, span);
        }

        const CapsuleInfo *info = resolved_.find(name->text);
        if (!info) {
            fail(span, "no such capsule: " + name->text);
            return nullptr;
        }
        // Arguments are evaluated in the CALLER's frame, before the callee's
        // frame exists. That ordering is what makes a recursive call see its
        // own argument rather than the activation it is about to create — the
        // failure §6 measured as fact() returning 1 for every input.
        std::vector<ValuePtr> capsule_argv;
        if (!eval_args(node.args, capsule_argv))
            return nullptr;
        return call_capsule(*info, name->text, capsule_argv, span);
    }

    // Every argument is fully reduced to a Value BEFORE anything that could
    // take a write lock. §7 is explicit that no satellite code may run inside
    // Library::update: std::mutex is not recursive, so my_list.append(
    // my_list.size()) would deadlock on the same variable.
    std::vector<ValuePtr> argv;
    if (!eval_args(node.args, argv))
        return nullptr;

    std::vector<std::string> path;
    if (const Member *m = std::get_if<Member>(node.target.get())) {
        if (m->target && flatten_path(*m->target, path)) {
            path.push_back(m->name);
            // satellite.library.<ns>.<var> is a variable; a call on it is a
            // method call on its value, not a module function.
            if (!(path.size() >= 2 && path[1] == "library")) {
                // ...and so is a call on a module CONSTANT:
                // satellite.bool.true.and(x) is `and` on the value
                // satellite.bool.true, not the module function
                // satellite.bool.true.and. Nothing in the shape of the path
                // distinguishes the two, so the constant table is what decides.
                const std::vector<std::string> receiver(path.begin(),
                                                        path.end() - 1);
                if (ValuePtr constant = module_constant(receiver))
                    return call_method(constant, *m->target, m->name, argv,
                                       span);
                return call_module(path, argv, span);
            }
        }

        // A spacesuit may name a method `append`, and its own method has to
        // win. The receiver's DECLARED type settles that without evaluating
        // it, which matters because the mutator path needs storage rather than
        // a value — evaluating first to find out would run the receiver's side
        // effects before rejecting `foo().append(x)`.
        if (is_mutator(m->name)) {
            const Slot slot = slot_of(*m->target);
            const Type *declared = slot.valid ? declared_type(slot) : nullptr;
            if (!declared || !declared->is_spacesuit())
                return call_mutator(*m->target, m->name, argv, span);
        }

        ValuePtr recv = eval(*m->target);
        if (failed() || !recv)
            return nullptr;
        return call_method(recv, *m->target, m->name, argv, span);
    }

    if (flatten_path(*node.target, path))
        return call_module(path, argv, span);

    fail(span, "this expression is not callable");
    return nullptr;
}

// ---------------------------------------------------------------------------
// Methods
// ---------------------------------------------------------------------------

namespace {

// "satellite.variable.number.plus takes 1 argument, got 2" beats a bare
// "wrong number of arguments" every time.
std::string arity_message(const char *module, const std::string &name,
                          size_t want, size_t got)
{
    std::string out = std::string(module) + "." + name + " takes " +
                      std::to_string(want) +
                      (want == 1 ? " argument, got " : " arguments, got ") +
                      std::to_string(got);
    return out;
}

} // namespace

ValuePtr Evaluator::call_method(const ValuePtr &recv, const Expr &recv_expr,
                                const std::string &name,
                                const std::vector<ValuePtr> &argv, Span span)
{
    (void)recv_expr;

    // An instance answers for its own members before any built-in table is
    // consulted, so a spacesuit may name a method `length` or `to_string`
    // without colliding with the language's.
    if (const ObjectPtr *self = std::get_if<ObjectPtr>(recv.get()))
        return call_object_method(*self, name, argv, span);

    const char *module = module_of(*recv);
    if (!module) {
        fail(span, "nil has no methods, so ." + name + "() has no receiver");
        return nullptr;
    }

    auto arity = [&](size_t want) {
        if (argv.size() == want)
            return true;
        fail(span, arity_message(module, name, want, argv.size()));
        return false;
    };
    auto number_arg = [&](size_t i, const Number *&out) {
        out = std::get_if<Number>(argv[i].get());
        if (!out) {
            fail(span, std::string(module) + "." + name +
                       " wants a satellite.variable.number, got " +
                       to_string(*argv[i]));
            return false;
        }
        return true;
    };

    // --- number ------------------------------------------------------------
    if (const Number *self = std::get_if<Number>(recv.get())) {
        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;
        if (name == "abs")
            return arity(0) ? make_value(self->abs()) : nullptr;
        if (name == "floor")
            return arity(0) ? make_value(self->floor()) : nullptr;
        if (name == "ceil")
            return arity(0) ? make_value(self->ceil()) : nullptr;
        if (name == "round")
            return arity(0) ? make_value(self->round()) : nullptr;

        const Number *rhs = nullptr;
        if (name == "plus")
            return arity(1) && number_arg(0, rhs)
                       ? make_value(Number::add(*self, *rhs)) : nullptr;
        if (name == "minus")
            return arity(1) && number_arg(0, rhs)
                       ? make_value(Number::sub(*self, *rhs)) : nullptr;
        if (name == "times")
            return arity(1) && number_arg(0, rhs)
                       ? make_value(Number::mul(*self, *rhs)) : nullptr;
        if (name == "divided_by") {
            if (!arity(1) || !number_arg(0, rhs))
                return nullptr;
            if (rhs->is_zero()) {
                fail(span, "division by zero");
                return nullptr;
            }
            return make_value(Number::divide(*self, *rhs, division_digits_));
        }
        if (name == "modulo") {
            if (!arity(1) || !number_arg(0, rhs))
                return nullptr;
            if (rhs->is_zero()) {
                fail(span, "modulo by zero");
                return nullptr;
            }
            return make_value(Number::modulo(*self, *rhs));
        }
    }

    // --- string ------------------------------------------------------------
    if (const SatString *self = as_string(*recv)) {
        auto string_arg = [&](size_t i, const SatString *&out) {
            out = as_string(*argv[i]);
            if (!out) {
                fail(span, std::string(module) + "." + name +
                           " wants a satellite.variable.string, got " +
                           to_string(*argv[i]));
                return false;
            }
            return true;
        };

        if (name == "length")
            return arity(0) ? make_value(Number(self->size())) : nullptr;
        if (name == "to_string")
            return arity(0) ? make_value(*self) : nullptr;

        const SatString *rhs = nullptr;
        if (name == "concat")
            return arity(1) && string_arg(0, rhs) ? make_value(*self + *rhs)
                                                  : nullptr;
        if (name == "contains")
            return arity(1) && string_arg(0, rhs)
                       ? make_value(self->find(*rhs) != SatString::npos)
                       : nullptr;
        if (name == "starts_with")
            return arity(1) && string_arg(0, rhs)
                       ? make_value(self->rfind(*rhs, 0) == 0)
                       : nullptr;
        if (name == "ends_with") {
            if (!arity(1) || !string_arg(0, rhs))
                return nullptr;
            bool ok = rhs->size() <= self->size() &&
                      self->compare(self->size() - rhs->size(), rhs->size(),
                                    *rhs) == 0;
            return make_value(ok);
        }
    }

    // --- bool --------------------------------------------------------------
    if (const bool *self = std::get_if<bool>(recv.get())) {
        auto bool_arg = [&](size_t i, bool &out) {
            const bool *b = std::get_if<bool>(argv[i].get());
            if (!b) {
                fail(span, std::string(module) + "." + name +
                           " wants a satellite.variable.bool, got " +
                           to_string(*argv[i]));
                return false;
            }
            out = *b;
            return true;
        };

        if (name == "negate")
            return arity(0) ? make_value(!*self) : nullptr;
        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;

        bool rhs = false;
        if (name == "and")
            return arity(1) && bool_arg(0, rhs) ? make_value(*self && rhs)
                                                : nullptr;
        if (name == "or")
            return arity(1) && bool_arg(0, rhs) ? make_value(*self || rhs)
                                                : nullptr;
    }

    // --- time --------------------------------------------------------------
    if (const Time *self = std::get_if<Time>(recv.get())) {
        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;

        // §8.2's whole arithmetic surface: the difference between two instants
        // is a NUMBER of nanoseconds, not a third instant, because
        // satellite.variable.duration is deferred and a time that could mean
        // either would need a mode flag on every value.
        if (name == "minus") {
            if (!arity(1))
                return nullptr;
            const Time *rhs = std::get_if<Time>(argv[0].get());
            if (!rhs) {
                fail(span, "satellite.variable.time.minus wants a "
                           "satellite.variable.time, got " +
                           to_string(*argv[0]));
                return nullptr;
            }
            return make_value(Number(self->ns - rhs->ns));
        }

        // Exact, all 61 bits of it. It was lossy above 2^53 while a number was
        // a double, which is why §8.2 insisted an ELAPSED time be spelled
        // a.minus(b); §8.1's decimal makes both spellings exact, and the
        // preference for minus() is now about durations rather than precision.
        if (name == "nanoseconds")
            return arity(0) ? make_value(Number(self->ns)) : nullptr;
    }

    // --- file ---------------------------------------------------------------
    if (const FilePtr *self = std::get_if<FilePtr>(recv.get())) {
        const FilePtr &file = *self;
        if (!file) {
            fail(span, "nil has no methods, so ." + name + "() has no receiver");
            return nullptr;
        }

        auto fail_reason = [&](const char *what) {
            const int code = file->last_error.load();
            fail(span, std::string(what) + " " + file->path +
                       (code ? ": " + std::string(strerror(code)) : ""));
        };

        if (name == "ok")
            return arity(0) ? make_value(file->fd.load() >= 0) : nullptr;
        if (name == "path")
            return arity(0) ? make_value(encode_raw(file->path)) : nullptr;
        if (name == "error") {
            if (!arity(0))
                return nullptr;
            const int code = file->last_error.load();
            return make_value(encode_raw(code ? strerror(code) : ""));
        }

        if (name == "read") {
            if (!arity(0))
                return nullptr;
            const int fd = file->fd.load();
            if (fd < 0) {
                fail_reason("cannot read from a closed file:");
                return nullptr;
            }

            std::string bytes;
            char buffer[65536];
            for (;;) {
                const ssize_t got = ::read(fd, buffer, sizeof buffer);
                if (got == 0)
                    break;
                if (got < 0) {
                    if (errno == EINTR)
                        continue;
                    file->last_error.store(errno);
                    fail_reason("cannot read");
                    return nullptr;
                }
                bytes.append(buffer, static_cast<size_t>(got));
            }

            // encode_raw, NEVER encode. §3.3 found this defect twice already —
            // encode() expands backslash escapes everywhere, so reading a
            // source file containing \home would rewrite it to the user's home
            // directory before the lexer ever saw it. Reading a file is the
            // third home of that same bug and the most damaging, because the
            // file being read is usually a program.
            return make_value(encode_raw(bytes));
        }

        if (name == "write") {
            if (!arity(1))
                return nullptr;
            const SatString *text = as_string(*argv[0]);
            if (!text) {
                fail(span, "satellite.variable.file.write wants a "
                           "satellite.variable.string, got " +
                           to_string(*argv[0]));
                return nullptr;
            }
            const int fd = file->fd.load();
            if (fd < 0) {
                fail_reason("cannot write to a closed file:");
                return nullptr;
            }

            const std::string bytes = decode(*text);
            size_t sent = 0;
            while (sent < bytes.size()) {
                const ssize_t put =
                    ::write(fd, bytes.data() + sent, bytes.size() - sent);
                if (put < 0) {
                    if (errno == EINTR)
                        continue;
                    file->last_error.store(errno);
                    return make_value(false);
                }
                sent += static_cast<size_t>(put);
            }
            return make_value(true);
        }

        if (name == "close") {
            if (!arity(0))
                return nullptr;
            // §8.3 requires this to return a status rather than be a
            // destructor: close() is where buffered writes commit, and it is
            // where ENOSPC and EIO are reported. A destructor has nobody left
            // to tell.
            //
            // exchange(), so two snapshots of the same file racing to close it
            // cannot both close the descriptor — the loser would be closing a
            // number the OS had already handed to something else.
            const int fd = file->fd.exchange(-1);
            if (fd < 0)
                return make_value(true);   // closing twice is not a failure
            if (::close(fd) < 0) {
                file->last_error.store(errno);
                return make_value(false);
            }
            return make_value(true);
        }
    }

    // --- list --------------------------------------------------------------
    if (const List *self = as_list(*recv)) {
        if (name == "length")
            return arity(0) ? make_value(Number(self->size())) : nullptr;
        if (name == "to_string")
            return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;
        if (name == "first" || name == "last") {
            if (!arity(0))
                return nullptr;
            if (self->empty()) {
                fail(span, "satellite.container.list." + name +
                           " on an empty list");
                return nullptr;
            }
            return name == "first" ? self->front() : self->back();
        }
        if (name == "contains") {
            if (!arity(1))
                return nullptr;
            for (const ValuePtr &item : *self)
                if (item && value_equals(*item, *argv[0]))
                    return make_value(true);
            return make_value(false);
        }
    }

    fail(span, std::string(module) + " has no method " + name);
    return nullptr;
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

ValuePtr Evaluator::call_module(const std::vector<std::string> &path,
                                const std::vector<ValuePtr> &argv, Span span)
{
    const std::string full = join_path(path);

    // The one constructor the language has that is not a spacesuit's: an
    // instant is read off the clock, never written down as a literal.
    //
    // system_clock rather than steady_clock, because §8.2 fixes the type as an
    // absolute instant since the Unix epoch and steady_clock's epoch is
    // unspecified. The cost is that a clock adjustment can move it; the benefit
    // is that the value means something outside this process.
    if (full == "satellite.time.now") {
        if (!argv.empty()) {
            fail(span, arity_message("satellite.time", "now", 0, argv.size()));
            return nullptr;
        }
        const auto since = std::chrono::system_clock::now().time_since_epoch();
        const auto ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(since);
        return make_value(Time{static_cast<long long>(ns.count())});
    }

    // satellite.file.open(path, mode) — the only way a file value comes into
    // existence, and the reason a declaration cannot make one: opening needs a
    // path and a place to report failure, and a declaration has neither.
    //
    // Modes are strings rather than flags because the language has no enum and
    // wants none; "read", "write" and "append" say what they do at the call
    // site, which a bitmask never does.
    if (full == "satellite.file.open") {
        if (argv.size() != 2) {
            fail(span, arity_message("satellite.file", "open", 2, argv.size()));
            return nullptr;
        }
        const SatString *path = as_string(*argv[0]);
        const SatString *mode = as_string(*argv[1]);
        if (!path || !mode) {
            fail(span, "satellite.file.open wants two "
                       "satellite.variable.string arguments");
            return nullptr;
        }

        const std::string name = decode(*path);
        const std::string how = decode(*mode);

        int flags = 0;
        bool writable = false;
        if (how == "read") {
            flags = O_RDONLY;
        } else if (how == "write") {
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            writable = true;
        } else if (how == "append") {
            flags = O_WRONLY | O_CREAT | O_APPEND;
            writable = true;
        } else {
            fail(span, "satellite.file.open mode must be \"read\", \"write\" "
                       "or \"append\", not \"" + how + "\"");
            return nullptr;
        }

        FilePtr handle = std::make_shared<FileHandle>();
        handle->path = name;
        handle->writable = writable;

        const int fd = ::open(name.c_str(), flags, 0644);
        if (fd < 0) {
            // A failed open is a VALUE, not an error: the caller asks .ok() and
            // decides. Failing here instead would make "does this file exist"
            // unanswerable without crashing the program that asked.
            handle->last_error.store(errno);
        } else {
            handle->fd.store(fd);
        }
        return make_value(Value(std::move(handle)));
    }

    // satellite.directory.* — the working directory.
    //
    // NOT satellite.terminal, however much "cd" feels like a terminal thing:
    // `satl --run script.satl` has no terminal at all and still has a working
    // directory, so naming it after one would be a lie in the headless case,
    // which is the common case. It is also not something the terminal could
    // do — §9 puts the window in a separate process that spawns this one, so
    // the interpreter has no way to tell it anything.
    if (full == "satellite.help") {
        if (argv.empty())
            return make_value(encode_raw(help_overview()));
        if (argv.size() == 1)
            return make_value(encode_raw(help_for(*argv[0])));
        fail(span, arity_message("satellite", "help", 1, argv.size()));
        return nullptr;
    }

    if (full == "satellite.directory.current") {
        if (!argv.empty()) {
            fail(span, arity_message("satellite.directory", "current", 0,
                                     argv.size()));
            return nullptr;
        }
        return make_value(encode_raw(cwd()));
    }

    if (full == "satellite.directory.exists" ||
        full == "satellite.directory.change") {
        const bool changing = full == "satellite.directory.change";
        const char *what = changing ? "change" : "exists";
        if (argv.size() != 1) {
            fail(span, arity_message("satellite.directory", what, 1,
                                     argv.size()));
            return nullptr;
        }
        const SatString *path = as_string(*argv[0]);
        if (!path) {
            fail(span, std::string("satellite.directory.") + what +
                       " wants a satellite.variable.string, got " +
                       to_string(*argv[0]));
            return nullptr;
        }

        const std::string name = decode(*path);
        struct stat info;
        const bool is_dir =
            ::stat(name.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
        if (!changing)
            return make_value(is_dir);

        // Checked before the move rather than reporting chdir's failure after,
        // because "not a directory" and "no such directory" are the two
        // answers a caller wants and chdir collapses several more into errno.
        // Like a failed satellite.file.open, this is a VALUE and not an error:
        // asking whether somewhere is reachable must not kill the program that
        // asked.
        if (!is_dir)
            return make_value(false);
        return make_value(::chdir(name.c_str()) == 0);
    }

    if (full == "satellite.console.display") {
        if (argv.size() != 1) {
            fail(span, arity_message("satellite.console", "display", 1,
                                     argv.size()));
            return nullptr;
        }
        output_ += to_string(*argv[0]);
        output_ += "\n";
        return make_value(std::monostate{});
    }

    fail(span, "no such module function: " + full);
    return nullptr;
}

// ---------------------------------------------------------------------------
// Capsule calls
// ---------------------------------------------------------------------------

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

ValuePtr Evaluator::eval_index(const Index &node, Span span)
{
    if (!node.target || !node.subscript) {
        fail(span, "malformed index");
        return nullptr;
    }
    ValuePtr target = eval(*node.target);
    if (failed() || !target)
        return nullptr;
    ValuePtr sub = eval(*node.subscript);
    if (failed() || !sub)
        return nullptr;

    long long i = 0;
    if (!as_index(*sub, i)) {
        fail(node.subscript->span,
             "index must be a whole satellite.variable.number, got " +
             to_string(*sub));
        return nullptr;
    }

    // An out-of-range INDEX is an error; only a slice clamps (§7).
    if (const List *list = as_list(*target)) {
        long long len = static_cast<long long>(list->size());
        if (i < 0)
            i += len;
        if (i < 0 || i >= len) {
            fail(node.subscript->span, "index " + to_string(*sub) +
                                       " is outside a list of length " +
                                       std::to_string(len));
            return nullptr;
        }
        // O(1) and no copy at all: the child pointer is returned as-is.
        ValuePtr item = (*list)[static_cast<size_t>(i)];
        return item ? item : make_value(std::monostate{});
    }

    if (const SatString *s = as_string(*target)) {
        // Selects satellite characters, not display characters: encode("hi
        // \home!") is 4 SatChars that decode to 16 display bytes, so s[2] is
        // one unit that displays as a whole home directory. It is the only
        // O(1), stable rule, and it is unique to satellite (§7).
        long long len = static_cast<long long>(s->size());
        if (i < 0)
            i += len;
        if (i < 0 || i >= len) {
            fail(node.subscript->span, "index " + to_string(*sub) +
                                       " is outside a string of length " +
                                       std::to_string(len));
            return nullptr;
        }
        return make_value(SatString(1, (*s)[static_cast<size_t>(i)]));
    }

    fail(span, to_string(*target) + " cannot be indexed");
    return nullptr;
}

ValuePtr Evaluator::eval_slice(const Slice &node, Span span)
{
    if (!node.target) {
        fail(span, "malformed slice");
        return nullptr;
    }
    ValuePtr target = eval(*node.target);
    if (failed() || !target)
        return nullptr;

    long long len = 0;
    if (const List *list = as_list(*target))
        len = static_cast<long long>(list->size());
    else if (const SatString *s = as_string(*target))
        len = static_cast<long long>(s->size());
    else {
        fail(span, to_string(*target) + " cannot be sliced");
        return nullptr;
    }

    auto bound = [&](const ExprPtr &e, long long fallback, long long &out) {
        if (!e) {
            out = fallback;
            return true;
        }
        ValuePtr v = eval(*e);
        if (failed() || !v)
            return false;
        if (!as_index(*v, out)) {
            fail(e->span, "slice bound must be a whole "
                          "satellite.variable.number, got " + to_string(*v));
            return false;
        }
        return true;
    };

    long long lo = 0;
    long long hi = 0;
    if (!bound(node.lo, 0, lo) || !bound(node.hi, len, hi))
        return nullptr;
    clamp_range(lo, hi, len);

    if (const List *list = as_list(*target)) {
        // A full slice is the same list: no copy, and pointer identity is
        // preserved so `l[:]` is genuinely free.
        if (lo == 0 && hi == len)
            return target;
        // Slicing copies POINTERS only — the children stay shared.
        return make_value(List(list->begin() + lo, list->begin() + hi));
    }

    const SatString &s = *as_string(*target);
    if (lo == 0 && hi == len)
        return target;
    // Unlike a list slice, a string slice really does copy characters.
    return make_value(s.substr(static_cast<size_t>(lo),
                               static_cast<size_t>(hi - lo)));
}

// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------

ValuePtr Evaluator::eval_unary(const Unary &node, Span span)
{
    if (!node.operand) {
        fail(span, "malformed unary " + node.op);
        return nullptr;
    }
    ValuePtr v = eval(*node.operand);
    if (failed() || !v)
        return nullptr;

    if (node.op == "-") {
        const Number *n = std::get_if<Number>(v.get());
        if (!n) {
            fail(span, "unary - wants a satellite.variable.number, got " +
                       to_string(*v));
            return nullptr;
        }
        return make_value(n->negated());
    }
    if (node.op == "!") {
        const bool *b = std::get_if<bool>(v.get());
        if (!b) {
            fail(span, "! wants a satellite.variable.bool, got " +
                       to_string(*v));
            return nullptr;
        }
        return make_value(!*b);
    }

    fail(span, "unknown unary operator " + node.op);
    return nullptr;
}

ValuePtr Evaluator::eval_binary(const Binary &node, Span span)
{
    if (!node.left || !node.right) {
        fail(span, "malformed " + node.op);
        return nullptr;
    }
    ValuePtr lhs = eval(*node.left);
    if (failed() || !lhs)
        return nullptr;
    ValuePtr rhs = eval(*node.right);
    if (failed() || !rhs)
        return nullptr;

    // Equality is defined on every pair of values, including across types,
    // where it is simply false. Ordering is not.
    if (node.op == "==")
        return make_value(value_equals(*lhs, *rhs));
    if (node.op == "!=")
        return make_value(!value_equals(*lhs, *rhs));

    const Number *ln = std::get_if<Number>(lhs.get());
    const Number *rn = std::get_if<Number>(rhs.get());
    const SatString *ls = as_string(*lhs);
    const SatString *rs = as_string(*rhs);

    if (node.op == "+" && ls && rs)
        return make_value(*ls + *rs);

    if (ln && rn) {
        if (node.op == "+") return make_value(Number::add(*ln, *rn));
        if (node.op == "-") return make_value(Number::sub(*ln, *rn));
        if (node.op == "*") return make_value(Number::mul(*ln, *rn));
        if (node.op == "/") {
            if (rn->is_zero()) {
                fail(span, "division by zero");
                return nullptr;
            }
            return make_value(Number::divide(*ln, *rn, division_digits_));
        }
        if (node.op == "%") {
            if (rn->is_zero()) {
                fail(span, "modulo by zero");
                return nullptr;
            }
            return make_value(Number::modulo(*ln, *rn));
        }
        const int order = Number::compare(*ln, *rn);
        if (node.op == "<")  return make_value(order < 0);
        if (node.op == "<=") return make_value(order <= 0);
        if (node.op == ">")  return make_value(order > 0);
        if (node.op == ">=") return make_value(order >= 0);
    }

    if (ls && rs) {
        if (node.op == "<")  return make_value(*ls < *rs);
        if (node.op == "<=") return make_value(*ls <= *rs);
        if (node.op == ">")  return make_value(*ls > *rs);
        if (node.op == ">=") return make_value(*ls >= *rs);
    }

    fail(span, node.op + " does not apply to " + to_string(*lhs) + " and " +
               to_string(*rhs));
    return nullptr;
}

} // namespace satellite
