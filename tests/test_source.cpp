#include "satellite/source.hpp"
#include <cstdio>
#include <string>
using namespace satellite;
static int fails=0, checks=0;
static void check(bool ok,const char* w){++checks; if(!ok){++fails; printf("  FAIL %s\n",w);} }

int main(){
  // exactly the vector ergonomics that were asked for
  Source s("alpha\nbeta\ngamma\n");
  check(s.size()==3, "3 lines");
  check(s.text_of(1)=="beta", "src.text_of(1) == beta");
  for (const std::string& line : s.lines) (void)line;      // plain range-for

  s.lines.insert(s.lines.begin()+1, "inserted\n");         // vector insert
  check(s.size()==4 && s.text_of(1)=="inserted", "insert_line via vector");
  s.lines.erase(s.lines.begin()+1);                        // vector erase
  check(s.size()==3 && s.text_of(1)=="beta", "erase via vector");
  s.lines[0] = "ALPHA\n";                                  // direct assign
  check(s.text_of(0)=="ALPHA", "assign a line directly");

  // THE correctness rule: newlines are kept, so bytes round-trip exactly
  const std::string orig =
      "satellite.cxx\n{\n    #include <string>\n    std::string s = \"hi\"  \n"
      "    satellite.return(s)\n}\n";
  Source c(orig);
  check(c.all_text()==orig, "all_text() reproduces the file byte for byte");
  check(c.all_text().size()==orig.size(), "byte count identical");

  std::string block = c.span_lines(1,5);
  check(block == "{\n    #include <string>\n    std::string s = \"hi\"  \n"
                 "    satellite.return(s)\n}\n", "cxx block spans lines exactly");
  check(block.find("\"hi\"  \n")!=std::string::npos, "trailing whitespace survives");

  // no trailing newline is remembered
  Source n("abc");
  check(n.size()==1 && n.all_text()=="abc", "file with no trailing newline");
  Source e("");
  check(e.size()==0 && e.all_text()=="", "empty file");
  Source blank("\n\n");
  check(blank.size()==2, "two blank lines");

  // offset -> line/col
  int L=0,C=0;
  Source p("ab\ncdef\n");
  p.locate(0,&L,&C); check(L==1&&C==1,"offset 0 -> 1:1");
  p.locate(3,&L,&C); check(L==2&&C==1,"offset 3 -> 2:1");
  p.locate(5,&L,&C); check(L==2&&C==3,"offset 5 -> 2:3");
  check(p.line_offset(1)==3, "line 1 starts at byte 3");
  check(p.byte_size()==8, "byte_size");

  // SourceMap: stable references while more files load
  SourceMap map;
  SourceId a = map.add_virtual("a.satl","one\ntwo\n");
  const Source& ref = map.get(a);
  for(int k=0;k<500;++k) map.add_virtual("f"+std::to_string(k)+".satl","x\n");
  check(ref.text_of(0)=="one", "reference survives 500 more loads");
  check(map.size()==501, "501 sources");

  SourceId b = map.add_virtual("b.satl","");
  map.get(b).included_from = a;
  check(map.include_chain(b)=="a.satl -> b.satl", "include chain");
  check(map.describe(a,4)=="a.satl:2:1", "describe offset");

  printf("\n%d checks, %d failures\n",checks,fails);
  return fails?1:0;
}
