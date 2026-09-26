// satellite.help — the whole language on one screen.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.
//
// Split once more when it reached 492: the long-form TOPIC pages -- arguments,
// random, the three tiers, wide -- are now src/evaluator/help_topics.cpp. What
// stays here is the three LISTINGS, which is what "help" means before anyone
// asks a second question: the whole language, one module, one value.
//
// help_overview() in particular cannot move. It reads version_line(), and the
// Makefile hands VERSION_DEFS to help.o and to no other object in the tree.

#include "evaluator/eval_internal.hpp"
#include "system_facts/version.hpp"

namespace satellite {

std::string help_overview()
{
    // Not a literal: the number comes from the same place --version reads it,
    // so the banner and the flag cannot drift into disagreeing. It said 0.1
    // until 2026-08-24, long after the language had stopped being 0.1.
    return "satellite " + version_line() +
" -- the whole language\n"
"\n"
"  declare    satellite.variable.number x = 1        bool number string time file\n"
"             satellite.container.list<satellite.variable.number> l\n"
"             satellite.container.map<satellite.variable.string, ...> m\n"
"             my_class thing            a spacesuit; thing(\"arg\") to construct\n"
"  values     satellite.bool.true  satellite.bool.false  satellite (the runtime)\n"
"  operators  + - * / %   == != < <= > >=   !   -x    l[i]  l[a:b]  l[:b]  l[a:]\n"
"\n"
"  capsule    satellite.capsule name(satellite.variable.number n)\n"
"                 satellite.returns(satellite.variable.number) { ... }\n"
"  spacesuit  satellite.spacesuit my_class(superclass) {\n"
"                 satellite.protected { ...fields... }\n"
"                 satellite.public    { my_class(args) {...}  ...capsules... } }\n"
"  control    satellite.statement.if (c) {} satellite.statement.else {}\n"
"             satellite.statement.while (c) {}   satellite.statement.for (i;c;s) {}\n"
"  return     satellite.return(value)\n"
"\n"
"  console    satellite.console.display(value)\n"
"                 display(100ms)              pause 100 ms between lines\n"
"  time       satellite.time.now()                  .minus(t) .nanoseconds()\n"
"             satellite.time.sleep(300)             stop the program 300 ms\n"
"  file       satellite.file.open(path, \"read\"|\"write\"|\"append\"|\"read_append\")\n"
"                 satellite.file.new(path[, mode])   refuses to clobber\n"
"                 .ok() .read() .write(s) .clear() .open() .close()\n"
"                 .error() .path()\n"
"  directory  satellite.directory.current()  .change(dir)  .exists(dir)\n"
"                 .list()  .list(dir)        the names in it, sorted\n"
"  random     satellite.random.ultra(40)      fast normal ultra; 40 digits\n"
"                 .range(low, high)           inclusive at both ends\n"
"                 satellite.help(random)      and (ultra) (random.ultra)\n"
"  system     satellite.system.home()               $HOME\n"
"                 .delete(path)  .delete(file)  a file, or an empty dir\n"
"  analyze    satellite.analyze(\"file.satl\")        what a spaceship holds\n"
"  globals    satellite.library.<namespace>.<name>\n"
"\n"
"  help       satellite.help          this text (the () is optional)\n"
"             satellite.help(value)   the methods that value answers to\n"
"             satellite.directory     what one module answers to\n"
"             satellite.help(topic)   a topic below, bare or in quotes\n"
"\n"
"  topics     arguments       what satellite.main is handed, and how any\n"
"                             capsule's arguments are passed. Also: args\n"
"                             arg argz argument argumentz, and each of\n"
"                             those with \"system \" in front.\n"
"             random          the three tiers, and what none of them is\n"
"             fast normal ultra          one tier each, with its cost\n"
"             random.ultra    the same, written the way the path is\n"
"             wide            numbers past the 100,000 digit ceiling\n";
}

// What ONE module answers to.
//
// `satellite.directory` is a path the language owns, so asking it what it can
// do should not have to start with remembering which of its commands you
// wanted -- the same argument that makes satellite.help reachable without
// parentheses, one level down the tree. An empty answer means the name is not
// a module, which is how both callers tell.
std::string help_for_module(const std::string &module)
{
    if (module == "console")
        return "satellite.console\n"
               "  .display(value)        print it, one line\n"
               "  .display(100ms)        pause that long between lines, from here on\n";
    if (module == "time")
        return "satellite.time\n"
               "  .now()                 an instant, read off the clock\n"
               "                         .minus(t) and .nanoseconds() on what it hands back\n"
               "  .sleep(ms)             stop the program for that many milliseconds\n"
               "                         the display pace meters OUTPUT; this stops the WALK\n";
    if (module == "file")
        return "satellite.file\n"
               "  .open(path, \"read\")    also \"write\", \"append\", \"read_append\"\n"
               "  .new(path)             make one that is NOT there; read_append\n"
               "  .new(path, mode)       the same, in the mode you name\n"
               "                         a path that exists is .ok() false, not an error\n"
               "                         there is no \"text\" or \"binary\": a string is bytes\n"
               "                         .ok() .read() .write(s) .clear() .open() .close()\n"
               "                         .error() .path()\n";
    if (module == "directory")
        return "satellite.directory\n"
               "  .current()             where you are\n"
               "  .change(dir)           go there; true or false, never an error\n"
               "  .exists(dir)           is there a directory there\n"
               "  .list()  .list(dir)    the names in it, sorted\n";
    if (module == "system")
        return "satellite.system\n"
               "  .home()                        this user's home directory\n"
               "  .delete(path)                  a file, or an EMPTY directory\n"
               "  .delete(file)                  the path that handle opened\n"
               "                                 true or false, never an error\n"
               "                                 does not recurse; walk .list()\n"
               "  .memory.used([unit])           and .free .total .swap .main\n"
               "  .memory.swap.used()            swap in use\n"
"  .memory.this.used()            this thread's stack; .free .available\n"
               "  .memory.frequency()  .bit()    from SMBIOS; 0 unless root\n"
               "  unit is \"b\" \"kb\" \"mb\" \"gb\" \"tb\"; mb by default\n";
    if (module == "random")
        return "satellite.random\n"
               "  .fast(digits)          also .normal(digits) and .ultra(digits)\n"
               "  .fast.range(low, high) inclusive at both ends; same three tiers\n";
    return std::string();
}

// What a particular value can do. The type table below is the same one
// call_method dispatches on, so a method that exists is a method that is
// listed -- a help text that drifts from the code is worse than none.
//
// That contract is why case 5 exists at all. An instance used to fall to the
// default and be told "nil has no methods", which was survivable while every
// method it had was its spacesuit's own; §8.7 gave it a built-in .size(), and
// the sentence became false about a language-owned method. .size() is on every
// line below for the same reason -- it answers for every receiver that has
// methods at all, so it belongs in every list, not in a footnote.
std::string help_for(const Value &value)
{
    switch (value.index()) {
    case 1:  return "satellite.variable.bool\n"
                    "  .negate()  .and(b)  .or(b)  .to_string()  .size()\n";
    case 2:  return "satellite.variable.number\n"
                    "  .plus(n) .minus(n) .times(n) .divided_by(n) .modulo(n)\n"
                    "  .abs() .floor() .ceil() .round() .to_string()\n"
                    "  .digits() -> decimal digits   .size() -> bytes\n"
                    "  no .length(): a number holds no items\n";
    case 3:  return "satellite.variable.string\n"
                    "  .length() .concat(s) .contains(s) .starts_with(s)\n"
                    "  .ends_with(s) .to_string() .size()  s[i]  s[a:b]\n";
    case 4:  return "satellite.container.list\n"
                    "  .length() .append(v) .first() .last() .contains(v)\n"
                    "  .to_string() .size()                l[i]  l[a:b]\n"
                    "  .search(p)      what is in there, one walk\n"
                    "  .orbit(p)       what orbit thinks it is, five phases\n"
                    "  .settle(p)      and again with its own answer, until\n"
                    "                  the question stops changing\n";
    case 5:  return "a spacesuit instance\n"
                    "  its own satellite.public capsules, which answer first\n"
                    "  .size() -> bytes, unless the spacesuit defines size()\n";
    case 6:  return "satellite.variable.time\n"
                    "  .minus(t) -> nanoseconds   .nanoseconds()  .to_string()\n"
                    "  .size()\n";
    case 9:  return "satellite.variable.binary / satellite.variable.hex\n"
                    "  .digits() -> how many digits, leading zeros included\n"
                    "  .bytes()  -> the packed size, rounded up to a byte\n"
                    "  .to_number() .to_hex() .to_binary() .to_string()\n"
                    "  .concat(v) -> same radix only          .size()\n"
                    "  the width is part of the value: x0009 != x9\n";
    case 7:  return "satellite.variable.file\n"
                    "  .ok() .read() .write(s) .clear() .close() .error() .path()\n"
                    "  .read() is the whole file; .write(s) adds no newline\n"
                    "  .open() reopens after a close; a live handle is left alone\n"
                    "  .size() -> the value's bytes, not the file's\n";
    case 8:  return "satellite.container.map\n"
                    "  .get(k) .set(k, v) .has(k) .remove(k) .length()\n"
                    "  .keys() .values() .to_string() .size()  m[k]\n"
                    "  a key is a string or a number; a missing key is an error\n"
                    "  .search(p) .orbit(p) .settle(p)   as a list answers them\n";
    case 10: return "the arguments object -- satellite.main's parameter\n"
                    "  a.name          a bare selector: a.cxx_compiler\n"
                    "  a[\"name\"] .get(\"name\") .has(\"name\") .names()\n"
                    "  .length() a[i] .first() .last() .contains(v)\n"
                    "                  the COMMAND LINE only, as always\n"
                    "  .count()        every entry, command line and machine\n"
                    "  .to_string()    one entry per line, names beside data\n"
                    "  satellite.help(arguments)   the whole topic\n";
    case 11: return "satellite.container.result -- what .orbit() hands back\n"
                    "  r.value() .key() .path() .why() .phase()\n"
                    "  r.attention()   how many times it is REALLY in there\n"
                    "  r.weight()      0..100, what orbit thinks of it\n"
                    "                  the two disagreeing is the answer\n"
                    "  r.phases()      what each of the five concluded\n"
                    "  r.alternatives()  what else it could have been\n"
                    "  r.lines()       the report, for a person to read\n"
                    "  every field is also a method and a key: r[\"why\"]\n"
                    "  .keys() .values() .length() .get(k) .has(k) .size()\n"
                    "  READ ONLY: a result is what a search concluded\n";
    default: break;
    }
    return "nil has no methods\n";
}

// A language-owned VALUE reached by a module path, as against a language-owned
// FUNCTION. §5 anticipated the shape when it warned not to make a trailing '('
// mandatory after a module path, "or module constants like satellite.math.pi
// become errors".
//
// satellite.bool.true and satellite.bool.false are the two satellite.variable.
// bool values, and they are the only way to write either one. The spelling
// costs no parser rule, no lexer keyword and no second reserved word: segment-1
// dispatch already sends `bool` down the module path like any other name.
//
// It cannot be satellite.variable.bool.true, however much that reads like the
// type it belongs to, because §4 reserves satellite.variable.* as a TYPE
// namespace in which no path is ever a value expression — and that reservation
// is what keeps '<' unambiguous. `bool` as a module segment and `bool` as a
// type name are different things, and keeping them apart is the point.
//
// The obvious alternative, bare `true` and `false`, would be the language's
// second and third reserved words. §1 has exactly one.

} // namespace satellite
