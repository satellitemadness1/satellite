// satellite.help — the whole language on one screen.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

namespace satellite {

std::string help_overview()
{
    return
"satellite 0.1 -- the whole language\n"
"\n"
"  declare    satellite.variable.number x = 1        bool number string time file\n"
"             satellite.container.list<satellite.variable.number> l\n"
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
"  time       satellite.time.now()                  .minus(t) .nanoseconds()\n"
"  file       satellite.file.open(path, \"read\"|\"write\"|\"append\")\n"
"                 .ok() .read() .write(s) .close() .error() .path()\n"
"  directory  satellite.directory.current()  .change(dir)  .exists(dir)\n"
"  globals    satellite.library.<namespace>.<name>\n"
"\n"
"  help       satellite.help          this text (the () is optional)\n"
"             satellite.help(value)   the methods that value answers to\n";
}

// What a particular value can do. The type table below is the same one
// call_method dispatches on, so a method that exists is a method that is
// listed -- a help text that drifts from the code is worse than none.
std::string help_for(const Value &value)
{
    switch (value.index()) {
    case 1:  return "satellite.variable.bool\n"
                    "  .negate()  .and(b)  .or(b)  .to_string()\n";
    case 2:  return "satellite.variable.number\n"
                    "  .plus(n) .minus(n) .times(n) .divided_by(n) .modulo(n)\n"
                    "  .abs() .floor() .ceil() .round() .to_string()\n";
    case 3:  return "satellite.variable.string\n"
                    "  .length() .concat(s) .contains(s) .starts_with(s)\n"
                    "  .ends_with(s) .to_string()          s[i]  s[a:b]\n";
    case 4:  return "satellite.container.list\n"
                    "  .length() .append(v) .first() .last() .contains(v)\n"
                    "  .to_string()                        l[i]  l[a:b]\n";
    case 6:  return "satellite.variable.time\n"
                    "  .minus(t) -> nanoseconds   .nanoseconds()  .to_string()\n";
    case 7:  return "satellite.variable.file\n"
                    "  .ok() .read() .write(s) .close() .error() .path()\n";
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
