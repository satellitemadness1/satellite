#pragma once

#include <string>
#include <vector>

#include "console_input/editor.hpp"
#include "console_input/keys.hpp"

// The harness this test's four case files share, and the sections themselves.
//
// Split for the 325-line rule, the same way every other test in this tree that
// outgrew one file was. What crosses the boundary is here and nothing else:
// three helpers that let a case be written as a string of bytes, and one
// prototype per section.

extern int failures;

void check(bool ok, const char *what);
void check_eq(const std::string &got, const std::string &want,
              const char *what);

// Feeds a whole byte string through one decoder and collects every key it
// completed. This is how a test types: "\033[A" is the up arrow, and it
// arrives here exactly as a terminal sends it.
std::vector<satellite::KeyEvent> decode(const std::string &bytes);

// The one key a byte string decodes to, or Key::None if it was not exactly
// one -- which fails the check that asked, and says so by not matching.
satellite::Key one_key(const std::string &bytes);

// Drives an Editor with a byte string, exactly as the read loop would, and
// returns what the line looks like afterwards with the cursor marked by '|'.
// One string is far easier to read in a failure than a buffer and an offset
// printed separately.
std::string type(satellite::Editor &editor, const std::string &bytes);

void test_keys();
void test_measuring();
void test_editing();
void test_history_browsing();
void test_history_store();
