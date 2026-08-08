#include "env.hpp"

#include <algorithm>
#include <unordered_set>

namespace satellite {
namespace {

// The resolver walks the tree recursively, so it needs the same kind of bound
// the evaluator does — and for the same reason: without one, a deeply nested
// expression segfaults the C++ stack instead of producing an error. Kept well
// under what an 8 MB stack holds for this frame size.
constexpr int MAX_RESOLVE_DEPTH = 2000;

struct DepthGuard {
    explicit DepthGuard(int &depth) : depth_(depth) { depth_++; }
    ~DepthGuard() { depth_--; }

    DepthGuard(const DepthGuard &) = delete;
    DepthGuard &operator=(const DepthGuard &) = delete;

private:
    int &depth_;
};

// The name a capsule is called by: bare for a user capsule, prefixed for a
// reserved one, exactly as §1 has it.
std::string capsule_key(const Capsule &capsule)
{
    return capsule.reserved ? "satellite." + capsule.name : capsule.name;
}

class Resolver {
public:
    explicit Resolver(ResolveResult &out) : out_(out) {}

    void run(const Program &program);

private:
    using Scope = std::unordered_map<std::string, int>;

    void collect(const Program &program);
    void collect_suits(const Program &program);
    void link_supers();
    void break_inheritance_cycles();
    void build_layout(SpacesuitInfo &info);
    void resolve_bodies(SpacesuitInfo &info);
    void resolve_suit(SpacesuitInfo &info);
    void resolve_capsule(const Capsule &capsule, CapsuleInfo &info);
    void resolve_stmt(const Stmt &stmt);
    void resolve_expr(const Expr &expr);
    void resolve_name(const Name &name, Span span);
    void resolve_call(const Call &call, Span span);
    void check_type(const Type &type, Span span);

    int declare(const std::string &name, const Type &type, Span span);
    const int *lookup(const std::string &name) const;
    int lookup_field(const std::string &name) const;

    void fail(Span span, std::string message)
    {
        out_.errors.push_back(ResolveError{std::move(message), span});
    }

    ResolveResult &out_;

    // Null at the top level, which is what switches the whole pass between
    // "allocate a frame slot" and "leave it to satellite.library".
    CapsuleInfo *info_ = nullptr;

    // The spacesuit whose member is being resolved, or null outside one. It is
    // a SECOND switch, independent of info_: a method body has both (a frame
    // and a suit), a field initialiser has only a suit, an ordinary capsule has
    // only a frame, and top-level code has neither.
    const SpacesuitInfo *suit_ = nullptr;

    // How many of suit_'s fields are in scope. Always every field inside a
    // method body; inside a field's own initialiser it is that field's index,
    // so `satellite.variable.number n = n` is an unknown-name error rather than
    // a read of the slot it is about to write — the same rule resolve_stmt
    // already applies to a local declaration.
    size_t field_limit_ = 0;

    std::vector<Scope> scopes_;
    std::unordered_set<const SpacesuitInfo *> laid_out_;

    // The members each suit declared ITSELF, kept between the layout pass and
    // the body pass. Inherited entries are copies and were already resolved in
    // the suit that declared them; resolving them again would stamp the same
    // AST twice, and once is the contract Name::slot documents.
    std::unordered_map<const SpacesuitInfo *, std::vector<const Method *>>
        own_methods_;

