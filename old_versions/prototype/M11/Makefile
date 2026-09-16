# Standalone Makefile for Milestone 11 (M11) in prototype/M11

CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O3 -pthread -I. -I../M8 -I../M7 -I../M6 -I../M5 -I../M4 -I../M3 -I../../src
INCLUDES := -I. -I../M8 -I../M7 -I../M6 -I../M5 -I../M4 -I../M3 -I../../src

OBJS = test.o \
       raw_mode.o \
       keys.o \
       history.o \
       editor.o \
       render.o \
       line_reader.o \
       block_scan.o \
       repl.o \
       search.o \
       search_walk.o \
       search_apply.o \
       interrupt.o \
       dispatch.o \
       runtime.o \
       compiler.o \
       closure.o \
       evaluator.o \
       console.o \
       value.o \
       number.o \
       resolver.o \
       resolver_scopes.o \
       resolver_suits.o \
       resolver_names.o \
       resolver_walk.o \
       diagnostic_code.o \
       diagnostic.o \
       suggester.o \
       source_view.o \
       renderer.o \
       reporter.o \
       lexer_parser_bridge.o \
       ast.o \
       unparse.o \
       parser.o \
       parser_decl.o \
       parser_stmt.o \
       parser_expr.o \
       lexer.o \
       satellite_string.o \
       host_facts.o \
       memory_facts.o \
       stack_facts.o

TARGET = test_m11

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

console.o: ../M8/console.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

value.o: ../M7/value.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

number.o: ../M7/number.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

resolver.o: ../M6/resolver.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

resolver_scopes.o: ../M6/resolver_scopes.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

resolver_suits.o: ../M6/resolver_suits.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

resolver_names.o: ../M6/resolver_names.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

resolver_walk.o: ../M6/resolver_walk.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

diagnostic_code.o: ../M5/diagnostic_code.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

diagnostic.o: ../M5/diagnostic.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

suggester.o: ../M5/suggester.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

source_view.o: ../M5/source_view.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

renderer.o: ../M5/renderer.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

reporter.o: ../M5/reporter.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

lexer_parser_bridge.o: ../M5/lexer_parser_bridge.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

ast.o: ../M4/ast.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

unparse.o: ../M4/unparse.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

parser.o: ../M4/parser.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

parser_decl.o: ../M4/parser_decl.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

parser_stmt.o: ../M4/parser_stmt.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

parser_expr.o: ../M4/parser_expr.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

lexer.o: ../M3/lexer.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

satellite_string.o: ../../src/satellite_string/satellite_string.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

host_facts.o: ../../src/system_facts/host_facts.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

memory_facts.o: ../../src/system_facts/memory_facts.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

stack_facts.o: ../../src/system_facts/stack_facts.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
