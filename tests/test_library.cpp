#include "satellite/library.hpp"
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
using namespace satellite;
static int fails=0, checks=0;
static void check(bool ok,const char* w){++checks; if(!ok){++fails; printf("  FAIL %s\n",w);} }

static std::vector<Token> lex(const std::string& src){ Lexer lx(src); return lx.scan(); }

int main(){
  init_tables();

  // =========================================================================
  // The call that was asked for: two strings, location then name.
  // =========================================================================
  {
    Library lib;
    Slot s = lib.define("satellite.main", "counter", Container::integer(41));
    check(s != kNoSlot, "define(\"satellite.main\", \"counter\") returns a slot");

    Container v;
    check(lib.get("satellite.main", "counter", &v), "get by two strings");
    check(v.type == Type::Int && v.i == 41, "value came back");
    check(lib.has("satellite.main", "counter"), "has()");
    check(!lib.has("satellite.main", "nothing"), "has() is false for a missing name");
    check(lib.slot_of("satellite.main", "counter") == s, "slot_of matches define");
    check(lib.slot_of("satellite.main", "nope") == kNoSlot, "missing name -> kNoSlot");
  }

  // =========================================================================
  // Every spelling of the same variable reaches the same slot.
  // =========================================================================
  {
    Library lib;
    Slot s = lib.define("satellite.main", "counter", Container::integer(7));

    check(lib.slot_of_path("satellite.library.main.counter") == s,
          "satellite.library.main.counter        (as satellite source spells it)");
    check(lib.slot_of_path("satellite.library.satellite.main.counter") == s,
          "satellite.library.satellite.main.counter  (main spelled in full)");
    check(lib.slot_of_path("satellite.main.counter") == s,
          "satellite.main.counter                (location + name)");
    check(lib.slot_of_path("main.counter") == s,
          "main.counter                          (canonical)");
    check(lib.slot_of("main", "counter") == s,
          "two strings, location unprefixed");
  }

  // =========================================================================
  // Path splitting -- the LAST dot separates the name.
  // =========================================================================
  {
    std::string loc, nm;
    check(Library::split_path("satellite.library.main.counter", &loc, &nm)
          && loc=="main" && nm=="counter", "split: prefix stripped");
    check(Library::split_path("helper.total", &loc, &nm)
          && loc=="helper" && nm=="total", "split: bare location.name");
    check(Library::split_path("loose", &loc, &nm)
          && loc=="" && nm=="loose", "split: no dot -> global location");

    // THE ESCAPE HATCH: satellite used as an ordinary variable name.
    check(Library::split_path("satellite.library.helper.satellite", &loc, &nm)
          && loc=="helper" && nm=="satellite",
          "escape hatch: ...helper.satellite -> name is \"satellite\"");

    check(!Library::split_path("", &loc, &nm), "split: empty is not a path");
    check(!Library::split_path("main.", &loc, &nm), "split: trailing dot rejected");
    check(!Library::split_path("satellite.library.", &loc, &nm), "split: bare prefix rejected");

    check(Library::normalize_location("satellite.main")=="main", "normalize satellite.main");
    check(Library::normalize_location("main")=="main", "normalize main");
    check(Library::normalize_location("satellite")=="satellite",
          "a capsule literally named satellite survives normalize");
    check(Library::join_path("satellite.main","x")=="main.x", "join_path");
    check(Library::join_path("","x")=="x", "join_path at global");
    check(Library::display_path("satellite.main","x")=="satellite.library.main.x",
          "display_path is what the user typed");
  }

  // =========================================================================
  // set() assigns; it never creates.  That is what catches a typo.
  // =========================================================================
  {
    Library lib;
    lib.define("main", "total", Container::integer(0));
    check(lib.set("main", "total", Container::integer(99)), "set an existing name");
    Container v; lib.get("main","total",&v);
    check(v.i == 99, "set overwrote the value");

    check(!lib.set("main", "totl", Container::integer(1)),
          "set on a MISSING name fails -- a typo is not a new variable");
    check(!lib.has("main","totl"), "and did not create it");
    check(!lib.set_path("satellite.library.main.mispelled", Container::integer(1)),
          "set_path on a missing name fails too");
  }

  // =========================================================================
  // Redefining keeps the slot, so handles already handed out stay right.
  // =========================================================================
  {
    Library lib;
    Slot s1 = lib.define("main", "x", Container::integer(1));
    Slot s2 = lib.define("main", "x", Container::str("now a string"));
    check(s1 == s2, "redefine returns the SAME slot");
    Container v; lib.read(s1, &v);
    check(v.type == Type::Str, "and the old handle sees the new value");
    check(lib.size() == 1, "still one name");
  }

  // =========================================================================
  // Slots: the fast path.  No string, no hash.
  // =========================================================================
  {
    Library lib;
    Slot s = lib.define("main", "n", Container::integer(5));
    Container v;
    check(lib.read(s, &v) && v.i == 5, "read(slot)");
    check(lib.write(s, Container::integer(6)), "write(slot)");
    check(lib.read(s, &v) && v.i == 6, "write took effect");
    check(!lib.read(kNoSlot, &v), "kNoSlot never reads");
    check(!lib.read(99999, &v), "out-of-range slot never reads");
    check(!lib.write(99999, Container::integer(0)), "out-of-range slot never writes");
  }

  // =========================================================================
  // address() stays valid as the library grows.  This is what lets the VM
  // hold raw pointers and take no lock per access.
  // =========================================================================
  {
    Library lib;
    Slot first = lib.define("main", "anchor", Container::integer(1234));
    Container* p = lib.address(first);
    check(p != nullptr, "address() of a live slot");

    for (int k = 0; k < 5000; ++k)
      lib.define("main", "v" + std::to_string(k), Container::integer(k));

    check(p == lib.address(first), "address unchanged after 5000 more defines");
    check(p->type == Type::Int && p->i == 1234, "and still reads correctly");
    *p = Container::integer(4321);
    Container v; lib.get("main","anchor",&v);
    check(v.i == 4321, "writing through the pointer is visible by name");
    check(lib.address(kNoSlot) == nullptr, "address(kNoSlot) is null");
    check(lib.size() == 5001, "5001 names");
  }

  // =========================================================================
  // Every satellite type survives a round trip.
  // =========================================================================
  {
    Library lib;
    lib.define("main","a",Container::nil());
    lib.define("main","b",Container::boolean(true));
    lib.define("main","c",Container::integer(-9));
    lib.define("main","d",Container::real(2.5));
    lib.define("main","e",Container::str("hello"));
    lib.define("main","f",Container::big_from_i64(1));
    lib.define("main","g",Container::list());

    Container v;
    lib.get("main","a",&v); check(v.type==Type::Nil,  "nil round trip");
    lib.get("main","b",&v); check(v.type==Type::Bool && v.b, "bool round trip");
    lib.get("main","c",&v); check(v.type==Type::Int && v.i==-9, "int round trip");
    lib.get("main","d",&v); check(v.type==Type::Real && v.d==2.5, "real round trip");
    lib.get("main","e",&v); check(v.type==Type::Str && v.to_string()=="hello", "string round trip");
    lib.get("main","f",&v); check(v.type==Type::Big || v.type==Type::Int, "bignum round trip");
    lib.get("main","g",&v); check(v.type==Type::List, "list round trip");

    // a string in the library is refcounted, not copied byte by byte
    Container s = Container::str("shared text");
    lib.define("main","s1",s);
    lib.define("main","s2",s);
    Container r1,r2; lib.get("main","s1",&r1); lib.get("main","s2",&r2);
    check(r1.obj == r2.obj, "two names share one string object (refcounted)");
  }

  // =========================================================================
  // Locations are separate namespaces.
  // =========================================================================
  {
    Library lib;
    lib.define("satellite.main", "x", Container::integer(1));
    lib.define("helper",         "x", Container::integer(2));
    lib.define(kGlobal,          "x", Container::integer(3));

    Container v;
    lib.get("main","x",&v);    check(v.i==1, "main.x");
    lib.get("helper","x",&v);  check(v.i==2, "helper.x is a different variable");
    lib.get("","x",&v);        check(v.i==3, "global x is a third");
    check(lib.size()==3, "three distinct names");

    auto locs = lib.locations();
    check(locs.size()==3, "three locations");
    check(locs[0]=="" && locs[1]=="helper" && locs[2]=="main", "locations sorted");

    lib.define("main","a",Container::nil());
    auto names = lib.names_in("satellite.main");
    check(names.size()==2 && names[0]=="a" && names[1]=="x", "names_in sorted");
    check(lib.names_in("nowhere").empty(), "names_in on an unknown location is empty");
  }

  // =========================================================================
  // Kinds -- main is a capsule, not a variable that happens to be called main.
  // =========================================================================
  {
    Library lib;
    Slot m = lib.define("satellite","main",Container::nil(),Kind::Capsule);
    lib.define("main","count",Container::integer(0));
    lib.define(kGlobal,"Ship",Container::nil(),Kind::Spacesuit);
    check(lib.kind_of(m)==Kind::Capsule, "main registered as a capsule");
    check(lib.kind_of(lib.slot_of("main","count"))==Kind::Variable, "count is a variable");
    check(lib.kind_of(lib.slot_of("","Ship"))==Kind::Spacesuit, "Ship is a spacesuit");

    Entry e;
    check(lib.describe(m,&e), "describe a slot");
    check(e.location=="satellite" && e.name=="main", "entry records where it lives");
    check(e.display()=="satellite.library.satellite.main", "entry prints its own path");
    check(!lib.describe(kNoSlot,&e), "describe(kNoSlot) fails");
    check(std::string(kind_name(Kind::Capsule))=="capsule", "kind_name");
  }

  // =========================================================================
  // erase unhooks the name but never recycles the slot.
  // =========================================================================
  {
    Library lib;
    Slot a = lib.define("main","gone",Container::integer(1));
    check(lib.erase("main","gone"), "erase an existing name");
    check(!lib.has("main","gone"), "name is gone");
    check(!lib.erase("main","gone"), "erasing twice fails");
    check(lib.size()==0, "size counts live names");

    Slot b = lib.define("main","fresh",Container::integer(2));
    check(b != a, "a new name never REUSES the erased slot");
    Container v;
    check(lib.read(a,&v) && v.type==Type::Nil, "the old slot still reads, as nil");

    lib.clear();
    check(lib.size()==0 && lib.locations().empty(), "clear empties the library");
  }

  // =========================================================================
  // dump() -- what `satellite.library` looks like to a human.
  // =========================================================================
  {
    Library lib;
    lib.define("satellite.main","counter",Container::integer(42));
    lib.define("satellite.main","name",Container::str("probe"));
    std::string d = lib.dump();
    check(d.find("satellite.library.main")!=std::string::npos, "dump names the location");
    check(d.find("counter = 42")!=std::string::npos, "dump shows the value");
    check(d.find("[str")!=std::string::npos || d.find("[string")!=std::string::npos,
          "dump shows the type");
  }

  // =========================================================================
  // find_main -- the entry point of every app.
  // =========================================================================
  {
    auto t = lex("satellite.include(satellite)\n"
                 "satellite.capsule satellite.main()\n{\n}\n");
    MainLocation m = find_main(t);
    check(m.found, "find_main finds satellite.capsule satellite.main");
    check(m.line == 2, "and reports the right line");
    check(m.has_parens, "and sees the parameter list");
  }
  {
    auto t = lex("satellite.include(satellite)\nsatellite.capsule helper()\n{\n}\n");
    check(!find_main(t).found, "a file with only a helper capsule has no main");
  }
  {
    // Neither comments nor string literals reach the token stream, so neither
    // can fake an entry point.
    auto t = lex("// satellite.capsule satellite.main()\n"
                 "satellite.variable.string s = \"satellite.capsule satellite.main()\"\n");
    check(!find_main(t).found, "a commented-out or quoted main is not an entry point");
  }
  {
    auto t = lex("satellite.capsule satellite.main\n");
    MainLocation m = find_main(t);
    check(m.found && !m.has_parens, "main without parens is found but flagged");
  }
  {
    auto t = lex("satellite.capsule satellite.mainly()\n");
    check(!find_main(t).found, "satellite.mainly is not satellite.main");
  }
  {
    auto t = lex("");
    check(!find_main(t).found, "empty source has no main");
  }
  {
    // main declared after other code is still main
    auto t = lex("satellite.variable.int x = 1\n"
                 "satellite.capsule alpha()\n{\n}\n"
                 "satellite.capsule satellite.main()\n{\n}\n");
    MainLocation m = find_main(t);
    check(m.found && m.line == 5, "main found further down the file");
  }

  // =========================================================================
  // Concurrent use -- the model this language is built for.
  // =========================================================================
  {
    Library lib;
    const int kThreads = 20, kEach = 200;

    std::vector<std::thread> ts;
    for (int t = 0; t < kThreads; ++t)
      ts.emplace_back([&lib, t]{
        for (int k = 0; k < kEach; ++k)
          lib.define("worker" + std::to_string(t), "v" + std::to_string(k),
                     Container::integer(t * 1000 + k));
      });
    for (auto& th : ts) th.join();

    check(lib.size() == (size_t)(kThreads * kEach), "20 threads defined 4000 names, none lost");

    bool all_right = true;
    for (int t = 0; t < kThreads; ++t)
      for (int k = 0; k < kEach; ++k) {
        Container v;
        if (!lib.get("worker" + std::to_string(t), "v" + std::to_string(k), &v)
            || v.i != t * 1000 + k) { all_right = false; break; }
      }
    check(all_right, "every value survived concurrent definition");

    // concurrent readers against a writer, all through resolved slots
    Slot s = lib.define("main","shared",Container::integer(0));
    std::vector<std::thread> rs;
    for (int t = 0; t < 8; ++t)
      rs.emplace_back([&lib, s]{
        for (int k = 0; k < 2000; ++k) { Container v; lib.read(s, &v); }
      });
    for (int k = 0; k < 2000; ++k) lib.write(s, Container::integer(k));
    for (auto& th : rs) th.join();
    Container v; lib.read(s,&v);
    check(v.type == Type::Int, "8 readers + 1 writer on one slot stayed consistent");
  }

  printf("test_library: %d checks, %d failures\n", checks, fails);
  return fails ? 1 : 0;
}
