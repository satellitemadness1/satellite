// satellite-004/satellite-numbers/satellite.directory.system()/satellite.directory.system().satellite.cpp
//
// satellite.directory.system()   1 18 6
// Where the machine's drives are mounted, one place a drive (the author, 2026-09-25).
// Typed alone at the prompt it draws their space instead (satellite/satl/drives.hpp).
//
// A library with no main: it exports satellite_number_describe (number_row.hpp),
// which the interpreter calls once at start-up. Built as 1.18.6.so by
// build_libraries.py. What it does is directory_words.hpp, shared with the other
// directory words.

#include "../directory_words.hpp"

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.directory.system()";
    row->numbers[0] = 1;
    row->numbers[1] = 18;
    row->numbers[2] = 6;
    row->depth = 3;
    row->scenarios.directory = satellite004::directory_words::drives;
    return satellite004::success;
}