    int depth_ = 0;
};

const int *Resolver::lookup(const std::string &name) const
{
    // Innermost scope first, so an inner declaration shadows an outer one.
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        auto found = scope->find(name);
        if (found != scope->end())
            return &found->second;
    }
    return nullptr;
}

int Resolver::declare(const std::string &name, const Type &type, Span span)
{
    // §1's one reserved word, enforced at the binding site.
    if (name == "satellite") {
        fail(span, "satellite is reserved and cannot be declared");
        return SLOT_GLOBAL;
    }

    // Top-level declarations are globals. §6 demotes capsule locals only, and
    // §10's main.x ruling means `satellite.variable.number x = 1` at the top
    // level has to stay observable as satellite.library.main.x.
    if (!info_)
        return SLOT_GLOBAL;

    Scope &scope = scopes_.back();
    auto found = scope.find(name);
    if (found != scope.end()) {
        fail(span, name + " is already declared in this scope");
        return found->second;
    }

    // Slots are never reused across scopes. Reuse would save a few pointers
    // per frame and cost the ability to name a slot in a diagnostic.
    int slot = static_cast<int>(info_->slot_count++);
    info_->slot_types.push_back(type);
    info_->slot_names.push_back(name);
    scope.emplace(name, slot);
    return slot;
}

// The fields of suit_ that are in scope right now, searched linearly because
// this runs once per name at resolve time and never during the walk.
int Resolver::lookup_field(const std::string &name) const
{
    if (!suit_)
        return -1;
    const size_t limit = std::min(field_limit_, suit_->fields.size());
    for (size_t i = 0; i < limit; i++)
        if (suit_->fields[i].name == name)
            return static_cast<int>(i);
    return -1;
}

// A spacesuit type names a spacesuit, and this is where that is checked —
// before anything runs, rather than at the declaration that would have
// constructed the instance. Recursive because the type may be a container's
// element type: satellite.container.list<my_class> is a list of instances.
void Resolver::check_type(const Type &type, Span span)
{
    if (type.is_spacesuit() && !out_.suits.count(type.name))
        fail(type.span.end > type.span.start ? type.span : span,
             "no such spacesuit: " + type.name);
    for (const Type &arg : type.args)
        check_type(arg, span);
}

void Resolver::resolve_name(const Name &name, Span span)
{
    // Innermost outwards: a local, then a field of the enclosing spacesuit,
    // then a method of it, then a capsule. Each shadows the next, which is the
    // same rule an inner block already follows against an outer one.
    if (const int *slot = lookup(name.text)) {
        name.slot = *slot;
        return;
    }

    if (int index = lookup_field(name.text); index >= 0) {
        name.slot = field_slot(index);
        return;
    }

    if (suit_ && suit_->find_method(name.text)) {
        name.slot = SLOT_METHOD;
        return;
    }

    if (out_.capsules.count(name.text)) {
        name.slot = SLOT_CAPSULE;
        return;
    }

    if (out_.suits.count(name.text)) {
        name.slot = SLOT_SUIT;
        return;
    }

    if (!info_ && !suit_) {
        // Top level. An unknown name here is NOT an error: the REPL declares a
        // variable on one line and reads it on the next, and those are two
        // separate Programs that no single resolve() call can see at once. It
        // resolves against satellite.library at run time.
        name.slot = SLOT_GLOBAL;
        return;
    }

    // Inside a capsule the opposite holds. A name that is not a parameter, not
    // a local and not a capsule is an error here rather than an implicit read
    // of a global — that is what makes capsules lexically closed, and it is
    // what makes §6's eight-thread result mean anything. Globals are still
    // reachable, by the four-segment satellite.library path.
    //
    // A spacesuit member is closed the same way, and for a second reason: a
    // name that silently fell through to a global would make a typo'd field
    // read a variable somewhere else in the program.
    fail(span, suit_ ? "unknown variable in spacesuit " + suit_->name + ": " +
                           name.text
                     : "unknown variable in capsule: " + name.text);
    name.slot = SLOT_GLOBAL;
}

void Resolver::resolve_call(const Call &call, Span span)
{
    if (!call.target) {
        for (const ExprPtr &arg : call.args)
            if (arg)
                resolve_expr(*arg);
        return;
    }

    if (const Name *callee = std::get_if<Name>(call.target.get())) {
        // The scope lookup comes FIRST so a local shadows a capsule of the
        // same name, matching how every other name resolves.
        if (!lookup(callee->text)) {
            // Arity is knowable for both kinds, so a wrong call count is a
            // static error instead of something the tree walk trips over later.
            auto arity = [&](const std::string &what, size_t want) {
                if (call.args.size() != want)
                    fail(span, what + " takes " + std::to_string(want) +
                               (want == 1 ? " argument, got "
                                          : " arguments, got ") +
                               std::to_string(call.args.size()));
                for (const ExprPtr &arg : call.args)
                    if (arg)
                        resolve_expr(*arg);
            };

            // A bare call inside a spacesuit means a method of that spacesuit
            // before it means a top-level capsule, so a suit can name a method
            // whatever it likes without colliding with the rest of the program.
            if (suit_) {
                if (const MethodInfo *method = suit_->find_method(callee->text)) {
                    callee->slot = SLOT_METHOD;
                    arity(callee->text, method->info->param_count);
                    return;
                }
            }

            auto found = out_.capsules.find(callee->text);
            if (found != out_.capsules.end()) {
                callee->slot = SLOT_CAPSULE;
                arity(callee->text, found->second.param_count);
                return;
            }

            // my_class_name(...) builds an instance, and its arguments are the
            // arguments of the suit's constructor. `my_suit x("data")` is the
            // same call, written by the parser.
            if (const SpacesuitInfo *built = out_.find_suit(callee->text)) {
                callee->slot = SLOT_SUIT;
                arity(callee->text, built->ctor_params());
                return;
            }
        }
    }

    resolve_expr(*call.target);
    for (const ExprPtr &arg : call.args)
        if (arg)
            resolve_expr(*arg);
}

void Resolver::resolve_expr(const Expr &expr)
{
    if (depth_ >= MAX_RESOLVE_DEPTH) {
        fail(expr.span, "expression nests deeper than the resolver can walk (" +
                        std::to_string(MAX_RESOLVE_DEPTH) + ")");
        return;
    }
    DepthGuard guard(depth_);

    if (const Name *name = std::get_if<Name>(&expr)) {
        resolve_name(*name, expr.span);
        return;
    }
    if (const Call *call = std::get_if<Call>(&expr)) {
        resolve_call(*call, expr.span);
        return;
    }
    if (const Member *member = std::get_if<Member>(&expr)) {
        // Only the receiver can contain a name; the member itself is a
        // selector, resolved against the receiver's value at run time.
        if (member->target)
            resolve_expr(*member->target);
        return;
    }
    if (const Index *index = std::get_if<Index>(&expr)) {
        if (index->target)
            resolve_expr(*index->target);
        if (index->subscript)
            resolve_expr(*index->subscript);
        return;
    }
    if (const Slice *slice = std::get_if<Slice>(&expr)) {
        if (slice->target)
            resolve_expr(*slice->target);
        if (slice->lo)
            resolve_expr(*slice->lo);
        if (slice->hi)
            resolve_expr(*slice->hi);
        return;
    }
    if (const Unary *unary = std::get_if<Unary>(&expr)) {
        if (unary->operand)
            resolve_expr(*unary->operand);
        return;
    }
    if (const Binary *binary = std::get_if<Binary>(&expr)) {
        if (binary->left)
            resolve_expr(*binary->left);
        if (binary->right)
            resolve_expr(*binary->right);
        return;
    }

    // NumberLit, StringLit and SatelliteLit hold no names.
}

void Resolver::resolve_stmt(const Stmt &stmt)
{
    if (depth_ >= MAX_RESOLVE_DEPTH) {
        fail(stmt.span, "statements nest deeper than the resolver can walk (" +
                        std::to_string(MAX_RESOLVE_DEPTH) + ")");
        return;
    }
    DepthGuard guard(depth_);

    if (const VarDecl *decl = std::get_if<VarDecl>(&stmt)) {
        // The initialiser is resolved BEFORE the name is declared, so
        // `satellite.variable.number x = x` is an unknown-variable error
        // rather than a read of the uninitialised slot it is writing.
        check_type(decl->type, stmt.span);

        // `my_suit x` with no arguments still constructs, so it still has to
        // satisfy the constructor. Checked here rather than at the construction
        // site, because the bare declaration has no Call node for resolve_call
        // to have checked already.
        if (!decl->init && decl->type.is_spacesuit()) {
            const SpacesuitInfo *suit = out_.find_suit(decl->type.name);
            if (suit && suit->ctor_params() != 0)
                fail(stmt.span, decl->type.name + "'s constructor takes " +
                                std::to_string(suit->ctor_params()) +
                                (suit->ctor_params() == 1 ? " argument"
                                                          : " arguments") +
                                ", so " + decl->name +
                                " has to be declared with them: " +
                                decl->type.name + " " + decl->name + "(...)");
        }

        if (decl->init)
            resolve_expr(*decl->init);
        decl->slot = declare(decl->name, decl->type, stmt.span);
        return;
    }

    if (const Assign *assign = std::get_if<Assign>(&stmt)) {
        if (assign->value)
            resolve_expr(*assign->value);
        if (assign->target)
            resolve_expr(*assign->target);
        return;
    }

    if (const ExprStmt *expr = std::get_if<ExprStmt>(&stmt)) {
        if (expr->expr)
            resolve_expr(*expr->expr);
        return;
    }

    if (const Return *ret = std::get_if<Return>(&stmt)) {
        if (ret->value)
            resolve_expr(*ret->value);
        return;
    }

    if (const Block *block = std::get_if<Block>(&stmt)) {
        scopes_.emplace_back();
        for (const StmtPtr &inner : block->statements)
            if (inner)
                resolve_stmt(*inner);
        scopes_.pop_back();
        return;
    }

    if (const If *node = std::get_if<If>(&stmt)) {
        if (node->condition)
            resolve_expr(*node->condition);
        if (node->then_branch)
            resolve_stmt(*node->then_branch);
        if (node->else_branch)
            resolve_stmt(*node->else_branch);
        return;
    }

    if (const While *node = std::get_if<While>(&stmt)) {
        if (node->condition)
            resolve_expr(*node->condition);
        if (node->body)
            resolve_stmt(*node->body);
        return;
    }

    if (const For *node = std::get_if<For>(&stmt)) {
        // The loop gets its own scope so that a declaration in the init clause
        // belongs to the loop and does not leak past it.
        scopes_.emplace_back();
        if (node->init)
            resolve_stmt(*node->init);
        if (node->condition)
            resolve_expr(*node->condition);
        if (node->step)
            resolve_stmt(*node->step);
        if (node->body)
            resolve_stmt(*node->body);
        scopes_.pop_back();
        return;
    }
}

void Resolver::resolve_capsule(const Capsule &capsule, CapsuleInfo &info)
{
    info_ = &info;
    scopes_.clear();
    scopes_.emplace_back();

    for (const Param &param : capsule.params) {
        check_type(param.type, param.span);
        // A slot is allocated for EVERY parameter, including one that fails to
        // bind, so slot positions keep matching argument positions. Dropping a
        // slot here would silently shift every later parameter by one.
        int slot = static_cast<int>(info.slot_count++);
        info.slot_types.push_back(param.type);
        info.slot_names.push_back(param.name);

        if (param.name == "satellite") {
            fail(param.span, "satellite is reserved and cannot name a parameter");
            continue;
        }
        if (!scopes_.back().emplace(param.name, slot).second)
            fail(param.span, "duplicate parameter " + param.name);
    }

    if (capsule.body)
        resolve_stmt(*capsule.body);

    info_ = nullptr;
    scopes_.clear();
}

// ---------------------------------------------------------------------------
// Spacesuits
// ---------------------------------------------------------------------------

void Resolver::collect_suits(const Program &program)
{
    for (const TopLevel &item : program.items) {
        const Spacesuit *suit = std::get_if<Spacesuit>(&item);
        if (!suit)
            continue;

        if (suit->name.empty())
            continue;   // the parser already reported why

        // One namespace for both, because both are named by a bare word and
        // `foo(1)` would otherwise have two readings.
        if (out_.capsules.count(suit->name)) {
            fail(suit->span, suit->name +
                             " is already a capsule; a spacesuit and a capsule "
                             "cannot share a name");
            continue;
        }

        auto inserted = out_.suits.emplace(suit->name, SpacesuitInfo{});
        if (!inserted.second) {
            fail(suit->span, "spacesuit " + suit->name + " is already defined");
            continue;
        }
        inserted.first->second.suit = suit;
        inserted.first->second.name = suit->name;
    }
}

void Resolver::link_supers()
{
    for (auto &entry : out_.suits) {
        SpacesuitInfo &info = entry.second;
        if (!info.suit || info.suit->super.empty())
            continue;

        auto found = out_.suits.find(info.suit->super);
        if (found == out_.suits.end()) {
            fail(info.suit->super_span,
                 "no such spacesuit to inherit from: " + info.suit->super);
            continue;
        }
        if (&found->second == &info) {
            fail(info.suit->super_span, info.name + " inherits from itself");
            continue;
        }
        info.super = &found->second;
    }
}

// Every later pass walks the superclass chain assuming it ends, so the cycles
// have to be gone before any of them runs — not reported by the first one that
// hangs. Both checks are needed: a suit ON a cycle finds itself, and a suit
// merely POINTING INTO one never does, so it needs the step bound.
void Resolver::break_inheritance_cycles()
{
    const size_t limit = out_.suits.size();
    for (auto &entry : out_.suits) {
        SpacesuitInfo &info = entry.second;
        size_t steps = 0;
        for (const SpacesuitInfo *walk = info.super; walk; walk = walk->super) {
            if (walk == &info) {
                fail(info.suit->super_span,
                     info.name + " inherits from itself through " +
                     info.suit->super);
                info.super = nullptr;
                break;
            }
            if (++steps > limit) {
                fail(info.suit->super_span,
                     "the inheritance chain above " + info.name +
                     " does not terminate");
                info.super = nullptr;
                break;
            }
        }
    }
}

// The layout: the superclass's fields and methods copied in, then this suit's
// own appended (fields) or overriding (methods). Copying rather than chaining
// is what leaves a field access as one index and a method call as one hash
// lookup, with inheritance costing nothing at the point of use.
void Resolver::build_layout(SpacesuitInfo &info)
{
    if (info.super) {
        info.fields = info.super->fields;
        info.methods = info.super->methods;
    }

    for (const SuitItem &item : info.suit->items) {
        const Field *field = std::get_if<Field>(&item);
        if (!field)
            continue;

        check_type(field->type, field->span);

        if (int at = info.find_field(field->name); at >= 0) {
            const SpacesuitInfo *owner = info.fields[static_cast<size_t>(at)].owner;
            fail(field->span,
                 "field " + field->name + " is already declared in " +
                 (owner && owner != &info ? owner->name
                                          : std::string("this spacesuit")));
            continue;
        }

        FieldInfo entry;
        entry.name = field->name;
        entry.type = field->type;
        entry.access = field->access;
        entry.init = field->init.get();
        entry.owner = &info;
        info.fields.push_back(std::move(entry));
    }

    // The constructor chain: the superclass's, then this suit's own.
    if (info.super)
        info.ctors = info.super->ctors;

    // Every method goes in BEFORE any body is walked, so a method may call one
    // declared further down the suit — the same reason capsule names are
    // collected before capsule bodies.
    std::vector<const Method *> own;
    for (const SuitItem &item : info.suit->items) {
        const Method *method = std::get_if<Method>(&item);
        if (!method || method->capsule.name.empty())
            continue;

        if (method->constructor) {
            if (!info.ctors.empty() && info.ctors.back().owner == &info) {
                fail(method->capsule.span,
                     info.name + " already has a constructor");
                continue;
            }
            MethodInfo entry;
            entry.access = method->access;
            entry.owner = &info;
            entry.info = std::make_shared<CapsuleInfo>();
            entry.info->capsule = &method->capsule;
            entry.info->param_count = method->capsule.params.size();
            info.ctors.push_back(std::move(entry));
            own.push_back(method);
            continue;
        }

        const std::string &name = method->capsule.name;
        if (info.find_field(name) >= 0) {
            fail(method->capsule.span,
                 name + " is already a field of " + info.name +
                 "; a field and a method cannot share a name");
            continue;
        }

        // A name defined twice in ONE suit is a mistake; the same name defined
        // in a superclass is an override, which is the point.
        bool duplicate = false;
        for (const Method *seen : own)
            duplicate = duplicate || seen->capsule.name == name;
        if (duplicate) {
            fail(method->capsule.span,
                 "method " + name + " is already defined in " + info.name);
            continue;
        }

        MethodInfo entry;
        entry.access = method->access;
        entry.owner = &info;
        entry.info = std::make_shared<CapsuleInfo>();
        entry.info->capsule = &method->capsule;
        entry.info->param_count = method->capsule.params.size();
        info.methods.insert_or_assign(name, std::move(entry));
        own.push_back(method);
    }

    // Only the last constructor in the chain is handed the site's arguments, so
    // every earlier one has to be callable with none. Reported here rather than
    // at the construction site, because it is a fact about the two spacesuits
    // and holds however many times an instance is built.
    for (size_t i = 0; i + 1 < info.ctors.size(); i++) {
        const MethodInfo &ctor = info.ctors[i];
        if (!ctor.info || ctor.info->param_count == 0)
            continue;
        fail(info.suit->super_span,
             info.name + " inherits from " +
             (ctor.owner ? ctor.owner->name : std::string("a spacesuit")) +
             ", whose constructor takes " +
             std::to_string(ctor.info->param_count) +
             (ctor.info->param_count == 1 ? " argument" : " arguments") +
             ", and there is no syntax yet for passing them");
    }

    own_methods_[&info] = std::move(own);
}

// Bodies, once every spacesuit's layout exists — a method may construct a
// spacesuit declared further down the file, and its constructor's arity has to
// be known before that call can be checked.
void Resolver::resolve_bodies(SpacesuitInfo &info)
{
    for (const Method *method : own_methods_[&info]) {
        const std::shared_ptr<CapsuleInfo> &target =
            method->constructor ? info.ctors.back().info
                                : info.methods.at(method->capsule.name).info;
        suit_ = &info;
        field_limit_ = info.fields.size();
        resolve_capsule(method->capsule, *target);
        suit_ = nullptr;
    }

    // Field initialisers, each with only the fields declared BEFORE it in
    // scope. They resolve with no frame, because construction happens at a
    // declaration rather than inside a call — an initialiser has a receiver but
    // no activation.
    const size_t inherited = info.super ? info.super->fields.size() : 0;
    for (size_t i = inherited; i < info.fields.size(); i++) {
        if (!info.fields[i].init)
            continue;
        info_ = nullptr;
        scopes_.clear();
        suit_ = &info;
        field_limit_ = i;
        resolve_expr(*info.fields[i].init);
        suit_ = nullptr;
    }
}

void Resolver::resolve_suit(SpacesuitInfo &info)
{
    // Exactly once each. A diamond (two suits inheriting one base) reaches the
    // base twice, and building a layout twice would append its own fields on
    // top of themselves and then report every one of them as a duplicate.
    if (!laid_out_.insert(&info).second)
        return;

    // Superclass first: this suit's layout is built on top of a finished one.
    // Cycles are already gone, so the recursion terminates.
    if (info.super)
        resolve_suit(out_.suits.at(info.suit->super));

    build_layout(info);
}

void Resolver::collect(const Program &program)
{
    for (const TopLevel &item : program.items) {
        const Capsule *capsule = std::get_if<Capsule>(&item);
        if (!capsule)
            continue;

        if (!capsule->reserved && capsule->name == "satellite") {
            fail(capsule->span, "satellite is reserved and cannot name a capsule");
            continue;
        }

        std::string key = capsule_key(*capsule);
        auto inserted = out_.capsules.emplace(key, CapsuleInfo{});
        if (!inserted.second) {
            fail(capsule->span, "capsule " + key + " is already defined");
            continue;
        }
        inserted.first->second.capsule = capsule;
        inserted.first->second.param_count = capsule->params.size();
    }
}

void Resolver::run(const Program &program)
{
    // Pass 1: every capsule name, before any body is walked. This is the pass
    // the parser could not have done — it is what makes a call to a capsule
    // defined further down, and mutual recursion, resolvable at all.
    collect(program);

    // Pass 2: every spacesuit name, then the links between them, then the
    // layouts. Names before links because a suit may inherit from one declared
    // further down; links before layouts because a layout is built on top of
    // its superclass's; cycles broken between the two because everything after
    // walks the chain assuming it ends.
    collect_suits(program);
    link_supers();
    break_inheritance_cycles();
    for (auto &entry : out_.suits)
        resolve_suit(entry.second);
    // Every layout before any body: a method may construct a spacesuit declared
    // further down the file, and that call's arity comes from the other suit's
    // constructor.
    for (auto &entry : out_.suits)
        resolve_bodies(entry.second);

    // Pass 3: top-level statements. Globals, not slots.
    info_ = nullptr;
    suit_ = nullptr;
    scopes_.clear();
    for (const TopLevel &item : program.items) {
        const StmtPtr *stmt = std::get_if<StmtPtr>(&item);
        if (stmt && *stmt)
            resolve_stmt(**stmt);
    }

    // Pass 4: capsule bodies.
    for (const TopLevel &item : program.items) {
        const Capsule *capsule = std::get_if<Capsule>(&item);
        if (!capsule)
            continue;
        auto found = out_.capsules.find(capsule_key(*capsule));
        // The == check skips a duplicate definition, whose name is owned by
        // the first one that claimed it.
        if (found != out_.capsules.end() && found->second.capsule == capsule)
            resolve_capsule(*capsule, found->second);
    }
}

} // namespace

