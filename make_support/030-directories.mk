# satellite 004 -- one variable per directory.
#
# Every path in every later fragment is spelled through one of these, so a
# directory that moves is one edit here. The interpreter's sources are under
# satellite/, one folder a subject (the author, 2026-09-15).
#
# TERM_DIR AND NOT TERM, 003's reason: every login shell exports TERM, and under
# `make -e` the environment would win.

BUILD      = build
OBJECTS    = $(BUILD)/objects
SATELLITE  = satellite
NUMBERS    = satellite-numbers
TERM_DIR   = satl-term
MACHINE    = $(SATELLITE)/machine
ARGUMENTS  = $(SATELLITE)/arguments
BYTECODE   = $(SATELLITE)/bytecode
OBJECT     = $(SATELLITE)/satellite_object
NUMBER     = $(SATELLITE)/satellite_variable_number
STRING16   = $(SATELLITE)/satellite_variable_string
BINARY     = $(SATELLITE)/satellite_variable_binary
PERCENTAGE = $(SATELLITE)/satellite_variable_percentage
VERSION_DIR = $(SATELLITE)/version
STRINGS32  = strings
