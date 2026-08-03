// The interpreter front door.
//
// This is the seam the real interpreter grows into.  Today it reaches Stage
// ::Validate and stops -- it lexes, checks that a main file declares
// `satellite.include(satellite)`, and executes any satellite.cxx blocks.  Parse,
// Compile and Execute are declared but not implemented, so callers (the CLI,
// the GUI) can be written once and keep working as each stage lands.
//
// Deliberately NOT implemented yet: `satellite.include` does not actually
// include anything.  Version 001 only CHECKS that the main file has it.
#pragma once

#include <string>
#include <vector>

#include "satellite/container.hpp"
#include "satellite/lexer.hpp"

namespace satellite {

// How far interpretation got before stopping.  Each new stage extends this.
enum class Stage {
    Lex,        // tokens produced
    Validate,   // structural requirements checked (main declares the include)
    Parse,      // NOT IMPLEMENTED -- no parser yet
    Compile,    // NOT IMPLEMENTED -- no bytecode compiler yet
    Execute,    // NOT IMPLEMENTED -- no VM yet
};

const char* stage_name(Stage s);

struct Diagnostic {
    std::string message;
    int  line     = 0;
    int  col      = 0;
    bool is_error = true;      // false for warnings and notes

    std::string format(const std::string& file) const;
};

struct InterpretResult {
    bool                     ok      = false;
    Stage                    reached = Stage::Lex;
    std::string              file;
    std::vector<Diagnostic>  diagnostics;
    std::vector<std::string> output;    // lines the program produced
    std::string              summary;   // one line, for a status bar

    bool has_errors() const;
    int  error_count() const;
};

// ---------------------------------------------------------------------------
// Path resolution.
//
// A user typing `interpret hello` should get hello.satl.  Tries, in order:
//   1. the path exactly as typed
//   2. the path with ".satl" appended
// Returns the path that exists, or empty if neither does.
// ---------------------------------------------------------------------------
std::string resolve_satl_path(const std::string& typed);

// ---------------------------------------------------------------------------
// Structural checks that do not need a parser.
// ---------------------------------------------------------------------------

// Does this token stream contain `satellite.include(satellite)`?
// Every main file is required to have it.
bool declares_satellite_include(const std::vector<Token>& tokens);

// Does it declare `satellite.capsule satellite.main(...)`?
bool declares_main(const std::vector<Token>& tokens);

// ---------------------------------------------------------------------------
// Interpretation
// ---------------------------------------------------------------------------
struct InterpretOptions {
    bool require_main_include = true;   // enforce satellite.include(satellite)
    bool require_main         = false;  // enforce a satellite.main capsule
    bool run_cxx_blocks       = true;   // execute satellite.cxx { } blocks
};

InterpretResult interpret_source(const std::string& source,
                                 const std::string& name,
                                 InterpretOptions opts = {});

InterpretResult interpret_file(const std::string& path,
                               InterpretOptions opts = {});

}  // namespace satellite
