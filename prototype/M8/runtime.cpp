// The satellite Program Runtime & Execution Engine implementation.
//
// Milestone 8 Prototype -- drives full execution from source through satellite.main().

#include "runtime.hpp"
#include "parser.hpp"
#include "resolver.hpp"
#include "dispatch.hpp"

#include <fstream>
#include <sstream>

namespace satellite {

void init_m8_dispatch()
{
    static bool s_initialized = false;
    if (s_initialized) return;
    s_initialized = true;

    auto &table = DispatchTable::instance();
    auto register_path = [&table](const char *path, HandlerFn fn, bool binds_receiver = false, uint32_t arity = 0) {
        words::Walk w = words::walk(path);
        if (w.error == words::WalkError::NONE && w.id != words::kNoPath) {
            table.register_handler(w.id, fn, binds_receiver, arity, path);
        }
    };

    // Override display (1 5 1) to route directly to Console::instance() with printer thread
    register_path("satellite.console.display", [](ExecContext &ctx, const std::vector<Value> &args) {
        if (args.empty()) {
            Console::instance().display("");
            ctx.record_output("");
            return Value::satellite_singleton();
        }
        std::string text;
        bool newline = true;
        size_t count = args.size();
        if (args.size() == 2 && args[1].is_bool()) {
            count = 1;
            newline = std::get<bool>(args[1]);
        }
        for (size_t i = 0; i < count; ++i) {
            if (i > 0) text += " ";
            if (args[i].is_string()) {
                text += decode(*std::get<Str>(args[i]));
            } else {
                text += args[i].to_string();
            }
        }
        Console::instance().display(text, newline);
        ctx.record_output(text);
        return Value::satellite_singleton();
    }, false, 1);

    // 1 15 0: return()
    register_path("satellite.return", [](ExecContext &, const std::vector<Value> &) {
        return Value::nil();
    }, false, 0);

    // 1 15 1: return(satellite) -> evaluates to satellite singleton (DESIGN §4: success)
    register_path("satellite.return(satellite)", [](ExecContext &, const std::vector<Value> &) {
        return Value::satellite_singleton();
    }, false, 1);

    // 1 15 2: return(value) -> passes through value
    register_path("satellite.return(value)", [](ExecContext &, const std::vector<Value> &args) {
        if (args.empty()) return Value::nil();
        return args[0];
    }, false, 1);
}

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
    : max_depth_(max_depth)
{
    init_m8_dispatch();
}

RunResult Runtime::run_string(const std::string &source)
{
    init_m8_dispatch();

    RunResult result;

    AstArena arena;
    words::Words words;

    // 1. Parse source text into Arena AST
    auto parse_res = parse(source, arena, words);
    if (!parse_res.ok()) {
        result.success = false;
        result.exit_code = 1;
        result.error_message = "Syntax error during parsing";
        return result;
    }

    // 2. Resolve AST identifiers, scopes, and spacesuits
    auto resolve_res = resolve(parse_res.program, arena, words);
    if (!resolve_res.ok()) {
        result.success = false;
        result.exit_code = 1;
        result.error_message = "Semantic error during resolution";
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
            Console::instance().drain();
            return result;
        }

        // Run satellite.main() if defined
        Value main_res = eval.run_main();
        result.return_value = main_res;
        result.exit_code = map_value_to_exit_code(main_res);
        result.output = eval.output();

        // 5. Enforce Console drain() barrier
        Console::instance().drain();
    } catch (const std::exception &e) {
        result.success = false;
        result.exit_code = 2;
        result.error_message = e.what();
        Console::instance().drain();
    }

    return result;
}

RunResult Runtime::run_file(const std::string &path)
{
    std::string source = read_file_contents(path);
    if (source.empty()) {
        RunResult res;
        res.success = false;
        res.exit_code = 3;
        res.error_message = "File not found or unreadable: " + path;
        return res;
    }
    return run_string(source);
}

} // namespace satellite
