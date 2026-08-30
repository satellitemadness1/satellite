// The satellite REPL & Interactive Prompt Engine implementation.
// Milestone 11 Prototype in prototype/M11.

#include "repl.hpp"
#include "block_scan.hpp"
#include "line_reader.hpp"
#include "console.hpp"
#include "system_facts/version.hpp"
#include "system_facts/system.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>

namespace satellite {

const char *color_user = "";
const char *color_cwd  = "";
const char *color_off  = "";

void choose_prompt_colors()
{
    if (!isatty(STDOUT_FILENO))
        return;

    if (const char *no_color = getenv("NO_COLOR"); no_color && *no_color)
        return;

    color_off = "\033[0m";

    if (getenv("SATL_TERM")) {
        color_user = "\033[1;38;2;255;255;255m";
        color_cwd  = "\033[1;38;2;0;0;0m";
        return;
    }

    color_user = "\033[1m";
    color_cwd  = "";
}

std::string eval_line(const std::string &line, ExecContext &ctx, bool echo)
{
    Runtime rt;
    rt.context() = ctx;
    std::string out = rt.eval_session_line(line, echo);
    ctx = rt.context();
    return out;
}

int run_repl(Runtime &rt)
{
    choose_prompt_colors();

    std::printf("satellite %s (run <file> interprets a file; :vars lists variables; :history lists history)\n",
                version_line().c_str());
    std::printf("type 'help' or ':history' for interactive help\n");
    std::fflush(stdout);

    LineReader reader;
    std::string line;
    std::string block;
    int depth = 0;
    bool hinted = false;

    for (;;) {
        std::string prompt;

        if (depth > 0) {
            rt.drain();
            prompt.assign(static_cast<size_t>(depth) * 4, ' ');
        } else {
            rt.drain();
            prompt = std::string(color_user) + username() + color_off +
                     ", " + color_cwd + cwd() + color_off + ": ";
        }

        const LineStatus status = reader.read(prompt, line);

        if (status == LineStatus::Interrupted) {
            if (depth > 0)
                abandon_block(block, depth);
            continue;
        }

        if (status == LineStatus::EndOfFile) {
            if (depth > 0) {
                abandon_block(block, depth);
                continue;
            }
            return 0;
        }

        reader.remember(line);

        if (depth > 0) {
            if (line == ":cancel") {
                abandon_block(block, depth);
                continue;
            }

            const BlockScan scan = scan_block(line);

            for (int i = 0; i < depth; i++)
                block += "    ";
            block += line;
            if (scan.opens_body) {
                block += " {";
                std::printf("{\n");
            }
            block += '\n';

            depth += scan.depth;
            if (depth <= 0) {
                std::string out = rt.eval_session_line(block, true);
                if (!out.empty()) {
                    std::fputs(out.c_str(), stdout);
                    std::fflush(stdout);
                }
                block.clear();
                depth = 0;
            }
            continue;
        }

        int exit_status = 0;
        if (exit_command(line, exit_status))
            return exit_status;

        if (line.empty())
            continue;

        if (line == ":history") {
            const auto &entries = reader.history().entries();
            for (size_t i = 0; i < entries.size(); i++) {
                std::printf("%5zu  %s\n", i + 1, entries[i].c_str());
            }
            if (reader.history().path().empty()) {
                std::printf("(%zu line%s, this session only)\n",
                            entries.size(), entries.size() == 1 ? "" : "s");
            } else {
                std::printf("(%zu line%s, kept in %s)\n",
                            entries.size(), entries.size() == 1 ? "" : "s",
                            reader.history().path().c_str());
            }
            continue;
        }

        if (line == ":vars") {
            for (const auto &pair : rt.context().globals()) {
                std::printf("%s = %s\n", pair.first.c_str(), pair.second.to_string().c_str());
            }
            continue;
        }

        if (line == "help") {
            std::printf("satellite interactive commands:\n"
                        "  run <file> [args]    - run a satellite program file\n"
                        "  :vars                - inspect global variables\n"
                        "  :history             - show command history\n"
                        "  exit / quit          - exit the REPL session\n");
            continue;
        }

        RunCommand run_cmd = parse_run_command(line);
        if (run_cmd.matched) {
            if (!run_cmd.error.empty()) {
                std::fputs(run_cmd.error.c_str(), stderr);
            } else {
                RunResult rr = rt.run_file(run_cmd.path);
                if (!rr.success) {
                    std::fprintf(stderr, "satellite: error running file: %s\n", rr.error_message.c_str());
                }
            }
            continue;
        }

        BlockScan scan = scan_block(line);
        if (!scan.lex_error && scan.depth > 0) {
            block = line;
            if (scan.opens_body) {
                block += " {";
                std::printf("{\n");
            }
            block += '\n';
            depth = scan.depth;

            if (!hinted) {
                std::printf("  (multi-line: the block ends at its closing brace; :cancel or Ctrl-D abandons it)\n");
                hinted = true;
            }
            continue;
        }

        std::string out = rt.eval_session_line(line, true);
        if (!out.empty()) {
            std::fputs(out.c_str(), stdout);
            std::fflush(stdout);
        }
    }
}

} // namespace satellite
