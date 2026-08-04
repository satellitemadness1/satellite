#include "satellite/container.hpp"

#include <string>

// A Capsule is deleted, rendered and tested for truth here, and all three need
// the complete type -- destruction goes through the type tag, so this file is
// the one place that has to know every heap payload there is.
#include "satellite/capsule.hpp"

namespace satellite {

const char* type_name(Type t) {
    switch (t) {
        case Type::Nil:     return "nil";
        case Type::Bool:    return "bool";
        case Type::Int:     return "int";
        case Type::Big:     return "big";
        case Type::Real:    return "real";
        case Type::Str:     return "string";
        case Type::List:    return "list";
        case Type::Capsule: return "capsule";
        default:            return "?";
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

// Takes the reference the caller already holds -- a fresh Capsule starts at
// rc 1, so wrapping it must not add a second one.
Container Container::capsule(Capsule* p) {
    Container c;
    c.type = Type::Capsule;
    c.obj  = p;
    return c;
}

Container Container::big_from_i64(int64_t v) { return big::from_i64(v); }

void Container::release() {
    if (!is_heap() || !obj) return;
    if (obj->rc.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        switch (obj->type) {
            case Type::Str:     delete static_cast<SatString*>(obj); break;
            case Type::Big:     delete static_cast<BigInt*>(obj);    break;
            case Type::List:    delete static_cast<List*>(obj);      break;
            case Type::Capsule: delete static_cast<Capsule*>(obj);   break;
            default:            delete obj;                          break;
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
        // A declared capsule is a thing that exists.  There is no such value as
        // an empty capsule -- a body with no statements is still a capsule.
        case Type::Capsule: return true;
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
        // The signature, not the body: printing a capsule should identify it,
        // and the body is a span into a file the reader can already open.
        case Type::Capsule: return as_capsule()->signature();
        default: return "?";
    }
}

}  // namespace satellite
