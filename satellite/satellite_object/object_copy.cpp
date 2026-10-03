// satellite/satellite_object/object_copy.cpp -- the header says what an exact copy is.

#include "object_copy.hpp"

#include "object_lock.hpp"
#include "satellite_index.hpp"
#include "satellite_list.hpp"
#include "satellite_spacesuit.hpp"

#include <memory>
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

// AND THE SAME FOR A LIST OR A MAP, which a program may hold in many places at once -- `x = {x, x}`
// twenty times over is one list reached a million ways -- so each is looked at once and copied once:
// the places that held one list hold one copy, which copy-on-write keeps apart from then on exactly as
// it kept the original apart (the review of 2026-10-02: a copy per path grew as 2^n, and looking
// again at every depth made a deep list quadratic).
struct Container {
    std::shared_ptr<const void> original;       // held, so its address stays its own
    bool looked_at = false;                      // holds_objects below is known
    bool holds_objects = false;
    bool copied = false;
    satelliteObject copy;                        // what goes where the original was held
};

struct Copying {
    std::unordered_map<const satelliteUserDefinedObject *, Made> made;
    std::unordered_map<const void *, Container> containers;
    std::vector<Made> to_fill;                  // made, and their fields not copied yet
    std::vector<satelliteObject *> slots;       // values in the copy that may still hold originals
    std::vector<satelliteObject *> pointers;    // pointers in the copy, aimed once every object is made
};

// THE LIST OR MAP A VALUE HOLDS, by its address -- null when the value is neither, or holds none yet.
const void *container_of(const satelliteObject &value)
{
    if (const ListHandle *list = value.as_list())
        return list->get();
    if (const IndexHandle *index = value.as_index())
        return index->get();
    return nullptr;
}

std::size_t items_in(const satelliteObject &container)
{
    if (const ListHandle *list = container.as_list())
        return (*list)->items.size();
    return (*container.as_index())->entries.size();
}

// ITEM n OF A LIST, OR ENTRY n's VALUE OF A MAP -- a key is never an object (satellite_index.hpp).
const satelliteObject &item_in(const satelliteObject &container, std::size_t n)
{
    if (const ListHandle *list = container.as_list())
        return (*list)->items[n];
    return (*container.as_index())->entries[n].value;
}

// DOES THIS LIST OR MAP HOLD AN OBJECT OR A POINTER, at any depth -- the only things a copy treats
// differently from `=`. Each list or map is looked at once however many places hold it, inner ones
// first, from a list of work; and only lists and maps are kept to look at later, so a list of a
// billion numbers is one pass over them and nothing more.
bool holds_objects(const satelliteObject &value, Copying &copying)
{
    struct Looking {
        const satelliteObject *container;
        std::size_t next;
        bool found;
    };
    std::vector<Looking> looking{{&value, 0, false}};
    bool answer = false;
    while (!looking.empty()) {
        Container &known = copying.containers[container_of(*looking.back().container)];
        if (!known.looked_at) {
            Looking &here = looking.back();
            if (known.original == nullptr) {
                if (const ListHandle *list = here.container->as_list())
                    known.original = *list;
                else
                    known.original = *here.container->as_index();
            }
            const std::size_t size = items_in(*here.container);
            bool inside = false;
            while (!here.found && here.next < size) {
                const satelliteObject &item = item_in(*here.container, here.next++);
                if (item.answers_as_an_object()) {
                    here.found = true;
                    break;
                }
                const void *inner = container_of(item);
                if (inner == nullptr)
                    continue;
                const auto seen = copying.containers.find(inner);
                if (seen != copying.containers.end() && seen->second.looked_at) {
                    here.found = seen->second.holds_objects;
                    continue;
                }
                // ONE BEING LOOKED AT ALREADY is one this list is inside of. Copy-on-write means a
                // list never holds itself, so this is never met -- and if it were, it is not a hang.
                if (seen != copying.containers.end() && seen->second.original != nullptr)
                    continue;
                looking.push_back({&item, 0, false});   // inside it first; `here` is not used again
                inside = true;
                break;
            }
            if (inside)
                continue;
            known.holds_objects = looking.back().found;
            known.looked_at = true;
        }
        answer = known.holds_objects;
        looking.pop_back();
        if (answer && !looking.empty())
            looking.back().found = true;
    }
    return answer;
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

// ONE VALUE OF THE COPY, which still holds what the original held: an object becomes its copy, and a
// list or a map holding objects becomes the one copy of it, whose items are looked at in turn the
// first time it is made. Everything else is already right.
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
    const void *key = container_of(slot);
    if (key == nullptr || !holds_objects(slot, copying))
        return;                      // a value already, shared until one side writes -- as `=` shares it
    Container &known = copying.containers[key];
    if (!known.copied) {
        if (const ListHandle *list = slot.as_list()) {
            const ListHandle copy = std::make_shared<satelliteList>(**list);
            for (satelliteObject &item : copy->items)
                copying.slots.push_back(&item);
            known.copy = satelliteObject::of_list(copy);
        } else {
            const IndexHandle copy = std::make_shared<satelliteIndex>(**slot.as_index());
            for (satelliteMapEntry &entry : copy->entries)
                copying.slots.push_back(&entry.value);
            known.copy = satelliteObject::of_index(copy);
        }
        known.copied = true;
    }
    slot = known.copy;
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
