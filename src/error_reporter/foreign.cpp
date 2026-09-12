// The foreign-syntax table. See foreign.hpp for why it is a table.

#include "error_reporter/foreign.hpp"

#include <algorithm>
#include <cctype>

namespace satellite::errors {
namespace {

// ORDER MATTERS AND LONGER PATTERNS COME FIRST, because `std::cout` and `std::`
// both match a `std::cout` line and only the first is worth saying. The search
// stops at the first hit.
constexpr Foreign kForeign[] = {

// --- printing, which is what everybody types first ------------------------
{ "C++",    "std::cout",           "satellite.console.display(x)", nullptr,
  "display takes one value and adds the newline" },
{ "C",      "printf(",             "satellite.console.display(x)", nullptr,
  "there is no format string -- build the line with `+`" },
{ "Python", "print(",              "satellite.console.display(x)", nullptr, nullptr },
{ "Java",   "System.out.println(", "satellite.console.display(x)", nullptr, nullptr },
{ "Rust",   "println!",            "satellite.console.display(x)", nullptr,
  "there is no format string -- build the line with `+`" },

// --- where a program starts -----------------------------------------------
{ "C++",    "int main(",           "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)", nullptr, nullptr },
{ "Java",   "public static void main", "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)", nullptr, nullptr },
{ "Rust",   "fn main(",            "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)", nullptr, nullptr },
{ "Python", "if __name__",         "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)", nullptr,
  "satellite runs `satellite.main` and nothing else, so there is no guard to write" },

// --- bringing the language in ---------------------------------------------
{ "C++",    "#include",            "satellite.include(satellite)", nullptr, nullptr },
{ "Python", "import ",             "satellite.include(satellite)", nullptr,
  "one include brings the whole language; there are no separate modules" },
{ "Rust",   "use std",             "satellite.include(satellite)", nullptr, nullptr },

// --- a function -----------------------------------------------------------
{ "Python", "def ",                "satellite.capsule name() { }", nullptr,
  "`satellite.returns(type)` after the parentheses when it answers something" },
{ "Rust",   "fn ",                 "satellite.capsule name() { }", nullptr, nullptr },

// --- an object ------------------------------------------------------------
{ "Python", "class ",              "satellite.spacesuit name() { satellite.protected { } satellite.public { } }", nullptr, nullptr },
{ "Java",   "public class ",       "satellite.spacesuit name() { satellite.protected { } satellite.public { } }", nullptr, nullptr },
{ "Rust",   "struct ",             "satellite.spacesuit name() { satellite.protected { } satellite.public { } }", nullptr,
  "the fields and the methods are one declaration here, so there is no `impl`" },
{ "Rust",   "impl ",               "satellite.public { }", nullptr,
  "a spacesuit holds its own methods -- DESIGN 7.4" },

// --- declaring something --------------------------------------------------
{ "C++",    "std::string ",        "satellite.variable.string name = \"\"", nullptr, nullptr },
{ "C++",    "std::vector<",        "satellite.container.list<T> name = satellite.container.list()", nullptr, nullptr },
{ "Java",   "ArrayList<",          "satellite.container.list<T> name = satellite.container.list()", nullptr, nullptr },
{ "Java",   "HashMap<",            "satellite.container.map<K, V> name = satellite.container.map()", nullptr, nullptr },
{ "Rust",   "Vec<",                "satellite.container.list<T> name = satellite.container.list()", nullptr, nullptr },
{ "Rust",   "let mut ",            "satellite.variable.number name = 0", nullptr,
  "every name here is mutable; there is no second spelling for one that is not" },
{ "Rust",   "let ",                "satellite.variable.number name = 0", nullptr, nullptr },

// --- truth and nothing ----------------------------------------------------
{ "Python", "True",                "satellite.bool.true", nullptr, nullptr },
{ "Python", "False",               "satellite.bool.false", nullptr, nullptr },
{ "Python", "None",                "a variant holding nothing -- ask it with .holding()", nullptr, nullptr },
{ "C++",    "nullptr",             "there is no pointer type; a spacesuit IS a reference", nullptr,
  "DESIGN 7.4 -- passing one passes the handle, so there is nothing to take the address of" },

// --- control flow satellite HAS -------------------------------------------
{ "Python", "elif",                "satellite.statement.else { satellite.statement.if(...) { } }", nullptr,
  "there is no `else if` -- nest the if inside the else" },
{ "Python", "for ",                "satellite.statement.for (i = 0; i < n; i = i + 1)", nullptr,
  "there is no iterator form; walk the index and subscript with l[i]" },
{ "C++",    "for(",                "satellite.statement.for (i = 0; i < n; i = i + 1)", nullptr, nullptr },

// --- control flow satellite DOES NOT HAVE, and the milestone that brings it -
{ "C++",    "&&",                  nullptr, "M28",
  "no logical operators yet -- nest two ifs, or keep a bool and test it" },
{ "C++",    "||",                  nullptr, "M28",
  "no logical operators yet -- nest two ifs, or keep a bool and test it" },
{ "Python", " and ",               nullptr, "M28", "no logical operators yet -- nest two ifs" },
{ "Python", " or ",                nullptr, "M28", "no logical operators yet -- nest two ifs" },
{ "C++",    "break",               nullptr, "M27",
  "no break yet -- end a loop by making its condition false, and keep the real count in another variable" },
{ "C++",    "continue",            nullptr, "M27", "no continue yet -- wrap the rest of the body in an if" },
{ "C++",    "switch",              nullptr, "M30", "no switch yet -- a chain of ifs" },
{ "Rust",   "match ",              nullptr, "M30", "no match yet -- a chain of ifs" },
{ "C++",    "template<",           nullptr, "M31",
  "no user generics yet -- list and map are generic, nothing a program writes is" },
{ "Python", "lambda",              nullptr, "M32",
  "a capsule is not a value yet -- DESIGN 12 defers `a bare name can be a value`" },
{ "C++",    "try {",               nullptr, "M33",
  "a refusal stops the run and cannot be caught yet -- DESIGN 9.1" },
{ "Java",   "catch (",             nullptr, "M33", "a refusal stops the run and cannot be caught yet" },
{ "C++",    "throw ",              nullptr, "M33", "a capsule reports by refusing, and the run stops" },

// --- things with no plan, said outright -----------------------------------
{ "C++",    "new ",                "declare the spacesuit -- `name thing` constructs it", nullptr,
  "there is no manual allocation and no delete; DESIGN 12 is refcounting" },
{ "C++",    "delete ",             "nothing -- a spacesuit is released when the last handle goes", nullptr,
  "a cycle is never freed, which DESIGN 12 accepts on purpose" },
{ "C++",    "goto ",               nullptr, nullptr, "satellite has no goto and none is planned" },
{ "assembly", "section .text",     nullptr, nullptr,
  "satellite has no inline assembly and none is planned -- it is an interpreted language with no machine layer to reach" },
{ "assembly", "global _start",     nullptr, nullptr, "satellite has no inline assembly and none is planned" },
{ "assembly", "mov ",              nullptr, nullptr,
  "satellite has no inline assembly and none is planned; `satellite.bits` is the lowest it goes" },
{ "assembly", "syscall",           nullptr, nullptr,
  "no direct syscalls -- satellite.system and satellite.file are the ways out" },
{ "assembly", "int 0x80",          nullptr, nullptr, "no direct syscalls -- see satellite.system" },

// --- C++ standard library, the parts people reach for ---------------------
{ "C++",    "std::optional",       "satellite.variable.variant -- ask it with .holding()", nullptr,
  "a variant holds a value or nothing, and says which" },
{ "C++",    "std::variant",        "satellite.variable.variant", nullptr,
  "one type that holds one of several, and .holding() names which" },
{ "C++",    "std::expected",       nullptr, "M33",
  "C++23's answer-or-error -- satellite.file already does this shape with ok() and error()" },
{ "C++",    "std::unordered_map<", "satellite.container.map<K, V> name = satellite.container.map()", nullptr, nullptr },
{ "C++",    "std::map<",           "satellite.container.map<K, V> name = satellite.container.map()", nullptr, nullptr },
{ "C++",    "std::sort(",          "my_list.sort_up() / .sort_down()", nullptr,
  "the list sorts itself; there is no free algorithm taking iterators" },
{ "C++",    "std::unique_ptr",     "declare the spacesuit -- it is already a counted reference", nullptr,
  "DESIGN 7.4, and nothing is deleted by hand" },
{ "C++",    "std::shared_ptr",     "declare the spacesuit -- it is already a counted reference", nullptr,
  "DESIGN 12 refcounts; a cycle is never freed" },
{ "C++",    "std::move(",          "nothing -- passing a spacesuit passes the handle", nullptr,
  "there is no copy to avoid, so there is no move to write" },
{ "C++",    "std::thread",         "satellite.variable.thread t = satellite.thread.new(f(x))", nullptr,
  "then t.start() and t.join() -- M23" },
{ "C++",    "std::mutex",          nullptr, "M40",
  "satellite.library locks and containers are copy-on-write; what is unguarded is a SPACESUIT two threads both write to" },
{ "C++",    "lock_guard",          nullptr, "M40", "there is no scope guard to write -- see FOREIGN_MILESTONES/CXX23/M46.md" },
{ "C++",    "std::atomic",         nullptr, "M40",
  "refcounts ARE atomic (shared_ptr); what is missing is an atomic read-modify-write a program can ask for" },
{ "C++",    "std::format",         "build the line with `+`", nullptr,
  "there is no format string; a number joins a string directly" },
{ "C++",    "std::print",          "satellite.console.display(x)", nullptr, nullptr },
{ "C++",    "std::ranges",         nullptr, "M36", "no pipelines yet -- walk the index" },
{ "C++",    "std::function",       nullptr, "M32", "a capsule is not a value yet" },
{ "C++",    "auto ",               "name the type -- satellite.variable.number, .string, .bool", nullptr,
  "every declaration says its type; there is no deduction and none planned" },
{ "C++",    "constexpr",           nullptr, "M34",
  "satellite.library.<name> is resolved at parse time, but a computation is not" },
{ "C++",    "consteval",           nullptr, "M34", nullptr },
{ "C++",    "virtual ",            "a spacesuit holds a copy of its superclass -- see DESIGN 7.4", nullptr,
  "inheritance exists; dynamic dispatch through a base handle does not yet" },
{ "C++",    "namespace ",          "nothing -- every path is dotted and the file is the scope", nullptr,
  "DESIGN 1: satellite.container.list IS the namespacing" },
{ "C++",    "operator",            nullptr, "M38",
  "name the verb instead -- a.call_plus(b) -- until operators can be declared" },
{ "C++",    "<=>",                 nullptr, "M38", nullptr },
{ "C++",    "if constexpr",        nullptr, "M34", nullptr },
{ "C++",    "co_await",            nullptr, nullptr, "satellite has no coroutines and none is planned; use a thread" },
{ "C++",    "co_return",           nullptr, nullptr, "satellite has no coroutines and none is planned" },
{ "C++",    "concept ",            nullptr, "M31", "concepts need generics first" },
{ "C++",    "auto [",              nullptr, "M35", "no destructuring yet -- one name per line" },
{ "C++",    "const ",              "nothing -- every name is mutable and there is no second spelling", nullptr,
  "DESIGN 12 decided against const; a capsule that must not change a thing does not change it" },
{ "C++",    "?",                   nullptr, "M37", "no conditional expression yet -- declare, then assign in an if" },

// --- Python, beyond the first four ----------------------------------------
{ "Python", "__init__",            "set the fields in the protected block, or a call_set_ verb", nullptr,
  "a bare declaration constructs; there is no constructor body yet" },
{ "Python", "self.",               "nothing -- there is no `this` and no `self`", nullptr,
  "a member capsule cannot name its own object; it is told its handle from outside" },
{ "Python", "len(",                "my_list.size() / my_string.size()", nullptr, nullptr },
{ "Python", "range(",              "satellite.statement.for (i = 0; i < n; i = i + 1)", nullptr, nullptr },
{ "Python", ".append(",            "my_list.append(x)", nullptr, "the same word -- 1 4 2 1" },
{ "Python", "except",              nullptr, "M33", "a refusal stops the run and cannot be caught yet" },
{ "Python", "raise ",              nullptr, "M33", "a capsule reports by refusing, and the run stops" },
{ "Python", "yield ",              nullptr, nullptr, "satellite has no generators and none is planned" },
{ "Python", "with ",               "open it, use it, close it -- f.close() is explicit", nullptr,
  "there is no scope guard; DESIGN 12 has no destructors to hang one on" },
{ "Python", "f\"",                 "build the line with `+`", nullptr, "there are no f-strings" },
{ "Python", "elif ",               "satellite.statement.else { satellite.statement.if(...) { } }", nullptr, nullptr },
{ "Python", "dict(",               "satellite.container.map<K, V> name = satellite.container.map()", nullptr, nullptr },
{ "Python", "@",                   nullptr, "M32", "decorators need a capsule to be a value" },

// --- Java, beyond main and println ----------------------------------------
{ "Java",   "StringBuilder",       "satellite.variable.string, and `+` joins", nullptr,
  "strings are values here; there is no builder to avoid copies" },
{ "Java",   "implements ",         nullptr, "M31", "interfaces need generics and a decision after them" },
{ "Java",   "interface ",          nullptr, "M31", nullptr },
{ "Java",   "extends ",            "satellite.spacesuit child(parent)", nullptr,
  "the superclass goes in the parentheses -- a suit holds a copy of it" },
{ "Java",   "synchronized",        nullptr, "M40",
  "satellite.library is already locked per access; a shared spacesuit is not" },
{ "Java",   "System.exit(",        "satellite.return(satellite) from satellite.main", nullptr,
  "the exit code says whether the run refused, not what main handed back" },
{ "Java",   "static ",             "a capsule at file scope is already shared", nullptr, nullptr },
{ "Java",   "@Override",           "nothing -- declare the capsule with the same name", nullptr, nullptr },

// --- Rust, beyond let and println! ----------------------------------------
{ "Rust",   "Option<",             "satellite.variable.variant -- ask it with .holding()", nullptr, nullptr },
{ "Rust",   "Result<",             nullptr, "M33",
  "the shape exists on files -- f.ok() and f.error() -- but not in general" },
{ "Rust",   ".unwrap()",           "ask the variant with .holding() first", nullptr,
  "there is nothing that stops the run on your behalf" },
{ "Rust",   ".expect(",            "ask the variant with .holding() first", nullptr, nullptr },
{ "Rust",   "HashMap<",            "satellite.container.map<K, V> name = satellite.container.map()", nullptr, nullptr },
{ "Rust",   "trait ",              nullptr, "M31", "traits need generics first" },
{ "Rust",   "Arc<",                nullptr, "M40",
  "a spacesuit is already a counted shared handle -- what is missing is guarding its FIELDS" },
{ "Rust",   "Mutex<",              nullptr, "M40",
  "containers are immutable and copy-on-write, so a shared list is safe; a shared spacesuit is not" },
{ "Rust",   "Box<",                "declare the spacesuit -- it is already a counted reference", nullptr, nullptr },
{ "Rust",   ".iter()",             nullptr, "M36", "no pipelines yet -- walk the index and subscript with l[i]" },
{ "Rust",   ".collect()",          nullptr, "M36", nullptr },
{ "Rust",   "panic!",              "a refusal stops the run; there is no way to raise one on purpose yet", nullptr, nullptr },
{ "Rust",   "#[derive",            "nothing -- write the verb", nullptr, "there is no code generation" },
{ "Rust",   "&mut ",               "nothing -- a spacesuit is a reference and everything is mutable", nullptr,
  "there is no borrow checker; two handles to one object is allowed and unchecked" },
{ "Rust",   "&str",                "satellite.variable.string", nullptr, "there is one string type" },

// --- more assembly --------------------------------------------------------
{ "assembly", "section .data",     nullptr, nullptr, "satellite has no inline assembly and none is planned" },
{ "assembly", "section .bss",      nullptr, nullptr, "satellite has no inline assembly and none is planned" },
{ "assembly", "lea ",              nullptr, nullptr, "no inline assembly; and there are no addresses to load" },
{ "assembly", "xor ",              nullptr, nullptr, "no inline assembly -- satellite.variable.bits does bitwise work" },
{ "assembly", "push ",             nullptr, nullptr, "no inline assembly and no stack a program can reach" },
{ "assembly", "__asm",             nullptr, nullptr, "satellite has no inline assembly and none is planned" },

// --- THE LAST TWO ROWS, AND THEY ARE LAST ON PURPOSE ----------------------
// `::` and `->` appear inside almost every C++ line above -- `std::optional`,
// `std::mutex`, `p->method()` -- so a table that matched them first would
// answer "every path is dotted" to every C++ question ever asked. They are the
// fallback for a line that is recognisably C++ and nothing more specific.
// Found 2026-09-12 by `std::optional<int> x;` answering ".".
{ "C++",    "->",                  ".", nullptr,
  "a spacesuit is a reference already, so member access is always a dot" },
{ "C++",    "::",                  ".", nullptr,
  "every path in satellite is dotted -- DESIGN 1" },
};

bool contains(std::string_view haystack, std::string_view needle)
{
    return haystack.find(needle) != std::string_view::npos;
}

} // namespace

const Foreign *foreign_table(std::size_t *count)
{
    if (count)
        *count = sizeof kForeign / sizeof kForeign[0];
    return kForeign;
}

std::string foreign_advice(std::string_view line)
{
    // THE GUARD, AND IT IS THE WHOLE REASON THIS IS SAFE TO RUN ON EVERY ERROR.
    // Correct satellite contains `new`, `let` and `for` all the time; a line
    // that names the language is a line whose author is already here, and
    // telling them how to spell something they just spelled is noise.
    if (contains(line, "satellite."))
        return {};

    // AND A COMMENT IS NOT CODE. The advice fires on the line the caret is
    // under, and a `// for each person` above a broken statement would
    // otherwise be answered as if somebody had typed a for loop.
    std::size_t first = 0;
    while (first < line.size() &&
           (line[first] == ' ' || line[first] == '\t'))
        ++first;
    if (line.compare(first, 2, "//") == 0)
        return {};

    // `#` IS A COMMENT ONLY WITH A SPACE AFTER IT. `# note` is Python's;
    // `#include` is C++'s and `#[derive(...)]` is Rust's, and killing those two
    // was this guard's first bug -- 2026-09-12.
    if (first < line.size() && line[first] == '#' &&
        first + 1 < line.size() &&
        (line[first + 1] == ' ' || line[first + 1] == '\t'))
        return {};

    for (const Foreign &row : kForeign) {
        if (!contains(line, row.pattern))
            continue;

        std::string out = "that looks like ";
        out += row.language;
        out += ". ";

        if (row.satellite != nullptr) {
            out += "satellite spells it:\n    ";
            out += row.satellite;
            out += "\n";
        } else if (row.milestone != nullptr) {
            out += "satellite cannot say it yet -- ";
            out += row.milestone;
            out += " brings it.\n";
        } else {
            out += "satellite does not have it.\n";
        }

        if (row.note != nullptr) {
            out += "    ";
            out += row.note;
            out += "\n";
        }
        return out;
    }
    return {};
}

} // namespace satellite::errors
