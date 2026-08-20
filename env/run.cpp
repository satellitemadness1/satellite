// Driving the passes, and the public ResolveResult surface.
//
// Part of env/, split from an 854-line env.cpp.

#include "env_internal.hpp"

namespace satellite {

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
            // Both definitions get named. While a program was one file the
            // reader could find the other one by scrolling; after §16 the
            // first definition may be in a spaceship they have never opened,
            // reached through an include two levels down.
            const Capsule *first = inserted.first->second.capsule;
            if (first)
                fail_with_note(capsule->span,
                               "capsule " + key + " is already defined",
                               first->span, "first defined here");
            else
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

ResolveResult resolve(const Program &program)
{
    ResolveResult result;
    Resolver resolver(result);
    resolver.run(program);
    return result;
}

namespace {

// The source line under a span, with a caret. Factored out of format_error
// when the note arrived, because a note needs exactly the same block drawn
// against a DIFFERENT span — often in a different spaceship, which is the
// whole reason it exists.
std::string caret_block(const Span &span, const SourceMap &sources)
{
    const std::string &source = sources.text(span.file);

    size_t at = std::min(span.start, source.size());
    size_t begin = source.rfind('\n', at == 0 ? 0 : at - 1);
    begin = (begin == std::string::npos) ? 0 : begin + 1;
    size_t end = source.find('\n', at);
    if (end == std::string::npos)
        end = source.size();

    std::string out = "    " + source.substr(begin, end - begin) + "\n";
    out += "    " + std::string(at - begin, ' ') + "^\n";
    return out;
}

} // namespace

std::string format_error(const ResolveError &error, const SourceMap &sources)
{
    std::string out = "satellite: " + error.message + " (" +
                      span_location(error.span, sources) + ")\n";
    out += caret_block(error.span, sources);

    if (!error.note.empty()) {
        out += "satellite: " + error.note + " (" +
               span_location(error.note_span, sources) + ")\n";
        out += caret_block(error.note_span, sources);
    }
    return out;
}

} // namespace satellite
