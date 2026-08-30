// Diagnostic Error Codes implementation -- Milestone 5.

#include "diagnostic_code.hpp"

namespace satellite {

ErrorCategory code_category(ErrorCode code)
{
    const uint16_t val = static_cast<uint16_t>(code);
    if (val < 100) return ErrorCategory::Lexical;
    if (val < 200) return ErrorCategory::Syntax;
    if (val < 300) return ErrorCategory::PathTrie;
    if (val < 400) return ErrorCategory::Resolve;
    if (val < 500) return ErrorCategory::Runtime;
    return ErrorCategory::System;
}

std::string_view code_to_string(ErrorCode code)
{
    switch (code) {
    case ErrorCode::None: return "E0000";

    case ErrorCode::E0001_InvalidCharacter: return "E0001";
    case ErrorCode::E0002_UnterminatedString: return "E0002";
    case ErrorCode::E0003_InvalidEscapeSequence: return "E0003";
    case ErrorCode::E0004_InvalidNumberLiteral: return "E0004";
    case ErrorCode::E0005_InvalidBitsLiteral: return "E0005";
    case ErrorCode::E0006_InvalidDurationLiteral: return "E0006";
    case ErrorCode::E0007_ReservedWordAsIdentifier: return "E0007";

    case ErrorCode::E0101_UnexpectedToken: return "E0101";
    case ErrorCode::E0102_ExpectedToken: return "E0102";
    case ErrorCode::E0103_ExpectedExpression: return "E0103";
    case ErrorCode::E0104_ExpectedStatement: return "E0104";
    case ErrorCode::E0105_ExpectedDeclaration: return "E0105";
    case ErrorCode::E0106_ExpectedType: return "E0106";
    case ErrorCode::E0107_UnclosedDelimiter: return "E0107";
    case ErrorCode::E0108_InvalidAssignmentTarget: return "E0108";
    case ErrorCode::E0109_InvalidCallSyntax: return "E0109";
    case ErrorCode::E0110_InvalidReturnSyntax: return "E0110";
    case ErrorCode::E0111_EmptyBlockOrClause: return "E0111";
    case ErrorCode::E0112_UnexpectedEof: return "E0112";

    case ErrorCode::E0201_PathNotRooted: return "E0201";
    case ErrorCode::E0202_NoSuchWord: return "E0202";
    case ErrorCode::E0203_NoSuchShape: return "E0203";
    case ErrorCode::E0204_TrailingPathSegment: return "E0204";
    case ErrorCode::E0205_DuplicateDeclaration: return "E0205";
    case ErrorCode::E0206_ReservedNameConflict: return "E0206";

    case ErrorCode::E0301_UndefinedVariable: return "E0301";
    case ErrorCode::E0302_UndefinedCapsule: return "E0302";
    case ErrorCode::E0303_UndefinedSpacesuit: return "E0303";
    case ErrorCode::E0304_UndefinedField: return "E0304";
    case ErrorCode::E0305_UndefinedMethod: return "E0305";
    case ErrorCode::E0306_AccessViolation: return "E0306";
    case ErrorCode::E0307_ArityMismatch: return "E0307";
    case ErrorCode::E0308_TypeMismatch: return "E0308";
    case ErrorCode::E0309_ReturnOutsideCapsule: return "E0309";

    case ErrorCode::E0401_RecursionLimitExceeded: return "E0401";
    case ErrorCode::E0402_MemoryLimitExceeded: return "E0402";
    case ErrorCode::E0403_DivisionByZero: return "E0403";
    case ErrorCode::E0404_IndexOutOfBounds: return "E0404";
    case ErrorCode::E0405_InvalidArgument: return "E0405";
    case ErrorCode::E0406_FileError: return "E0406";
    case ErrorCode::E0501_ConfigSyntaxError: return "E0501";
    }
    return "E9999";
}

std::string_view code_name(ErrorCode code)
{
    switch (code) {
    case ErrorCode::None: return "None";

    case ErrorCode::E0001_InvalidCharacter: return "InvalidCharacter";
    case ErrorCode::E0002_UnterminatedString: return "UnterminatedString";
    case ErrorCode::E0003_InvalidEscapeSequence: return "InvalidEscapeSequence";
    case ErrorCode::E0004_InvalidNumberLiteral: return "InvalidNumberLiteral";
    case ErrorCode::E0005_InvalidBitsLiteral: return "InvalidBitsLiteral";
    case ErrorCode::E0006_InvalidDurationLiteral: return "InvalidDurationLiteral";
    case ErrorCode::E0007_ReservedWordAsIdentifier: return "ReservedWordAsIdentifier";

    case ErrorCode::E0101_UnexpectedToken: return "UnexpectedToken";
    case ErrorCode::E0102_ExpectedToken: return "ExpectedToken";
    case ErrorCode::E0103_ExpectedExpression: return "ExpectedExpression";
    case ErrorCode::E0104_ExpectedStatement: return "ExpectedStatement";
    case ErrorCode::E0105_ExpectedDeclaration: return "ExpectedDeclaration";
    case ErrorCode::E0106_ExpectedType: return "ExpectedType";
    case ErrorCode::E0107_UnclosedDelimiter: return "UnclosedDelimiter";
    case ErrorCode::E0108_InvalidAssignmentTarget: return "InvalidAssignmentTarget";
    case ErrorCode::E0109_InvalidCallSyntax: return "InvalidCallSyntax";
    case ErrorCode::E0110_InvalidReturnSyntax: return "InvalidReturnSyntax";
    case ErrorCode::E0111_EmptyBlockOrClause: return "EmptyBlockOrClause";
    case ErrorCode::E0112_UnexpectedEof: return "UnexpectedEof";

    case ErrorCode::E0201_PathNotRooted: return "PathNotRooted";
    case ErrorCode::E0202_NoSuchWord: return "NoSuchWord";
    case ErrorCode::E0203_NoSuchShape: return "NoSuchShape";
    case ErrorCode::E0204_TrailingPathSegment: return "TrailingPathSegment";
    case ErrorCode::E0205_DuplicateDeclaration: return "DuplicateDeclaration";
    case ErrorCode::E0206_ReservedNameConflict: return "ReservedNameConflict";

    case ErrorCode::E0301_UndefinedVariable: return "UndefinedVariable";
    case ErrorCode::E0302_UndefinedCapsule: return "UndefinedCapsule";
    case ErrorCode::E0303_UndefinedSpacesuit: return "UndefinedSpacesuit";
    case ErrorCode::E0304_UndefinedField: return "UndefinedField";
    case ErrorCode::E0305_UndefinedMethod: return "UndefinedMethod";
    case ErrorCode::E0306_AccessViolation: return "AccessViolation";
    case ErrorCode::E0307_ArityMismatch: return "ArityMismatch";
    case ErrorCode::E0308_TypeMismatch: return "TypeMismatch";
    case ErrorCode::E0309_ReturnOutsideCapsule: return "ReturnOutsideCapsule";

    case ErrorCode::E0401_RecursionLimitExceeded: return "RecursionLimitExceeded";
    case ErrorCode::E0402_MemoryLimitExceeded: return "MemoryLimitExceeded";
    case ErrorCode::E0403_DivisionByZero: return "DivisionByZero";
    case ErrorCode::E0404_IndexOutOfBounds: return "IndexOutOfBounds";
    case ErrorCode::E0405_InvalidArgument: return "InvalidArgument";
    case ErrorCode::E0406_FileError: return "FileError";
    case ErrorCode::E0501_ConfigSyntaxError: return "ConfigSyntaxError";
    }
    return "Unknown";
}

std::string_view code_summary(ErrorCode code)
{
    switch (code) {
    case ErrorCode::None: return "No error";

    case ErrorCode::E0001_InvalidCharacter: return "Invalid or unrecognized character in source";
    case ErrorCode::E0002_UnterminatedString: return "String literal was not terminated before newline or EOF";
    case ErrorCode::E0003_InvalidEscapeSequence: return "Unknown or invalid escape sequence in string literal";
    case ErrorCode::E0004_InvalidNumberLiteral: return "Malformed number literal format";
    case ErrorCode::E0005_InvalidBitsLiteral: return "Malformed binary or hexadecimal literal";
    case ErrorCode::E0006_InvalidDurationLiteral: return "Malformed duration literal suffix or value";
    case ErrorCode::E0007_ReservedWordAsIdentifier: return "Language-owned keyword or literal cannot name a user variable";

    case ErrorCode::E0101_UnexpectedToken: return "Unexpected token encountered during parsing";
    case ErrorCode::E0102_ExpectedToken: return "Expected a specific token";
    case ErrorCode::E0103_ExpectedExpression: return "Expected an expression";
    case ErrorCode::E0104_ExpectedStatement: return "Expected a statement";
    case ErrorCode::E0105_ExpectedDeclaration: return "Expected a declaration";
    case ErrorCode::E0106_ExpectedType: return "Expected a type specification";
    case ErrorCode::E0107_UnclosedDelimiter: return "Unclosed parenthesis, bracket, or brace";
    case ErrorCode::E0108_InvalidAssignmentTarget: return "Left-hand side of assignment is not assignable";
    case ErrorCode::E0109_InvalidCallSyntax: return "Malformed function or method call syntax";
    case ErrorCode::E0110_InvalidReturnSyntax: return "Malformed return statement";
    case ErrorCode::E0111_EmptyBlockOrClause: return "Block or clause requires at least one statement";
    case ErrorCode::E0112_UnexpectedEof: return "Unexpected end of file while parsing";

    case ErrorCode::E0201_PathNotRooted: return "Language-owned path must be rooted at 'satellite'";
    case ErrorCode::E0202_NoSuchWord: return "Path segment does not exist under parent node in the words trie";
    case ErrorCode::E0203_NoSuchShape: return "Call shape or argument signature does not exist for this word";
    case ErrorCode::E0204_TrailingPathSegment: return "Unexpected trailing characters after path match";
    case ErrorCode::E0205_DuplicateDeclaration: return "Identifier already declared in the current scope";
    case ErrorCode::E0206_ReservedNameConflict: return "Name conflicts with a frozen language-owned word";

    case ErrorCode::E0301_UndefinedVariable: return "Variable is not defined in current scope";
    case ErrorCode::E0302_UndefinedCapsule: return "Capsule is not defined";
    case ErrorCode::E0303_UndefinedSpacesuit: return "Spacesuit is not defined";
    case ErrorCode::E0304_UndefinedField: return "Field does not exist on spacesuit";
    case ErrorCode::E0305_UndefinedMethod: return "Method does not exist on spacesuit";
    case ErrorCode::E0306_AccessViolation: return "Member is protected and cannot be accessed from outside";
    case ErrorCode::E0307_ArityMismatch: return "Wrong number of arguments passed to function or method";
    case ErrorCode::E0308_TypeMismatch: return "Incompatible types in assignment or operation";
    case ErrorCode::E0309_ReturnOutsideCapsule: return "Return statement outside of capsule body";

    case ErrorCode::E0401_RecursionLimitExceeded: return "Capsule call recursion depth limit exceeded";
    case ErrorCode::E0402_MemoryLimitExceeded: return "Process memory ceiling (MEMORY_MAX) exceeded";
    case ErrorCode::E0403_DivisionByZero: return "Exact division by zero";
    case ErrorCode::E0404_IndexOutOfBounds: return "Index or slice out of container bounds";
    case ErrorCode::E0405_InvalidArgument: return "Invalid argument value passed to module function";
    case ErrorCode::E0406_FileError: return "Filesystem or I/O operation error";
    case ErrorCode::E0501_ConfigSyntaxError: return "Syntax error in satellite_config.ini";
    }
    return "Unknown error code";
}

} // namespace satellite

