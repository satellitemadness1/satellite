#pragma once

// Private to src/interpreter/. Nothing outside this directory may include it —
// the public surface is src/interpreter/interp.hpp, and that has not changed.
//
// interp.cpp was 433 lines. Split at the seams the file already had, into
// three: interp.cpp keeps the retained-image machinery and the session path
// (run_source, eval_line), interp_run.cpp holds the --run path (args_to_list,
// run_program, run_file), and interp_prompt.cpp holds what the prompt does to a
// raw line before evaluating it (parse_run_command, scan_block).
//
// What is declared below lived in an anonymous namespace when there was one
// translation unit, and is shared by the first two of those files. It is
// declared here and defined once now, because internal linkage cannot cross a
// file boundary. That is the only semantic change the split makes; every
// function body moved verbatim. The same reasoning, and the same wording, as
// src/evaluator/eval_internal.hpp.

#include "interpreter/interp.hpp"

#include "environment/env.hpp"
#include "evaluator/eval.hpp"
#include "spaceship_loader/loader.hpp"
#include "syntax_parser/parser.hpp"

#include <memory>
#include <string>
#include <vector>

namespace satellite {

// The tree and its resolution have ONE lifetime: resolve() records a pointer
// into the Program for every capsule and every method, and an Object records a
// pointer into the ResolveResult for its spacesuit.
//
// The LoadResult is what the parse used to be, and it holds three things that
// have to stay put: the merged Program those pointers point INTO, the
// SourceMap every error is rendered against, and the errors themselves. It is
// stored by value in an Image that is never moved after resolve() runs, which
// is what makes the pointers safe — the same contract as before §16, with a
// merged Program in place of a single file's.
struct Image {
    LoadResult loaded;
    ResolveResult resolved;
};

// Both defined in interp.cpp, beside the images() vector they are the two ends
// of, and both used from interp_run.cpp as well.
void retain(std::shared_ptr<Image> image);
int status_of(const ValuePtr &returned, bool ok);

// Defined HERE and not in a .cpp because it is a template: run_source and
// run_program instantiate it in two translation units now.
//
// Collects a parse or eval failure into the same text channel, so a caller
// never has to ask which stage broke.
//
// One template rather than three near-identical overloads, now that all three
// error types render through the same two-argument call. The three format_error
// implementations stay separate — they draw different things — but the loop
// over them never had a reason to be written out three times.
template <typename Error>
void report(std::string &out, const std::vector<Error> &errors,
            const SourceMap &sources)
{
    for (const Error &error : errors)
        out += format_error(error, sources);
}

} // namespace satellite
