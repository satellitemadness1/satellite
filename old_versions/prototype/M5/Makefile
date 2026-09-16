# Standalone Makefile for the M5 Diagnostic Error Reporter Prototype in flash/

LLVM_BIN = $(HOME)/opt/clang-current/bin
ifneq ($(wildcard $(LLVM_BIN)/clang++),)
  CXX ?= $(LLVM_BIN)/clang++
else
  CXX := c++
endif

CXXFLAGS ?= -std=c++20 -Wall -Wextra -O3
INCLUDES = -I. -I../M3 -I../M4 -I../../src

SRCS = test.cpp \
       diagnostic_code.cpp \
       diagnostic.cpp \
       suggester.cpp \
       source_view.cpp \
       renderer.cpp \
       reporter.cpp \
       lexer_parser_bridge.cpp \
       ../M4/ast.cpp \
       ../M4/unparse.cpp \
       ../M4/parser.cpp \
       ../M4/parser_decl.cpp \
       ../M4/parser_stmt.cpp \
       ../M4/parser_expr.cpp \
       ../M3/lexer.cpp \
       ../../src/satellite_string/satellite_string.cpp

TARGET = test_error_reporter

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SRCS)

run: $(TARGET)
	./$(TARGET)

demo: $(TARGET)
	./$(TARGET) --demo

clean:
	rm -f $(TARGET)

.PHONY: all run demo clean

