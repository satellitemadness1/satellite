// The satellite Handler Dispatch Table & M11 Handlers implementation.
// Milestone 11 Prototype in prototype/M11.

#include "dispatch.hpp"
#include "evaluator.hpp"
#include "console.hpp"
#include "search.hpp"
#include "render.hpp"
#include "satellite_string/satellite_string.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

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

int terminal_rows()
{
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_row > 0)
        return size.ws_row;
    return 24;
}

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

void init_m11_dispatch()
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
    // M11 Console Input & Terminal Facts (1 5 2 .. 1 5 9)
    // -----------------------------------------------------------------------
    register_path("satellite.console.input()", [](ExecContext &, const std::vector<Value> &) {
        Console::instance().drain();
        std::string line;
        if (std::getline(std::cin, line)) {
            return Value::sat_string(encode(line));
        }
        return Value::sat_string(SatString{});
    }, false, 0);

    register_path("satellite.console.input(prompt)", [](ExecContext &, const std::vector<Value> &args) {
        Console::instance().drain();
        if (!args.empty()) {
            std::string prompt_str = args[0].is_string() ? decode(*std::get<Str>(args[0])) : args[0].to_string();
            std::fputs(prompt_str.c_str(), stdout);
            std::fflush(stdout);
        }
        std::string line;
        if (std::getline(std::cin, line)) {
            return Value::sat_string(encode(line));
        }
        return Value::sat_string(SatString{});
    }, false, 1);

    register_path("satellite.console.input(prompt, target)", [](ExecContext &, const std::vector<Value> &args) {
        Console::instance().drain();
        if (!args.empty()) {
            std::string prompt_str = args[0].is_string() ? decode(*std::get<Str>(args[0])) : args[0].to_string();
            std::fputs(prompt_str.c_str(), stdout);
            std::fflush(stdout);
        }
        std::string line;
        if (std::getline(std::cin, line)) {
            return Value::sat_string(encode(line));
        }
        return Value::sat_string(SatString{});
    }, false, 2);

    register_path("satellite.console.width", [](ExecContext &, const std::vector<Value> &) {
        return Value::number(Number(static_cast<int64_t>(terminal_columns())));
    }, false, 0);

    register_path("satellite.console.height", [](ExecContext &, const std::vector<Value> &) {
        return Value::number(Number(static_cast<int64_t>(terminal_rows())));
    }, false, 0);

    register_path("satellite.console.clear()", [](ExecContext &, const std::vector<Value> &) {
        Console::instance().drain();
        std::fputs("\033[H\033[2J", stdout);
        std::fflush(stdout);
        return Value::satellite_singleton();
    }, false, 0);

    register_path("satellite.console.home()", [](ExecContext &, const std::vector<Value> &) {
        Console::instance().drain();
        std::fputs("\033[H", stdout);
        std::fflush(stdout);
        return Value::satellite_singleton();
    }, false, 0);

    // -----------------------------------------------------------------------
    // M11.A Window Console (1 24 2 (0) & 1 24 2 1)
    // -----------------------------------------------------------------------
    register_path("satellite.window.console", [](ExecContext &, const std::vector<Value> &) {
        auto map_body = std::make_shared<MapBody>();
        auto set_field = [&](const std::string &k, const Value &v) {
            std::string canonical;
            Value key_val = Value::sat_string(encode(k));
            map_key_of(key_val, canonical);
            map_body->index[canonical] = map_body->entries.size();
            map_body->entries.push_back(MapEntry{
                std::make_shared<Value>(key_val),
                std::make_shared<Value>(v)
            });
        };
        set_field("type", Value::sat_string(encode("satellite.window.console")));
        return Value(std::const_pointer_cast<const MapBody>(map_body));
    }, false, 0);

    register_path("satellite.window.console.new(title, width, height)", [](ExecContext &, const std::vector<Value> &args) {
        auto map_body = std::make_shared<MapBody>();
        auto set_field = [&](const std::string &k, const Value &v) {
            std::string canonical;
            Value key_val = Value::sat_string(encode(k));
            map_key_of(key_val, canonical);
            map_body->index[canonical] = map_body->entries.size();
            map_body->entries.push_back(MapEntry{
                std::make_shared<Value>(key_val),
                std::make_shared<Value>(v)
            });
        };
        Value title_val = (args.size() > 0) ? args[0] : Value::sat_string(encode("satellite"));
        Value width_val = (args.size() > 1) ? args[1] : Value::number(Number(800));
        Value height_val = (args.size() > 2) ? args[2] : Value::number(Number(600));

        set_field("type", Value::sat_string(encode("satellite.window.console")));
        set_field("title", title_val);
        set_field("width", width_val);
        set_field("height", height_val);
        return Value(std::const_pointer_cast<const MapBody>(map_body));
    }, false, 3);

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
    // 1 6 1: Strings and 16 Methods
    // -----------------------------------------------------------------------
    register_path("satellite.variable.string", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::sat_string(SatString{});
        if (args[0].is_string()) return args[0];
        return Value::sat_string(encode(args[0].to_string()));
    }, false, 1);

    register_path("satellite.variable.string.size", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::number(Number(0));
        const auto *str = get_sat_string(args[0]);
        return Value::number(Number(static_cast<int64_t>(str ? str->size() : 0)));
    }, true, 0);

    register_path("satellite.variable.string.empty", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::boolean(true);
        const auto *str = get_sat_string(args[0]);
        return Value::boolean(!str || str->empty());
    }, true, 0);

    register_path("satellite.variable.string.find(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 2) return Value::number(Number(-1));
        const auto *hay = get_sat_string(args[0]);
        const auto *needle = get_sat_string(args[1]);
        if (!hay || !needle) return Value::number(Number(-1));
        size_t pos = hay->find(*needle);
        return Value::number(Number(pos == SatString::npos ? -1 : static_cast<int64_t>(pos)));
    }, true, 1);

    register_path("satellite.variable.string.contains(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 2) return Value::boolean(false);
        const auto *hay = get_sat_string(args[0]);
        const auto *needle = get_sat_string(args[1]);
        if (!hay || !needle) return Value::boolean(false);
        return Value::boolean(hay->find(*needle) != SatString::npos);
    }, true, 1);

    register_path("satellite.variable.string.substring(start, end)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::sat_string(SatString{});
        const auto *str = get_sat_string(args[0]);
        if (!str) return Value::sat_string(SatString{});
        long long start = 0;
        if (args.size() > 1 && args[1].is_number()) std::get<Number>(args[1]).to_integer(start);
        long long end = static_cast<long long>(str->size());
        if (args.size() > 2 && args[2].is_number()) std::get<Number>(args[2]).to_integer(end);
        if (start < 0) start = 0;
        if (end < start) end = start;
        if (static_cast<size_t>(start) >= str->size()) return Value::sat_string(SatString{});
        size_t count = static_cast<size_t>(end - start);
        return Value::sat_string(str->substr(static_cast<size_t>(start), count));
    }, true, 2);

    register_path("satellite.variable.string.starts_with(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 2) return Value::boolean(false);
        const auto *hay = get_sat_string(args[0]);
        const auto *needle = get_sat_string(args[1]);
        if (!hay || !needle) return Value::boolean(false);
        return Value::boolean(hay->size() >= needle->size() && hay->compare(0, needle->size(), *needle) == 0);
    }, true, 1);

    register_path("satellite.variable.string.ends_with(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 2) return Value::boolean(false);
        const auto *hay = get_sat_string(args[0]);
        const auto *needle = get_sat_string(args[1]);
        if (!hay || !needle) return Value::boolean(false);
        return Value::boolean(hay->size() >= needle->size() && hay->compare(hay->size() - needle->size(), needle->size(), *needle) == 0);
    }, true, 1);

    register_path("satellite.variable.string.lower", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::sat_string(SatString{});
        const auto *str = get_sat_string(args[0]);
        if (!str) return Value::sat_string(SatString{});
        return Value::sat_string(to_lower_sat_string(*str));
    }, true, 0);

    register_path("satellite.variable.string.upper", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::sat_string(SatString{});
        const auto *str = get_sat_string(args[0]);
        if (!str) return Value::sat_string(SatString{});
        return Value::sat_string(to_upper_sat_string(*str));
    }, true, 0);

    register_path("satellite.variable.string.split(separator)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value(std::make_shared<const List>());
        const auto *str = get_sat_string(args[0]);
        if (!str) return Value(std::make_shared<const List>());
        SatString sep = (args.size() > 1 && args[1].is_string()) ? *get_sat_string(args[1]) : encode(" ");
        auto list = std::make_shared<List>();
        if (sep.empty()) {
            list->push_back(std::make_shared<Value>(args[0]));
            return Value(std::const_pointer_cast<const List>(list));
        }
        size_t start = 0;
        size_t pos = str->find(sep);
        while (pos != SatString::npos) {
            list->push_back(std::make_shared<Value>(Value::sat_string(str->substr(start, pos - start))));
            start = pos + sep.size();
            pos = str->find(sep, start);
        }
        list->push_back(std::make_shared<Value>(Value::sat_string(str->substr(start))));
        return Value(std::const_pointer_cast<const List>(list));
    }, true, 1);

    register_path("satellite.variable.string.trim", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::sat_string(SatString{});
        const auto *str = get_sat_string(args[0]);
        if (!str) return Value::sat_string(SatString{});
        return Value::sat_string(trim_sat_string(*str));
    }, true, 0);

    register_path("satellite.variable.string.replace(a, b)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 3) return args.empty() ? Value::sat_string(SatString{}) : args[0];
        const auto *str = get_sat_string(args[0]);
        const auto *from = get_sat_string(args[1]);
        const auto *to = get_sat_string(args[2]);
        if (!str || !from || !to || from->empty()) return args[0];
        SatString result;
        size_t start = 0;
        size_t pos = str->find(*from);
        while (pos != SatString::npos) {
            result.append(str->substr(start, pos - start));
            result.append(*to);
            start = pos + from->size();
            pos = str->find(*from, start);
        }
        result.append(str->substr(start));
        return Value::sat_string(result);
    }, true, 2);

    register_path("satellite.variable.string.to_number", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::number(Number(0));
        const auto *str = get_sat_string(args[0]);
        if (!str) return Value::number(Number(0));
        std::string dec = decode(*str);
        Number num;
        if (Number::parse(dec, num)) {
            return Value::number(num);
        }
        return Value::number(Number(0));
    }, true, 0);

    register_path("satellite.variable.string.append(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::sat_string(SatString{});
        const auto *str = get_sat_string(args[0]);
        SatString result = str ? *str : SatString{};
        if (args.size() > 1) {
            if (args[1].is_string()) {
                result.append(*get_sat_string(args[1]));
            } else {
                result.append(encode(args[1].to_string()));
            }
        }
        return Value::sat_string(result);
    }, true, 1);

    register_path("satellite.variable.string.clear", [](ExecContext &, const std::vector<Value> &) {
        return Value::sat_string(SatString{});
    }, true, 0);

    register_path("satellite.variable.string.at(n)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 2) return Value::sat_string(SatString{});
        const auto *str = get_sat_string(args[0]);
        if (!str) return Value::sat_string(SatString{});
        long long n = 0;
        if (args[1].is_number()) std::get<Number>(args[1]).to_integer(n);
        if (n < 0 || static_cast<size_t>(n) >= str->size()) return Value::sat_string(SatString{});
        return Value::sat_string(SatString(1, (*str)[static_cast<size_t>(n)]));
    }, true, 1);

    // -----------------------------------------------------------------------
    // Containers: Map (1 4 1 (0) & 9 methods)
    // -----------------------------------------------------------------------
    register_path("satellite.container", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const List>());
    }, false, 0);

    register_path("satellite.container.arguments", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const List>());
    }, false, 0);

    register_path("satellite.container.result", [](ExecContext &, const std::vector<Value> &) {
        return Value::nil();
    }, false, 0);

    register_path("satellite.container.map", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const MapBody>());
    }, false, 0);

    register_path("satellite.container.map.set(k, v)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value(std::make_shared<const MapBody>());
        auto map_ref = std::holds_alternative<MapRef>(args[0]) ? std::get<MapRef>(args[0]) : nullptr;
        auto new_map = map_ref ? std::make_shared<MapBody>(*map_ref) : std::make_shared<MapBody>();

        if (args.size() >= 3) {
            Value k = args[1];
            Value v = args[2];
            std::string canonical;
            if (map_key_of(k, canonical)) {
                auto it = new_map->index.find(canonical);
                if (it != new_map->index.end()) {
                    new_map->entries[it->second].value = std::make_shared<Value>(std::move(v));
                } else {
                    new_map->index[canonical] = new_map->entries.size();
                    new_map->entries.push_back(MapEntry{
                        std::make_shared<Value>(std::move(k)),
                        std::make_shared<Value>(std::move(v))
                    });
                }
            }
        }
        return Value(std::const_pointer_cast<const MapBody>(new_map));
    }, true, 2);

    register_path("satellite.container.map.get(k)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 2 || !std::holds_alternative<MapRef>(args[0])) return Value::nil();
        auto map_ref = std::get<MapRef>(args[0]);
        if (!map_ref) return Value::nil();
        std::string canonical;
        if (map_key_of(args[1], canonical)) {
            auto it = map_ref->index.find(canonical);
            if (it != map_ref->index.end() && it->second < map_ref->entries.size()) {
                const auto &val_ptr = map_ref->entries[it->second].value;
                if (val_ptr) return *val_ptr;
            }
        }
        return Value::nil();
    }, true, 1);

    register_path("satellite.container.map.has(k)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 2 || !std::holds_alternative<MapRef>(args[0])) return Value::boolean(false);
        auto map_ref = std::get<MapRef>(args[0]);
        if (!map_ref) return Value::boolean(false);
        std::string canonical;
        if (map_key_of(args[1], canonical)) {
            return Value::boolean(map_ref->index.find(canonical) != map_ref->index.end());
        }
        return Value::boolean(false);
    }, true, 1);

    register_path("satellite.container.map.size", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !std::holds_alternative<MapRef>(args[0])) return Value::number(Number(0));
        auto map_ref = std::get<MapRef>(args[0]);
        return Value::number(Number(static_cast<int64_t>(map_ref ? map_ref->entries.size() : 0)));
    }, true, 0);

    register_path("satellite.container.map.empty", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !std::holds_alternative<MapRef>(args[0])) return Value::boolean(true);
        auto map_ref = std::get<MapRef>(args[0]);
        return Value::boolean(!map_ref || map_ref->entries.empty());
    }, true, 0);

    register_path("satellite.container.map.clear", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const MapBody>());
    }, true, 0);

    register_path("satellite.container.map.remove(k)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !std::holds_alternative<MapRef>(args[0])) return Value(std::make_shared<const MapBody>());
        auto map_ref = std::get<MapRef>(args[0]);
        if (!map_ref) return Value(std::make_shared<const MapBody>());
        auto new_map = std::make_shared<MapBody>();
        std::string target_key;
        if (args.size() > 1) map_key_of(args[1], target_key);

        for (const auto &entry : map_ref->entries) {
            std::string cur_key;
            if (entry.key && map_key_of(*entry.key, cur_key)) {
                if (cur_key != target_key) {
                    new_map->index[cur_key] = new_map->entries.size();
                    new_map->entries.push_back(entry);
                }
            }
        }
        return Value(std::const_pointer_cast<const MapBody>(new_map));
    }, true, 1);

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
    // Containers: List (1 4 2 (0) & 25 methods)
    // -----------------------------------------------------------------------
    register_path("satellite.container.list", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const List>());
    }, false, 0);

    register_path("satellite.container.list.append", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value(std::make_shared<const List>());
        auto list_ref = args[0].is_list() ? std::get<ListRef>(args[0]) : nullptr;
        auto new_list = list_ref ? std::make_shared<List>(*list_ref) : std::make_shared<List>();
        if (args.size() > 1) {
            new_list->push_back(std::make_shared<Value>(args[1]));
        }
        return Value(std::const_pointer_cast<const List>(new_list));
    }, true, 1);

    register_path("satellite.container.list.size", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value::number(Number(0));
        auto list_ref = std::get<ListRef>(args[0]);
        return Value::number(Number(static_cast<int64_t>(list_ref ? list_ref->size() : 0)));
    }, true, 0);

    auto sort_list_fn = [](const List &in_list, bool desc, const std::string &key_name) -> Value {
        auto sorted = std::make_shared<List>(in_list);
        std::stable_sort(sorted->begin(), sorted->end(), [&](const ValuePtr &ap, const ValuePtr &bp) {
            if (!ap || !bp) return bp != nullptr;
            Value a_val = extract_key_field(*ap, key_name);
            Value b_val = extract_key_field(*bp, key_name);
            if (desc) {
                return compare_values_less(b_val, a_val);
            }
            return compare_values_less(a_val, b_val);
        });
        return Value(std::const_pointer_cast<const List>(sorted));
    };

    register_path("satellite.container.list.sort()", [sort_list_fn](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        return list_ref ? sort_list_fn(*list_ref, false, "") : Value(std::make_shared<const List>());
    }, true, 0);

    register_path("satellite.container.list.sort(direction)", [sort_list_fn](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value(std::make_shared<const List>());
        bool desc = false;
        if (args.size() > 1 && args[1].is_string()) {
            std::string d = decode(*std::get<Str>(args[1]));
            desc = (d == "down" || d == "desc" || d == "descending");
        }
        return sort_list_fn(*list_ref, desc, "");
    }, true, 1);

    register_path("satellite.container.list.sort_down()", [sort_list_fn](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        return list_ref ? sort_list_fn(*list_ref, true, "") : Value(std::make_shared<const List>());
    }, true, 0);

    register_path("satellite.container.list.sort_down(key)", [sort_list_fn](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value(std::make_shared<const List>());
        std::string key_str = (args.size() > 1 && args[1].is_string()) ? decode(*std::get<Str>(args[1])) : "";
        return sort_list_fn(*list_ref, true, key_str);
    }, true, 1);

    register_path("satellite.container.list.sort_up(key)", [sort_list_fn](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value(std::make_shared<const List>());
        std::string key_str = (args.size() > 1 && args[1].is_string()) ? decode(*std::get<Str>(args[1])) : "";
        return sort_list_fn(*list_ref, false, key_str);
    }, true, 1);

    register_path("satellite.container.list.contains(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 2 || !args[0].is_list()) return Value::boolean(false);
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value::boolean(false);
        for (const auto &item : *list_ref) {
            if (item && *item == args[1]) return Value::boolean(true);
        }
        return Value::boolean(false);
    }, true, 1);

    register_path("satellite.container.list.index_of(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() < 2 || !args[0].is_list()) return Value::number(Number(-1));
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value::number(Number(-1));
        for (size_t i = 0; i < list_ref->size(); ++i) {
            if ((*list_ref)[i] && *(*list_ref)[i] == args[1]) {
                return Value::number(Number(static_cast<int64_t>(i)));
            }
        }
        return Value::number(Number(-1));
    }, true, 1);

    register_path("satellite.container.list.empty", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value::boolean(true);
        auto list_ref = std::get<ListRef>(args[0]);
        return Value::boolean(!list_ref || list_ref->empty());
    }, true, 0);

    register_path("satellite.container.list.clear", [](ExecContext &, const std::vector<Value> &) {
        return Value(std::make_shared<const List>());
    }, true, 0);

    register_path("satellite.container.list.first", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value::nil();
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref || list_ref->empty()) return Value::nil();
        return (*list_ref)[0] ? *(*list_ref)[0] : Value::nil();
    }, true, 0);

    register_path("satellite.container.list.last", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value::nil();
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref || list_ref->empty()) return Value::nil();
        return list_ref->back() ? *list_ref->back() : Value::nil();
    }, true, 0);

    register_path("satellite.container.list.truncate(n)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value(std::make_shared<const List>());
        long long n_val = static_cast<long long>(list_ref->size());
        if (args.size() > 1 && args[1].is_number()) std::get<Number>(args[1]).to_integer(n_val);
        size_t n = static_cast<size_t>(std::max<long long>(0, n_val));
        auto truncated = std::make_shared<List>();
        for (size_t i = 0; i < n && i < list_ref->size(); ++i) {
            truncated->push_back((*list_ref)[i]);
        }
        return Value(std::const_pointer_cast<const List>(truncated));
    }, true, 1);

    register_path("satellite.container.list.reserve(n)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value(std::make_shared<const List>());
        return args[0];
    }, true, 1);

    register_path("satellite.container.list.remove_first()", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref || list_ref->empty()) return Value(std::make_shared<const List>());
        auto new_list = std::make_shared<List>();
        new_list->reserve(list_ref->size() - 1);
        for (size_t i = 1; i < list_ref->size(); ++i) {
            new_list->push_back((*list_ref)[i]);
        }
        return Value(std::const_pointer_cast<const List>(new_list));
    }, true, 0);

    register_path("satellite.container.list.remove_last()", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref || list_ref->empty()) return Value(std::make_shared<const List>());
        auto new_list = std::make_shared<List>();
        new_list->reserve(list_ref->size() - 1);
        for (size_t i = 0; i + 1 < list_ref->size(); ++i) {
            new_list->push_back((*list_ref)[i]);
        }
        return Value(std::const_pointer_cast<const List>(new_list));
    }, true, 0);

    register_path("satellite.container.list.remove_at(n)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value(std::make_shared<const List>());
        long long n = -1;
        if (args.size() > 1 && args[1].is_number()) std::get<Number>(args[1]).to_integer(n);
        auto new_list = std::make_shared<List>();
        for (size_t i = 0; i < list_ref->size(); ++i) {
            if (static_cast<long long>(i) != n) {
                new_list->push_back((*list_ref)[i]);
            }
        }
        return Value(std::const_pointer_cast<const List>(new_list));
    }, true, 1);

    register_path("satellite.container.list.remove(x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value(std::make_shared<const List>());
        Value target = args.size() > 1 ? args[1] : Value::nil();
        auto new_list = std::make_shared<List>();
        for (const auto &item : *list_ref) {
            if (!item || !(*item == target)) {
                new_list->push_back(item);
            }
        }
        return Value(std::const_pointer_cast<const List>(new_list));
    }, true, 1);

    register_path("satellite.container.list.insert(n, x)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        auto new_list = std::make_shared<List>();
        long long n = 0;
        if (args.size() > 1 && args[1].is_number()) std::get<Number>(args[1]).to_integer(n);
        Value item = args.size() > 2 ? args[2] : Value::nil();
        if (n < 0) n = 0;
        size_t ins_pos = static_cast<size_t>(n);
        if (list_ref) {
            if (ins_pos > list_ref->size()) ins_pos = list_ref->size();
            for (size_t i = 0; i < ins_pos; ++i) new_list->push_back((*list_ref)[i]);
            new_list->push_back(std::make_shared<Value>(item));
            for (size_t i = ins_pos; i < list_ref->size(); ++i) new_list->push_back((*list_ref)[i]);
        } else {
            new_list->push_back(std::make_shared<Value>(item));
        }
        return Value(std::const_pointer_cast<const List>(new_list));
    }, true, 2);

    register_path("satellite.container.list.join(separator)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value::sat_string(SatString{});
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref || list_ref->empty()) return Value::sat_string(SatString{});
        SatString sep = (args.size() > 1 && args[1].is_string()) ? *get_sat_string(args[1]) : SatString{};
        SatString result;
        for (size_t i = 0; i < list_ref->size(); ++i) {
            if (i > 0) result.append(sep);
            const auto &item = (*list_ref)[i];
            if (item) {
                if (item->is_string()) result.append(*get_sat_string(*item));
                else result.append(encode(item->to_string()));
            }
        }
        return Value::sat_string(result);
    }, true, 1);

    register_path("satellite.container.list.reverse", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value(std::make_shared<const List>());
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value(std::make_shared<const List>());
        auto rev = std::make_shared<List>(list_ref->rbegin(), list_ref->rend());
        return Value(std::const_pointer_cast<const List>(rev));
    }, true, 0);

    register_path("satellite.container.list.sum", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value::number(Number(0));
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref) return Value::number(Number(0));
        Number sum(0);
        for (const auto &item : *list_ref) {
            if (item && item->is_number()) {
                sum = sum + std::get<Number>(*item);
            }
        }
        return Value::number(sum);
    }, true, 0);

    register_path("satellite.container.list.max", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value::nil();
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref || list_ref->empty()) return Value::nil();
        Value best = *(*list_ref)[0];
        for (size_t i = 1; i < list_ref->size(); ++i) {
            if ((*list_ref)[i] && compare_values_less(best, *(*list_ref)[i])) {
                best = *(*list_ref)[i];
            }
        }
        return best;
    }, true, 0);

    register_path("satellite.container.list.min", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty() || !args[0].is_list()) return Value::nil();
        auto list_ref = std::get<ListRef>(args[0]);
        if (!list_ref || list_ref->empty()) return Value::nil();
        Value best = *(*list_ref)[0];
        for (size_t i = 1; i < list_ref->size(); ++i) {
            if ((*list_ref)[i] && compare_values_less(*(*list_ref)[i], best)) {
                best = *(*list_ref)[i];
            }
        }
        return best;
    }, true, 0);

    // -----------------------------------------------------------------------
    // System Threshold Dials (1 22 5 & 1 22 6)
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
        return Value::number(Number(static_cast<int64_t>(search_threshold())));
    }, false, 1);
}

} // namespace satellite
