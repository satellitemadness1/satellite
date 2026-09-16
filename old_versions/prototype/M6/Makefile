# Standalone Makefile for the M6 Resolver Prototype in prototype/M6

LLVM_BIN = $(HOME)/opt/clang-current/bin
ifneq ($(wildcard $(LLVM_BIN)/clang++),)
  CXX ?= $(LLVM_BIN)/clang++
else
  CXX := c++
endif

CXXFLAGS ?= -std=c++20 -Wall -Wextra -O3
INCLUDES = -I. -I../M3 -I../M4 -I../M5 -I../../src

OBJS = test.o \
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
       satellite_string.o

TARGET = test_resolver

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp
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

lexer.o: ../M3/lexer.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

satellite_string.o: ../../src/satellite_string/satellite_string.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o

.PHONY: all run clean

