// The capsule as a VALUE.
//
// Split from test_capsule.cpp, which asks how a capsule is read out of SOURCE.
// This file asks what a capsule IS once it exists: 16 bytes, refcounted,
// destroyed through the type tag, and one row in each dispatch table.  Two
// questions, and one file holding both had grown past the size ceiling.
#include "satellite/capsule.hpp"
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
using namespace satellite;
static int fails=0, checks=0;
static void check(bool ok,const char* w){++checks; if(!ok){++fails; printf("  FAIL %s\n",w);} }

int main(){
  init_tables();

  // =========================================================================
  // The value model: 16 bytes, refcounted, destroyed through the type tag.
  // =========================================================================
  {
    check(sizeof(Container) == 16, "sizeof(Container) is still 16");
    check(std::string(type_name(Type::Capsule)) == "capsule", "type_name");

    auto* p = new Capsule();
    p->name = "held";
    p->location = "held";
    p->params.push_back(Param{"a", ""});
    Container c = Container::capsule(p);
    check(c.type == Type::Capsule, "Container::capsule builds a capsule container");
    check(c.is_heap(), "a capsule is a heap value -- otherwise it would leak");
    check(c.as_capsule() == p, "as_capsule reaches the payload");
    check(p->rc.load() == 1, "the factory takes the reference, it does not add one");
    check(c.truthy(), "a capsule is truthy");
    check(c.to_string() == "held(a)", "to_string renders the signature");

    {
      std::vector<Container> v;
      for (int k = 0; k < 1000; ++k) v.push_back(c);          // copy IN
      check(p->rc.load() == 1001, "1000 copies -> 1001 references");

      std::vector<Container> w;
      for (const auto& x : v) w.push_back(x);                 // copy OUT
      check(p->rc.load() == 2001, "1000 more -> 2001");

      w.clear();
      check(p->rc.load() == 1001, "clearing one vector released exactly its own");
      v.assign(3, c);
      check(p->rc.load() == 4, "reassigning the vector released the rest");
    }
    check(p->rc.load() == 1, "every copy is gone; back to the one reference");

    Container m = std::move(c);
    check(p->rc.load() == 1, "a move transfers the reference, it does not add one");
    check(c.type == Type::Nil, "and leaves the source nil, so it is never freed twice");

    Container assigned;
    assigned = m;
    check(p->rc.load() == 2, "copy-assignment retains");
    assigned = Container::integer(0);
    check(p->rc.load() == 1, "and assigning over it releases");
    // m goes out of scope here and deletes the Capsule as a Capsule, through
    // the type tag.  Under ASan a wrong tag here is a heap corruption report.
  }

  // =========================================================================
  // Dispatch.  A new type joins the tables as unsupported for arithmetic and
  // comparable for ==, because init_tables fills every cell before it names
  // any.  The one exception is deliberate and predates capsules: strings
  // absorb everything on +.
  // =========================================================================
  {
    auto* p1 = new Capsule(); p1->name = "one";
    auto* p2 = new Capsule(); p2->name = "two";
    Container a = Container::capsule(p1), b = Container::capsule(p2);

    check(add(a, b).type == Type::Nil, "capsule + capsule is unsupported");
    check(sub(a, b).type == Type::Nil, "capsule - capsule is unsupported");
    check(mul(a, b).type == Type::Nil, "capsule * capsule is unsupported");
    check(div(a, b).type == Type::Nil, "capsule / capsule is unsupported");
    check(add(a, Container::integer(1)).type == Type::Nil, "capsule + int is unsupported");
    check(less(a, b).type == Type::Nil, "capsule < capsule is unsupported");
    check(equals(a, b).type == Type::Bool, "but == is defined for every pair");
    check(add(Container::str("f: "), a).to_string() == "f: one()",
          "a string still absorbs a capsule on +, as it absorbs every type");
  }

  // =========================================================================
  // REGRESSION -- equality.  same() reached a capsule through branches that
  // read the union as a number: to_double() answered 0.0 for it, so every
  // capsule compared equal to 0.0, and as_bigint() read the heap POINTER as a
  // magnitude, so `capsule == <that address>` was true and the address was
  // program-visible.  Identity comes first now.
  // =========================================================================
  {
    auto* p = new Capsule(); p->name = "f";
    Container cap  = Container::capsule(p);
    Container alias = cap;                       // the same declaration
    auto* q = new Capsule(); q->name = "g";
    Container other = Container::capsule(q);

    check(equals(cap, alias).b, "a capsule equals itself -- identity, not structure");
    check(!equals(cap, other).b, "two declarations are two capsules");
    check(!equals(cap, Container::real(0.0)).b, "a capsule is not 0.0");
    check(!equals(cap, Container::integer(0)).b, "nor 0");
    check(!equals(cap, Container::big_from_i64((int64_t)(void*)p)).b,
          "nor the integer that happens to be its own address");
    check(!equals(Container::str("f()"), cap).b,
          "nor the string its signature renders to");
    check(equals(cap, alias).type == Type::Bool && equals(cap, other).type == Type::Bool,
          "== still answers Bool for every capsule pair");
  }

  printf("test_capsule_value: %d checks, %d failures\n", checks, fails);
  return fails ? 1 : 0;
}
