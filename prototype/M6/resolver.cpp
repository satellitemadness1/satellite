// The 4-pass Resolver Driver and Entry Point -- Milestone 6 Prototype.
//
// DESIGN §7.3 & PLAN §8:
// Pass 1: Collect every capsule declaration (enables mutual recursion & forward refs).
// Pass 2: Collect spacesuits, link superclasses, break cycles, flatten layouts, resolve bodies.
// Pass 3: Resolve top-level statements (globals).
// Pass 4: Resolve capsule bodies into frame slots.

#include "resolver.hpp"

namespace satellite {

Resolver::Resolver(const Program &program, AstArena &arena, words::Words &words, ResolveResult &out)
    : program_(program), arena_(arena), words_(words), out_(out)
{
}

void Resolver::collect_capsules()
{
    for (NodeIndex idx : program_.items) {
        const auto &node = arena_.get(idx);
        if (node.kind != NodeKind::Capsule)
            continue;

        const auto &capsule = std::get<CapsuleDecl>(node.data);

        if (!capsule.reserved && capsule.name == "satellite") {
            error(ErrorCode::E0007_ReservedWordAsIdentifier, node.span,
                  "satellite is reserved and cannot name a capsule");
            continue;
        }

        std::string key = capsule_key(capsule);
        auto inserted = out_.capsules.emplace(key, CapsuleInfo{});
        if (!inserted.second) {
            std::string msg = "capsule " + key + " is already defined";
            const CapsuleInfo &first = inserted.first->second;
            if (first.capsule) {
                error_with_note(ErrorCode::E0205_DuplicateDeclaration, node.span, msg,
                                first.capsule->span, "first defined here");
            } else {
                error(ErrorCode::E0205_DuplicateDeclaration, node.span, msg);
            }
            continue;
        }

        CapsuleInfo &info = inserted.first->second;
        info.capsule = &capsule;
        info.node = idx;
        info.name = capsule.name;
        info.reserved = capsule.reserved;
        info.param_count = capsule.params.size();
        info.path_id = capsule.path_id;

        out_.table.set_slot(idx, SLOT_CAPSULE);
    }
}

void Resolver::resolve_top_level()
{
    info_ = nullptr;
    suit_ = nullptr;
    scopes_.clear();

    for (NodeIndex idx : program_.items) {
        const auto &node = arena_.get(idx);
        if (node.kind == NodeKind::VarDecl ||
            node.kind == NodeKind::Assign  ||
            node.kind == NodeKind::ExprStmt ||
            node.kind == NodeKind::Block   ||
            node.kind == NodeKind::If      ||
            node.kind == NodeKind::While   ||
            node.kind == NodeKind::For) {
            resolve_stmt(idx);
        } else if (node.kind == NodeKind::GlobalDecl) {
            const auto &glob = std::get<GlobalDecl>(node.data);
            if (glob.init != kNullNode)
                resolve_expr(glob.init);
            out_.table.set_slot(idx, SLOT_GLOBAL);
        }
    }
}

void Resolver::resolve_all_capsules()
{
    for (NodeIndex idx : program_.items) {
        const auto &node = arena_.get(idx);
        if (node.kind != NodeKind::Capsule)
            continue;

        const auto &capsule = std::get<CapsuleDecl>(node.data);
        auto it = out_.capsules.find(capsule_key(capsule));
        if (it != out_.capsules.end() && it->second.capsule == &capsule) {
            resolve_capsule(idx, capsule, it->second);
        }
    }
}

void Resolver::run(const ResolveResult *inherited)
{
    // Pass 1: Every capsule declaration
    collect_capsules();

    // Pass 2: Every spacesuit declaration, inheritance links, layout flattening
    collect_suits();

    std::unordered_set<std::string> borrowed;
    if (inherited) {
        for (const auto &entry : inherited->capsules)
            out_.capsules.emplace(entry.first, entry.second);
        for (const auto &entry : inherited->suits) {
            if (out_.suits.emplace(entry.first, entry.second).second)
                borrowed.insert(entry.first);
        }
    }

    link_supers();
    break_inheritance_cycles();

    for (auto &entry : out_.suits) {
        if (!borrowed.count(entry.first))
            resolve_suit(entry.second);
    }

    for (auto &entry : out_.suits) {
        if (!borrowed.count(entry.first))
            resolve_suit_bodies(entry.second);
    }

    // Pass 3: Top-level statements (globals)
    resolve_top_level();

    // Pass 4: Capsule bodies
    resolve_all_capsules();
}

ResolveResult resolve(const Program &program, AstArena &arena, words::Words &words,
                      const ResolveResult *inherited)
{
    ResolveResult result;
    Resolver resolver(program, arena, words, result);
    resolver.run(inherited);
    return result;
}

} // namespace satellite

