#include "satellite/charmap.hpp"

namespace satellite {

// ===========================================================================
// The satellite character map
// ===========================================================================
namespace charmap {

// index 0 is void; '?' is only a placeholder so the table indexes line up
static const char kTable[] =
    "?"                                 // 0     void / error
    "abcdefghijklmnopqrstuvwxyz"        // 1-26
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"        // 27-52
    "0123456789"                        // 53-62
    "!@#$%^&*()"                        // 63-72
    "-_=+"                              // 73-76
    "[{]}\\|"                           // 77-82
    ";:'\","                            // 83-87
    "<.>/?"                             // 88-92
    "`~"                                // 93-94
    " \n\t";                            // 95-97  space, newline, tab

const char* table() { return kTable; }

// reverse lookup, built once
static const unsigned char* reverse() {
    static unsigned char rev[256] = {0};
    static bool built = false;
    if (!built) {
        for (uint32_t code = 1; code < TEXT_COUNT; ++code)
            rev[(unsigned char)kTable[code]] = (unsigned char)code;
        built = true;
    }
    return rev;
}

char32_t from_ascii(char c) { return reverse()[(unsigned char)c]; }

char to_ascii(char32_t code) {
    return code > 0 && code < TEXT_COUNT ? kTable[code] : '\0';
}

// ---- satellite math tokens ------------------------------------------------
static const char* kOps[] = {
    "+", "-", "*", "/", "%", "^", "=", "==", "!=", "<", ">", "<=", ">=",
};

bool is_operator(char32_t code) { return code >= OP_PLUS && code < COUNT; }

const char* op_text(char32_t code) {
    return is_operator(code) ? kOps[code - OP_PLUS] : "";
}

}  // namespace charmap

uint32_t charmap::collation_key(char32_t code) {
    if (code == VOID) return 0;
    if (code < TEXT_COUNT) return (unsigned char)kTable[code];   // ASCII order
    if (is_operator(code)) return 256 + (code - OP_PLUS);
    return 512;
}

}  // namespace satellite
