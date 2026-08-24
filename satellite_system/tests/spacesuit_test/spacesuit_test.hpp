#pragma once

#include <string>

// The shared harness for spacesuit_test, and one declaration per section of it.
//
// Every case in this binary was one 1046-line main() until the file outgrew
// being read. The cases themselves did not change: each `// --- ... ---` block
// became a void function of its own, grouped by topic into one .cpp per topic,
// and this header is the whole of what those files share. Nothing here belongs
// to a single section; anything that does stays private to the file that uses
// it, the way NAMED stays inside the constructor section.
//
// main() lives in spacesuit_test.cpp and calls the sections below IN THE ORDER
// THEY ARE DECLARED, which is the order they were written in. That order is not
// cosmetic: a later case is written assuming the earlier ones already hold, so
// reading the output top to bottom is how a failure gets narrowed down.

// The failure count, added to by the check family and read by main(). It is
// defined once, in spacesuit_test.cpp, rather than per file: a copy per
// translation unit would let a section print FAIL and still leave main()
// looking at a zero, so the binary would report PASS over a real failure.
extern int failures;

// Run the source and compare what it printed against want.
void check_output(const std::string &source, const std::string &want,
                  const std::string &what);

// Run the source, expect it NOT to succeed, and expect the message it failed
// with to contain fragment.
void check_error(const std::string &source, const std::string &fragment,
                 const std::string &what);

// unparse(parse(src)) == src, which is the parser's contract for every form in
// the grammar.
void check_roundtrip(const std::string &source, const std::string &what);

// The design's own example, with a superclass to inherit from and a typed
// parameter, used by most of the cases below. Several sections build on it and
// one round-trips it directly, so it is declared here rather than copied: two
// copies that drifted apart would make the round-trip case prove nothing about
// the source the other sections actually run.
extern const char *PAIR;

// PAIR followed by tail, which is the program most cases really run.
std::string with(const std::string &tail);

// The sections, in the order main() runs them.
void spacesuit_test_feature();
void spacesuit_test_canonical_source();
void spacesuit_test_reference_semantics();
void spacesuit_test_inheritance();
void spacesuit_test_access();
void spacesuit_test_scope();
void spacesuit_test_constructors();
void spacesuit_test_time();
void spacesuit_test_static_errors();
void spacesuit_test_nil();
