// The satellite Program Runtime & Execution Engine implementation.
// Milestone 11 Prototype in prototype/M11.

#include "runtime.hpp"
#include "parser.hpp"
#include "resolver.hpp"
#include "dispatch.hpp"
#include "interrupt.hpp"

#include <climits>
#include <fstream>
#include <sstream>

namespace satellite {

namespace {

std::string read_file_contents(const std::string &path)
{
    for (const std::string &candidate : {path, "../" + path, "../../" + path}) {
        std::ifstream file(candidate);
        if (file) {
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
    }
    return "";
}

int map_value_to_exit_code(const Value &val)
{
    if (val.is_nil()) {
        return 0;
    }
    if (val == Value::satellite_singleton()) {
        return 0;
    }
    if (val.is_bool()) {
        return std::get<bool>(val) ? 0 : 1;
    }
    if (val.is_number()) {
        const auto &num = std::get<Number>(val);
        if (num.is_zero()) return 0;
        long long val64 = 0;
        if (num.to_integer(val64)) {
            return static_cast<int>(val64);
        }
        return 1;
    }
    return 0;
}

} // namespace

Runtime::Runtime(int max_depth)
    : max_depth_(max_depth), session_ctx_(max_depth)
{
    install_interrupt_handler();
    init_m11_dispatch();
}

RunResult Runtime::run_string(const std::string &source)
{
    install_interrupt_handler();
    init_m11_dispatch();
    clear_interrupt();

    RunResult result;

    AstArena arena;
    words::Words words;

    // 1. Parse source text into Arena AST
    auto parse_res = parse(source, arena, words);
    if (!parse_res.ok()) {
        result.success = false;
        result.exit_code = 1;
        result.error_message = "Syntax error during parsing: ";
        for (const auto &err : parse_res.errors) {
            result.error_message += err.message + " (line " + std::to_string(err.span.line) + "); ";
        }
        return result;
    }

    // 2. Resolve AST identifiers, scopes, and spacesuits
    auto resolve_res = resolve(parse_res.program, arena, words);
    if (!resolve_res.ok()) {
        result.success = false;
        result.exit_code = 1;
        result.error_message = "Semantic error during resolution: ";
        for (const auto &d : resolve_res.diagnostics) {
            result.error_message += d.message + " (line " + std::to_string(d.primary_span.line) + "); ";
        }
        return result;
    }

    // 3. Compile AST to executable Closure Tree
    auto compiled = compile(parse_res.program, arena, resolve_res, words);

    // 4. Initialize Evaluator and run
    try {
        Evaluator eval(std::move(compiled), max_depth_);

        // Run top-level statements
        ExecResult top_res = eval.run_top_level();
        if (top_res.status == ExecStatus::Return) {
            result.return_value = top_res.value;
            result.exit_code = map_value_to_exit_code(top_res.value);
            result.output = eval.output();
            return result;
        }

        // Run satellite.main() if defined
        std::vector<Value> main_args;
        Value main_ret = eval.run_main(main_args);

        if (interrupt_requested()) {
            result.interrupted = true;
            result.exit_code = INTERRUPT_EXIT_STATUS;
            result.output = eval.output();
            return result;
        }

        result.return_value = main_ret;
        result.exit_code = map_value_to_exit_code(main_ret);
        result.output = eval.output();

        // Enforce Console drain() barrier before process termination
        drain();

    } catch (const std::exception &ex) {
        result.success = false;
        result.exit_code = 1;
        result.error_message = ex.what();
    }

    return result;
}

RunResult Runtime::run_file(const std::string &path)
{
    std::string content = read_file_contents(path);
    if (content.empty()) {
        RunResult res;
        res.success = false;
        res.exit_code = 3;
        res.error_message = "File not found or empty: " + path;
        return res;
    }
    return run_string(content);
}

std::string Runtime::eval_session_line(const std::string &line, bool echo)
{
    install_interrupt_handler();
    init_m11_dispatch();
    clear_interrupt();

    AstArena arena;
    words::Words words;

    auto parse_res = parse(line, arena, words);
    if (!parse_res.ok()) {
        std::string err = "satellite: ";
        for (const auto &e : parse_res.errors) {
            err += e.message + "\n";
        }
        return err;
    }

    auto resolve_res = resolve(parse_res.program, arena, words);
    if (!resolve_res.ok()) {
        std::string err = "satellite: ";
        for (const auto &d : resolve_res.diagnostics) {
            err += d.message + "\n";
        }
        return err;
    }

    auto compiled = compile(parse_res.program, arena, resolve_res, words);

    try {
        session_ctx_.clear_output();
        Evaluator eval(std::move(compiled), session_ctx_);
        ExecResult res = eval.run_top_level();
        drain();

        std::string out;
        for (const auto &l : session_ctx_.output_lines()) {
            out += l + "\n";
        }

        if (echo && !res.value.is_nil() && res.value != Value::satellite_singleton()) {
            if (res.value.is_string()) {
                out += decode(*std::get<Str>(res.value)) + "\n";
            } else {
                out += res.value.to_string() + "\n";
            }
        }
        return out;
    } catch (const std::exception &ex) {
        return std::string("satellite error: ") + ex.what() + "\n";
    }
}

} // namespace satellite

