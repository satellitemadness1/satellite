#pragma once

#include <string>

// satellite.random tests — DESIGN §18.
//
// Split in two, because the two halves cost different amounts of time. The
// SAMPLER is driven by a stub generator that does not spin at all, so its
// distribution can be measured over hundreds of thousands of draws in
// milliseconds; that is where the correctness lives, and it is the half a
// uniformity claim has to be checked in. The TIERS are then exercised end to
// end a handful of times on `fast`, whose throwaway window is 50-100 ms, so
// this binary stays under a second.
//
// Nothing here asserts a particular VALUE, because there is no particular value
// to assert. What is asserted is what §18 actually promises: every draw lands
// inside its interval, both ends of the interval are reachable, the
// distribution is flat, and a call takes at least as long as its tier says.
//
// Split again on disk, for a different reason than the one above: the file had
// grown past what one screenful of a reader's attention covers, and it already
// carried its own seams as section dividers. Those dividers are now the file
// boundaries — random_test_sampler.cpp, random_test_tiers.cpp and
// random_test_surface.cpp — with this header holding what crosses between
// them. random_test.cpp keeps the check() family and main(). The Makefile
// picks up every .cpp in this folder by wildcard, so nothing outside had to
// learn about the split.

// The failure counter. It is ONE counter for the whole binary rather than one
// per file, because main() is what turns it into an exit code: a section whose
// failures landed in a per-file copy would report them on stdout and then exit
// 0, which is the one way a test can lie.
extern int failures;

// The check family. check() is the plain assertion; check_output() and
// check_error() run a snippet of satellite source through the interpreter and
// compare what came back. All three are defined in random_test.cpp so that
// every section counts its failures in the same place.
void check(bool ok, const std::string &what);
void check_output(const std::string &source, const std::string &want,
                  const std::string &what);
void check_error(const std::string &source, const std::string &fragment,
                 const std::string &what);

// One declaration per section, listed in the order main() calls them. The order
// is not arbitrary: the sampler is checked before the tiers so that a tier
// failure is read as a tier failure rather than as a sampler that was already
// broken underneath it.
void test_bounds();
void test_uniform();
void test_leading_zeros();
void test_refusals();
void test_tiers();
void test_spin();
void test_surface();
