LLVM_BIN = $(HOME)/.local/llvm/bin
CXX      = $(LLVM_BIN)/clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2

SATELLITE = ../../satl

all: tour

tour: tour.cpp
	$(CXX) $(CXXFLAGS) -o $@ tour.cpp

run: tour
	./tour

run-satellite:
	$(SATELLITE) --run tour.satl

diff: tour
	./tour > cxx.out
	$(SATELLITE) --run tour.satl > satellite.out
	diff cxx.out satellite.out && echo "identical output"

lines:
	@printf '%-12s %6s %6s %8s\n' language lines code bytes
	@printf '%-12s %6s %6s %8s\n' c++ \
		"$$(wc -l < tour.cpp)" \
		"$$(grep -cve '^[[:space:]]*$$' tour.cpp)" \
		"$$(wc -c < tour.cpp)"
	@printf '%-12s %6s %6s %8s\n' satellite \
		"$$(wc -l < tour.satl)" \
		"$$(grep -cve '^[[:space:]]*$$' tour.satl)" \
		"$$(wc -c < tour.satl)"

clean:
	rm -f tour cxx.out satellite.out

.PHONY: all run run-satellite diff lines clean
