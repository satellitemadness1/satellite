// satellite/satellite_object/object_copy.cpp -- the header says what an exact copy is.

#include "object_copy.hpp"

#include "object_lock.hpp"
#include "satellite_index.hpp"
#include "satellite_list.hpp"
#include "satellite_spacesuit.hpp"

#include <unordered_map>
#include <utility>
#include <vector>

namespace satellite004 {
namespace {

// EACH ORIGINAL, HELD, AND ITS COPY. Holding the original for the whole copy is what makes its
// address a key: nothing else can be made at that address while the copy is going on.
struct Made {
    UserDefinedHandle original;
    UserDefinedHandle copy;
};

struct Copying {
    std::unordered_map<const satelliteUserDefinedObject *, Made> made;
    std::vector<Made> to_fill;                  // made, and their fields not copied yet
    std::vector<satelliteObject *> slots;       // values in the copy that may still hold originals
    std::vector<satelliteObject *> pointers;    // pointers in the copy, aimed once every object is made
};

// DOES THIS LIST OR MAP HOLD AN OBJECT OR A POINTER, at any depth -- the only things a copy
// treats differently from `=`. Only the containers inside it are kept to look at later, so a
// list of a billion numbers is one pass over them and nothing more.
bool holds_objects(const satelliteObject &value)
{
    std::vector<const satelliteObject *> containers{&value};
    while (!containers.empty()) {
        const satelliteObject *one = containers.back();
        containers.pop_back();
        const auto look_at = [&containers](const satelliteObject &item) {
            if (item.answers_as_an_object())
                return true;
            if (item.is_list() || item.is_index())
                containers.push_back(&item);
            return false;
        };
        if (const ListHandle *list = one->as_list()) {
            if (*list != nullptr)
                for (const satelliteObject &item : (*list)->items)
                    if (look_at(item)) return true;
        } else if (const IndexHandle *index = one->as_index()) {
            if (*index != nullptr)
                for (const satelliteMapEntry &entry : (*index)->entries)
                    if (look_at(entry.value)) return true;   // a key is never an object (satellite_index.hpp)
        }
    }
    return false;
}

// THE COPY OF ONE ORIGINAL, made the first time it is reached and the same copy every time after.
UserDefinedHandle copy_for(const UserDefinedHandle &original, Copying &copying)
{
    const auto found = copying.made.find(original.get());
    if (found != copying.made.end())
        return found->second.copy;
    UserDefinedHandle copy = std::make_shared<satelliteUserDefinedObject>();
    copy->layout = original->layout;
    copy->lock.on.store(original->lock.on.load(std::memory_order_acquire), std::memory_order_relaxed);
    copying.made.emplace(original.get(), Made{original, copy});
    copying.to_fill.push_back(Made{original, copy});
    return copy;
}

// ONE VALUE OF THE COPY, which still holds what the original held: an object becomes its copy,
// and a list or a map holding objects becomes a list or a map of its own whose items are looked
// at in turn. Everything else is already right.
void copy_into(satelliteObject &slot, Copying &copying)
{
    if (UserDefinedHandle *object = slot.as_user_defined()) {
        if (*object != nullptr)
            *object = copy_for(*object, copying);
        return;
    }
    if (slot.is_pointer()) {
        copying.pointers.push_back(&slot);
        return;
    }
    if (ListHandle *list = slot.as_list()) {
        if (*list == nullptr || !holds_objects(slot))
            return;
        *list = std::make_shared<satelliteList>(**list);
        for (satelliteObject &item : (*list)->items)
            copying.slots.push_back(&item);
        return;
    }
    if (IndexHandle *index = slot.as_index()) {
        if (*index == nullptr || !holds_objects(slot))
            return;
        *index = std::make_shared<satelliteIndex>(**index);
        for (satelliteMapEntry &entry : (*index)->entries)
            copying.slots.push_back(&entry.value);
    }
}

// A POINTER IN THE COPY: at the copy of its object when the copy made one, and otherwise where it
// pointed -- which, for a pointer whose object is gone, is nowhere, as it was.
void aim(satelliteObject &slot, const Copying &copying)
{
    const ObjectPointer *pointer = slot.as_pointer();
    const UserDefinedHandle object = object_of(*pointer);
    if (object == nullptr)
        return;
    const auto found = copying.made.find(object.get());
    if (found == copying.made.end())
        return;
    slot = satelliteObject::of_pointer(ObjectPointer{found->second.copy, pointer->layout});
}

} // namespace

signed long long int exact_copy_of(const UserDefinedHandle &object, UserDefinedHandle &copy)
{
    copy = nullptr;
    if (object == nullptr)
        return success;
    Copying copying;
    UserDefinedHandle root = copy_for(object, copying);
    while (!copying.to_fill.empty() || !copying.slots.empty()) {
        if (!copying.slots.empty()) {
            satelliteObject *slot = copying.slots.back();
            copying.slots.pop_back();
            copy_into(*slot, copying);
            continue;
        }
        const Made next = std::move(copying.to_fill.back());
        copying.to_fill.pop_back();
        {
            // READ AS A STATEMENT READING IT WOULD BE: while a write to it is going on, this waits.
            const ObjectHold reading(&next.original->lock, LockUse::reading);
            if (reading.code() != success)
                return reading.code();
            next.copy->fields = next.original->fields;
        }
        for (satelliteObject &field : next.copy->fields)
            copying.slots.push_back(&field);
    }
    for (satelliteObject *slot : copying.pointers)
        aim(*slot, copying);
    copy = std::move(root);
    return success;
}

} // namespace satellite004
