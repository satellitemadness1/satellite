#pragma once
// satellite/bytecode/satellite_legal.hpp -- satellite.legal: ONE MASTER LIST of every place an operation is
// legal in, and global_object: what every named object carries of it. NEW_MILESTONES.md NM-2a.
//
// The author, 2026-10-06: "each object could carry around what it belongs to, and we can then run a check to
// see if the operation is legal or not -- "SUPER" Type Checking!.. so for this we need to develop another
// list, something that holds all of the legal operation places, we will call this list satellite.legal! and
// then we ourselves will use satellite.legal for... checking whether operations are legal or not, so just to
// start out, there will be satellite.legal.main (for satellite.main) and it will contain the "satellite"
// object, thus, using the name "satellite" for anything becomes illegal, "satellite" becomes the first legal
// object"; then "I suggest a single master satellite.legal list", "Satellite should have a special variable
// suggesting that it is immutable", and his "Yes" to global_object being the BOX a named object lives in
// (value.hpp's Variable), with its place's name kept once, here, and pointed at -- not a copy in every value.
//
// A PLACE is a capsule -- satellite.legal.main, satellite.legal.greet, satellite.legal.box.call_take -- or the
// prompt, satellite.legal.prompt, and it holds the names legal in it, `satellite` first: the one object legal
// in every place, and immutable.
//
// THE CHECKER READS THE PLACE ITSELF (LegalNames, below -- since 2026-10-07, on his "Can you build that part
// of it now then?"). As it reads a capsule, every name a line declares is written into the capsule's place,
// every name a } ends is ended there, and every name a line uses is looked up THERE and nowhere else -- the
// author's "we ourselves will use satellite.legal for checking whether operations are legal or not". Until
// then the checker judged each line from a list of its own that this one only mirrored, and nothing read the
// master list. WHILE arguments.access IS ON, the default, a name declared inside a block -- { ... } -- stops
// being legal at that block's }: the author, "this prevents a single kind of programming, and allows for super
// faster other types of programming, it's a trade off that i'm willing to do". With arguments.access off, the
// old way: a name is legal to the end of its capsule, "bad programming habits that some people have" allowed.
//
// AT RUN TIME the check is the bool every box carries (global_object::accessible, flipped at its block's } by
// program_walk.cpp's run_block, read by VariableTable::seen) -- his "a simple bool ... so it doesn't slow
// anything down" -- and never a search of this list: the checker has refused, before anything ran, the lines
// that name an ended name, and seen() answers for whatever it could not judge. The one block the walker does
// not end its names at is a bare { } with no if, while or for before it (run_statements returns at its },
// and a bare block inside main ends main there -- pre-existing, found 2026-10-06, the author's to rule).

#include "token_codes.hpp"

#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace satellite004 {

// WHAT EVERY NAMED OBJECT CARRIES: the Variable box a name lives in (value.hpp), and each name of a place.
struct global_object {
    const std::string *legal = nullptr;   // its place in satellite.legal, "satellite.legal.main" -- shared, not copied
    bool accessible = true;                // false once the block that declared it has ended; the record stays
    bool immutable = false;                // satellite's own: nothing may be named it, change it or delete it
};

// ONE NAME OF A PLACE, once however often it is declared: accessible while it is legal where the checker is
// reading, and the word that declared it, which the checker judges its later lines by.
struct LegalName : global_object {
    std::string name;
    token::Code declared = 0;
};

struct LegalPlace {
    std::string name;                                   // "satellite.legal.main"
    std::vector<LegalName> names;                       // satellite first
    std::unordered_map<std::string, std::size_t> at;    // where each name is in `names`
    // WHETHER ITS NAMES END AT THEIR BLOCK, as the checker last read the capsule: the prompt makes a declared
    // capsule's site again for every statement, and the new site takes this, not the switch as it is now.
    bool names_end_with_their_block = true;
};

// THE MASTER LIST, one a satl. A deque: a place never moves, so a box may point at its name.
std::deque<LegalPlace> &satellite_legal();

