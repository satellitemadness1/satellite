#include "value.hpp"

#include <cstdio>
#include <ctime>

#include <unistd.h>

namespace satellite {

// §8.1's migration moved every question about how a number RENDERS into
// Number::to_string (bignum.cpp), because the answer is a property of the type
// rather than of the printer. The history is worth keeping in one sentence: the
// original snprintf("%g") printed six significant digits, so 123456789 came out
// as 1.23457e+08 and 2000000 + 1 as 2e+06 — wrong values, through the only
// output path the language has, since satellite.console.display, the REPL echo,
// .to_string() and every error message quoting a number all arrive here.

// How an instance renders: the spacesuit's name in angle brackets.
//
// Angle brackets because the result is deliberately not something a program
// could have written — there is no object literal, so anything that looked like
// one would mislead. What it is NOT is a dump of the fields: a field is
// reachable only from inside its spacesuit (§12 keeps bare field access out of
// the language), so printing them would hand out through the printer exactly
// what `satellite.protected` exists to withhold. A cyclic object graph would
// also not terminate, and objects are the first values that can build one.
//
// A spacesuit that wants a readable form writes `.to_string()`, which is an
// ordinary public method and takes precedence at the call site.
std::string ValuePrinter::operator()(const ObjectPtr &object) const
{
    return "<" + suit_name(object ? object->suit : nullptr) + ">";
}

// How a satellite.variable.time renders: ISO-8601, UTC, to the nanosecond.
//
// All nine fractional digits, always, rather than trimming trailing zeros: the
// value carries nanosecond resolution and a printed instant that silently
// changed width would be unsortable as text — which is most of what an ISO-8601
// timestamp is for. §8.2 fixes the type as an absolute instant in UTC, so the
// 'Z' is a fact about the type and never a guess about a timezone.
std::string ValuePrinter::operator()(Time time) const
{
    // Floor division, not truncation: C++ integer division rounds toward zero,
    // so a pre-epoch instant would otherwise land one second late with a
    // negative fraction. Instants before 1970 are rare and being quietly wrong
    // about them is not better than being right.
    long long seconds = time.ns / 1000000000;
    long long fraction = time.ns % 1000000000;
    if (fraction < 0) {
        fraction += 1000000000;
        seconds -= 1;
    }

    const std::time_t as_time = static_cast<std::time_t>(seconds);
    std::tm broken{};
    if (!gmtime_r(&as_time, &broken)) {
        // Outside what the C library can break down. The raw count is still the
        // value, so say it rather than nothing.
        char raw[40];
        snprintf(raw, sizeof raw, "%lldns", time.ns);
        return raw;
    }

    char buffer[64];
    snprintf(buffer, sizeof buffer, "%04d-%02d-%02dT%02d:%02d:%02d.%09lldZ",
             broken.tm_year + 1900, broken.tm_mon + 1, broken.tm_mday,
             broken.tm_hour, broken.tm_min, broken.tm_sec, fraction);
    return buffer;
}

// The backstop, and only the backstop. A program that cares whether its bytes
// reached the disk calls close() and reads the status; this is what catches the
// one that forgot. Nothing is reported from here because there is nobody left
// to report it to.
FileHandle::~FileHandle()
{
    const int open_fd = fd.exchange(-1);
    if (open_fd >= 0)
        ::close(open_fd);
}

// How a file renders. The path and whether it is still open, because those are
// the two things a person debugging one wants, and neither is the contents.
std::string ValuePrinter::operator()(const FilePtr &file) const
{
    if (!file)
        return "<file>";
    const bool open = file->fd.load() >= 0;
    return "<file " + file->path + (open ? "" : ", closed") + ">";
}

} // namespace satellite