const CapsuleInfo *ResolveResult::find(const std::string &name) const
{
    auto found = capsules.find(name);
    return found == capsules.end() ? nullptr : &found->second;
}

const SpacesuitInfo *ResolveResult::find_suit(const std::string &name) const
{
    auto found = suits.find(name);
    return found == suits.end() ? nullptr : &found->second;
}

bool SpacesuitInfo::is_a(const std::string &other) const
{
    // Walks rather than consults a set: a chain is short, and the walk is only
    // reached by a type check at a declaration or a call, never in a loop body
    // over a container. Cycles were broken by resolve(), so it terminates.
    for (const SpacesuitInfo *walk = this; walk; walk = walk->super)
        if (walk->name == other)
            return true;
    return false;
}

const MethodInfo *SpacesuitInfo::find_method(const std::string &name) const
{
    auto found = methods.find(name);
    return found == methods.end() ? nullptr : &found->second;
}

int SpacesuitInfo::find_field(const std::string &name) const
{
    for (size_t i = 0; i < fields.size(); i++)
        if (fields[i].name == name)
            return static_cast<int>(i);
    return -1;
}

// Declared in value.hpp, which sits below env.hpp and therefore knows
// SpacesuitInfo only as a forward declaration. The fallback is not decoration:
// to_string() prints a Value from anywhere, including from an error message
// about an object whose construction did not finish.
const std::string &suit_name(const SpacesuitInfo *suit)
{
    static const std::string unknown = "spacesuit";
    return suit ? suit->name : unknown;
}

ResolveResult resolve(const Program &program)
{
    ResolveResult result;
    Resolver resolver(result);
    resolver.run(program);
    return result;
}

std::string format_error(const ResolveError &error, const std::string &source)
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

} // namespace satellite
