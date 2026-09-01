#pragma once

// satellite_string: a string of 16-bit chars with satellite's own code table.
//
// THIS IS THE LANGUAGE'S ALPHABET (DESIGN §5). The lexer walks SatString, so
// the ranges below are not a detail of one module -- they are what "letter",
// "digit" and "punctuation" MEAN in satellite, and lexical_analyzer/ is written
// against them rather than against C's ctype.
//
// PORTED AT M3 AND SCHEDULED AT M9, which is a pull-forward and is recorded
// rather than left to be discovered. PLAN §6.1 surveys this module and PLAN M9
// owns it -- "`Str` is the port of `satellite_string`" -- but DESIGN §5's first
// sentence makes it M3's dependency, and a milestone cannot lex without an
// alphabet. What came forward is the CHARACTER half only; see the next
// paragraph for the half that did not, and PLAN M3 for the same note from the
// milestone's side.
//
// CODES 95-100 ARE LIVE VALUES AND M9 ANSWERED THEM, FROM ONE MODULE UP. They
// resolve at decode time to the user's home directory, their name, the hardware
// thread count, total and used memory, and the working directory -- readers
// that live in system_facts/. This port stubbed all six, and what unstubbed
// them is the second decode() below: the caller hands in a `Live` table, and
// satellite_value/render.cpp is the caller that fills it from the machine.
//
// THE READERS ARE NOT CALLED HERE, AND THE REASON IS THE LEXER. It calls
// decode() on the text of every token, a string literal's body included, and
// `Token::text` is what `--unparse` prints back -- so a live decode in this
// module would write this machine's thread count into the source of any program
// containing "\threads". The one-argument decode() below is therefore the
// SAME function it always was, and nothing about the lexer changed at M9.
//
// The stub was safe here for a stronger reason than that while it lasted, and
// it is still why nothing in the lexer can reach a live code: encode_raw() maps
// every source byte to a letter, a digit, a punctuation code or the raw area,
// so no code in 95..100 can occur in a program's text at all. They are
// reachable only through encode()'s backslash names, inside a string literal
// body, which is a VALUE and did not exist until M9 built one.
//
// 16 BITS, NOT 32. The width is a property of the DATA, not of the code table:
// the table needs 101 codes and the raw area needs 256, so 8 bits (357 > 256)
// cannot hold both and 16 bits holds them with 65,000 to spare. Halving the
// element halves what a corpus costs in memory and what a scan costs in
// bandwidth, and neither length() counting characters nor encode_raw's
// one-byte-one-SatChar property depends on the width.
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
// Codes not yet assigned by the language (space, ...) round-trip through a raw
// area at SAT_RAW_BASE + byte until the table grows.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace satellite {

using SatChar = char16_t;
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
    // Top bit set, so a raw code is recognisable on sight in a dump -- the same
    // property 0x40000000 had in the 32-bit space, moved to fit 16 bits. The
    // area is [0x8000, 0x8100), and 0x101..0x7FFF stays free for the table to
    // grow into, which is what the comment above anticipates.
    SAT_RAW_BASE = 0x8000,
};

// Individual punctuation codes the lexer needs by name, as offsets into the
// 63..94 block. '_' is the interesting one: it lives in the punctuation block
// of the code table, but the lexer treats it as an identifier character, so
// my_time is one word and not three (DESIGN §5.1). The code table says what a
// character IS; the lexer says what role it plays.
//
// TEN NAMES AND NOT THIRTY-TWO, deliberately. PUNCT in the .cpp is the table;
// naming every entry here would be a second copy of it, and §1's "one place per
// fact" is the rule that forbids that. What earns a name is a code some other
// module has to SAY -- these ten are exactly the ones lexical_analyzer/ spells
// out, and satellite_string.cpp static_asserts every one against PUNCT, so they
// cannot drift.
enum : SatChar {
    SAT_BANG      = SAT_PUNCT_BASE + 0,    // !
    SAT_UNDERSCORE= SAT_PUNCT_BASE + 11,   // _
    SAT_EQUAL     = SAT_PUNCT_BASE + 12,   // =
    SAT_BACKSLASH = SAT_PUNCT_BASE + 18,   // backslash
    SAT_APOSTROPHE= SAT_PUNCT_BASE + 22,   // '
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
// THIS IS WHAT SOURCE CODE MUST BE LEXED WITH (DESIGN §5.3). encode() expands
// escapes everywhere, which would rewrite a program's text before the lexer
// ever saw it -- "C:\home" in a comment would silently become the user's home
// directory. The lexer applies encode() only to the BODY of a string literal.
//
// The one-byte-one-SatChar property also means an index into the result is a
// byte offset into `text`, which is what error carets need. decode() cannot be
// used for that: it is not injective and not even stable, since \cwd expands to
// a different width after a chdir.
SatString encode_raw(const std::string &text);

// THE SIX LIVE VALUES, IN CODE ORDER 95..100 -- home, username, threads,
// mem_total_mb, mem_used_mb, cwd. An entry left empty decodes to its
// placeholder, which is what a caller with no machine to ask gets.
//
// A TABLE THE CALLER FILLS RATHER THAN SIX CALLS THIS MODULE MAKES, and PLAN
// §6.1 predicted the other shape: "finishing it is replacing six lines with six
// calls". The calls are real and they are in satellite_value/render.cpp, one
// module up. They cannot be here, for a reason that only became visible once
// there was something to move: THE LEXER CALLS decode(), on the text of every
// token including a string literal's body, and `Token::text` is what
// `--unparse` prints back. A live decode inside this module would put this
// machine's thread count into the source of any program that wrote
// `"\threads"`, silently, and round-tripping would stop being a fixpoint.
//
// So the split is the one lexer.hpp already draws between a token's two halves:
// `text` is what the file SAYS and gets the placeholder, `str` is what the
// program MEANS and gets the machine. DESIGN §5's table calls these codes live,
// and live means read when the value is used rather than when it is lexed.
using Live = std::array<std::string, 6>;

inline constexpr size_t kLiveCount = 6;

static_assert(SAT_CWD - SAT_LINUX_HOME + 1 == kLiveCount,
              "satellite_string.hpp: Live has one entry per live code, indexed "
              "by code - SAT_LINUX_HOME. Adding a live code to the table above "
              "without widening this array walks off the end of it");

// Satellite codes -> displayable text.
//
// Codes 95-100 emit their placeholder; see the file-top comment for why
// nothing in the lexer can reach one.
std::string decode(const SatString &s);

// The same walk, with the six live codes answered from `live`. One decoder and
// not two -- an entry `live` leaves empty falls back to the placeholder, so the
// two entry points cannot disagree about anything except the six.
std::string decode(const SatString &s, const Live &live);

} // namespace satellite
