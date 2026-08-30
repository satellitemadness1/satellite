// The satellite Handler Dispatch Table & M9 Handlers implementation.
// Milestone 9 Prototype in prototype/M9.

#include "dispatch.hpp"
#include "evaluator.hpp"
#include "console.hpp"
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

// Helper: Extract SatString from Value
const SatString *get_sat_string(const Value &val)
{
    if (val.is_string()) {
        auto str_ptr = std::get<Str>(val);
        if (str_ptr) return str_ptr.get();
    }
    return nullptr;
}

// Helper: Trim SatString
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

// Helper: Lowercase SatString
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

// Helper: Uppercase SatString
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

} // namespace

void init_m9_dispatch()
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

    // 1 17 1: satellite.bool.false
    register_path("satellite.bool.false", [](ExecContext &, const std::vector<Value> &) {
        return Value::boolean(false);
    }, false, 0);

    // 1 17 2: satellite.bool.true
    register_path("satellite.bool.true", [](ExecContext &, const std::vector<Value> &) {
        return Value::boolean(true);
    }, false, 0);

    // 1 6 6: satellite.variable.bool
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
    // 1 4 2: Containers (List helper methods)
    // -----------------------------------------------------------------------
    register_path("satellite.container.list.size", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            size_t sz = list_ref ? list_ref->size() : 0;
            return Value::number(Number(static_cast<uint64_t>(sz)));
        }
        return Value::number(Number(0));
    }, true, 0);

    register_path("satellite.container.list.empty", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            return Value::boolean(!list_ref || list_ref->empty());
        }
        return Value::boolean(true);
    }, true, 0);

    register_path("satellite.container.list.append", [](ExecContext &, const std::vector<Value> &args) {
        if (args.size() >= 2 && args[0].is_list()) {
            auto list_ref = std::get<ListRef>(args[0]);
            auto mutable_list = std::make_shared<List>(list_ref ? *list_ref : List{});
            mutable_list->push_back(std::make_shared<Value>(args[1]));
            return Value(std::const_pointer_cast<const List>(mutable_list));
        }
        return Value::nil();
    }, true, 1);

    // -----------------------------------------------------------------------
    // 1 6 1: satellite.variable.string and its 16 methods
    // -----------------------------------------------------------------------
    register_path("satellite.variable.string", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::sat_string(SatString{});
        if (args[0].is_string()) return args[0];
        return Value::sat_string(encode(args[0].to_string()));
    }, false, 1);

    // 1 6 1 1: size
    register_path("satellite.variable.string.size", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            size_t sz = s ? s->size() : 0;
            return Value::number(Number(static_cast<uint64_t>(sz)));
        }
        return Value::number(Number(0));
    }, true, 0);

    // 1 6 1 2: empty
    register_path("satellite.variable.string.empty", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            return Value::boolean(!s || s->empty());
        }
        return Value::boolean(true);
    }, true, 0);

    // 1 6 1 3: find(x)
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

    // 1 6 1 4: contains(x)
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

    // 1 6 1 5: substring(start, end)
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

    // 1 6 1 6: starts_with(x)
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

    // 1 6 1 7: ends_with(x)
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

    // 1 6 1 8: lower
    register_path("satellite.variable.string.lower", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s) return Value::sat_string(SatString{});
            return Value::sat_string(to_lower_sat_string(*s));
        }
        return Value::sat_string(SatString{});
    }, true, 0);

    // 1 6 1 9: upper
    register_path("satellite.variable.string.upper", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s) return Value::sat_string(SatString{});
            return Value::sat_string(to_upper_sat_string(*s));
        }
        return Value::sat_string(SatString{});
    }, true, 0);

    // 1 6 1 10: split(separator)
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

    // 1 6 1 11: trim
    register_path("satellite.variable.string.trim", [](ExecContext &, const std::vector<Value> &args) {
        if (!args.empty() && args[0].is_string()) {
            const auto *s = get_sat_string(args[0]);
            if (!s) return Value::sat_string(SatString{});
            return Value::sat_string(trim_sat_string(*s));
        }
        return Value::sat_string(SatString{});
    }, true, 0);

    // 1 6 1 12: replace(a, b)
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

    // 1 6 1 13: to_number
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

    // 1 6 1 14: append(x)
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

    // 1 6 1 15: clear
    register_path("satellite.variable.string.clear", [](ExecContext &, const std::vector<Value> &) {
        return Value::sat_string(SatString{});
    }, true, 0);

    // 1 6 1 16: at(n)
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
