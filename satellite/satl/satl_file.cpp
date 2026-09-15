#include "satl_file.hpp"

#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>

namespace satellite004 {

namespace {

const std::string kInclude = "satellite.include(satellite)";
const std::string kMainStart = "satellite.capsule satellite.main(";
const std::string kReturn = "satellite.return(satellite)";
const std::string kDisplay = "satellite.console.display";

// The line without its `//` comment (a `//` inside a string literal is text)
// and without surrounding spaces.
std::string code_of(const std::string &line)
{
    bool in_string = false;
    size_t end = line.size();
    for (size_t i = 0; i < line.size(); i++) {
        if (in_string && line[i] == '\\') {
            i++;
            continue;
        }
        if (line[i] == '"')
            in_string = !in_string;
        else if (!in_string && line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') {
            end = i;
            break;
        }
    }
    size_t first = 0;
    while (first < end && (line[first] == ' ' || line[first] == '\t' || line[first] == '\r'))
        first++;
    while (end > first && (line[end - 1] == ' ' || line[end - 1] == '\t' || line[end - 1] == '\r'))
        end--;
    return line.substr(first, end - first);
}

std::vector<std::string> lines_of(const std::string &source)
{
    std::vector<std::string> lines;
    size_t start = 0;
    while (start <= source.size()) {
        size_t end = source.find('\n', start);
        if (end == std::string::npos)
            end = source.size();
        lines.push_back(source.substr(start, end - start));
        start = end + 1;
    }
    return lines;
}

bool starts_with(const std::string &text, const std::string &prefix)
{
    return text.compare(0, prefix.size(), prefix) == 0;
}

// `"..."` with \" \\ \n \t. False when it is not exactly one string literal.
bool string_literal(const std::string &argument, std::string &text)
{
    if (argument.size() < 2 || argument.front() != '"' || argument.back() != '"')
        return false;
    text.clear();
    for (size_t i = 1; i + 1 < argument.size(); i++) {
        char c = argument[i];
        if (c == '"')
            return false;                       // a second string: "a" + "b" is not this scenario
        if (c == '\\') {
            if (i + 2 >= argument.size())
                return false;
            const char next = argument[++i];
            c = next == 'n' ? '\n' : next == 't' ? '\t' : next;
        }
        text += c;
    }
    return true;
}

std::string where(unsigned int line, const std::string &code)
{
    return "line " + std::to_string(line) + ": " + code;
}

} // namespace

signed long long int load_satl(const std::string &path, std::string &source, MachineState &state)
{
    state.set("satl.file(loading " + path + ")", success);
    std::FILE *file = path.empty() ? nullptr : std::fopen(path.c_str(), "rb");
    if (file == nullptr)
        return report_error("satl.file(missing): " + (path.empty() ? std::string("no .satl file was named") : path),
                            missing_satl_file);
    source.clear();
    char buffer[65536];
    size_t got;
    while ((got = std::fread(buffer, 1, sizeof buffer, file)) > 0)
        source.append(buffer, got);
    std::fclose(file);
    return state.set("satl.file(loaded " + path + ")", successfully_loaded_satl_file);
}

signed long long int check_satl(const std::string &source, MachineState &state)
{
    bool include = false, main = false, returns = false;
    for (const std::string &line : lines_of(source)) {
        const std::string code = code_of(line);
        include = include || code == kInclude;
        main = main || starts_with(code, kMainStart);
        returns = returns || code == kReturn;
    }
    if (!include)
        return report_error("satl.file(check): no line says " + kInclude,
                            satl_file_missing_satellite_include_satellite);
    if (!main)
        return report_error("satl.file(check): no satellite.capsule satellite.main(...)",
                            satl_file_missing_satellite_main);
    if (!returns)
        return report_error("satl.file(check): no line says " + kReturn,
                            satl_file_missing_satellite_return_satellite);
    return state.set("satl.file(checked)", success);
}

signed long long int compile_satl(const std::string &source, const NumberIndex &index,
                                  std::vector<Call> &calls, MachineState &state)
{
    state.set("satl.file(compiling)", success);
    const NumberRow *display = index.find(kDisplay);       // looked up ONCE, not per line
    const std::vector<std::string> lines = lines_of(source);
    calls.clear();

    for (size_t i = 0; i < lines.size(); i++) {
        const unsigned int number = static_cast<unsigned int>(i + 1);
        const std::string code = code_of(lines[i]);
        if (code.empty() || code == "{" || code == "}" || code == kInclude || starts_with(code, kMainStart))
            continue;
        if (code == kReturn)
            break;                                          // nothing after the return runs

        if (!starts_with(code, kDisplay + "(") || code.back() != ')')
            return report_error("satl.file(compile): " + where(number, code), satl_line_not_understood);
        if (display == nullptr)
            return report_error("satl.file(compile): " + kDisplay + " is not in the number index, " +
                                    where(number, code),
                                satl_line_not_understood);

        Call call;
        call.row = display;
        call.line = number;
        const std::string argument = code.substr(kDisplay.size() + 1, code.size() - kDisplay.size() - 2);

        if (string_literal(argument, call.text)) {
            call.kind = ArgumentKind::text;
        } else if (!argument.empty() && argument.find_first_not_of("0123456789") == std::string::npos) {
            errno = 0;
            call.count = std::strtoull(argument.c_str(), nullptr, 10);
            if (errno == ERANGE)
                return report_error("satl.file(compile): number too large for this runner, " + where(number, code),
                                    int_error);
            call.kind = ArgumentKind::count;
        } else if (argument == "satellite.bool.true" || argument == "satellite.bool.false") {
            call.kind = ArgumentKind::flag;
            call.flag = argument == "satellite.bool.true";
        } else if (!argument.empty() && argument.front() == '"') {
            return report_error("satl.file(compile): not one string literal, " + where(number, code), string_error);
        } else {
            return report_error("satl.file(compile): no display scenario for this, " + where(number, code),
                                satl_line_not_understood);
        }

        const Scenarios &have = display->scenarios;
        const bool has = (call.kind == ArgumentKind::text && have.text != nullptr) ||
                         (call.kind == ArgumentKind::count && have.count != nullptr) ||
                         (call.kind == ArgumentKind::flag && have.flag != nullptr);
        if (!has)
            return report_error("satl.file(compile): the library has no scenario for this, " + where(number, code),
                                satl_line_not_understood);
        calls.push_back(std::move(call));
    }
    return state.set("satl.file(compiled " + std::to_string(calls.size()) + " calls)", success);
}

signed long long int run_calls(const std::vector<Call> &calls, MachineState &state)
{
    for (const Call &call : calls) {
        if (state.debug_mode)
            state.set(call.row->name + "(running line " + std::to_string(call.line) + ")", success);

        signed long long int code = success;
        switch (call.kind) {
        case ArgumentKind::text: code = call.row->scenarios.text(call.text, true); break;
        case ArgumentKind::count: code = call.row->scenarios.count(call.count, true); break;
        case ArgumentKind::flag: code = call.row->scenarios.flag(call.flag, true); break;
        case ArgumentKind::number: // a signed config number; no .satl line compiles to one yet
        case ArgumentKind::size: code = satl_line_not_understood; break;
        }
        if (code != success)
            return report_error("satl.run(error) at line " + std::to_string(call.line), code);
    }

    // std::cout reports a refused write only when it really writes, so the
    // last few kilobytes are checked here.
    std::cout.flush();
    if (!std::cout)
        return report_error("satl.run(error): the output refused the last lines", display_error);
    return state.set("satl.run(finished)", success);
}

} // namespace satellite004
