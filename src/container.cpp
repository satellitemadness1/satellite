#include "satellite/container.hpp"

#include <string>

namespace satellite {

const char* type_name(Type t) {
    switch (t) {
        case Type::Nil:  return "nil";
        case Type::Bool: return "bool";
        case Type::Int:  return "int";
        case Type::Big:  return "big";
        case Type::Real: return "real";
        case Type::Str:  return "string";
        case Type::List: return "list";
        default:         return "?";
    }
}
// ===========================================================================
// Container lifecycle
// ===========================================================================
Container Container::str(const char* s) {
    return str(std::string(s));
}

Container Container::str(const std::string& s) {
    auto* p = new SatString();
    p->append_text(s);
    Container c;
    c.type = Type::Str;
    c.obj  = p;
    return c;
}

Container Container::list() {
    Container c;
    c.type = Type::List;
    c.obj  = new List();
    return c;
}

Container Container::big_from_i64(int64_t v) { return big::from_i64(v); }

void Container::release() {
    if (!is_heap() || !obj) return;
    if (obj->rc.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        switch (obj->type) {
            case Type::Str:  delete static_cast<SatString*>(obj); break;
            case Type::Big:  delete static_cast<BigInt*>(obj);    break;
            case Type::List: delete static_cast<List*>(obj);      break;
            default:         delete obj;                          break;
        }
    }
    obj = nullptr;
}

Container& Container::operator=(const Container& o) {
    if (this == &o) return *this;
    Container tmp(o);            // retain first: handles aliasing safely
    release();
    type = tmp.type;
    i    = tmp.i;
    tmp.type = Type::Nil;
    tmp.i    = 0;
    return *this;
}

Container& Container::operator=(Container&& o) noexcept {
    if (this == &o) return *this;
    release();
    type = o.type;
    i    = o.i;
    o.type = Type::Nil;
    o.i    = 0;
    return *this;
}

bool Container::truthy() const {
    switch (type) {
        case Type::Nil:  return false;
        case Type::Bool: return b;
        case Type::Int:  return i != 0;
        case Type::Real: return d != 0.0;
        case Type::Big:  return !as_big()->is_zero();
        case Type::Str:  return as_str()->len != 0;
        case Type::List: return !as_list()->items.empty();
        default:         return false;
    }
}

std::string Container::to_string() const {
    switch (type) {
        case Type::Nil:  return "nil";
        case Type::Bool: return b ? "true" : "false";
        case Type::Int:  return std::to_string(i);
        case Type::Real: return std::to_string(d);
        case Type::Big:  return big::to_string(*as_big());
        case Type::Str:  return as_str()->text();
        case Type::List: {
            std::string s = "[";
            const auto& v = as_list()->items;
            for (size_t k = 0; k < v.size(); ++k) {
                if (k) s += ", ";
                s += v[k].to_string();
            }
            return s + "]";
        }
        default: return "?";
    }
}

}  // namespace satellite
