#pragma once

#include <string>

// The shared surface of the loader test binary (loader_test): the little
// harness every section uses, and one declaration per section.
//
// The sections live in loader_test_<topic>.cpp, one file per group of related
// checks; the harness below is DEFINED once in loader_test.cpp, which also owns
// main(). Nothing here is a test — it is the vocabulary the tests are written
// in, kept in one place so that a section file only has to say what it is
// checking.
//
// The order the sections run in is fixed by main(), and it matters: the quoted
// path sections build a directory tree that the sections after them read back.

// Counted rather than fatal, so that one broken rule does not hide the state of
// the other forty. main() reports the total and returns non-zero.
extern int failures;

void check(bool ok, const std::string &what);

// The per-run scratch directory every spaceship is written into; see the
// definition in loader_test.cpp for why it is per-run and when it survives.
std::string dir();

// Writes <name>.satl into dir() and hands back the path to it.
std::string write_ship(const std::string &name, const std::string &body);

// Source text for a capsule that displays one word, and for a satellite.main
// with the given body — the two shapes nearly every section needs.
std::string says(const std::string &name, const std::string &word);
std::string main_calling(const std::string &body);

bool contains(const std::string &haystack, const std::string &needle);

// The sections, in the order main() runs them.

// loader_test_merge.cpp
void loader_test_the_merge();
void loader_test_include_once_diamond();
void loader_test_cycle_terminates();
void loader_test_self_include();
void loader_test_included_body_runs_first();
void loader_test_include_satellite_is_ceremony();

// loader_test_quoted_paths.cpp
void loader_test_quoted_include_reaches_subdirectory();
void loader_test_quoted_include_without_extension();
void loader_test_quoted_include_climbs_upward();
void loader_test_include_once_across_spellings();
void loader_test_quoted_include_of_empty_string();
void loader_test_quoted_include_that_names_nothing();

// loader_test_errors.cpp
void loader_test_bare_include_that_names_nothing();
void loader_test_include_of_a_non_name();
void loader_test_language_owned_include();
void loader_test_parse_error_in_included_spaceship();
void loader_test_name_defined_in_two_spaceships();

// loader_test_source_map.cpp
void loader_test_source_map_one_entry_per_spaceship();
void loader_test_search_order();
