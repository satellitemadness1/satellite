#pragma once

#include "value.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace satellite {

// satellite.library — the global registry of every variable everywhere.
// A variable lives at satellite.library.<function_name>.<var_name>.
//
// Concurrency contract (locks are for writing only):
//   - set()/update() take that variable's write_lock, then publish the
//     new value with one atomic pointer swap
//   - get() never locks: it atomically loads the current snapshot, so a
//     reader racing a writer sees the old or the new value, never a torn one
class Library {
public:
    static Library &instance();

    // satellite.library.<function_name>.<var_name> = value
    void set(const std::string &function_name, const std::string &var_name,
             Value value);

    // Read-modify-write as one atomic unit — this is what the write lock
    // exists for: two concurrent `x = x + 1` can't lose an increment.
    // fn receives nullptr if the variable doesn't exist yet.
    void update(const std::string &function_name, const std::string &var_name,
                const std::function<Value(const Value *current)> &fn);

    // Lock-free; nullptr if the variable doesn't exist.
    ValuePtr get(const std::string &function_name,
                 const std::string &var_name) const;

    // Dotted-path access, with the "satellite.library." prefix optional:
    // "satellite.library.main.x" and "main.x" are the same variable.
    // The last dot splits <function_name> from <var_name>.
    ValuePtr get_path(const std::string &path) const;
    bool set_path(const std::string &path, Value value);

    // Sorted "<function_name>.<var_name>" keys, for the repl / debugging.
    std::vector<std::string> list() const;

private:
    struct Variable {
        std::mutex write_lock;       // taken by writers only
        std::atomic<ValuePtr> slot;  // readers just load this
    };

    // The directory of variables is itself published copy-on-write so
    // lookups don't lock either; intern_lock_ only serializes creation
    // of new variables.
    using Directory =
        std::unordered_map<std::string, std::shared_ptr<Variable>>;

    Library();

    std::shared_ptr<Variable> intern(const std::string &key);
    std::shared_ptr<Variable> find(const std::string &key) const;
    void set_key(const std::string &key, Value value);
    static bool normalize_path(const std::string &path, std::string &key);

    std::mutex intern_lock_;
    std::atomic<std::shared_ptr<const Directory>> directory_;
};

} // namespace satellite
