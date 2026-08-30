# Standalone Makefile for the M3 Lexer Prototype in flash/

CXX ?= c++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -O3
INCLUDES = -I. -I../../src

SRCS = test.cpp lexer.cpp ../../src/satellite_string/satellite_string.cpp
TARGET = test_lexer

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

