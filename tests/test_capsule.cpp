#include "satellite/capsule.hpp"
#include "satellite/cxx.hpp"
#include "satellite/library.hpp"
#include "satellite/source.hpp"
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
using namespace satellite;
static int fails=0, checks=0;
static void check(bool ok,const char* w){++checks; if(!ok){++fails; printf("  FAIL %s\n",w);} }

static std::vector<Token> lex(const std::string& src){ Lexer lx(src); return lx.scan(); }
static CapsuleScan scan(const std::string& src, SourceId sid = kNoSource) {
  auto t = lex(src);
  return scan_capsules(t, sid);
}
static bool mentions(const std::vector<Diagnostic>& ds, const char* what) {
  for (const auto& d : ds) if (d.message.find(what) != std::string::npos) return true;
  return false;
}

int main(){
  init_tables();

  // =========================================================================
  // The entry point.  satellite.main takes no arguments here.
  // =========================================================================
  {
    CapsuleScan s = scan("satellite.capsule satellite.main()\n{\n}\n");
    check(s.ok, "a well-formed capsule scans without diagnostics");
    check(s.capsules.size() == 1, "one capsule found");
    const Capsule* c = s.capsules[0].as_capsule();
    check(s.capsules[0].type == Type::Capsule, "it arrives as a Type::Capsule container");
    check(c->name == "main", "named main -- the bare name, not satellite.main");
    check(c->is_main, "satellite.main is the entry point");
    check(c->arity() == 0, "arity 0");
    check(c->params.empty(), "and no parameter names");
    check(c->signature() == "satellite.main()", "signature spells the required form");
  }

  // =========================================================================
  // Parameters are NAMES.  Nothing here is a value.
  // =========================================================================
  {
    CapsuleScan s = scan("satellite.capsule satellite.main(argv)\n{\n}\n");
    check(s.ok && s.capsules.size() == 1, "main(argv) scans");
    const Capsule* c = s.capsules[0].as_capsule();
    check(c->arity() == 1, "arity 1");
    check(c->params[0].name == "argv", "the parameter is named argv");
    check(c->params[0].declared_type.empty(), "and is untyped");
    check(c->signature() == "satellite.main(argv)", "signature");
  }
  {
    CapsuleScan s = scan("satellite.capsule helper(a, b)\n{\n}\n");
    check(s.ok && s.capsules.size() == 1, "helper(a, b) scans");
    const Capsule* c = s.capsules[0].as_capsule();
    check(!c->is_main, "helper is not the entry point");
    check(c->arity() == 2, "arity 2");
    check(c->params[0].name == "a" && c->params[1].name == "b", "both names, in order");
    check(c->signature() == "helper(a, b)", "signature carries no satellite. prefix");
    check(c->name == "helper" && c->location == "helper",
          "its canonical location is its bare name -- satellite.library.helper.<var>");
  }
  {
    // A capsule named main WITHOUT the satellite. prefix is a capsule that
    // happens to be called main.  find_main() does not accept it either.
    CapsuleScan s = scan("satellite.capsule main()\n{\n}\n");
    check(s.ok && s.capsules.size() == 1, "a bare main() is still a capsule");
    check(!s.capsules[0].as_capsule()->is_main, "but it is not the entry point");
  }

  // =========================================================================
  // Typed parameters: satellite.variable.<type> name
  // =========================================================================
  {
    CapsuleScan s = scan("satellite.capsule stats(satellite.variable.int n, "
                         "satellite.variable.string label, raw)\n{\n}\n");
    check(s.ok && s.capsules.size() == 1, "mixed typed and untyped parameters scan");
    const Capsule* c = s.capsules[0].as_capsule();
    check(c->arity() == 3, "arity 3");
    check(c->params[0].name == "n" && c->params[0].declared_type == "int",
          "satellite.variable.int n -> name n, type int");
    check(c->params[1].name == "label" && c->params[1].declared_type == "string",
          "satellite.variable.string label -> name label, type string");
    check(c->params[2].name == "raw" && c->params[2].declared_type.empty(),
          "raw stays untyped");
    check(c->signature() == "stats(int n, string label, raw)", "signature shows the types");
  }
  {
    // The lexer suppresses the newline inside ( ), so a parameter list may span
    // as many lines as it likes.
    CapsuleScan s = scan("satellite.capsule helper(\n    a,\n    b)\n{\n}\n");
    check(s.ok && s.capsules.size() == 1, "a parameter list may span lines");
    const Capsule* c = s.capsules[0].as_capsule();
    check(c->arity() == 2, "both parameters read across the newlines");
    check(c->body_first == 4 && c->body_last == 5, "and the body still measures right");
  }

  // =========================================================================
  // The body is a SPAN.  1-based lines, both ends inclusive.
  // =========================================================================
  {
    CapsuleScan s = scan("satellite.include(satellite)\n"       // 1
                         "\n"                                   // 2
                         "satellite.capsule satellite.main()\n" // 3
                         "{\n"                                  // 4
                         "    satellite.console.display(\"hi\")\n" // 5
                         "}\n"                                  // 6
                         "\n"                                   // 7
                         "satellite.capsule helper(a)\n"        // 8
                         "{\n"                                  // 9
                         "    a = a + 1\n"                      // 10
                         "}\n");                                // 11
    check(s.ok && s.capsules.size() == 2, "two capsules in one file");
    const Capsule* m = s.capsules[0].as_capsule();
    const Capsule* h = s.capsules[1].as_capsule();
    check(m->header_line == 3, "main's header line");
    check(m->body_first == 4 && m->body_last == 6, "main's body spans lines 4..6");
    check(h->header_line == 8, "helper's header line");
    check(h->body_first == 9 && h->body_last == 11, "helper's body spans lines 9..11");
    check(m->is_main && !h->is_main, "only one of them is the entry point");
  }

  // =========================================================================
  // Brace counting: nested braces, cxx blocks, strings.
  // =========================================================================
  {
    CapsuleScan s = scan("satellite.capsule outer()\n"   // 1
                         "{\n"                           // 2
                         "    satellite.if (x)\n"        // 3
                         "    {\n"                       // 4
                         "        y = 1\n"               // 5
                         "    }\n"                       // 6
                         "}\n");                         // 7
    check(s.ok && s.capsules.size() == 1, "a body with nested braces scans");
    const Capsule* c = s.capsules[0].as_capsule();
    check(c->body_first == 2 && c->body_last == 7,
          "measured to the OUTER brace, not the inner one");
  }
  {
    // Tok::CxxBlock is ONE token: scan_cxx_block already ate its own braces,
    // including the ones inside C++ strings and comments.
    CapsuleScan s = scan("satellite.capsule c()\n"                    // 1
                         "{\n"                                        // 2
                         "    satellite.cxx {\n"                      // 3
                         "        if (true) { int x = 1; }\n"         // 4
                         "        // }  a brace in a C++ comment\n"   // 5
                         "        const char* s = \"}\";\n"           // 6
                         "    }\n"                                    // 7
                         "}\n");                                      // 8
    check(s.ok && s.capsules.size() == 1, "a body holding a satellite.cxx block scans");
    const Capsule* c = s.capsules[0].as_capsule();
    check(c->body_first == 2 && c->body_last == 8,
          "the cxx block cannot unbalance the capsule's braces");
  }
  {
    CapsuleScan s = scan("satellite.capsule c()\n"                          // 1
                         "{\n"                                              // 2
                         "    satellite.variable.string s = \"}{}{\"\n"     // 3
                         "    satellite.console.display(\"{\")\n"           // 4
                         "}\n");                                            // 5
    check(s.ok && s.capsules.size() == 1, "a body holding braces in string literals scans");
    check(s.capsules[0].as_capsule()->body_last == 5,
          "a brace inside a string literal never reaches the token stream");
  }

  // =========================================================================
  // The token extent, for the parser that comes next.
  // =========================================================================
  {
    std::string src = "satellite.capsule helper(a)\n{\n    a = 1\n}\n";
    auto t = lex(src);
    CapsuleScan s = scan_capsules(t);
    check(s.capsules.size() == 1, "one capsule");
    const Capsule* c = s.capsules[0].as_capsule();
    check(t[c->first_token].kind == Tok::Satellite, "first_token opens the header");
    check(t[c->last_token].kind == Tok::RBrace, "last_token is the closing brace");
    check(c->first_token < c->last_token, "and the extent runs forward");
  }

  // =========================================================================
  // The body materializes from the span, byte for byte.  This is the whole
  // reason the body is not copied at scan time.
  // =========================================================================
  {
    SourceMap map;
    const std::string text = "satellite.capsule helper(a)\n{\n    a = a + 1\n}\n";
    SourceId sid = map.add_virtual("helper.satl", text);

    auto t = lex(text);
    CapsuleScan s = scan_capsules(t, sid);
    check(s.ok && s.capsules.size() == 1, "scanned with a source id");
    const Capsule* c = s.capsules[0].as_capsule();
    check(c->source_id == sid, "the capsule remembers which file it came from");

    // lines are 1-based; span_lines indexes the vector
    std::string body = map.get(sid).span_lines(c->body_first - 1, c->body_last - 1);
    check(body == "{\n    a = a + 1\n}\n", "the body comes back exactly, newlines intact");
  }

  // =========================================================================
  // Neither a comment nor a string literal can declare a capsule.
  // =========================================================================
  {
    CapsuleScan s = scan("// satellite.capsule fake()\n"
                         "satellite.variable.string s = \"satellite.capsule ghost() {}\"\n");
    check(s.ok, "a file with no capsules is not an error");
    check(s.capsules.empty(), "a commented-out or quoted capsule is not a declaration");
  }
  {
    CapsuleScan s = scan("");
    check(s.ok, "an empty file scans cleanly");
    check(s.capsules.empty() && s.diagnostics.empty(), "and yields nothing");
  }

  // =========================================================================
  // Malformed declarations are DIAGNOSTICS.  Never a crash, never an assert.
  // =========================================================================
  {
    CapsuleScan s = scan("satellite.capsule helper()\n{\n    x = 1\n");
    check(!s.ok, "an unterminated body is an error");
    check(s.diagnostics.size() == 1, "one diagnostic");
    check(mentions(s.diagnostics, "unterminated"), "and it says so");
    check(s.diagnostics[0].line == 1, "pointing at the declaration");
    check(s.capsules.empty(), "an unterminated capsule is not registered");
  }
  {
    CapsuleScan s = scan("satellite.capsule helper()\n{\n}\n"
                         "satellite.capsule helper(a)\n{\n}\n");
    check(!s.ok, "a duplicate capsule name is an error");
    check(mentions(s.diagnostics, "already declared"), "and it says so");
    check(s.capsules.size() == 1, "the first declaration is the one that stands");
    check(s.capsules[0].as_capsule()->arity() == 0, "and it is the first one, not the second");
  }
  {
    CapsuleScan s = scan("satellite.capsule helper\n{\n}\n");
    check(!s.ok, "a header with no parameter list is an error");
    check(mentions(s.diagnostics, "no parameter list"), "and it says so");
    check(s.capsules.empty(), "nothing is registered from it");
  }
  {
    CapsuleScan s = scan("satellite.capsule helper()\nsatellite.variable.int x = 1\n");
    check(!s.ok, "a header with no body is an error");
    check(mentions(s.diagnostics, "no body"), "and it says so");
    check(s.capsules.empty(), "nothing is registered from it");
  }
  {
    CapsuleScan s = scan("satellite.capsule ()\n{\n}\n");
    check(!s.ok && s.capsules.empty(), "a header with no name is an error");
    check(mentions(s.diagnostics, "capsule name"), "and it says so");
  }
  {
    CapsuleScan s = scan("satellite.capsule helper(a b)\n{\n}\n");
    check(!s.ok && s.capsules.empty(), "a parameter list missing its comma is an error");
  }
  {
    // One bad capsule must not hide the good ones that follow it.
    CapsuleScan s = scan("satellite.capsule bad\n{\n}\n"
                         "satellite.capsule good(a)\n{\n}\n");
    check(!s.ok, "the file has an error");
    check(s.capsules.size() == 1 && s.capsules[0].as_capsule()->name == "good",
          "and scanning continued past it");
  }

  // =========================================================================
  // Registration.  satellite.library.main IS the capsule; its variables hang
  // off it as satellite.library.main.<var>.
  // =========================================================================
  {
    Library lib;
    CapsuleScan s = scan("satellite.capsule satellite.main()\n{\n}\n"
                         "satellite.capsule helper(a, b)\n{\n}\n");
    std::vector<Diagnostic> ds;
    check(register_capsules(lib, s, &ds), "registration succeeds");
    check(ds.empty(), "with no diagnostics");
    check(lib.size() == 2, "two names defined");

    Slot m = lib.slot_of(kGlobal, "main");
    check(m != kNoSlot, "main is in the library");
    check(lib.kind_of(m) == Kind::Capsule, "as a capsule, not a variable");
    check(lib.slot_of_path("satellite.library.main") == m,
          "and satellite.library.main reaches it");
    check(lib.kind_of(lib.slot_of(kGlobal, "helper")) == Kind::Capsule, "so is helper");

    Container v;
    check(lib.get_path("satellite.library.main", &v), "read it back");
    check(v.type == Type::Capsule && v.as_capsule()->is_main, "it is the capsule itself");

    // the hierarchy the global location buys
    lib.define("main", "counter", Container::integer(7));
    check(lib.get_path("satellite.library.main.counter", &v) && v.i == 7,
          "a variable inside main is satellite.library.main.counter");
    check(lib.kind_of(lib.slot_of("main", "counter")) == Kind::Variable,
          "and it is a variable, one level below the capsule");
  }
  {
    Library lib;
    lib.define(kGlobal, "helper", Container::integer(1));      // a variable first
    CapsuleScan s = scan("satellite.capsule helper()\n{\n}\n");
    std::vector<Diagnostic> ds;
    check(!register_capsules(lib, s, &ds), "a name that is already a variable collides");
    check(ds.size() == 1 && mentions(ds, "collides"), "and reports it");
    check(lib.kind_of(lib.slot_of(kGlobal, "helper")) == Kind::Variable,
          "the existing variable is not overwritten");
    check(register_capsules(lib, s, nullptr) == false, "a null diagnostic sink is fine");
  }
  {
    // Re-registering a capsule over a capsule is the Library's ordinary
    // redefine: same slot, so handles already handed out stay valid.
    Library lib;
    CapsuleScan s = scan("satellite.capsule helper()\n{\n}\n");
    check(register_capsules(lib, s, nullptr), "first registration");
    Slot a = lib.slot_of(kGlobal, "helper");
    check(register_capsules(lib, s, nullptr), "second registration");
    check(lib.slot_of(kGlobal, "helper") == a, "keeps the same slot");
    check(lib.size() == 1, "and does not duplicate the name");
  }


  // =========================================================================
  // REGRESSION -- a comment between `satellite.cxx` and its '{'.
  //
  // The lexer's lookahead skipped whitespace only, so a comment there hid the
  // '{' and the C++ was lexed as satellite.  Both consequences were silent.
  // The body span shrank, because a '}' the C++ hid inside a char literal is
  // not hidden from the satellite lexer and reached the brace counter; and
  // when the C++ happened to lex cleanly the block just stopped being a block,
  // with no diagnostic from anyone.  This is what makes the exactness claim in
  // take()'s brace loop true, so it is tested from both ends: the token stream
  // and the materialized span.
  // =========================================================================
  {
    const char* gaps[] = {"  // build the key", " /* build the key */", ""};
    const char* what[] = {"line comment", "block comment", "nothing at all"};
    for (int g = 0; g < 3; ++g) {
      SourceMap map;
      std::string text =
          std::string("satellite.capsule f()\n")            // 1
                    + "{\n"                                 // 2
                    + "    satellite.cxx" + gaps[g] + "\n"  // 3
                    + "    {\n"                             // 4
                    + "        char c = '}';\n"             // 5
                    + "        int n = 1;\n"                // 6
                    + "    }\n"                             // 7
                    + "    a = 1\n"                         // 8
                    + "}\n";                                // 9
      SourceId sid = map.add_virtual("gap.satl", text);

      Lexer lx(text);
      std::vector<Token> t = lx.scan();
      check(lx.ok(), (std::string("a ") + what[g] + " before the brace lexes cleanly").c_str());

      size_t blocks = 0;
      for (const Token& tk : t) if (tk.kind == Tok::CxxBlock) ++blocks;
      check(blocks == 1, (std::string("the block survives a ") + what[g]).c_str());

      CapsuleScan s = scan_capsules(t, sid);
      check(s.ok && s.capsules.size() == 1, "the capsule scans");
      const Capsule* c = s.capsules[0].as_capsule();
      check(c->body_first == 2, "body opens on line 2");
      check(c->body_last == 9,
            (std::string("and closes on line 9 -- the '}' in the C++ char literal "
                         "is not the capsule's, past a ") + what[g]).c_str());
      std::string body = map.get(sid).span_lines(c->body_first - 1, c->body_last - 1);
      check(body.find("a = 1") != std::string::npos,
            "the span still reaches the statement after the block");
      check(body.size() == text.size() - std::string("satellite.capsule f()\n").size(),
            "the span is the whole body, byte for byte");
    }
  }
  {
    // The gap skip must not swallow a '/' that is not a comment, or eat a
    // whole file looking for a brace that was never coming.
    Lexer lx("x = satellite.cxx / 2\n");
    std::vector<Token> t = lx.scan();
    size_t blocks = 0, slashes = 0;
    for (const Token& tk : t) {
      if (tk.kind == Tok::CxxBlock) ++blocks;
      if (tk.kind == Tok::Slash)    ++slashes;
    }
    check(blocks == 0 && slashes == 1, "a division after satellite.cxx is still division");
  }
  {
    Lexer lx("satellite.cxx /* never closed\n{\n}\n");
    lx.scan();
    check(!lx.ok(), "an unterminated comment in the gap is still reported");
  }

  // =========================================================================
  // REGRESSION -- the dispatch tables are [TCOUNT][TCOUNT] and the operators
  // that index them are inline in container.hpp, so a satellite.cxx module
  // compiles the ROW STRIDE into itself.  Adding Type::Capsule moved that
  // stride.  The cache is keyed by a hash of the generated unit, and nothing
  // in a block's own text changes when Type grows -- so a module built before
  // this change was served straight back out of the cache and indexed the new
  // tables at the old stride, reaching a different cell.  generate() now
  // stamps the contract into the unit, which is what moves the key.
  //
  // This lives here rather than in test_cxx because Type::Capsule is what
  // moved the stride; the test is a guard on the next type to be added.
  // =========================================================================
  {
    std::string stamp = "\n// contract: abi " + std::to_string(cxx::ABI) + ", " +
                        std::to_string(TCOUNT) + " types, container " +
                        std::to_string(sizeof(Container)) + " bytes\n";
    std::string unit = cxx::generate("satellite.return(1)\n");
    check(unit.find(stamp) != std::string::npos,
          "the generated unit stamps the live ABI, type count and container size");
    check(unit.find(stamp) < unit.find("#include"),
          "on its own comment line above the code, so it changes the hash and nothing else");
    check(unit == cxx::generate("satellite.return(1)\n"),
          "and the stamp is deterministic -- a varying one would defeat the cache");

    // The backstop for a module that lands on the current key anyway.  The
    // symbol has to report the ABI of the headers the module COMPILED against,
    // so it must name the constant; writing its value here would report the
    // host that generated the text, which is never the stale half.
    check(unit.find("satellite_cxx_abi() { return satellite::cxx::ABI; }") !=
              std::string::npos,
          "the module reports the ABI it was compiled against, not a baked-in number");
  }

  printf("test_capsule: %d checks, %d failures\n", checks, fails);
  return fails ? 1 : 0;
}
