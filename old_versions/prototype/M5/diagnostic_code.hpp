#pragma once

// Diagnostic Error Codes -- Milestone 5 Error Reporter.
//
// Structured, categorized error codes for all compiler & interpreter stages.
// Every code is a unique identifier (e.g. E0101) with a human-readable name,
// category, and summary.
//
// DESIGN §9: "What is missing is everything that makes an error addressable:
// a code to look up, a stack to place it in, a suggestion to act on."

#include <cstdint>
#include <string_view>

namespace satellite {

enum class ErrorCategory : uint8_t {
    Lexical,    // E0001 - E0099
    Syntax,     // E0101 - E0199
    PathTrie,   // E0201 - E0299
    Resolve,    // E0301 - E0399
    Runtime,    // E0401 - E0499
    System,     // E0501 - E0599
};

enum class ErrorCode : uint16_t {
    None = 0,

    // Lexical Errors (E00xx)
    E0001_InvalidCharacter = 1,
    E0002_UnterminatedString = 2,
    E0003_InvalidEscapeSequence = 3,
    E0004_InvalidNumberLiteral = 4,
    E0005_InvalidBitsLiteral = 5,
    E0006_InvalidDurationLiteral = 6,
    E0007_ReservedWordAsIdentifier = 7,

    // Syntax & Parse Errors (E01xx)
    E0101_UnexpectedToken = 101,
    E0102_ExpectedToken = 102,
    E0103_ExpectedExpression = 103,
    E0104_ExpectedStatement = 104,
    E0105_ExpectedDeclaration = 105,
    E0106_ExpectedType = 106,
    E0107_UnclosedDelimiter = 107,
    E0108_InvalidAssignmentTarget = 108,
    E0109_InvalidCallSyntax = 109,
    E0110_InvalidReturnSyntax = 110,
    E0111_EmptyBlockOrClause = 111,
    E0112_UnexpectedEof = 112,

    // Path & Words Trie Errors (E02xx)
    E0201_PathNotRooted = 201,
    E0202_NoSuchWord = 202,
    E0203_NoSuchShape = 203,
    E0204_TrailingPathSegment = 204,
    E0205_DuplicateDeclaration = 205,
    E0206_ReservedNameConflict = 206,

    // Resolve & Semantic Errors (E03xx)
    E0301_UndefinedVariable = 301,
    E0302_UndefinedCapsule = 302,
    E0303_UndefinedSpacesuit = 303,
    E0304_UndefinedField = 304,
    E0305_UndefinedMethod = 305,
    E0306_AccessViolation = 306,
    E0307_ArityMismatch = 307,
    E0308_TypeMismatch = 308,
    E0309_ReturnOutsideCapsule = 309,

    // Runtime, Limits & System Errors (E04xx, E05xx)
    E0401_RecursionLimitExceeded = 401,
    E0402_MemoryLimitExceeded = 402,
    E0403_DivisionByZero = 403,
    E0404_IndexOutOfBounds = 404,
    E0405_InvalidArgument = 405,
    E0406_FileError = 406,
    E0501_ConfigSyntaxError = 501,
};

ErrorCategory code_category(ErrorCode code);
std::string_view code_to_string(ErrorCode code);
std::string_view code_name(ErrorCode code);
std::string_view code_summary(ErrorCode code);

} // namespace satellite
