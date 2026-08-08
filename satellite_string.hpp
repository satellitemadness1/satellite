#pragma once

#include <cstdint>
#include <string>

namespace satellite {

// satellite_string: a string of 32-bit chars with satellite's own code table.
//
//   0        void (decodes to nothing)
//   1..26    a..z                     (a is 1)
//   27..52   A..Z
//   53..62   0..9
//   63..94   !@#$%^&*()-_=+[{]}\|;:'",<.>/?`~   (in that order)
//   95       linux_home      -> the user's home directory, live
//   96       linux_username  -> the user's name, live
//   97       threads         -> hardware thread count, live
//   98       mem_total_mb    -> total machine memory in MB, live
//   99       mem_used_mb     -> used machine memory in MB, live
//   100      cwd             -> current working directory, live
//
// Codes not yet assigned by the language (space, ...) round-trip through
// a raw area at RAW_BASE + byte until the table grows.
using SatChar = char32_t;
using SatString = std::basic_string<SatChar>;

enum : SatChar {
    SAT_VOID = 0,
    SAT_A = 1,
    SAT_UPPER_A = 27,
    SAT_DIGIT_0 = 53,
    SAT_PUNCT_BASE = 63,
    SAT_LINUX_HOME = 95,
    SAT_LINUX_USERNAME = 96,
    SAT_THREADS = 97,
    SAT_MEM_TOTAL_MB = 98,
    SAT_MEM_USED_MB = 99,
    SAT_CWD = 100,
    SAT_RAW_BASE = 0x40000000,
};

// Individual punctuation codes the lexer needs by name, as offsets into the
// 63..94 block. '_' is the interesting one: it lives in the punctuation block
// of the code table, but the lexer treats it as an identifier character, so
// my_time is one word and not three. The code table says what a character IS;
// the lexer says what role it plays. (satellite_string.cpp static_asserts
// every one of these against PUNCT, so they cannot drift.)
enum : SatChar {
    SAT_BANG      = SAT_PUNCT_BASE + 0,    // !
    SAT_UNDERSCORE= SAT_PUNCT_BASE + 11,   // _
    SAT_EQUAL     = SAT_PUNCT_BASE + 12,   // =
    SAT_BACKSLASH = SAT_PUNCT_BASE + 18,   // backslash
    SAT_QUOTE     = SAT_PUNCT_BASE + 23,   // "
    SAT_LESS      = SAT_PUNCT_BASE + 25,   // <
    SAT_DOT       = SAT_PUNCT_BASE + 26,   // .
    SAT_GREATER   = SAT_PUNCT_BASE + 27,   // >
    SAT_SLASH     = SAT_PUNCT_BASE + 28,   // /
};

// Text -> satellite codes. Backslash names insert the system chars:
// \home \user \threads \memtotal \memused \cwd \void
// and the characters the code table has no room for: \n \t \r \\ \"
SatString encode(const std::string &text);

// Text -> satellite codes with NO escape processing: every input byte becomes
// exactly one SatChar, so a backslash stays a backslash.
//
// This is what source code must be lexed with. encode() expands escapes
// everywhere, which would rewrite a program's text before the lexer ever saw
// it — "C:\home" in a comment would silently become the user's home directory.
// The lexer applies encode() only to the body of a string literal.
//
// The one-byte-one-SatChar property also means an index into the result is a
// byte offset into `text`, which is what error carets need. decode() cannot be
// used for that: it is not injective and not even stable, since \cwd expands to
// a different width after a chdir.
SatString encode_raw(const std::string &text);

// Satellite codes -> displayable text; system chars expand to live values.
std::string decode(const SatString &s);

} // namespace satellite
