# Standalone Makefile for the M4 Arena AST & Parser Prototype in flash/

LLVM_BIN = $(HOME)/opt/clang-current/bin
ifneq ($(wildcard $(LLVM_BIN)/clang++),)
  CXX ?= $(LLVM_BIN)/clang++
else
  CXX := c++
endif

CXXFLAGS ?= -std=c++20 -Wall -Wextra -O3
INCLUDES = -I. -I../M3 -I../../src

SRCS = test.cpp ast.cpp unparse.cpp parser.cpp parser_decl.cpp parser_stmt.cpp parser_expr.cpp ../M3/lexer.cpp ../../src/satellite_string/satellite_string.cpp
TARGET = test_parser

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
