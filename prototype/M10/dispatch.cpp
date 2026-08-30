// The satellite Handler Dispatch Table & M10 Handlers implementation.
// Milestone 10 Prototype in prototype/M10.

#include "dispatch.hpp"
#include "evaluator.hpp"
#include "console.hpp"
#include "search.hpp"
#include "satellite_string/satellite_string.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace satellite {

DispatchTable &DispatchTable::instance()
{
    static DispatchTable table;
    return table;
}

void DispatchTable::register_handler(words::PathId id, HandlerFn fn,
                                     bool binds_receiver, uint32_t arity,
                                     std::string path_string)
{
    DispatchEntry entry;
    entry.path_id = id;
    entry.fn = std::move(fn);
    entry.binds_receiver = binds_receiver;
    entry.arity = arity;
    entry.path_string = std::move(path_string);
    entries_[id] = std::move(entry);
}

const DispatchEntry *DispatchTable::get(words::PathId id) const
{
    auto it = entries_.find(id);
    if (it != entries_.end())
        return &it->second;
    return nullptr;
}

void DispatchTable::clear()
{
    entries_.clear();
}

namespace {

const SatString *get_sat_string(const Value &val)
{
    if (val.is_string()) {
        auto str_ptr = std::get<Str>(val);
        if (str_ptr) return str_ptr.get();
    }
    return nullptr;
}

SatString trim_sat_string(const SatString &s)
{
    if (s.empty()) return s;
    auto is_ws = [](SatChar c) {
        return c == (SAT_RAW_BASE + ' ') || c == (SAT_RAW_BASE + '\t') ||
               c == (SAT_RAW_BASE + '\n') || c == (SAT_RAW_BASE + '\r') ||
               c == ' ' || c == '\t' || c == '\n' || c == '\r';
    };
    size_t start = 0;
    while (start < s.size() && is_ws(s[start])) ++start;
    if (start == s.size()) return SatString{};
    size_t end = s.size() - 1;
    while (end > start && is_ws(s[end])) --end;
    return s.substr(start, end - start + 1);
}

SatString to_lower_sat_string(const SatString &s)
{
    SatString out;
    out.reserve(s.size());
    for (SatChar c : s) {
        if (c >= SAT_UPPER_A && c < SAT_DIGIT_0) {
            out.push_back(SAT_A + (c - SAT_UPPER_A));
        } else if (c >= SAT_RAW_BASE && c < SAT_RAW_BASE + 256) {
            char ch = static_cast<char>(c - SAT_RAW_BASE);
            if (std::isupper(static_cast<unsigned char>(ch))) {
                out.push_back(SAT_RAW_BASE + static_cast<SatChar>(std::tolower(static_cast<unsigned char>(ch))));
            } else {
                out.push_back(c);
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

SatString to_upper_sat_string(const SatString &s)
{
    SatString out;
    out.reserve(s.size());
    for (SatChar c : s) {
        if (c >= SAT_A && c < SAT_UPPER_A) {
            out.push_back(SAT_UPPER_A + (c - SAT_A));
        } else if (c >= SAT_RAW_BASE && c < SAT_RAW_BASE + 256) {
            char ch = static_cast<char>(c - SAT_RAW_BASE);
            if (std::islower(static_cast<unsigned char>(ch))) {
                out.push_back(SAT_RAW_BASE + static_cast<SatChar>(std::toupper(static_cast<unsigned char>(ch))));
            } else {
                out.push_back(c);
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

// Comparison helper for sorting Values
bool compare_values_less(const Value &a, const Value &b)
{
    if (a.is_number() && b.is_number()) {
        return std::get<Number>(a) < std::get<Number>(b);
    }
    if (a.is_string() && b.is_string()) {
        auto as = std::get<Str>(a);
        auto bs = std::get<Str>(b);
        if (as && bs) return *as < *bs;
        return as != nullptr;
    }
    if (a.is_bool() && b.is_bool()) {
        return !std::get<bool>(a) && std::get<bool>(b);
    }
    return a.to_string() < b.to_string();
}

Value extract_key_field(const Value &elem, const std::string &key)
{
    if (std::holds_alternative<MapRef>(elem)) {
        auto map_ref = std::get<MapRef>(elem);
        if (map_ref) {
            std::string canonical;
            Value key_val = Value::sat_string(encode(key));
            map_key_of(key_val, canonical);
            auto it = map_ref->index.find(canonical);
            if (it != map_ref->index.end()) {
                const auto &val_ptr = map_ref->entries[it->second].value;
                if (val_ptr) return *val_ptr;
            }
        }
    }
    return elem;
}

} // namespace

void init_m10_dispatch()
{
    static bool s_initialized = false;
    if (s_initialized) return;
    s_initialized = true;

    auto &table = DispatchTable::instance();
    auto register_path = [&table](const char *path, HandlerFn fn, bool binds_receiver = false, uint32_t arity = 0) {
        words::Walk w = words::walk(path);
        if (w.error == words::WalkError::NONE && w.id != words::kNoPath) {
            table.register_handler(w.id, fn, binds_receiver, arity, path);
        }
    };

    // -----------------------------------------------------------------------
    // Core & Console (from M8)
    // -----------------------------------------------------------------------
    register_path("satellite.console.display", [](ExecContext &ctx, const std::vector<Value> &args) {
        if (args.empty()) {
            Console::instance().display("");
            ctx.record_output("");
            return Value::satellite_singleton();
        }
        std::string text;
        bool newline = true;
        size_t count = args.size();
        if (args.size() == 2 && args[1].is_bool()) {
            count = 1;
            newline = std::get<bool>(args[1]);
        }
        for (size_t i = 0; i < count; ++i) {
            if (i > 0) text += " ";
            if (args[i].is_string()) {
                text += decode(*std::get<Str>(args[i]));
            } else {
                text += args[i].to_string();
            }
        }
        Console::instance().display(text, newline);
        ctx.record_output(text);
        return Value::satellite_singleton();
    }, false, 1);

    register_path("satellite.return", [](ExecContext &, const std::vector<Value> &) {
        return Value::nil();
    }, false, 0);

    register_path("satellite.return(satellite)", [](ExecContext &, const std::vector<Value> &) {
        return Value::satellite_singleton();
    }, false, 1);

    register_path("satellite.return(value)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::nil();
        return args[0];
    }, false, 1);

    // -----------------------------------------------------------------------
    // 1 17: satellite.bool and module constants
    // -----------------------------------------------------------------------
    register_path("satellite.bool", [](ExecContext &, const std::vector<Value> &) {
        return Value::nil();
    }, false, 0);

    register_path("satellite.bool.false", [](ExecContext &, const std::vector<Value> &) {
        return Value::boolean(false);
    }, false, 0);

    register_path("satellite.bool.true", [](ExecContext &, const std::vector<Value> &) {
        return Value::boolean(true);
    }, false, 0);

    register_path("satellite.variable.bool", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::boolean(false);
        return Value::boolean(args[0].is_truthy());
    }, false, 1);

    // -----------------------------------------------------------------------
    // 1 13: Statements
    // -----------------------------------------------------------------------
    register_path("satellite.statement", [](ExecContext &, const std::vector<Value> &) {
        return Value::nil();
    }, false, 0);

    register_path("satellite.statement.if", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::boolean(false);
        return Value::boolean(args[0].is_truthy());
    }, false, 1);

    register_path("satellite.statement.for", [](ExecContext &, const std::vector<Value> &) {
        return Value::nil();
    }, false, 0);

    register_path("satellite.statement.while", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::boolean(false);
        return Value::boolean(args[0].is_truthy());
    }, false, 1);

    register_path("satellite.statement.else", [](ExecContext &, const std::vector<Value> &) {
        return Value::nil();
    }, false, 0);

    // -----------------------------------------------------------------------
    // 1 4: Containers (satellite.container)
    // -----------------------------------------------------------------------
    register_path("satellite.container", [](ExecContext &, const std::vector<Value> &) {
        return Value::nil();
    }, false, 0);

    // -----------------------------------------------------------------------
    // 1 4 1: satellite.container.map & its 9 methods
    // -----------------------------------------------------------------------
    register_path("satellite.container.map", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const MapBody>());
    }, false, 0);

    // 1 4 1 1: set(k, v)
    register_path("satellite.container.map.set(k, v)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 3) {
            auto map_ref = std::holds_alternative<MapRef>(args[0]) ? std::get<MapRef>(args[0]) : std::make_shared<const MapBody>();
            auto next_map = std::make_shared<MapBody>(map_ref ? *map_ref : MapBody{});
            std::string canonical;
            if (map_key_of(args[1], canonical)) {
                auto it = next_map->index.find(canonical);
                if (it != next_map->index.end()) {
                    next_map->entries[it->second].value = std::make_shared<Value>(args[2]);
                } else {
                    next_map->index[canonical] = next_map->entries.size();
                    next_map->entries.push_back(MapEntry{
                        std::make_shared<Value>(args[1]),
                        std::make_shared<Value>(args[2])
                    });
                }
                return Value(std::const_pointer_cast<const MapBody>(next_map));
            }
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 2);

    // 1 4 1 2: get(k)
    register_path("satellite.container.map.get(k)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && std::holds_alternative<MapRef>(args[0])) {
            auto map_ref = std::get<MapRef>(args[0]);
            if (map_ref) {
                std::string canonical;
                if (map_key_of(args[1], canonical)) {
                    auto it = map_ref->index.find(canonical);
                    if (it != map_ref->index.end()) {
                        const auto &val_ptr = map_ref->entries[it->second].value;
                        return val_ptr ? *val_ptr : Value::nil();
                    }
                }
            }
        }
        return Value::nil();
    }, true, 1);

