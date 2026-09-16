// The satellite Value Model implementation.

#include "value.hpp"

#include <sstream>

namespace satellite {

bool Value::is_truthy() const
{
    return std::visit(
        [](const auto &val) -> bool {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return false;
            } else if constexpr (std::is_same_v<T, bool>) {
                return val;
            } else if constexpr (std::is_same_v<T, Number>) {
                return !val.is_zero();
            } else if constexpr (std::is_same_v<T, Str>) {
                return val && !val->empty();
            } else if constexpr (std::is_same_v<T, ListRef>) {
                return val && !val->empty();
            } else {
                return true;
            }
        },
        *this);
}

std::string Value::to_string() const
{
    return std::visit(
        [](const auto &val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return "nil";
            } else if constexpr (std::is_same_v<T, bool>) {
                return val ? "satellite.bool.true" : "satellite.bool.false";
            } else if constexpr (std::is_same_v<T, Number>) {
                return val.to_string();
            } else if constexpr (std::is_same_v<T, Str>) {
                return val ? decode(*val) : "";
            } else if constexpr (std::is_same_v<T, ListRef>) {
                if (!val) return "[]";
                std::string s = "[";
                for (size_t i = 0; i < val->size(); ++i) {
                    if (i > 0) s += ", ";
                    s += (*val)[i] ? (*val)[i]->to_string() : "nil";
                }
                s += "]";
                return s;
            } else if constexpr (std::is_same_v<T, ObjectPtr>) {
                return "<object>";
            } else if constexpr (std::is_same_v<T, Time>) {
                return "<time " + std::to_string(val.ns) + ">";
            } else if constexpr (std::is_same_v<T, FilePtr>) {
                return val ? "<file " + val->path + ">" : "<file (closed)>";
            } else if constexpr (std::is_same_v<T, MapRef>) {
                return "<map>";
            } else if constexpr (std::is_same_v<T, BitsRef>) {
                if (!val) return "x0";
                return (val->radix == 2 ? "b" : "x") + val->digits;
            } else if constexpr (std::is_same_v<T, ArgsRef>) {
                return "<arguments>";
            } else if constexpr (std::is_same_v<T, ResultRef>) {
                return val && val->ok ? "<result: ok>" : "<result: fail>";
            } else {
                return "<unknown>";
            }
        },
        *this);
}

bool Value::operator==(const Value &other) const
{
    if (index() != other.index())
        return false;

    return std::visit(
        [&other](const auto &lhs) -> bool {
            using T = std::decay_t<decltype(lhs)>;
            const auto &rhs = std::get<T>(other);
            if constexpr (std::is_same_v<T, std::monostate>) {
                return true;
            } else if constexpr (std::is_same_v<T, bool>) {
                return lhs == rhs;
            } else if constexpr (std::is_same_v<T, Number>) {
                return lhs == rhs;
            } else if constexpr (std::is_same_v<T, Str>) {
                if (lhs == rhs) return true;
                if (!lhs || !rhs) return false;
                return *lhs == *rhs;
            } else if constexpr (std::is_same_v<T, ListRef>) {
                if (lhs == rhs) return true;
                if (!lhs || !rhs) return false;
                if (lhs->size() != rhs->size()) return false;
                for (size_t i = 0; i < lhs->size(); ++i) {
                    const auto &li = (*lhs)[i];
                    const auto &ri = (*rhs)[i];
                    if (li == ri) continue;
                    if (!li || !ri) return false;
                    if (*li != *ri) return false;
                }
                return true;
            } else if constexpr (std::is_same_v<T, ObjectPtr>) {
                return lhs == rhs; // Reference identity
            } else if constexpr (std::is_same_v<T, Time>) {
                return lhs.ns == rhs.ns;
            } else if constexpr (std::is_same_v<T, FilePtr>) {
                return lhs == rhs; // Reference identity
            } else if constexpr (std::is_same_v<T, BitsRef>) {
                if (lhs == rhs) return true;
                if (!lhs || !rhs) return false;
                return *lhs == *rhs;
            } else {
                return lhs == rhs;
            }
        },
        *this);
}

} // namespace satellite
