#include "library.hpp"

#include <algorithm>

namespace satellite {

Library &Library::instance()
{
    static Library lib;
    return lib;
}

Library::Library() : directory_(std::make_shared<const Directory>()) {}

std::shared_ptr<Library::Variable> Library::find(const std::string &key) const
{
    auto dir = directory_.load(std::memory_order_acquire);
    auto it = dir->find(key);
    return it == dir->end() ? nullptr : it->second;
}

std::shared_ptr<Library::Variable> Library::intern(const std::string &key)
{
    std::lock_guard lock(intern_lock_);
    auto dir = directory_.load(std::memory_order_acquire);
    if (auto it = dir->find(key); it != dir->end())
        return it->second;

    auto var = std::make_shared<Variable>();
    auto next = std::make_shared<Directory>(*dir);
    (*next)[key] = var;
    directory_.store(std::move(next), std::memory_order_release);
    return var;
}

void Library::set_key(const std::string &key, Value value)
{
    auto var = intern(key);
    auto next = std::make_shared<const Value>(std::move(value));

    // `doomed` is declared BEFORE the lock_guard so that it is destroyed AFTER
    // it — locals die in reverse declaration order. That ordering is the whole
    // fix. A plain store() drops the slot's last reference to the previous
    // Value inside the store call, so ~Value ran with write_lock held; for a
    // FilePtr that is close(fd) under the lock, which blocks for seconds on NFS
    // or FUSE, and for a window it would be GTK teardown from a non-GTK thread,
    // which is undefined behavior rather than a stall.
    //
    // Trap: switching to exchange() alone does not fix this. The returned
    // shared_ptr would be a temporary dying at the end of the full expression,
    // still inside the lock. It has to be bound to a name declared out here.
    ValuePtr doomed;
    {
        std::lock_guard lock(var->write_lock);
        doomed = var->slot.exchange(std::move(next), std::memory_order_acq_rel);
    }
}

void Library::set(const std::string &function_name,
                  const std::string &var_name, Value value)
{
    set_key(function_name + "." + var_name, std::move(value));
}

void Library::update(const std::string &function_name,
                     const std::string &var_name,
                     const std::function<Value(const Value *)> &fn)
{
    auto var = intern(function_name + "." + var_name);

    // Same fix as set_key, different mechanism: `current` used to be declared
    // after the lock_guard, so reverse-order destruction killed the previous
    // Value at the closing brace with the lock still held. Hoisting the
    // survivor out and scoping the guard means the release happens first.
    //
    // fn() and the make_shared stay inside the lock deliberately. This is a
    // read-modify-write and the whole point of the lock is that nothing else
    // publishes between the load and the store; hoisting either one out would
    // turn a correct update into a lost one. What keeps that affordable is that
    // make_shared<const Value>(Value &&) MOVES from fn's return temporary, so
    // the temporary dies holding nothing. Changing that to a copying overload
    // would silently put a real destructor back under the lock.
    ValuePtr doomed;
    {
        std::lock_guard lock(var->write_lock);
        ValuePtr current = var->slot.load(std::memory_order_acquire);
        auto next = std::make_shared<const Value>(fn(current.get()));
        var->slot.store(std::move(next), std::memory_order_release);
        doomed = std::move(current);
    }
}

ValuePtr Library::get(const std::string &function_name,
                      const std::string &var_name) const
{
    auto var = find(function_name + "." + var_name);
    return var ? var->slot.load(std::memory_order_acquire) : nullptr;
}

bool Library::normalize_path(const std::string &path, std::string &key)
{
    static const std::string prefix = "satellite.library.";
    key = path;
    if (key.rfind(prefix, 0) == 0)
        key.erase(0, prefix.size());
    auto dot = key.rfind('.');
    // Exactly one dot, with something on each side. Testing only rfind meant
    // `:set a.b.c 5` split at the LAST dot and interned a variable named
    // "a.b" holding key "a.b.c" — a fabricated global that no satellite program
    // could ever name, sitting in satellite.library alongside the real ones and
    // printed by :vars as though it were one.
    //
    // Every key in the registry is function_name + "." + var_name, so one dot
    // is the whole shape. Verified against what is actually interned: `main.x`
    // and `system.min_free_mb`. Both surviving callers pass keys of that shape
    // — :vars round-trips them straight back out of list(), and library_test
    // hands in the fully-prefixed "satellite.library.main.x".
    return dot != std::string::npos && dot > 0 && dot + 1 < key.size() &&
           key.find('.') == dot;
}

ValuePtr Library::get_path(const std::string &path) const
{
    std::string key;
    if (!normalize_path(path, key))
        return nullptr;
    auto var = find(key);
    return var ? var->slot.load(std::memory_order_acquire) : nullptr;
}

bool Library::set_path(const std::string &path, Value value)
{
    std::string key;
    if (!normalize_path(path, key))
        return false;
    set_key(key, std::move(value));
    return true;
}

std::vector<std::string> Library::list() const
{
    auto dir = directory_.load(std::memory_order_acquire);
    std::vector<std::string> keys;
    keys.reserve(dir->size());
    for (const auto &entry : *dir)
        keys.push_back(entry.first);
    std::sort(keys.begin(), keys.end());
    return keys;
}

} // namespace satellite
