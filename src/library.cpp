#include "satellite/library.hpp"

#include <algorithm>
#include <sstream>

namespace satellite {

const char* kind_name(Kind k) {
    switch (k) {
        case Kind::Variable:  return "variable";
        case Kind::Capsule:   return "capsule";
        case Kind::Spacesuit: return "spacesuit";
        case Kind::Native:    return "native";
    }
    return "?";
}

std::string Entry::path() const {
    return Library::join_path(location, name);
}

std::string Entry::display() const {
    return Library::display_path(location, name);
}

// ---------------------------------------------------------------------------
// Path arithmetic
// ---------------------------------------------------------------------------
namespace {

bool starts_with(const std::string& s, const char* prefix) {
    size_t n = std::char_traits<char>::length(prefix);
    return s.size() >= n && s.compare(0, n, prefix) == 0;
}

}  // namespace

std::string Library::normalize_location(const std::string& location) {
    std::string s = location;

    // The source-level prefix, if the caller passed a whole path fragment.
    if (starts_with(s, "satellite.library.")) s = s.substr(18);
    else if (s == "satellite.library")        s = "";

    // `satellite.main` and `main` are the same capsule.  Only strip when a dot
    // follows, so a capsule literally named `satellite` (the escape hatch)
    // stays as it is.
    if (starts_with(s, "satellite.")) s = s.substr(10);

    return s;
}

std::string Library::join_path(const std::string& location, const std::string& name) {
    std::string loc = normalize_location(location);
    if (loc.empty()) return name;
    return loc + "." + name;
}

std::string Library::display_path(const std::string& location, const std::string& name) {
    return "satellite.library." + join_path(location, name);
}

// The LAST dot separates the name.  Everything before it is the location.
bool Library::split_path(const std::string& dotted,
                         std::string* location, std::string* name) {
    std::string s = dotted;
    if (starts_with(s, "satellite.library.")) s = s.substr(18);
    if (s.empty()) return false;

    size_t dot = s.rfind('.');
    if (dot == std::string::npos) {
        if (location) *location = "";        // global
        if (name)     *name     = s;
        return !s.empty();
    }
    if (dot + 1 >= s.size()) return false;   // trailing dot, no name

    std::string loc = normalize_location(s.substr(0, dot));
    std::string nm  = s.substr(dot + 1);
    if (nm.empty()) return false;

    if (location) *location = loc;
    if (name)     *name     = nm;
    return true;
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------
Slot Library::find_locked(const std::string& location, const std::string& name) const {
    auto it = by_key_.find(join_path(location, name));
    return it == by_key_.end() ? kNoSlot : it->second;
}

Slot Library::slot_of(const std::string& location, const std::string& name) const {
    std::shared_lock lk(mu_);
    return find_locked(location, name);
}

bool Library::has(const std::string& location, const std::string& name) const {
    return slot_of(location, name) != kNoSlot;
}

bool Library::get(const std::string& location, const std::string& name,
                  Container* out) const {
    std::shared_lock lk(mu_);
    Slot s = find_locked(location, name);
    if (s == kNoSlot) return false;
    if (out) *out = entries_[s - 1].value;
    return true;
}

// ---------------------------------------------------------------------------
// Definition and assignment
// ---------------------------------------------------------------------------
Slot Library::define(const std::string& location, const std::string& name,
                     Container value, Kind kind) {
    if (name.empty()) return kNoSlot;

    std::unique_lock lk(mu_);
    std::string key = join_path(location, name);

    auto it = by_key_.find(key);
    if (it != by_key_.end()) {                 // redefine in place, same slot
        Entry& e = entries_[it->second - 1];
        e.value  = std::move(value);
        e.kind   = kind;
        return e.slot;
    }

    entries_.emplace_back();
    Entry& e   = entries_.back();
    e.location = normalize_location(location);
    e.name     = name;
    e.value    = std::move(value);
    e.kind     = kind;
    e.slot     = (Slot)entries_.size();        // 1-based; 0 stays kNoSlot

    by_key_[key] = e.slot;
    return e.slot;
}

bool Library::set(const std::string& location, const std::string& name,
                  Container value) {
    std::unique_lock lk(mu_);
    Slot s = find_locked(location, name);
    if (s == kNoSlot) return false;            // never create by assignment
    entries_[s - 1].value = std::move(value);
    return true;
}

// ---------------------------------------------------------------------------
// The one-string forms
// ---------------------------------------------------------------------------
Slot Library::define_path(const std::string& dotted, Container value, Kind kind) {
    std::string loc, nm;
    if (!split_path(dotted, &loc, &nm)) return kNoSlot;
    return define(loc, nm, std::move(value), kind);
}

bool Library::set_path(const std::string& dotted, Container value) {
    std::string loc, nm;
    if (!split_path(dotted, &loc, &nm)) return false;
    return set(loc, nm, std::move(value));
}

bool Library::get_path(const std::string& dotted, Container* out) const {
    std::string loc, nm;
    if (!split_path(dotted, &loc, &nm)) return false;
    return get(loc, nm, out);
}

bool Library::has_path(const std::string& dotted) const {
    return slot_of_path(dotted) != kNoSlot;
}

Slot Library::slot_of_path(const std::string& dotted) const {
    std::string loc, nm;
    if (!split_path(dotted, &loc, &nm)) return kNoSlot;
    return slot_of(loc, nm);
}

// ---------------------------------------------------------------------------
// By slot -- the path the VM takes
// ---------------------------------------------------------------------------
bool Library::read(Slot s, Container* out) const {
    std::shared_lock lk(mu_);
    if (s == kNoSlot || s > entries_.size()) return false;
    if (out) *out = entries_[s - 1].value;
    return true;
}

bool Library::write(Slot s, Container value) {
    std::unique_lock lk(mu_);
    if (s == kNoSlot || s > entries_.size()) return false;
    entries_[s - 1].value = std::move(value);
    return true;
}

Kind Library::kind_of(Slot s) const {
    std::shared_lock lk(mu_);
    if (s == kNoSlot || s > entries_.size()) return Kind::Variable;
    return entries_[s - 1].kind;
}

Container* Library::address(Slot s) {
    std::shared_lock lk(mu_);
    if (s == kNoSlot || s > entries_.size()) return nullptr;
    return &entries_[s - 1].value;             // deque: never relocates
}

const Container* Library::address(Slot s) const {
    std::shared_lock lk(mu_);
    if (s == kNoSlot || s > entries_.size()) return nullptr;
    return &entries_[s - 1].value;
}

bool Library::describe(Slot s, Entry* out) const {
    std::shared_lock lk(mu_);
    if (s == kNoSlot || s > entries_.size()) return false;
    if (out) *out = entries_[s - 1];
    return true;
}

// ---------------------------------------------------------------------------
// Whole-store
// ---------------------------------------------------------------------------
// Erasing unhooks the NAME.  The entry itself stays put, so every slot already
// handed out keeps pointing at what it always pointed at.  Reusing slot
// numbers would be a use-after-free with extra steps.
bool Library::erase(const std::string& location, const std::string& name) {
    std::unique_lock lk(mu_);
    std::string key = join_path(location, name);
    auto it = by_key_.find(key);
    if (it == by_key_.end()) return false;
    entries_[it->second - 1].value = Container::nil();
    by_key_.erase(it);
    return true;
}

void Library::clear() {
    std::unique_lock lk(mu_);
    entries_.clear();
    by_key_.clear();
}

size_t Library::size() const {
    std::shared_lock lk(mu_);
    return by_key_.size();                     // live names, not graveyard slots
}

std::vector<std::string> Library::locations() const {
    std::shared_lock lk(mu_);
    std::vector<std::string> out;
    for (const auto& kv : by_key_) {
        const std::string& loc = entries_[kv.second - 1].location;
        if (std::find(out.begin(), out.end(), loc) == out.end()) out.push_back(loc);
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::string> Library::names_in(const std::string& location) const {
    std::string want = normalize_location(location);
    std::shared_lock lk(mu_);
    std::vector<std::string> out;
    for (const auto& kv : by_key_) {
        const Entry& e = entries_[kv.second - 1];
        if (e.location == want) out.push_back(e.name);
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<Slot> Library::slots() const {
    std::shared_lock lk(mu_);
    std::vector<Slot> out;
    out.reserve(by_key_.size());
    for (const auto& e : entries_)
        if (by_key_.count(join_path(e.location, e.name))) out.push_back(e.slot);
    std::sort(out.begin(), out.end());         // define order
    return out;
}

std::string Library::dump() const {
    std::vector<std::string> locs = locations();
    std::ostringstream ss;
    for (const auto& loc : locs) {
        ss << (loc.empty() ? "satellite.library" : "satellite.library." + loc) << "\n";
        for (const auto& nm : names_in(loc)) {
            Container v;
            get(loc, nm, &v);
            Slot s = slot_of(loc, nm);
            Entry e;
            describe(s, &e);
            ss << "    " << nm << " = " << v.to_string()
               << "  [" << type_name(v.type) << ", " << kind_name(e.kind)
               << ", slot " << s << "]\n";
        }
    }
    return ss.str();
}

// ---------------------------------------------------------------------------
// find_main
//
// Matches the token sequence, not the text, so a `satellite.main` inside a
// comment or a string literal can never be mistaken for the entry point --
// neither ever reaches the token stream.
//
// The word lexes differently on each side of a dot: `satellite` at the start
// of the statement is Tok::Satellite, but `capsule` and `main` follow dots and
// so arrive as Tok::Ident.  The second `satellite` follows `capsule`, not a
// dot, so it is Tok::Satellite again.
//
//     satellite . capsule satellite . main (
//     ^Satellite  ^Ident  ^Satellite  ^Ident
// ---------------------------------------------------------------------------
namespace {

bool is_ident(const Token& t, const char* name) {
    return t.kind == Tok::Ident && t.text == name;
}

}  // namespace

MainLocation find_main(const std::vector<Token>& tokens) {
    MainLocation m;
    if (tokens.size() < 6) return m;

    for (size_t i = 0; i + 5 < tokens.size(); ++i) {
        if (tokens[i].kind     != Tok::Satellite) continue;
        if (tokens[i + 1].kind != Tok::Dot)       continue;
        if (!is_ident(tokens[i + 2], "capsule"))  continue;
        if (tokens[i + 3].kind != Tok::Satellite) continue;
        if (tokens[i + 4].kind != Tok::Dot)       continue;
        if (!is_ident(tokens[i + 5], "main"))     continue;

        m.found       = true;
        m.line        = tokens[i].line;
        m.col         = tokens[i].col;
        m.token_index = i;
        m.has_parens  = (i + 6 < tokens.size() && tokens[i + 6].kind == Tok::LParen);
        return m;
    }
    return m;
}

}  // namespace satellite
