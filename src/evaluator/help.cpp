// satellite.help — the whole language on one screen.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

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
"  topics     random          the three tiers, and what none of them is\n"
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
               "                         .minus(t) and .nanoseconds() on what it hands back\n";
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

// A TOPIC, as against a value or a module.
//
// satellite.help already answered two questions -- "what can this VALUE do"
// and "what does this MODULE answer to" -- and neither of them is the one
// somebody has when they type satellite.help(ultra). That question is about a
// thing the language does, not about a receiver or a namespace, and the honest
// answers to it are longer than a method list and carry a warning a method list
// has nowhere to put.
//
// The topic name is matched BARE (satellite.help(ultra)) and QUOTED
// (satellite.help("ultra")) and dotted (satellite.help(random.ultra)), and all
// three arrive here as the same string. §1 is not weakened by the bare form:
// see the note in src/environment/names.cpp, which is what lets a bare word
// reach this table only after every scope the user owns has said no.
//
// An empty answer means the name is not a topic, which is how the callers tell.
//
// EVERY NUMBER BELOW WAS MEASURED on the machine this was written on, not
// reasoned about. The tier timings come from timing one call of each; the
// concatenation figures from building a million digits and counting them.
std::string help_for_topic(const std::string &topic)
{
    // ONE topic, however it is spelled. `ultra`, `random.ultra` and
    // `satellite.random.ultra` are the same question, and so are `if` and
    // `satellite.statement.if`. The long form is how the language actually
    // writes the path; the short form is what somebody types once they know
    // which one they mean. Answering one spelling and not the other would make
    // the unanswered one look like it named nothing.
    //
    // Stripped HERE, in one place, so no caller has to know: the quoted
    // spelling in src/evaluator/modules.cpp and the bare one in
    // src/evaluator/expr.cpp both arrive with whatever the user wrote.
    //
    // One asymmetry, and it is a property of the grammar rather than an
    // oversight: the control-flow topics can only be reached QUOTED.
    // satellite.help(satellite.statement.if) does not parse at all, because §5
    // dispatches on segment 1 and `statement` has a parse rule of its own -- so
    // the parser sees the beginning of an if STATEMENT where an argument
    // should be. satellite.help("satellite.statement.if") has no such trouble,
    // for §19.1's reason: a string literal was never a name, so it can carry a
    // spelling without that spelling having to parse as one.
    static const std::string kPrefixes[] = {
        "satellite.statement.",
        "satellite.random.",
        "random.",
    };

    std::string key = topic;
    for (const std::string &prefix : kPrefixes) {
        if (key.rfind(prefix, 0) == 0) {
            key = key.substr(prefix.size());
            break;
        }
    }

    if (key == "random")
        return
"satellite.random -- three tiers, and what none of them is\n"
"\n"
"  .fast(digits)          50-100 ms of spin per call\n"
"  .normal(digits)        250-300 ms\n"
"  .ultra(digits)         2000-3000 ms\n"
"  .<tier>.range(lo, hi)  inclusive at BOTH ends\n"
"\n"
"  ultra(40) is uniform over [0, 10^40) -- zero through forty nines. About\n"
"  one draw in ten prints 39 digits or fewer, because a leading zero is not\n"
"  printed and a uniform draw has one a tenth of the time. That is what\n"
"  uniform means; a draw that always printed 40 digits would not be one.\n"
"\n"
"  NOT SECURE, and no tier is. PCG makes no cryptographic claim and its\n"
"  state is recoverable from its output. A longer spin buys a wider spread\n"
"  of possible seeds and nothing else -- roughly 30 bits, measured, whatever\n"
"  the tier. Do not key anything on this.\n"
"\n"
"  satellite.help(ultra)  one tier in full\n"
"  satellite.help(wide)   numbers past the 100,000 digit ceiling\n";

    if (key == "fast")
        return
"satellite.random.fast(digits) -- the short tier\n"
"\n"
"  50-100 ms of throwaway spin per call, then the draw.\n"
"  Uniform over [0, 10^digits). digits is at most 100000.\n"
"  .range(low, high) is inclusive at both ends.\n"
"\n"
"  The spin is a fixed cost per CALL and not per digit.\n"
"  Not secure -- see satellite.help(random).\n";

    if (key == "normal")
        return
"satellite.random.normal(digits) -- the middle tier\n"
"\n"
"  250-300 ms of throwaway spin per call, then the draw.\n"
"  Uniform over [0, 10^digits). digits is at most 100000.\n"
"  .range(low, high) is inclusive at both ends.\n"
"\n"
"  The spin is a fixed cost per CALL and not per digit.\n"
"  Not secure -- see satellite.help(random).\n";

    if (key == "ultra")
        return
"satellite.random.ultra(digits) -- the long tier\n"
"\n"
"  2000-3000 ms of throwaway spin per call, then the draw.\n"
"  Uniform over [0, 10^digits). digits is at most 100000; asking for more\n"
"  is REFUSED and never clamped, because a program that asks for a million\n"
"  digits has made a mistake and a quiet hundred thousand would hide it.\n"
"\n"
"  THE SPIN IS PER CALL, NOT PER DIGIT. Measured here:\n"
"      ultra(1)        2.40 s  ->      1 digit\n"
"      ultra(1000)     2.12 s  ->   1000 digits\n"
"      ultra(100000)   2.13 s  -> 100000 digits\n"
"  One digit costs what a hundred thousand costs. Chunking a wider number\n"
"  out of several calls therefore MULTIPLIES the spin: ten calls is about\n"
"  21 s of spin before any arithmetic. See satellite.help(wide).\n"
"\n"
"  ULTRA IS NOT A SECURITY TIER, and the name is about how long it takes.\n"
"  Every tier seeds from the kernel and then funnels that seed through a\n"
"  32-bit accumulator, keeping about 30 bits of spread -- measured, and the\n"
"  same 30 bits however long the spin runs, because the fold is a moving\n"
"  average and not a sum. A longer spin is a statistical character, not\n"
"  more entropy. PCG state is recoverable from PCG output. Do not key\n"
"  anything on this. The route to a tier that could carry the word is\n"
"  getrandom(2), and it is not taken.\n";

    if (key == "wide")
        return
"random numbers past the 100,000 digit ceiling\n"
"\n"
"  CONCATENATE draws, never add them. Addition carries, it does not append:\n"
"  ten 40-digit draws ADDED make a 41-digit number, not a 400-digit one, and\n"
"  the sum is bell-shaped rather than uniform. Concatenation is\n"
"\n"
"      acc = acc * 10^digits + satellite.random.fast(digits)\n"
"\n"
"  and it IS uniform: each chunk is uniform on [0, 10^digits), so the chunks\n"
"  are the base-10^digits digits of the answer, each one uniform and\n"
"  independent. Verified at a million digits: every digit within 0.023%% of\n"
"  10%%, chi-square 2.62 against 16.9 at p=0.05.\n"
"\n"
"  Cost, measured, ten chunks of 100000 digits:\n"
"      fast    about 18 s     ultra   about 43 s\n"
"  The arithmetic is quadratic in the total width, so widening is not free.\n"
"\n"
"  LENGTH IS NOT STRENGTH. Ten chunks carry about 300 bits of\n"
"  unpredictability however many digits come out. See satellite.help(ultra).\n";

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
                    "  .to_string() .size()                l[i]  l[a:b]\n";
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
                    "  a key is a string or a number; a missing key is an error\n";
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
