// satellite-004/satellite-numbers/satellite.directory.list()/satellite.directory.list().satellite.cpp
//
// satellite.directory.list()   1 18 4
// The names in the working directory, sorted, `.` and `..` dropped and real dotfiles kept.
// Typed alone at the prompt it draws the table instead (satellite/satl/listing.hpp).
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.18.4.so by
// build_libraries.py. What it does is directory_words.hpp, shared with the other
// two directory words; written 2026-09-17 for PLAN M0.6.

#include "../directory_words.hpp"

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.directory.list()";
    row->numbers[0] = 1;
    row->numbers[1] = 18;
    row->numbers[2] = 4;
    row->depth = 3;
    row->scenarios.directory = satellite004::directory_words::list;
    return satellite004::success;
}
