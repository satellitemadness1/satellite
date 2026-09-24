# time_test/ — every timing, raced on the author's machine

The author, 2026-09-23: *"let's build a /time_test/satellite.library/main.cpp that races in
C++, so I can just... run every test myself, and I can compile the C++ myself"*, then *"we
can just put everything time testing related into /time_test/"*.

**Every timing Claude quotes belongs here, as a test the author can compile and run
himself.** Each folder is one test: a `main.cpp`, built with one line, run from its own
folder. There's no Python and no make.

| Folder | What it races | Build, then run |
|---|---|---|
| `satellite.library/` | satl against satl, on the satellite programs in `programs/`. Each run is printed, then best, median and worst. Answers are compared between satls. | `clang++ -std=c++20 -O2 main.cpp -o race` then `./race 5 ~/.satl/satl ../../build/satl` |
| `locks/` | Four threads adding 1 to one counter through a bool, two bools, a lock and "doubling". RIGHT or WRONG is printed beside each. | `clang++ -std=c++20 -O2 -pthread main.cpp -o locks` then `./locks` |
| `threads/` | Adding two numbers from 100 to 100 million digits; handing work to a thread and back; a threaded add checked against one thread. | `clang++ -std=c++20 -O2 -pthread main.cpp -o threads` then `./threads` |

`g++` works wherever `clang++` is written.

**Not moved here yet.** These are the older races, which `make` and `check.sh` use where
they are:

- `make race` (`satellite/race/`: display against `std::cout`);
- `make number-race` (`satellite/satellite_variable_number/number_race.sh`);
- the string races in `satellite/satellite_variable_string/`.
