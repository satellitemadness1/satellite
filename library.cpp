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
    std::lock_guard lock(var->write_lock);
    var->slot.store(std::move(next), std::memory_order_release);
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
    std::lock_guard lock(var->write_lock);
    ValuePtr current = var->slot.load(std::memory_order_acquire);
    auto next = std::make_shared<const Value>(fn(current.get()));
    var->slot.store(std::move(next), std::memory_order_release);
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
    return dot != std::string::npos && dot > 0 && dot + 1 < key.size();
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