    // 1 4 1 3: has(k)
    register_path("satellite.container.map.has(k)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && std::holds_alternative<MapRef>(args[0])) {
            auto map_ref = std::get<MapRef>(args[0]);
            if (map_ref) {
                std::string canonical;
                if (map_key_of(args[1], canonical)) {
                    return Value::boolean(map_ref->index.find(canonical) != map_ref->index.end());
                }
            }
        }
        return Value::boolean(false);
    }, true, 1);

    // 1 4 1 4: size
    register_path("satellite.container.map.size", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && std::holds_alternative<MapRef>(args[0])) {
            auto map_ref = std::get<MapRef>(args[0]);
            size_t sz = map_ref ? map_ref->entries.size() : 0;
            return Value::number(Number(static_cast<uint64_t>(sz)));
        }
        return Value::number(Number(0));
    }, true, 0);

    // 1 4 1 5: empty
    register_path("satellite.container.map.empty", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && std::holds_alternative<MapRef>(args[0])) {
            auto map_ref = std::get<MapRef>(args[0]);
            return Value::boolean(!map_ref || map_ref->entries.empty());
        }
        return Value::boolean(true);
    }, true, 0);

    // 1 4 1 6: clear
    register_path("satellite.container.map.clear", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const MapBody>());
    }, true, 0);

    // 1 4 1 7: remove(k)
    register_path("satellite.container.map.remove(k)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && std::holds_alternative<MapRef>(args[0])) {
            auto map_ref = std::get<MapRef>(args[0]);
            if (map_ref) {
                std::string canonical;
                if (map_key_of(args[1], canonical)) {
                    auto it = map_ref->index.find(canonical);
                    if (it != map_ref->index.end()) {
                        auto next_map = std::make_shared<MapBody>();
                        size_t gone = it->second;
                        next_map->entries.reserve(map_ref->entries.size() - 1);
                        for (size_t i = 0; i < map_ref->entries.size(); ++i) {
                            if (i != gone) {
                                next_map->entries.push_back(map_ref->entries[i]);
                            }
                        }
                        for (size_t i = 0; i < next_map->entries.size(); ++i) {
                            std::string k;
                            if (next_map->entries[i].key && map_key_of(*next_map->entries[i].key, k)) {
                                next_map->index[k] = i;
                            }
                        }
                        return Value(std::const_pointer_cast<const MapBody>(next_map));
                    }
                }
            }
            return args[0];
        }
        return Value(std::make_shared<const MapBody>());
    }, true, 1);

    // 1 4 1 8: keys
    register_path("satellite.container.map.keys", [](ExecContext &, const std::vector<Value> &args) {
        auto list = std::make_shared<List>();
        if (!args.empty() && std::holds_alternative<MapRef>(args[0])) {
            auto map_ref = std::get<MapRef>(args[0]);
            if (map_ref) {
                list->reserve(map_ref->entries.size());
                for (const auto &entry : map_ref->entries) {
                    list->push_back(entry.key ? entry.key : std::make_shared<Value>(Value::nil()));
                }
            }
        }
        return Value(std::const_pointer_cast<const List>(list));
    }, true, 0);

    // 1 4 1 9: values
    register_path("satellite.container.map.values", [](ExecContext &, const std::vector<Value> &args) {
        auto list = std::make_shared<List>();
        if (!args.empty() && std::holds_alternative<MapRef>(args[0])) {
            auto map_ref = std::get<MapRef>(args[0]);
            if (map_ref) {
                list->reserve(map_ref->entries.size());
                for (const auto &entry : map_ref->entries) {
                    list->push_back(entry.value ? entry.value : std::make_shared<Value>(Value::nil()));
                }
            }
        }
        return Value(std::const_pointer_cast<const List>(list));
    }, true, 0);

    // -----------------------------------------------------------------------
    // 1 4 2: satellite.container.list & its 25 methods
    // -----------------------------------------------------------------------
    register_path("satellite.container.list", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const List>());
    }, false, 0);

    // 1 4 2 1: append
    register_path("satellite.container.list.append", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            mutable_list->push_back(std::make_shared<Value>(args[1]));
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 1);

    // 1 4 2 2: size
    register_path("satellite.container.list.size", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            size_t sz = list_ref ? list_ref->size() : 0;
            return Value::number(Number(static_cast<uint64_t>(sz)));
        }
        return Value::number(Number(0));
    }, true, 0);

    // 1 4 2 3: sort()
    register_path("satellite.container.list.sort()", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            std::stable_sort(mutable_list->begin(), mutable_list->end(),
                             [](const ValuePtr &a, const ValuePtr &b) {
                                 if (!a || !b) return a != nullptr;
                                 return compare_values_less(*a, *b);
                             });
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 0);

    // 1 4 2 4: sort(direction)
    register_path("satellite.container.list.sort(direction)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            bool descending = false;
            if (args[1].is_string()) {
                auto str_ptr = std::get<Str>(args[1]);
                if (str_ptr && decode(*str_ptr) == "down") descending = true;
            }
            if (descending) {
                std::stable_sort(mutable_list->begin(), mutable_list->end(),
                                 [](const ValuePtr &a, const ValuePtr &b) {
                                     if (!a || !b) return b != nullptr;
                                     return compare_values_less(*b, *a);
                                 });
            } else {
                std::stable_sort(mutable_list->begin(), mutable_list->end(),
                                 [](const ValuePtr &a, const ValuePtr &b) {
                                     if (!a || !b) return a != nullptr;
                                     return compare_values_less(*a, *b);
                                 });
            }
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 1);

    // 1 4 2 5: sort_down()
    register_path("satellite.container.list.sort_down()", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            std::stable_sort(mutable_list->begin(), mutable_list->end(),
                             [](const ValuePtr &a, const ValuePtr &b) {
                                 if (!a || !b) return b != nullptr;
                                 return compare_values_less(*b, *a);
                             });
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 0);

    // 1 4 2 6: sort_down(key)
    register_path("satellite.container.list.sort_down(key)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            std::string key_str = args[1].is_string() ? decode(*std::get<Str>(args[1])) : args[1].to_string();
            std::stable_sort(mutable_list->begin(), mutable_list->end(),
                             [&key_str](const ValuePtr &a, const ValuePtr &b) {
                                 if (!a || !b) return b != nullptr;
                                 Value va = extract_key_field(*a, key_str);
                                 Value vb = extract_key_field(*b, key_str);
                                 return compare_values_less(vb, va);
                             });
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 1);

    // 1 4 2 7: sort_up(key)
    register_path("satellite.container.list.sort_up(key)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            std::string key_str = args[1].is_string() ? decode(*std::get<Str>(args[1])) : args[1].to_string();
            std::stable_sort(mutable_list->begin(), mutable_list->end(),
                             [&key_str](const ValuePtr &a, const ValuePtr &b) {
                                 if (!a || !b) return a != nullptr;
                                 Value va = extract_key_field(*a, key_str);
                                 Value vb = extract_key_field(*b, key_str);
                                 return compare_values_less(va, vb);
                             });
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 1);

    // 1 4 2 8: contains(x)
    register_path("satellite.container.list.contains(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            if (list_ref) {
                for (const auto &item : *list_ref) {
                    if (item && *item == args[1]) return Value::boolean(true);
                }
            }
        }
        return Value::boolean(false);
    }, true, 1);

    // 1 4 2 9: index_of(x)
    register_path("satellite.container.list.index_of(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            if (list_ref) {
                for (size_t i = 0; i < list_ref->size(); ++i) {
                    const auto &item = (*list_ref)[i];
                    if (item && *item == args[1]) {
                        return Value::number(Number(static_cast<int64_t>(i)));
                    }
                }
            }
        }
        return Value::number(Number(static_cast<int64_t>(-1)));
    }, true, 1);

    // 1 4 2 10: empty
    register_path("satellite.container.list.empty", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            return Value::boolean(!list_ref || list_ref->empty());
        }
        return Value::boolean(true);
    }, true, 0);

    // 1 4 2 11: clear
    register_path("satellite.container.list.clear", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const List>());
    }, true, 0);

    // 1 4 2 12: first
    register_path("satellite.container.list.first", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            if (list_ref && !list_ref->empty() && (*list_ref)[0]) {
                return *(*list_ref)[0];
            }
        }
        return Value::nil();
    }, true, 0);

    // 1 4 2 13: last
    register_path("satellite.container.list.last", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            if (list_ref && !list_ref->empty() && list_ref->back()) {
                return *list_ref->back();
            }
        }
        return Value::nil();
    }, true, 0);

    // 1 4 2 14: truncate(n)
    register_path("satellite.container.list.truncate(n)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list() && args[1].is_number()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            long long n = 0;
            std::get<Number>(args[1]).to_integer(n);
            if (n < 0) n = 0;
            if (static_cast<size_t>(n) < mutable_list->size()) {
                mutable_list->resize(static_cast<size_t>(n));
            }
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 1);

    // 1 4 2 15: reserve(n)
    register_path("satellite.container.list.reserve(n)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list() && args[1].is_number()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            long long n = 0;
            std::get<Number>(args[1]).to_integer(n);
            if (n > 0) mutable_list->reserve(static_cast<size_t>(n));
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 1);

    // 1 4 2 16: remove_first()
    register_path("satellite.container.list.remove_first()", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            if (!mutable_list->empty()) {
                mutable_list->erase(mutable_list->begin());
            }
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 0);

    // 1 4 2 17: remove_last()
    register_path("satellite.container.list.remove_last()", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            if (!mutable_list->empty()) {
                mutable_list->pop_back();
            }
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 0);

    // 1 4 2 18: remove_at(n)
    register_path("satellite.container.list.remove_at(n)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list() && args[1].is_number()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            long long idx = 0;
            std::get<Number>(args[1]).to_integer(idx);
            if (idx >= 0 && static_cast<size_t>(idx) < mutable_list->size()) {
                mutable_list->erase(mutable_list->begin() + static_cast<long>(idx));
            }
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 1);

    // 1 4 2 19: remove(x)
    register_path("satellite.container.list.remove(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            for (auto it = mutable_list->begin(); it != mutable_list->end(); ++it) {
                if (*it && **it == args[1]) {
                    mutable_list->erase(it);
                    break;
                }
            }
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 1);

    // 1 4 2 20: insert(n, x)
    register_path("satellite.container.list.insert(n, x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 3 && args[0].is_list() && args[1].is_number()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            long long idx = 0;
            std::get<Number>(args[1]).to_integer(idx);
            if (idx < 0) idx = 0;
            if (static_cast<size_t>(idx) > mutable_list->size()) idx = static_cast<long long>(mutable_list->size());
            mutable_list->insert(mutable_list->begin() + static_cast<long>(idx),
                                 std::make_shared<Value>(args[2]));
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 2);

    // 1 4 2 21: join(separator)
    register_path("satellite.container.list.join(separator)", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            std::string sep = ", ";
            if (args.size() >= 2) {
                sep = args[1].is_string() ? decode(*std::get<Str>(args[1])) : args[1].to_string();
            }
            std::string result;
            if (list_ref) {
                for (size_t i = 0; i < list_ref->size(); ++i) {
                    if (i > 0) result += sep;
                    const auto &item = (*list_ref)[i];
                    if (item) {
                        result += item->is_string() ? decode(*std::get<Str>(*item)) : item->to_string();
                    }
                }
            }
            return Value::sat_string(encode(result));
        }
        return Value::sat_string(SatString{});
    }, true, 1);

    // 1 4 2 22: reverse
    register_path("satellite.container.list.reverse", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            std::reverse(mutable_list->begin(), mutable_list->end());
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return args.empty() ? Value::nil() : args[0];
    }, true, 0);

    // 1 4 2 23: sum
    register_path("satellite.container.list.sum", [](ExecContext &, const std::vector<Value> &args) {
        Number sum(0);
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            if (list_ref) {
                for (const auto &item : *list_ref) {
                    if (item && item->is_number()) {
                        sum = sum + std::get<Number>(*item);
                    }
                }
            }
        }
        return Value::number(sum);
    }, true, 0);

    // 1 4 2 24: max
    register_path("satellite.container.list.max", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            if (list_ref && !list_ref->empty()) {
                ValuePtr max_val = nullptr;
                for (const auto &item : *list_ref) {
                    if (!item) continue;
                    if (!max_val || compare_values_less(*max_val, *item)) {
                        max_val = item;
                    }
                }
                if (max_val) return *max_val;
            }
        }
        return Value::nil();
    }, true, 0);

    // 1 4 2 25: min
    register_path("satellite.container.list.min", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            if (list_ref && !list_ref->empty()) {
                ValuePtr min_val = nullptr;
                for (const auto &item : *list_ref) {
                    if (!item) continue;
                    if (!min_val || compare_values_less(*item, *min_val)) {
                        min_val = item;
                    }
                }
                if (min_val) return *min_val;
            }
        }
        return Value::nil();
    }, true, 0);

    // -----------------------------------------------------------------------
    // Search Power on Containers: .search(pattern)
    // -----------------------------------------------------------------------
    auto search_handler = [](ExecContext &ctx, const std::vector<Value> &args) {
        if (args.size() >= 2) {
            auto target_ptr = std::make_shared<Value>(args[0]);
            auto pattern_ptr = std::make_shared<Value>(args[1]);
            std::string error;
            ValuePtr res = search_collect(target_ptr, pattern_ptr, true, ctx.max_depth(), error);
            if (res) return *res;
        }
        return Value(std::make_shared<const List>());
    };

    // Register .search on list and map
    table.register_handler(words::kNoPath, search_handler, true, 1, "search");

    // -----------------------------------------------------------------------
    // 1 22 5 & 1 22 6: satellite.system.threshold dials
    // -----------------------------------------------------------------------
    register_path("satellite.system.threshold()", [](ExecContext &, const std::vector<Value> &) {
        return Value::number(Number(static_cast<int64_t>(search_threshold())));
    }, false, 0);

    register_path("satellite.system.threshold(n)", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_number()) {
            long long lvl = 1;
            std::get<Number>(args[0]).to_integer(lvl);
            if (lvl >= SEARCH_TIGHTEST && lvl <= SEARCH_LOOSEST) {
                set_search_threshold(static_cast<int>(lvl));
            }
        }
        return Value::nil();
    }, false, 1);

    // -----------------------------------------------------------------------
    // 1 6 1: satellite.variable.string and its 16 methods (from M9)
    // -----------------------------------------------------------------------
    register_path("satellite.variable.string", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::sat_string(SatString{});
        if (args[0].is_string()) return args[0];
        return Value::sat_string(encode(args[0].to_string()));
    }, false, 1);

    register_path("satellite.variable.string.size", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            size_t sz = s ? s->size() : 0;
            return Value::number(Number(static_cast<uint64_t>(sz)));
        }
        return Value::number(Number(0));
    }, true, 0);

    register_path("satellite.variable.string.empty", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            return Value::boolean(!s || s->empty());
        }
        return Value::boolean(true);
    }, true, 0);

    register_path("satellite.variable.string.find(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_string()) {
            const auto *haystack = get_sat_string(args[0]);
            if (!haystack) return Value::number(Number(static_cast<int64_t>(-1)));

            SatString needle_sat;
            if (args[1].is_string()) {
                const auto *n = get_sat_string(args[1]);
                if (n) needle_sat = *n;
            } else {
                needle_sat = encode(args[1].to_string());
            }

            size_t pos = haystack->find(needle_sat);
            if (pos == SatString::npos) {
                return Value::number(Number(static_cast<int64_t>(-1)));
            }
            return Value::number(Number(static_cast<uint64_t>(pos)));
        }
        return Value::number(Number(static_cast<int64_t>(-1)));
    }, true, 1);

    register_path("satellite.variable.string.contains(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_string()) {
            const auto *haystack = get_sat_string(args[0]);
            if (!haystack) return Value::boolean(false);

            SatString needle_sat;
            if (args[1].is_string()) {
                const auto *n = get_sat_string(args[1]);
                if (n) needle_sat = *n;
            } else {
                needle_sat = encode(args[1].to_string());
            }

            return Value::boolean(haystack->find(needle_sat) != SatString::npos);
        }
        return Value::boolean(false);
    }, true, 1);

    register_path("satellite.variable.string.substring(start, end)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s || s->empty()) return Value::sat_string(SatString{});

            long long start = 0;
            if (args[1].is_number()) {
                std::get<Number>(args[1]).to_integer(start);
            }
            if (start < 0) start = 0;
            if (static_cast<size_t>(start) >= s->size()) return Value::sat_string(SatString{});

            long long end = static_cast<long long>(s->size());
            if (args.size() >= 3 && args[2].is_number()) {
                std::get<Number>(args[2]).to_integer(end);
            }
            if (end < start) end = start;
            if (static_cast<size_t>(end) > s->size()) end = static_cast<long long>(s->size());

            size_t len = static_cast<size_t>(end - start);
            return Value::sat_string(s->substr(static_cast<size_t>(start), len));
        }
        return Value::sat_string(SatString{});
    }, true, 2);

    register_path("satellite.variable.string.starts_with(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s) return Value::boolean(false);

            SatString prefix;
            if (args[1].is_string()) {
                const auto *p = get_sat_string(args[1]);
                if (p) prefix = *p;
            } else {
                prefix = encode(args[1].to_string());
            }

            if (prefix.size() > s->size()) return Value::boolean(false);
            return Value::boolean(s->compare(0, prefix.size(), prefix) == 0);
        }
        return Value::boolean(false);
    }, true, 1);

    register_path("satellite.variable.string.ends_with(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s) return Value::boolean(false);

            SatString suffix;
            if (args[1].is_string()) {
                const auto *p = get_sat_string(args[1]);
                if (p) suffix = *p;
            } else {
                suffix = encode(args[1].to_string());
            }

            if (suffix.size() > s->size()) return Value::boolean(false);
            return Value::boolean(s->compare(s->size() - suffix.size(), suffix.size(), suffix) == 0);
        }
        return Value::boolean(false);
    }, true, 1);

    register_path("satellite.variable.string.lower", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s) return Value::sat_string(SatString{});
            return Value::sat_string(to_lower_sat_string(*s));
        }
        return Value::sat_string(SatString{});
    }, true, 0);

    register_path("satellite.variable.string.upper", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s) return Value::sat_string(SatString{});
            return Value::sat_string(to_upper_sat_string(*s));
        }
        return Value::sat_string(SatString{});
    }, true, 0);

    register_path("satellite.variable.string.split(separator)", [](ExecContext &, const std::vector<Value> &args) {
        auto list = std::make_shared<List>();
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (s && !s->empty()) {
                SatString delim = encode(" ");
                if (args.size() >= 2 && args[1].is_string()) {
                    const auto *d = get_sat_string(args[1]);
                    if (d) delim = *d;
                }
                if (delim.empty()) {
                    for (SatChar c : *s) {
                        list->push_back(std::make_shared<Value>(Value::sat_string(SatString{c})));
                    }
                } else {
                    size_t pos = 0;
                    while (pos < s->size()) {
                        size_t next = s->find(delim, pos);
                        if (next == SatString::npos) {
                            list->push_back(std::make_shared<Value>(Value::sat_string(s->substr(pos))));
                            break;
                        }
                        list->push_back(std::make_shared<Value>(Value::sat_string(s->substr(pos, next - pos))));
                        pos = next + delim.size();
                    }
                }
            }
        }
        return Value(std::const_pointer_cast<const List>(list));
    }, true, 1);

    register_path("satellite.variable.string.trim", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s) return Value::sat_string(SatString{});
            return Value::sat_string(trim_sat_string(*s));
        }
        return Value::sat_string(SatString{});
    }, true, 0);

    register_path("satellite.variable.string.replace(a, b)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 3 && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s) return Value::sat_string(SatString{});

            SatString target = args[1].is_string() ? *get_sat_string(args[1]) : encode(args[1].to_string());
            SatString repl = args[2].is_string() ? *get_sat_string(args[2]) : encode(args[2].to_string());

            if (target.empty()) return Value::sat_string(*s);

            SatString result;
            size_t pos = 0;
            while (pos < s->size()) {
                size_t found = s->find(target, pos);
                if (found == SatString::npos) {
                    result += s->substr(pos);
                    break;
                }
                result += s->substr(pos, found - pos);
                result += repl;
                pos = found + target.size();
            }
            return Value::sat_string(result);
        }
        return Value::sat_string(SatString{});
    }, true, 2);

    register_path("satellite.variable.string.to_number", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (s) {
                std::string decoded = decode(*s);
                Number num;
                if (Number::parse(decoded, num)) {
                    return Value::number(num);
                }
            }
        }
        return Value::number(Number(0));
    }, true, 0);

    register_path("satellite.variable.string.append(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            SatString base = s ? *s : SatString{};
            if (args[1].is_string()) {
                const auto *extra = get_sat_string(args[1]);
                if (extra) base += *extra;
            } else {
                base += encode(args[1].to_string());
            }
            return Value::sat_string(base);
        }
        return Value::sat_string(SatString{});
    }, true, 1);

    register_path("satellite.variable.string.clear", [](ExecContext &, const std::vector<Value> &) {
        return Value::sat_string(SatString{});
    }, true, 0);

    register_path("satellite.variable.string.at(n)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_string() && args[1].is_number()) {
            const auto *s = get_sat_string(args[0]);
            if (s) {
                long long idx = 0;
                std::get<Number>(args[1]).to_integer(idx);
                if (idx >= 0 && static_cast<size_t>(idx) < s->size()) {
                    return Value::sat_string(SatString{(*s)[static_cast<size_t>(idx)]});
                }
            }
        }
        return Value::sat_string(SatString{});
    }, true, 1);
}

} // namespace satellite

