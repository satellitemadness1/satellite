#pragma once

// Private to the two listing files, src/evaluator/helpers_listing.cpp and
// src/evaluator/helpers_file_facts.cpp. Nothing else in the evaluator asks what
// a file IS, so this is deliberately not in eval_internal.hpp: a table of
// filename suffixes is not part of what the evaluator shares, and putting it
// there would offer it to twenty files that have no use for it.
//
// These five were static in helpers.cpp when list_lines and the facts it prints
// were one translation unit. They are declared here and defined once now,
// because internal linkage cannot cross a file boundary -- the same reason, and
// the same remedy, as the block at the top of eval_internal.hpp. That is the
// only semantic change the split makes; every function body moved verbatim.

#include <string>

#include <sys/stat.h>

namespace satellite {

// True when `name` ends, case-insensitively, with any suffix in the
// nullptr-terminated table.
bool named_in(const std::string &name, const char *const *table);

extern const char *const COMPRESSED[];
extern const char *const TEXTUAL[];

// The nine mode bits as rwxr-xr-x, and the creation date as a person writes it.
std::string permission_bits(mode_t mode);
std::string created_on(const std::string &name, const struct stat &info);

} // namespace satellite
