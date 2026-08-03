// The satellite character map.
#pragma once

#include <cstdint>

namespace satellite {

// ---------------------------------------------------------------------------
// The satellite character map.
//
// A satellite character is NOT a Unicode codepoint -- it is an index into this
// table.  Code 0 is void/error; everything else is assigned in order:
//
//   0        void / error
//   1-26     a-z          27-52    A-Z          53-62    0-9
//   63-72    !@#$%^&*()   73-76    -_=+         77-82    [{]}\|
//   83-87    ;:'",        88-92    <.>/?        93-94    `~
//   95-97    space, newline, tab   <-- added; nothing works without a space
//
// Stored in 32 bits so the map can grow without a migration.  Text outside the
// map converts to 0 (void).
// ---------------------------------------------------------------------------
namespace charmap {
constexpr char32_t VOID  = 0;
constexpr char32_t SPACE = 95;
constexpr char32_t TEXT_COUNT = 98;    // codes 0-97 are ordinary text

// ---- satellite math tokens ------------------------------------------------
// Deliberately NOT the same codes as the ASCII characters that look like them.
// A hyphen inside a string is code 73; SUBTRACTION is code 99.  Nothing can
// confuse the two, which is what lets tokenized code be stored as an ordinary
// satellite string and read back exactly.
constexpr char32_t OP_PLUS   = 98;
constexpr char32_t OP_MINUS  = 99;
constexpr char32_t OP_TIMES  = 100;
constexpr char32_t OP_DIVIDE = 101;
constexpr char32_t OP_MODULO = 102;
constexpr char32_t OP_POWER  = 103;
constexpr char32_t OP_ASSIGN = 104;
constexpr char32_t OP_EQ     = 105;
constexpr char32_t OP_NE     = 106;
constexpr char32_t OP_LT     = 107;
constexpr char32_t OP_GT     = 108;
constexpr char32_t OP_LE     = 109;
constexpr char32_t OP_GE     = 110;
constexpr char32_t COUNT     = 111;

char32_t    from_ascii(char c);        // ASCII -> TEXT code, never an operator
char        to_ascii(char32_t code);   // text code -> ASCII, 0 if not text
const char* table();                   // index by code; [0] is a placeholder

bool        is_operator(char32_t code);
const char* op_text(char32_t code);    // "+", "-", "<=" ... for display

// Classification is a contiguous range check for every category -- the table
// was laid out in blocks, so none of these need a lookup.
constexpr bool is_lower (char32_t c) { return c >= 1  && c <= 26; }
constexpr bool is_upper (char32_t c) { return c >= 27 && c <= 52; }
constexpr bool is_letter(char32_t c) { return c >= 1  && c <= 52; }
constexpr bool is_digit (char32_t c) { return c >= 53 && c <= 62; }
constexpr bool is_symbol(char32_t c) { return c >= 63 && c <= 94; }
constexpr bool is_space (char32_t c) { return c >= 95 && c <= 97; }

// 'a' is 1 and 'A' is 27, so case conversion is plain arithmetic -- no mask,
// no lookup table.  ASCII needs one because its layout has gaps; this doesn't.
constexpr char32_t to_upper(char32_t c) { return is_lower(c) ? c + 26 : c; }
constexpr char32_t to_lower(char32_t c) { return is_upper(c) ? c - 26 : c; }

// '0' is 53, so the numeric value of a digit is a subtraction, not a parse.
constexpr int digit_value(char32_t c) { return is_digit(c) ? (int)(c - 53) : -1; }

// Sort key.  Raw code order would put lowercase before uppercase before digits
// ("zebra" < "Apple", "apple" < "9"), which is not what anyone expects from a
// sorted list of names.  The key maps back to ASCII order so comparison behaves
// conventionally while storage keeps the satellite layout.
uint32_t collation_key(char32_t code);
}  // namespace charmap

}  // namespace satellite
