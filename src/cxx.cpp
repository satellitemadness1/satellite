#include "satellite/cxx.hpp"

#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cctype>
#include <cstring>
#include <unistd.h>

#ifndef SATELLITE_INCLUDE_DIR
#define SATELLITE_INCLUDE_DIR ""
#endif
#ifndef SATELLITE_CORE_LIB
#define SATELLITE_CORE_LIB ""
#endif

namespace satellite {
namespace cxx {

// ---------------------------------------------------------------------------
CxxConfig default_config() {
    CxxConfig c;
    c.compiler    = "g++";
    c.include_dir = SATELLITE_INCLUDE_DIR;
    c.lib         = SATELLITE_CORE_LIB;
    c.flags       = "-O2 -std=c++20";

    const char* home = std::getenv("HOME");
    const char* tmp  = std::getenv("TMPDIR");
    c.cache_dir = home ? std::string(home) + "/.satellite/cache"
                : tmp  ? std::string(tmp)  + "/satellite-cache"
                       : "/tmp/satellite-cache";
    return c;
}

namespace {

void make_dirs(const std::string& path) {
    std::string acc;
    for (size_t k = 0; k < path.size(); ++k) {
        acc += path[k];
        if (path[k] == '/' && acc.size() > 1) ::mkdir(acc.c_str(), 0700);
    }
    ::mkdir(path.c_str(), 0700);
}

std::string hash_hex(const std::string& s) {
    uint64_t h = 1469598103934665603ull;           // FNV-1a
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ull; }
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", (unsigned long long)h);
    return buf;
}

bool file_exists(const std::string& p) {
    struct stat st;
    return ::stat(p.c_str(), &st) == 0;
}

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Net change in brace depth across a line, ignoring braces inside string and
// char literals and after a line comment.
int brace_delta(const std::string& line) {
    int d = 0;
    for (size_t k = 0; k < line.size(); ++k) {
        char c = line[k];
        if (c == '/' && k + 1 < line.size() && line[k + 1] == '/') break;
        if (c == '"' || c == '\'') {
            char q = c;
            ++k;
            while (k < line.size() && line[k] != q) {
                if (line[k] == '\\') ++k;
                ++k;
            }
            continue;
        }
        if (c == '{') ++d;
        else if (c == '}') --d;
    }
    return d;
}

bool starts_with_word(const std::string& t, const char* w) {
    size_t n = std::strlen(w);
    if (t.compare(0, n, w) != 0) return false;
    return t.size() == n || !(std::isalnum((unsigned char)t[n]) || t[n] == '_');
}

// Declarations that are illegal inside a function body and must go to file
// scope: templates, types, namespaces -- and free functions, since C++ has no
// nested functions.
bool opens_file_scope_construct(const std::string& line) {
    std::string t = trim(line);
    if (t.empty()) return false;

    static const char* kKeywords[] = {
        "template", "struct", "class", "enum", "union", "namespace", "typedef",
    };
    for (const char* k : kKeywords)
        if (starts_with_word(t, k)) return true;

    // A free function definition: something(...) {   -- but not a control
    // statement, which has the same shape.
    static const char* kControl[] = {
        "if", "for", "while", "switch", "do", "else", "return", "try", "catch",
    };
    for (const char* k : kControl)
        if (starts_with_word(t, k)) return false;

    size_t open  = t.find('(');
    size_t brace = t.rfind('{');
    if (open == std::string::npos || brace == std::string::npos) return false;
    if (brace < open) return false;
    if (t.find(';') != std::string::npos) return false;      // a call, not a definition
    if (t.find('=') != std::string::npos) return false;      // an initialisation
    // needs a return type AND a name before the parenthesis: "int helper(...)"
    std::string head = trim(t.substr(0, open));
    return head.find(' ') != std::string::npos || head.find('*') != std::string::npos;
}

}  // namespace

// ---------------------------------------------------------------------------
// satellite.return(EXPR) -- find the expression, matching nested parentheses so
// satellite.return(f(x, g(y))) works.
// ---------------------------------------------------------------------------
std::string return_expression(const std::string& src) {
    static const std::string marker = "satellite.return";
    size_t at = src.find(marker);
    if (at == std::string::npos) return "";

    size_t open = src.find('(', at + marker.size());
    if (open == std::string::npos) return "";

    int depth = 0;
    for (size_t k = open; k < src.size(); ++k) {
        if (src[k] == '(') ++depth;
        else if (src[k] == ')') {
            if (--depth == 0) return trim(src.substr(open + 1, k - open - 1));
        }
    }
    return "";
}

// remove the satellite.return(...) call from the body, leaving the rest
static std::string strip_return(const std::string& src) {
    static const std::string marker = "satellite.return";
    size_t at = src.find(marker);
    if (at == std::string::npos) return src;
    size_t open = src.find('(', at + marker.size());
    if (open == std::string::npos) return src;

    int depth = 0;
    for (size_t k = open; k < src.size(); ++k) {
        if (src[k] == '(') ++depth;
        else if (src[k] == ')' && --depth == 0)
            return src.substr(0, at) + src.substr(k + 1);
    }
    return src;
}

// ---------------------------------------------------------------------------
// Generate the translation unit for a block.
// ---------------------------------------------------------------------------
std::string generate(const std::string& block_source) {
    std::string ret  = return_expression(block_source);
    std::string body = strip_return(block_source);

    // Split the block into what must live at file scope and what goes inside
    // the function.  #include and #define cannot appear in a function body, and
    // neither can templates, type definitions, or free functions -- C++ has no
    // nested functions.
    std::ostringstream prologue, code;
    std::istringstream in(body);
    std::string line;
    int  depth = 0;
    bool hoisting = false;

    while (std::getline(in, line)) {
        std::string t = trim(line);
        bool to_prologue = hoisting;

        if (!hoisting && depth == 0) {
            if (!t.empty() && t[0] == '#') {
                prologue << line << '\n';
                continue;
            }
            if (t.rfind("using namespace", 0) == 0) {
                prologue << line << '\n';
                continue;
            }
            if (opens_file_scope_construct(line)) { hoisting = true; to_prologue = true; }
        }

        if (to_prologue) prologue << line << '\n';
        else             code     << "        " << line << '\n';

        depth += brace_delta(line);
        if (depth < 0) depth = 0;
        if (hoisting && depth == 0) hoisting = false;   // construct complete
    }

    std::ostringstream out;
    out << "// generated by satellite.cxx -- do not edit\n"
        << "#include \"satellite/cxx.hpp\"\n"
        << "#include <string>\n"
        << "#include <vector>\n"
        << "#include <exception>\n"
        << prologue.str()
        << "\n"
        << "using namespace satellite;\n"
        << "using namespace satellite::cxx;\n"
        << "\n"
        << "extern \"C\" int satellite_cxx_abi() { return " << ABI << "; }\n"
        << "\n"
        << "extern \"C\" satellite::Container satellite_cxx_block(\n"
        << "        const satellite::Container* args, int argc) {\n"
        << "    (void)args; (void)argc;\n"
        // An exception unwinding into the interpreter's frames is undefined
        // behaviour, so nothing is allowed to escape this function.
        << "    try {\n"
        << code.str();
    if (!ret.empty())
        out << "        return satellite::cxx::to_container(" << ret << ");\n";
    else
        out << "        return satellite::Container::nil();\n";
    out << "    } catch (const std::exception& e) {\n"
        << "        return satellite::Container::str(\n"
        << "            std::string(\"cxx error: \") + e.what());\n"
        << "    } catch (...) {\n"
        << "        return satellite::Container::str(\"cxx error: unknown exception\");\n"
        << "    }\n"
        << "}\n";
    return out.str();
}

// ---------------------------------------------------------------------------
Bridge::Bridge(CxxConfig cfg) : cfg_(std::move(cfg)) {
    if (!cfg_.cache_dir.empty()) make_dirs(cfg_.cache_dir);
}

Bridge::~Bridge() {
    // Deliberately NOT dlclose'd: static destructors in a module can run while
    // satellite values still point into it.  Modules live for the process.
    handles_.clear();
}

bool Bridge::build(const std::string& block_source, std::string* so_path,
                   std::string* err) {
    std::string unit = generate(block_source);
    std::string key  = hash_hex(unit);
    std::string so   = cfg_.cache_dir + "/cxx_" + key + ".so";

    if (file_exists(so)) {                          // same source, same binary
        ++hits_;
        if (so_path) *so_path = so;
        return true;
    }

    std::string cpp = cfg_.cache_dir + "/cxx_" + key + ".cpp";
    { std::ofstream f(cpp); if (!f) { if (err) *err = "cannot write " + cpp; return false; } f << unit; }

    std::ostringstream cmd;
    cmd << cfg_.compiler << ' ' << cfg_.flags << " -shared -fPIC"
        << " -I" << cfg_.include_dir
        << ' '   << cpp;
    if (!cfg_.lib.empty()) {
        size_t slash = cfg_.lib.find_last_of('/');
        std::string dir = slash == std::string::npos ? "." : cfg_.lib.substr(0, slash);
        cmd << ' ' << cfg_.lib << " -Wl,-rpath," << dir;
    }
    cmd << " -o " << so << " 2>&1";

    FILE* p = popen(cmd.str().c_str(), "r");
    if (!p) { if (err) *err = "cannot start the compiler"; return false; }
    std::string diag;
    char buf[512];
    while (std::fgets(buf, sizeof buf, p)) diag += buf;
    int rc = pclose(p);

    if (rc != 0) {
        ::unlink(so.c_str());                       // never cache a failure
        if (err) *err = diag.empty() ? "compilation failed" : diag;
        return false;
    }

    ++compiles_;
    if (so_path) *so_path = so;
    return true;
}

Container Bridge::run(const std::string& block_source, std::string* err) {
    std::string so;
    if (!build(block_source, &so, err)) return Container::nil();

    void* h = ::dlopen(so.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        if (err) *err = std::string("dlopen: ") + dlerror();
        return Container::nil();
    }

    auto abi = (int (*)())::dlsym(h, "satellite_cxx_abi");
    if (!abi) {
        if (err) *err = "module has no satellite_cxx_abi symbol";
        return Container::nil();
    }
    if (abi() != ABI) {
        if (err) *err = "ABI mismatch: module " + std::to_string(abi()) +
                        ", host " + std::to_string(ABI) + " -- refusing to load";
        return Container::nil();
    }

    using BlockFn = Container (*)(const Container*, int);
    auto fn = (BlockFn)::dlsym(h, "satellite_cxx_block");
    if (!fn) {
        if (err) *err = "module has no satellite_cxx_block symbol";
        return Container::nil();
    }

    handles_.push_back(h);
    return fn(nullptr, 0);
}

}  // namespace cxx
}  // namespace satellite
