#pragma once
// satl --license -- every licence in the binary, three ways to reach it.
//
//     satl --license            the MIT licence, then a numbered menu; pick one
//     satl --license 7          by number, as the menu lists them
//     satl --license harfbuzz   by name
//     satl --license all        all of them, one after another
//
// THREE WAYS ON PURPOSE, and the author asked for it knowing it is more than one
// way: "that gives them... like 3 ways to access the licenses for the app". A
// person who has been handed a binary and wants to know what is inside it should
// not have to guess the spelling of the flag's argument. The menu teaches the
// numbers and the names, so the other two ways need no documentation at all.
//
// THE TEXT IS COMPILED IN. license_data.cpp is generated from licenses/<project>/
// license.txt -- the same files the website and THIRD-PARTY-NOTICES.txt are built
// from. A notice that lives beside the binary is gone the moment somebody copies
// the binary, and several of these licences require the notice to travel WITH the
// product: gtk/roaring's BSD-3-Clause clause 2 says "binary" outright, libpng's
// clause 3 forbids removing the notice, pcre2's condition 2 wants every copyright
// line. 284 KB in an 89 MB binary is 0.3%.

#include <string>
#include <vector>

namespace satellite004 {

struct licence_row {
    std::string name;   // the folder name under licenses/, which is also the argument
    std::string text;   // byte-for-byte what upstream shipped
};

// From the generated license_data.cpp. satellite is always first: it is the licence
// FOR the language, and the rest are what the language carries. The order is the same
// as THIRD-PARTY-NOTICES.txt's, so a number means the same thing in both.
const std::vector<licence_row> &licence_rows();

// `satl --license [which]`. `which` empty is the MIT licence and the menu; a number,
// a name, or "all" answers directly. Says why on stderr and answers a machine code
// when the argument names nothing.
signed long long int run_licence(const std::string &which);

} // namespace satellite004
