#pragma once
// satellite/satellite_object/satellite_index.hpp -- AN INDEX: KEYS TO VALUES.
//
// The author, 2026-09-18: *"satellite.container.index<key, value> where key is a
// certain type, and value is the value of that type, an index is a python
// dictionary, but I think it's just a std::map, but I could be wrong"*.
//
//     satellite.container.index<satellite.variable.string, satellite.variable.number> scores
//     scores["alice"] = 10
//     satellite.console.display(scores["alice"])
//
// ---------------------------------------------------------------------------
// IT IS A PYTHON DICT AND NOT A std::map, AND THE DIFFERENCE IS NOT PEDANTRY.
// ---------------------------------------------------------------------------
//
// The author guessed std::map and said he might be wrong. He was, and the way he
// was wrong matters to what programs print:
//
//   std::map      a SORTED tree. Walking it gives keys in order, always, and
//                 every lookup costs log n comparisons of whole keys.
//   a python dict a HASH TABLE THAT REMEMBERS THE ORDER THINGS WERE PUT IN.
//                 Walking it gives them back in that order; a lookup is one hash.
//
// So `{"zoe": 1, "al": 2}` walks as zoe, al in Python and as al, zoe in a
// std::map. Two different programs, from the same source, depending only on
// which was built. THIS IS THE DICT, because that is the behaviour that was
// asked for by name -- and because a hash is the right cost for satellite's
// own client, which reads corpora and looks up far more often than it iterates.
//
// HOW BOTH AT ONCE: a vector holds the entries IN THE ORDER THEY ARRIVED, and a
// hash table holds key -> position in that vector. Insertion appends; lookup is
// one hash; iteration is the vector. This is what Python itself does.
//
// ---------------------------------------------------------------------------
// WHICH TYPES MAY BE A KEY, and why not all of them.
// ---------------------------------------------------------------------------
//
// A number, a string, a bool, a binary and a percentage may. A list, an index, a
// file, a spacesuit and a capsule MAY NOT, and the reason is the same for all
// five: **a key must not be able to change after it is filed under.** A list put
// in as a key, then written to through `a[1] = x`, would sit in the table under
// a hash that no longer describes it -- findable by nothing, including itself.
// Refusing is the version with nothing to get wrong.
//
// THE KEY IS FILED UNDER ITS KIND AND ITS TEXT, so `1` and `"1"` are two keys.
// A dict where a number and its spelling collide is a dict that loses data
// quietly, which is the only kind of losing data that matters.
//
// COPY-ON-WRITE, exactly as satellite_list.hpp describes it, and the same
// warning applies with the same force: reach an index to WRITE it through a
// reference, never a copy, or every write silently copies the whole table. Read
// that file's note about 003 before touching this.

#include "satellite_object.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace satellite004 {

struct satelliteIndex {
    // IN THE ORDER THEY WERE PUT IN. This vector IS the order the author asked
    // for when he said "python dictionary".
    std::vector<std::pair<satelliteObject, satelliteObject>> entries;

    // WHERE EACH KEY IS IN THAT VECTOR. Rebuilt whenever entries move, which is
    // only on a copy -- an append never moves an earlier entry's position.
    std::unordered_map<std::string, std::size_t> where;

    satelliteIndex() = default;
};

// THE FILING NAME OF A KEY: its kind, then its text. The kind goes first and a
// byte that cannot appear in text separates them, so `1` (a number) and `"1"`
// (a string) can never be filed under one name.
//
// Answers false when this kind may not be a key at all, so the caller refuses in
// its own words rather than this inventing a name for something unfileable.
inline bool key_name_of(const satelliteObject &key, std::string &out)
{
    switch (key.kind()) {
    case satelliteObject::number:
    case satelliteObject::string:
    case satelliteObject::boolean:
    case satelliteObject::binary:
    case satelliteObject::percentage:
        break;
    default:
        return false;
    }
    satellite_string text;
    std::string why;
    if (key.to_string(text, why) != success)
        return false;
    out = std::to_string(static_cast<unsigned int>(key.kind()));
    out += '\x01';                      // never a byte of satellite text
    out += text.to_utf8();
    return true;
}

using IndexHandle = std::shared_ptr<satelliteIndex>;

inline IndexHandle make_index()
{
    return std::make_shared<satelliteIndex>();
}

// COPY-ON-WRITE. satellite_list.hpp's about_to_change, for this container, and
// the same rule: the handle passed in must be the real one.
inline satelliteIndex &about_to_change(IndexHandle &handle)
{
    if (handle == nullptr) {
        handle = make_index();
        return *handle;
    }
    if (handle.use_count() > 1)
        handle = std::make_shared<satelliteIndex>(*handle);
    return *handle;
}

// THE VALUE UNDER A KEY, or nullptr when there is none. Never inserts: a read
// that quietly creates an entry is how a dict fills up with keys nobody meant
// to put in it.
inline const satelliteObject *value_at(const satelliteIndex &index, const std::string &key_name)
{
    const std::unordered_map<std::string, std::size_t>::const_iterator found = index.where.find(key_name);
    if (found == index.where.end()) return nullptr;
    return &index.entries[found->second].second;
}

inline satelliteObject *value_at(satelliteIndex &index, const std::string &key_name)
{
    const std::unordered_map<std::string, std::size_t>::iterator found = index.where.find(key_name);
    if (found == index.where.end()) return nullptr;
    return &index.entries[found->second].second;
}

// PUT A KEY IN, OR FIND THE ONE ALREADY THERE, and answer where its value is.
//
// AN INDEX GROWS ON A WRITE AND A LIST DOES NOT, which looks inconsistent and is
// not: `a[5] = x` on a three-item list names a POSITION that does not exist, and
// positions are not something a program invents. A key IS something a program
// invents -- putting a new one in is the ordinary way a dict is filled, and
// there is no other way to fill it.
//
// AN EXISTING KEY KEEPS ITS PLACE IN THE ORDER. Writing scores["alice"] a second
// time changes the value and does not move alice to the end, which is what
// Python does and what anybody reading the output expects.
inline satelliteObject &value_for_writing(satelliteIndex &index, const std::string &key_name,
                                          const satelliteObject &key)
{
    const std::unordered_map<std::string, std::size_t>::iterator found = index.where.find(key_name);
    if (found != index.where.end())
        return index.entries[found->second].second;
    index.entries.push_back(std::make_pair(key, satelliteObject()));
    index.where.emplace(key_name, index.entries.size() - 1);
    return index.entries.back().second;
}

} // namespace satellite004
