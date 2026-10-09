// satellite/bytecode/satellite_legal.cpp -- satellite.legal's master list, and the checker's view of one place
// (satellite_legal.hpp).

#include "satellite_legal.hpp"

#include <utility>

namespace satellite004 {

const char kSatelliteIsImmutable[] = "satellite is the first legal object of every place in satellite.legal, and it "
                                     "is immutable -- nothing else may be named satellite, and it is never changed "
                                     "or deleted";

namespace {

// EVERY PLACE BEGINS WITH satellite, its first legal object, immutable -- the master list's places and the
// unlisted place of a statement judged alone alike.
void a_place_begins(LegalPlace &place, const std::string &name)
{
    place.name = name;
    LegalName first;
    first.name = kTheFirstLegalObject;
    first.immutable = true;
    first.legal = &place.name;
    place.at[first.name] = 0;
    place.names.push_back(std::move(first));
}

} // namespace

std::deque<LegalPlace> &satellite_legal()
{
    static std::deque<LegalPlace> places;
    return places;
}

// THE PLACES BY NAME, beside the deque whose places never move: a program of thousands of capsules finds each
// in one lookup, where a walk along the list made the check quadratic (the fresh reader, 2026-10-06).
std::unordered_map<std::string, LegalPlace *> &places_by_name()
{
    static std::unordered_map<std::string, LegalPlace *> index;
    return index;
}

LegalPlace &legal_place(const std::string &key, const std::string &name)
{
    const std::unordered_map<std::string, LegalPlace *>::const_iterator found = places_by_name().find(key);
    if (found != places_by_name().end())
        return *found->second;
    satellite_legal().emplace_back();
    LegalPlace &place = satellite_legal().back();
    places_by_name()[key] = &place;
    a_place_begins(place, name);
    return place;
}

LegalPlace &legal_place(const std::string &name)
{
    return legal_place(name, name);
}

std::string legal_place_name(const std::string &capsule_shown)
{
    return "satellite.legal." + (capsule_shown == "satellite.main" ? std::string("main") : capsule_shown);
}

// A READ BEGINS: nothing is legal yet but satellite. The names stay, as the record of what the place has held.
LegalNames::LegalNames(LegalPlace &place) : place_(&place)
{
    for (LegalName &each : place.names)
        if (!each.immutable) each.accessible = false;
}

LegalNames::LegalNames() : own_(std::in_place), place_(&*own_)
{
    a_place_begins(*own_, std::string());
}

LegalNames::const_iterator LegalNames::find(const std::string &name) const
{
    const std::unordered_map<std::string, std::size_t>::const_iterator found = place_->at.find(name);
    if (found == place_->at.end())
        return end();
    const LegalName &held = place_->names[found->second];
    if (!held.accessible || held.immutable)
        return end();
    return const_iterator{place_, found->second};
}

std::pair<LegalNames::const_iterator, bool> LegalNames::emplace(const std::string &name, token::Code declared)
{
    const std::unordered_map<std::string, std::size_t>::const_iterator found = place_->at.find(name);
    if (found != place_->at.end()) {
        LegalName &held = place_->names[found->second];
        if (held.accessible || held.immutable)
            return {const_iterator{place_, found->second}, false};
        held.accessible = true;
        held.declared = declared;
        return {const_iterator{place_, found->second}, true};
    }
    LegalName made;
    made.name = name;
    made.declared = declared;
    made.legal = &place_->name;
    const std::size_t index = place_->names.size();
    place_->at[name] = index;
    place_->names.push_back(std::move(made));
    return {const_iterator{place_, index}, true};
}

std::size_t LegalNames::erase(const std::string &name)
{
    const std::unordered_map<std::string, std::size_t>::const_iterator found = place_->at.find(name);
    if (found == place_->at.end())
        return 0;
    LegalName &held = place_->names[found->second];
    if (!held.accessible || held.immutable)
        return 0;
    held.accessible = false;
    return 1;
}

token::Code &LegalNames::operator[](const std::string &name)
{
    const std::unordered_map<std::string, std::size_t>::const_iterator found = place_->at.find(name);
    if (found != place_->at.end()) {
        LegalName &held = place_->names[found->second];
        if (!held.immutable) held.accessible = true;
        return held.declared;
    }
    LegalName made;
    made.name = name;
    made.legal = &place_->name;
    place_->at[name] = place_->names.size();
    place_->names.push_back(std::move(made));
    return place_->names.back().declared;
}

} // namespace satellite004
