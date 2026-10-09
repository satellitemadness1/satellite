# time_test/ — every timing, raced on the author's machine

The author, 2026-09-23: *"let's build a /time_test/satellite.library/main.cpp that races in
C++, so I can just... run every test myself, and I can compile the C++ myself"*, then *"we
can just put everything time testing related into /time_test/"*.

**Every timing Claude quotes belongs here, as a test the author can compile and run
himself.** Each folder is one test: a `main.cpp`, built with one line, run from its own
folder. There's no Python and no make.

| Folder | What it races | Build, then run |
|---|---|---|
| `satellite.library/` | satl, Python and C++ on the same eleven programs: `programs/*.satl`, `python/*.py`, `cpp/*.cpp`. Each run is printed, then best, median and worst. C++'s compile is timed on its own and added once ("with its compile"). Every answer is checked against satl's. | `clang++ -std=c++20 -O2 main.cpp -o race` then `./race 5 ~/.satl/satl` |
| `locks/` | Four threads adding 1 to one counter through a bool, two bools, a lock and "doubling". RIGHT or WRONG is printed beside each. | `clang++ -std=c++20 -O2 -pthread main.cpp -o locks` then `./locks` |
| `threads/` | Adding two numbers from 100 to 100 million digits; handing work to a thread and back; a threaded add checked against one thread. | `clang++ -std=c++20 -O2 -pthread main.cpp -o threads` then `./threads` |

`g++` works wherever `clang++` is written.

**What the race's Python and C++ are.**

- **Python.** It is CPython (`/usr/bin/python3`) unless `--python=` names another. On the
  author's Xeon, a plain `python3` is PyPy, a JIT, and the race prints which one ran.
- **C++.** It is clang++ when it is there, or `--cxx=`.
- **The same answers.** Each Python and C++ version prints exactly what satl prints, byte
  for byte. That was checked with `diff` against satl when they were written (2026-09-23),
  and the race checks it again on every run.
- **Numbers and floats.** satl never lets a number overflow, so Python uses its own ints
  and C++ uses `cpp/bignum.hpp`. That header uses the same schoolbook multiply and Knuth
  division that satl does, and was checked against Python on 4,410 pairs. satl's 128-place
  floats are `decimal.Decimal` in Python and a number scaled by 10^128 in C++.
- **Where a translation differs.** Each file's first comment says wherever it is not line
  for line. For example, CPython joins `"satl" + "-" + "004"` into one constant when it
  compiles `everything.py`.

**Not moved here yet.** These are the older races, which `make` and `check.sh` use where
they are:

- `make race` (`satellite/race/`: display against `std::cout`);
- `make number-race` (`satellite/satellite_variable_number/number_race.sh`);
- the string races in `satellite/satellite_variable_string/`.
