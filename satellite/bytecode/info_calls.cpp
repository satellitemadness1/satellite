// satellite.info's words. See info_calls.hpp.

#include "info_calls.hpp"

#include "word_codes.hpp"
#include "../machine/filesystems.hpp"
#include "../machine/shown.hpp"
#include "../machine/stop_flag.hpp"
#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_variable_float/float_scaled.hpp"
#include "../satl/listing.hpp"
#include "../satl/listing_counts.hpp"

#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>

namespace satellite004 {

namespace {

using token::Code;

// A name as a string. A name that is not UTF-8 -- Linux allows any bytes but / and
// NUL -- is written with its odd bytes shown as \xNN, as the table shows it, rather
// than cut short or dropped.
Value text(const std::string &utf8)
{
    Value made;
    std::size_t bad_offset = 0;
    if (Value::of_utf8(utf8, made, bad_offset) != success)
        Value::of_utf8(shown(utf8), made, bad_offset);
    return made;
}

void put(satelliteIndex &keys, const char *key, Value value)
{
    const Value name = text(key);
    std::string filed;
    key_name_of(name, filed);
    value_for_writing(keys, filed, name) = std::move(value);
}

Value a_number(unsigned long long int n)
{
    return Value::of_number(satellite_number(n));
}

void put_size(satelliteIndex &keys, unsigned long long int bytes)
{
    const SizeParts parts = size_parts(bytes);
    const satellite_float size = parts.unit == 0 ? float_of_number(satellite_number(parts.whole))
                                                 : float_from_scaled(satellite_number(parts.whole * 1000 + parts.thousandths), 3);
    put(keys, "size", Value::of_float(size));
    put(keys, "size_type", text(size_unit_names[parts.unit]));
}

std::string leaf_of(const std::string &path)
{
    std::string trimmed = path;
    while (trimmed.size() > 1 && trimmed.back() == '/')
        trimmed.pop_back();
    const std::size_t slash = trimmed.rfind('/');
    return slash == std::string::npos || trimmed == "/" ? trimmed : trimmed.substr(slash + 1);
}

std::string folder_of(const std::string &path)
{
    const std::size_t slash = path.rfind('/');
    return slash == std::string::npos ? std::string(".") : slash == 0 ? std::string("/") : path.substr(0, slash);
}

// ONE NAME'S INDEX. False only when Ctrl-C stopped the walk or the read under it.
bool one_entry(const std::string &path, const std::string &name, std::vector<unsigned char> &piece, Value &out)
{
    IndexHandle index = make_index();
    satelliteIndex &keys = about_to_change(index);
    struct stat about;
    if (::lstat(path.c_str(), &about) != 0) {
        // gone between the read and the stat: its name, and nothing it cannot say
        put(keys, "name", text(name));
        out = Value::of_index(index);
        return true;
    }
    struct statfs system {};
    const bool known = ::statfs(path.c_str(), &system) == 0;
    const bool nothing_stored = known && filesystems::stores_nothing(static_cast<unsigned long long int>(system.f_type));
    const NameFacts facts = facts_of(path, about);

    bool has_size = false, text_file = false;
    unsigned long long int bytes = 0, lines = 0;
    Contents in;
    if (S_ISREG(about.st_mode)) {
        struct stat parent {};
        if (!nothing_stored) {
            has_size = true;
            bytes = static_cast<unsigned long long int>(about.st_size);
        } else if (static_cast<unsigned long long int>(system.f_type) == filesystems::proc_type && name == "kcore" &&
                   ::stat(folder_of(path).c_str(), &parent) == 0 && parent.st_ino == 1) {
            has_size = true;
            bytes = filesystems::memory_bytes();
        }
        if (!count_lines(path, stop_flag(), piece, text_file, lines))
            return false;
    } else if (S_ISDIR(about.st_mode)) {
        if (!count_contents(path, stop_flag(), nullptr, in))
            return false;
        has_size = in.opened;
        bytes = in.bytes;
    }

    put(keys, "name", text(name));
    if (has_size)
        put_size(keys, bytes);
    if (text_file)
        put(keys, "line_count", a_number(lines));
    put(keys, "permissions", text(facts.permissions));
    put(keys, "type", text(facts.type));
    if (has_size)
        put(keys, "bytes", a_number(bytes));
    if (S_ISDIR(about.st_mode) && in.opened) {
        put(keys, "files", a_number(in.files));
        put(keys, "sub", a_number(in.below));
        put(keys, "complete", Value::of_bool(in.whole && in.files_whole));
        put(keys, "approximate", Value::of_bool(in.about));
    }
    put(keys, "owner", text(facts.owner));
    put(keys, "created", text(facts.created));
    put(keys, "modified", text(facts.modified));
    out = Value::of_index(index);
    return true;
}

} // namespace

bool is_info_word(Code code)
{
    return code == word::fixed_code<1, 30, 1> || code == word::fixed_code<1, 30, 2> ||
           code == word::fixed_code<1, 18, 7>;
}

Value call_info_word(Code code, const std::vector<Value> &arguments, ExpressionContext &context)
{
    const std::string written_as(word::spelling_of(code));
    const std::string spelling = written_as.substr(0, written_as.find('('));
    if (arguments.size() != 1 || !arguments[0].is_string()) {
        context.refuse(types_do_not_meet, spelling + " takes a path written as a string" +
                                              (arguments.size() == 1 ? std::string(", and was given ") +
                                                                           arguments[0].kind_name()
                                                                     : std::string()));
        return Value();
    }
    const std::string path = arguments[0].text_utf8();
    if (path.find('\0') != std::string::npos) {
        context.refuse(path_holds_a_nul, spelling + ": the path holds a NUL, which no name can");
        return Value();
    }

    // satellite.directory.free(d): the free space where d is, as the listing's line says it.
    if (code == word::code_of(1, 18, 7)) {
        struct statvfs about;
        if (::statvfs(path.c_str(), &about) != 0) {
            const int why = errno;
            context.refuse(why == ENOENT ? directory_not_found : directory_unreadable,
                           spelling + " " + shown(path) + ": " + std::strerror(why));
            return Value();
        }
        const unsigned long long int block = about.f_frsize != 0 ? about.f_frsize : about.f_bsize;
        const unsigned long long int bytes = static_cast<unsigned long long int>(about.f_bavail) * block;
        constexpr unsigned long long int megabyte = 1024ull * 1024ull;
        const unsigned long long int thousandths = (bytes % megabyte * 1000 + megabyte / 2) / megabyte;
        return Value::of_float(float_from_scaled(satellite_number(bytes / megabyte * 1000 + thousandths), 3));
    }

    std::vector<unsigned char> piece;   // count_lines' read buffer, one for the whole call
    std::vector<Value> entries;
    const auto stopped = [&]() {
        context.refuse(interrupted, spelling + " " + shown(path) + " -- stopped by Ctrl-C");
        return Value();
    };

    if (code == word::code_of(1, 30, 1)) {
        struct stat about;
        if (::lstat(path.c_str(), &about) != 0) {
            context.refuse(file_not_found, spelling + ": nothing is at " + shown(path));
            return Value();
        }
        Value entry;
        if (!one_entry(path, leaf_of(path), piece, entry))
            return stopped();
        entries.push_back(std::move(entry));
        return Value::of_list(make_list(std::move(entries)));
    }

    // THE NAMES ARE satellite.directory.list(d)'s, from its own library, so the two
    // can never list a directory differently -- its refusals are this word's too.
    const NumberRow *library = context.functions[word::code_of(1, 18, 5)];
    if (library == nullptr || library->scenarios.directory == nullptr) {
        context.refuse(not_built_yet, spelling + " reads its names through satellite.directory.list(d), "
                                                 "which has no library built for it yet");
        return Value();
    }
    const DirectoryReply reply = library->scenarios.directory(path, true, stop_flag());
    if (stops_the_program(reply.code)) {
        context.refuse(reply.code, spelling + " " + shown(path) + (reply.reason.empty() ? std::string() : ": " + reply.reason));
        return Value();
    }
    entries.reserve(reply.names.size());
    for (const std::string &name : reply.names) {
        Value entry;
        if (!one_entry(path + "/" + name, name, piece, entry))
            return stopped();
        entries.push_back(std::move(entry));
    }
    return Value::of_list(make_list(std::move(entries)));
}

} // namespace satellite004