// THE PLACE CALLED `name`, made -- with satellite as its first legal object -- if it is not there yet. A
// CAPSULE'S PLACE IS FOUND BY THE CAPSULE (`key`, its shown name: "satellite.main", "main", "box.call_take"),
// and only CALLED by its place name: a capsule a program names `main` or `prompt` would otherwise read and
// write satellite.main's place, or the prompt's, now that the checker works in the place itself (the fresh
// reader, 2026-10-07). Two places may share a spelling; they never share a record. The one-argument form is
// the prompt's, whose key is its name.
LegalPlace &legal_place(const std::string &key, const std::string &name);
LegalPlace &legal_place(const std::string &name);

// "satellite.legal." and the capsule as a report names it: main for satellite.main, greet, box.call_take.
std::string legal_place_name(const std::string &capsule_shown);

// THE CHECKER'S VIEW OF ONE PLACE: the names legal in it as the checker reads it, read and written in the
// master list itself (program_check.cpp's DeclaredNames). Making one BEGINS A READ of the place: every name it
// holds is ended, but satellite, and the read declares them again line by line, so the place says exactly what
// the checker has read so far. The shape is the unordered_map's the checker was written against -- find, end,
// count, emplace, erase, [] -- and a name found is one legal here and now: an ended name is not found, and
// nor is satellite, which is no variable.
class LegalNames {
public:
    explicit LegalNames(LegalPlace &place);
    // A STATEMENT JUDGED ALONE -- a spacesuit's field, whose value may name nothing -- reads a place of its own
    // that the master list does not hold.
    LegalNames();
    LegalNames(const LegalNames &) = delete;
    LegalNames &operator=(const LegalNames &) = delete;

    // ONE NAME FOUND: `first` its spelling, `second` the word that declared it, as the map's pair had them.
    struct Entry {
        const std::string &first;
        const token::Code &second;
    };
    struct const_iterator {
        const LegalPlace *place = nullptr;   // null: end()
        std::size_t index = 0;               // into place->names, which may grow -- never a pointer into it
        struct Arrow {
            Entry entry;
            const Entry *operator->() const { return &entry; }
        };
        Arrow operator->() const { return Arrow{Entry{place->names[index].name, place->names[index].declared}}; }
        Entry operator*() const { return Entry{place->names[index].name, place->names[index].declared}; }
        bool operator==(const const_iterator &other) const { return place == other.place && index == other.index; }
        bool operator!=(const const_iterator &other) const { return !(*this == other); }
    };

    const_iterator find(const std::string &name) const;
    const_iterator end() const { return const_iterator{}; }
    std::size_t count(const std::string &name) const { return find(name) == end() ? 0 : 1; }
    // A NAME DECLARED: true, or false when it is legal here already -- the second declaration a program is
    // refused for. An ended name declared again is legal again, with its new word. `satellite` answers false
    // here and is never found, whatever is written over it: no path reaches it -- it lexes as a word, and every
    // declaring line that spells it is refused first (program_check.cpp's kSatelliteIsImmutable refusals).
    std::pair<const_iterator, bool> emplace(const std::string &name, token::Code declared);
    // A NAME ENDED -- at its block's }, at its for's end, by satellite.delete: 1 when it was legal here. Its
    // record stays in the place.
    std::size_t erase(const std::string &name);
    // A NAME DECLARED HERE WHATEVER IT WAS -- a parameter, a field, a name the prompt kept, an object made
    // again -- and its word, to write.
    token::Code &operator[](const std::string &name);

    LegalPlace &place() const { return *place_; }

private:
    std::optional<LegalPlace> own_;   // the unlisted place of a statement judged alone; empty over a master place
    LegalPlace *place_;
};

// THE ONE OBJECT LEGAL IN EVERY PLACE, which nothing else may be called.
inline constexpr const char *kTheFirstLegalObject = "satellite";
extern const char kSatelliteIsImmutable[];

} // namespace satellite004
