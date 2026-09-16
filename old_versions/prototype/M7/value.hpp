#pragma once

// The satellite Value Model -- Milestone 7 Prototype.
//
// DESIGN §8.2 & PLAN §8: 40-byte Value variant on 64-bit systems.
// Small exact numbers, bools, and nil never allocate heap buffers.
// Strings, lists, maps, objects, and handles are shared via immutable/reference handles.

#include "number.hpp"
#include "satellite_string/satellite_string.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace satellite {

struct Value;
struct SpacesuitInfo;

// Snapshot handle used in containers and objects
using ValuePtr = std::shared_ptr<const Value>;

// Containers
using List = std::vector<ValuePtr>;
using Str = std::shared_ptr<const SatString>;
using ListRef = std::shared_ptr<const List>;

// satellite.container.map<K, V>
struct MapEntry {
    ValuePtr key;
    ValuePtr value;
};

struct MapBody {
    std::vector<MapEntry> entries;
    std::unordered_map<std::string, size_t> index;
};
using MapRef = std::shared_ptr<const MapBody>;

// satellite.variable.time
struct Time {
    long long ns = 0;
    bool operator==(const Time &) const = default;
};

// satellite.spacesuit instance (Reference semantics, DESIGN §13)
struct Object {
    const SpacesuitInfo *suit = nullptr;
    std::vector<ValuePtr> fields;

    explicit Object(size_t count = 0) : fields(count) {}
};
using ObjectPtr = std::shared_ptr<Object>;

// satellite.variable.binary and satellite.variable.hex (DESIGN §8.5)
struct Bits {
    unsigned radix = 16; // 2 or 16
    std::string digits;  // Normalised upper-case characters
    bool operator==(const Bits &) const = default;
};
using BitsRef = std::shared_ptr<const Bits>;

// satellite.variable.file (Reference semantics, DESIGN §8)
struct FileHandle {
    int fd = -1;
    int last_error = 0;
    std::string path;
    bool readable = false;
    bool writable = false;
    int reopen_flags = 0;
};
using FilePtr = std::shared_ptr<FileHandle>;

// Special arguments object (DESIGN §7.7, M19 append)
struct ArgsBody {
    std::string username;
    unsigned threads = 0;
    unsigned long mem_total_mb = 0;
    unsigned long mem_used_mb = 0;
    std::string cwd;
};
using ArgsRef = std::shared_ptr<const ArgsBody>;

// Result object (M24 append)
struct ResultBody {
    bool ok = true;
    std::string message;
};
using ResultRef = std::shared_ptr<const ResultBody>;

// ---------------------------------------------------------------------------
// Value Variant (Append-only, 40 bytes on 64-bit)
// ---------------------------------------------------------------------------

// Variant indices:
// 0: monostate (nil)
// 1: bool
// 2: Number
// 3: Str
// 4: ListRef
// 5: ObjectPtr
// 6: Time
// 7: FilePtr
// 8: MapRef
// 9: BitsRef
// 10: ArgsRef
// 11: ResultRef
using ValueBase = std::variant<std::monostate, bool, Number, Str, ListRef,
                               ObjectPtr, Time, FilePtr, MapRef, BitsRef,
                               ArgsRef, ResultRef>;

struct Value : ValueBase {
    using ValueBase::ValueBase;

    Value() : ValueBase(std::monostate{}) {}

    // Convenience constructors
    static Value nil() { return Value(std::monostate{}); }
    static Value boolean(bool b) { return Value(b); }
    static Value number(Number n) { return Value(n); }
    static Value string(const std::string &s) {
        return Value(std::make_shared<const SatString>(encode(s)));
    }
    static Value sat_string(const SatString &s) {
        return Value(std::make_shared<const SatString>(s));
    }
    static Value satellite_singleton() {
        // Sentinel value representing the `satellite` runtime singleton
        return Value(true);
    }

    bool is_nil() const { return std::holds_alternative<std::monostate>(*this); }
    bool is_bool() const { return std::holds_alternative<bool>(*this); }
    bool is_number() const { return std::holds_alternative<Number>(*this); }
    bool is_string() const { return std::holds_alternative<Str>(*this); }
    bool is_list() const { return std::holds_alternative<ListRef>(*this); }
    bool is_object() const { return std::holds_alternative<ObjectPtr>(*this); }
    bool is_truthy() const;

    std::string to_string() const;
    bool operator==(const Value &other) const;
    bool operator!=(const Value &other) const { return !(*this == other); }
};

// Invariant: Value must stay 40 bytes on 64-bit systems (DESIGN §8.2)
static_assert(sizeof(void *) != 8 || sizeof(Value) == 40,
              "Value must stay 40 bytes on 64-bit (DESIGN §8.2, PLAN §6.1)");

} // namespace satellite
