# Standalone Makefile for Milestone 10 (M10) Containers & Search Power in prototype/M10

CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O3 -pthread
INCLUDES = -I. -I../M8 -I../M7 -I../M6 -I../M5 -I../M4 -I../M3 -I../../src

OBJS = test.o \
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

TARGET = test_m10

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

console.o: ../M8/console.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

value.o: ../M7/value.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

number.o: ../M7/number.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

resolver.o: ../M6/resolver.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

resolver_scopes.o: ../M6/resolver_scopes.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

resolver_suits.o: ../M6/resolver_suits.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

resolver_names.o: ../M6/resolver_names.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

resolver_walk.o: ../M6/resolver_walk.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

diagnostic_code.o: ../M5/diagnostic_code.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

diagnostic.o: ../M5/diagnostic.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

suggester.o: ../M5/suggester.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

source_view.o: ../M5/source_view.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

renderer.o: ../M5/renderer.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

reporter.o: ../M5/reporter.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

lexer_parser_bridge.o: ../M5/lexer_parser_bridge.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

ast.o: ../M4/ast.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

unparse.o: ../M4/unparse.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

parser.o: ../M4/parser.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

parser_decl.o: ../M4/parser_decl.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

parser_stmt.o: ../M4/parser_stmt.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

parser_expr.o: ../M4/parser_expr.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

lexer.o: ../M3/lexer.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

satellite_string.o: ../../src/satellite_string/satellite_string.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

host_facts.o: ../../src/system_facts/host_facts.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

memory_facts.o: ../../src/system_facts/memory_facts.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

stack_facts.o: ../../src/system_facts/stack_facts.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test

